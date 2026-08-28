// Umbra
// Copyright (C) 2026 David Ambrozio
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.

#include "app/updater.h"

#include <gtest/gtest.h>

using umbra::IHttpClient;
using umbra::UpdateCheckResult;
using umbra::Updater;

namespace {

// Only checkForUpdate() is covered here: applyUpdate() does a real file
// write + CreateProcessW launch regardless of which IHttpClient it's given,
// so it stays manual-only (see TESTING.md) rather than actually spawning a
// process during a unit test run.
class FakeHttpClient : public IHttpClient {
   public:
    // std::nullopt means "the request fails" (mirrors WinHttpClient
    // returning std::nullopt on any connect/send/receive failure).
    explicit FakeHttpClient(std::optional<std::string> response) : response_(std::move(response)) {}

    std::optional<std::vector<char>> get(const std::string& /*url*/,
                                         const std::vector<std::string>& /*headers*/) override {
        if (!response_) return std::nullopt;
        return std::vector<char>(response_->begin(), response_->end());
    }

   private:
    std::optional<std::string> response_;
};

std::string releaseJsonWithAsset(const std::string& tagName, const std::string& assetName) {
    return R"({"tag_name":")" + tagName + R"(","assets":[{"name":")" + assetName +
           R"(","browser_download_url":"https://example.invalid/)" + assetName + R"("}]})";
}

}  // namespace

TEST(Updater, ReportsNoUpdateWhenAlreadyOnTheLatestVersion) {
    FakeHttpClient client(releaseJsonWithAsset("v0.1.0", "UmbraSetup-0.1.0.exe"));
    Updater updater("davidambz", "umbra", client);

    const UpdateCheckResult result = updater.checkForUpdate("0.1.0");
    EXPECT_TRUE(result.checkSucceeded);
    EXPECT_FALSE(result.updateAvailable);
}

TEST(Updater, ReportsAnAvailableUpdateWithItsDownloadUrl) {
    FakeHttpClient client(releaseJsonWithAsset("v0.2.0", "UmbraSetup-0.2.0.exe"));
    Updater updater("davidambz", "umbra", client);

    const UpdateCheckResult result = updater.checkForUpdate("0.1.0");
    EXPECT_TRUE(result.checkSucceeded);
    EXPECT_TRUE(result.updateAvailable);
    EXPECT_EQ(result.latestVersion, "0.2.0");
    EXPECT_EQ(result.downloadUrl, "https://example.invalid/UmbraSetup-0.2.0.exe");
}

TEST(Updater, DoesNotOfferAnUpdateWithNoMatchingInstallerAsset) {
    // A release exists but its only asset isn't an UmbraSetup-*.exe (e.g.
    // release.yml is still mid-run) — don't offer an update with nowhere
    // to download it from.
    FakeHttpClient client(R"({"tag_name":"v0.2.0","assets":[{"name":"source.zip",)"
                          R"("browser_download_url":"https://example.invalid/source.zip"}]})");
    Updater updater("davidambz", "umbra", client);

    const UpdateCheckResult result = updater.checkForUpdate("0.1.0");
    EXPECT_TRUE(result.checkSucceeded);
    EXPECT_FALSE(result.updateAvailable);
}

TEST(Updater, SurfacesAFailedNetworkRequestAsAnUnsuccessfulCheck) {
    FakeHttpClient client(std::nullopt);
    Updater updater("davidambz", "umbra", client);

    const UpdateCheckResult result = updater.checkForUpdate("0.1.0");
    EXPECT_FALSE(result.checkSucceeded);
    EXPECT_FALSE(result.error.empty());
}

TEST(Updater, SurfacesAnUnparseableResponseAsAnUnsuccessfulCheck) {
    FakeHttpClient client(std::string("not json"));
    Updater updater("davidambz", "umbra", client);

    const UpdateCheckResult result = updater.checkForUpdate("0.1.0");
    EXPECT_FALSE(result.checkSucceeded);
    EXPECT_FALSE(result.error.empty());
}

TEST(Updater, RejectsAnUnparseableCurrentVersionBeforeMakingAnyRequest) {
    FakeHttpClient client(releaseJsonWithAsset("v0.2.0", "UmbraSetup-0.2.0.exe"));
    Updater updater("davidambz", "umbra", client);

    const UpdateCheckResult result = updater.checkForUpdate("not-a-version");
    EXPECT_FALSE(result.checkSucceeded);
    EXPECT_FALSE(result.error.empty());
}

TEST(Updater, ReportsSuccessWithNoUpdateWhenTheLatestTagIsntAVersion) {
    // Talking to a live network response, not this repo's own CI — an
    // unversioned "latest" tag shouldn't happen against release.yml's tag
    // filter, but is reported as success/nothing-to-offer rather than an
    // error if it ever does.
    FakeHttpClient client(releaseJsonWithAsset("nightly", "UmbraSetup-nightly.exe"));
    Updater updater("davidambz", "umbra", client);

    const UpdateCheckResult result = updater.checkForUpdate("0.1.0");
    EXPECT_TRUE(result.checkSucceeded);
    EXPECT_FALSE(result.updateAvailable);
}
