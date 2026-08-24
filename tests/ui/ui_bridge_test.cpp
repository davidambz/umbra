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

#include "ui/ui_bridge.h"

#include <gtest/gtest.h>

#include <fstream>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;
using json = nlohmann::json;
using umbra::IUiBridgeHost;
using umbra::LibraryManager;
using umbra::MonitorInfo;
using umbra::Settings;
using umbra::UiBridge;
using umbra::WallpaperType;

namespace {

class FakeUiBridgeHost : public IUiBridgeHost {
   public:
    explicit FakeUiBridgeHost(fs::path storageRoot) : library_(std::move(storageRoot)) {
        monitors_ = {
            MonitorInfo{
                .id = "primary", .x = 0, .y = 0, .width = 1920, .height = 1080, .isPrimary = true},
            MonitorInfo{.id = "secondary", .x = 1920, .y = 0, .width = 1920, .height = 1080},
        };
    }

    Settings& settings() override { return settings_; }
    LibraryManager& library() override { return library_; }
    std::vector<MonitorInfo> monitors() override { return monitors_; }
    std::string currentTheme() override { return theme_; }

    void persistSettings() override { persistCount_++; }
    void persistSettingsAndRebuildMonitorHosts() override {
        persistCount_++;
        rebuildCount_++;
    }

    std::string pickImportSource(WallpaperType /*type*/) override { return nextPickResult_; }

    // Stands in for the real Windows-only ThumbnailGenerator: writes a
    // fake PNG at the convention path if generateThumbnailShouldSucceed_
    // is set, so tests can verify importWallpaper's response actually
    // carries a thumbnailUrl once this hook has run — same as it would
    // after a real Media Foundation/WIC-backed generation.
    void generateThumbnail(const std::string& title, WallpaperType type,
                           const fs::path& contentDir) override {
        generateThumbnailCallCount_++;
        lastThumbnailTitle_ = title;
        lastThumbnailType_ = type;
        lastThumbnailContentDir_ = contentDir;
        if (generateThumbnailShouldSucceed_) {
            std::ofstream thumbnail(library_.thumbnailPathForTitle(title), std::ios::binary);
            thumbnail << "fake png bytes";
        }
    }

    Settings settings_;
    LibraryManager library_;
    std::vector<MonitorInfo> monitors_;
    std::string theme_ = "dark";
    std::string nextPickResult_;
    int persistCount_ = 0;
    int rebuildCount_ = 0;
    int generateThumbnailCallCount_ = 0;
    std::string lastThumbnailTitle_;
    WallpaperType lastThumbnailType_ = WallpaperType::Video;
    fs::path lastThumbnailContentDir_;
    bool generateThumbnailShouldSucceed_ = false;
};

class UiBridgeTest : public ::testing::Test {
   protected:
    void SetUp() override {
        const std::string testName = testing::UnitTest::GetInstance()->current_test_info()->name();
        root_ = fs::temp_directory_path() / fs::path("umbra_ui_bridge_test_" + testName);
        fs::remove_all(root_);
        fs::create_directories(root_);
        host_ = std::make_unique<FakeUiBridgeHost>(root_ / "storage");
        bridge_ = std::make_unique<UiBridge>(*host_);
    }

    void TearDown() override { fs::remove_all(root_); }

    fs::path writeSourceFile(const std::string& name, const std::string& contents) {
        const fs::path path = root_ / name;
        std::ofstream file(path, std::ios::binary);
        file << contents;
        return path;
    }

    json call(const std::string& method, json params = json::object(), int id = 1) {
        const json request{{"id", id}, {"method", method}, {"params", std::move(params)}};
        return json::parse(bridge_->handleRequest(request.dump()));
    }

    fs::path root_;
    std::unique_ptr<FakeUiBridgeHost> host_;
    std::unique_ptr<UiBridge> bridge_;
};

}  // namespace

TEST_F(UiBridgeTest, GetMonitorsReturnsEveryMonitor) {
    const json response = call("getMonitors");
    ASSERT_TRUE(response.contains("result"));
    EXPECT_EQ(response["result"].size(), 2u);
    EXPECT_EQ(response["result"][0]["id"], "primary");
}

TEST_F(UiBridgeTest, GetAssignmentIsNoneWhenNothingIsAssigned) {
    const json response = call("getAssignment", {{"monitorId", "primary"}});
    EXPECT_EQ(response["result"]["kind"], "none");
}

TEST_F(UiBridgeTest, AssignSingleThenGetAssignmentRoundTrips) {
    host_->nextPickResult_ = writeSourceFile("rain.mp4", "fake video").string();
    ASSERT_EQ(call("importWallpaper", {{"title", "Rainy Day"}, {"type", "video"}})["result"]["id"],
              "Rainy Day");

    const json assignResponse = call(
        "assignSingle", {{"monitorId", "primary"}, {"wallpaperId", "Rainy Day"}, {"fpsCap", 30}});
    ASSERT_TRUE(assignResponse.contains("result"));
    EXPECT_EQ(host_->rebuildCount_, 1);

    const json assignment = call("getAssignment", {{"monitorId", "primary"}})["result"];
    EXPECT_EQ(assignment["kind"], "single");
    EXPECT_EQ(assignment["wallpaperId"], "Rainy Day");
    EXPECT_EQ(assignment["fpsCap"], 30);
}

TEST_F(UiBridgeTest, AssignSingleFailsForAnUnknownWallpaperId) {
    const json response = call(
        "assignSingle", {{"monitorId", "primary"}, {"wallpaperId", "nonexistent"}, {"fpsCap", 30}});
    EXPECT_TRUE(response.contains("error"));
    EXPECT_EQ(host_->persistCount_, 0);
    EXPECT_EQ(host_->rebuildCount_, 0);
}

TEST_F(UiBridgeTest, AssignSingleFailsForAnUnknownMonitorId) {
    host_->nextPickResult_ = writeSourceFile("rain.mp4", "fake video").string();
    call("importWallpaper", {{"title", "Rainy Day"}, {"type", "video"}});

    const json response =
        call("assignSingle",
             {{"monitorId", "nonexistent"}, {"wallpaperId", "Rainy Day"}, {"fpsCap", 30}});
    EXPECT_TRUE(response.contains("error"));
}

TEST_F(UiBridgeTest, AssignPlaylistThenGetAssignmentRoundTrips) {
    host_->nextPickResult_ = writeSourceFile("rain.mp4", "fake video").string();
    call("importWallpaper", {{"title", "Rainy Day"}, {"type", "video"}});
    host_->nextPickResult_ = writeSourceFile("snow.mp4", "fake video 2").string();
    call("importWallpaper", {{"title", "Snowy Day"}, {"type", "video"}});

    const json playlist = {{"wallpaperIds", {"Rainy Day", "Snowy Day"}},
                           {"intervalSeconds", 120},
                           {"mode", "shuffle"}};
    call("assignPlaylist", {{"monitorId", "primary"}, {"playlist", playlist}, {"fpsCap", 15}});

    const json assignment = call("getAssignment", {{"monitorId", "primary"}})["result"];
    EXPECT_EQ(assignment["kind"], "playlist");
    EXPECT_EQ(assignment["playlist"]["wallpaperIds"], json({"Rainy Day", "Snowy Day"}));
    EXPECT_EQ(assignment["playlist"]["intervalSeconds"], 120);
    EXPECT_EQ(assignment["playlist"]["mode"], "shuffle");
    EXPECT_EQ(assignment["fpsCap"], 15);
}

TEST_F(UiBridgeTest, ClearAssignmentResetsAMonitorBackToNone) {
    host_->nextPickResult_ = writeSourceFile("rain.mp4", "fake video").string();
    call("importWallpaper", {{"title", "Rainy Day"}, {"type", "video"}});
    call("assignSingle", {{"monitorId", "primary"}, {"wallpaperId", "Rainy Day"}, {"fpsCap", 30}});

    call("clearAssignment", {{"monitorId", "primary"}});

    EXPECT_EQ(call("getAssignment", {{"monitorId", "primary"}})["result"]["kind"], "none");
}

TEST_F(UiBridgeTest, ImportWallpaperReturnsNullWhenThePickerIsCancelled) {
    host_->nextPickResult_.clear();
    const json response = call("importWallpaper", {{"title", "Anything"}, {"type", "video"}});
    EXPECT_TRUE(response["result"].is_null());
    EXPECT_TRUE(call("getLibrary")["result"].empty());
    EXPECT_EQ(host_->generateThumbnailCallCount_, 0);
}

TEST_F(UiBridgeTest, ImportWallpaperCallsGenerateThumbnailWithTheImportedContentDir) {
    host_->nextPickResult_ = writeSourceFile("rain.mp4", "fake video").string();
    call("importWallpaper", {{"title", "Rainy Day"}, {"type", "video"}});

    EXPECT_EQ(host_->generateThumbnailCallCount_, 1);
    EXPECT_EQ(host_->lastThumbnailTitle_, "Rainy Day");
    EXPECT_EQ(host_->lastThumbnailType_, WallpaperType::Video);
    EXPECT_EQ(host_->lastThumbnailContentDir_, host_->library_.pathForTitle("Rainy Day"));
}

TEST_F(UiBridgeTest, ImportWallpaperResponseCarriesThumbnailUrlWhenGenerationSucceeds) {
    host_->generateThumbnailShouldSucceed_ = true;
    host_->nextPickResult_ = writeSourceFile("rain.mp4", "fake video").string();

    const json response = call("importWallpaper", {{"title", "Rainy Day"}, {"type", "video"}});

    ASSERT_TRUE(response["result"].contains("thumbnailUrl"));
    EXPECT_TRUE(response["result"]["thumbnailUrl"].get<std::string>().starts_with(
        "data:image/png;base64,"));
}

TEST_F(UiBridgeTest, ImportWallpaperResponseOmitsThumbnailUrlWhenGenerationFails) {
    host_->generateThumbnailShouldSucceed_ = false;
    host_->nextPickResult_ = writeSourceFile("rain.mp4", "fake video").string();

    const json response = call("importWallpaper", {{"title", "Rainy Day"}, {"type", "video"}});

    EXPECT_FALSE(response["result"].contains("thumbnailUrl"));
}

TEST_F(UiBridgeTest, GetLibraryCarriesThumbnailUrlForAnAlreadyGeneratedThumbnail) {
    host_->generateThumbnailShouldSucceed_ = true;
    host_->nextPickResult_ = writeSourceFile("rain.mp4", "fake video").string();
    call("importWallpaper", {{"title", "Rainy Day"}, {"type", "video"}});

    const json library = call("getLibrary")["result"];
    ASSERT_EQ(library.size(), 1u);
    EXPECT_TRUE(library[0]["thumbnailUrl"].get<std::string>().starts_with(
        "data:image/png;base64,"));
}

TEST_F(UiBridgeTest, RenameWallpaperUpdatesAnExistingSingleAssignmentsPath) {
    host_->nextPickResult_ = writeSourceFile("rain.mp4", "fake video").string();
    call("importWallpaper", {{"title", "Rainy Day"}, {"type", "video"}});
    call("assignSingle", {{"monitorId", "primary"}, {"wallpaperId", "Rainy Day"}, {"fpsCap", 30}});

    call("renameWallpaper", {{"id", "Rainy Day"}, {"newTitle", "Stormy Day"}});

    const json assignment = call("getAssignment", {{"monitorId", "primary"}})["result"];
    EXPECT_EQ(assignment["wallpaperId"], "Stormy Day");
    // One rebuild from assignSingle above, plus another since the rename
    // changed an active assignment's path — playback needs restarting.
    EXPECT_EQ(host_->rebuildCount_, 2);
}

TEST_F(UiBridgeTest, RenamingAWallpaperNotAssignedToAnyMonitorSkipsTheRebuild) {
    host_->nextPickResult_ = writeSourceFile("rain.mp4", "fake video").string();
    call("importWallpaper", {{"title", "Rainy Day"}, {"type", "video"}});

    call("renameWallpaper", {{"id", "Rainy Day"}, {"newTitle", "Stormy Day"}});

    EXPECT_EQ(host_->rebuildCount_, 0);
    EXPECT_GE(host_->persistCount_, 1);
}

TEST_F(UiBridgeTest, RemoveWallpaperClearsASingleAssignmentReferencingIt) {
    host_->nextPickResult_ = writeSourceFile("rain.mp4", "fake video").string();
    call("importWallpaper", {{"title", "Rainy Day"}, {"type", "video"}});
    call("assignSingle", {{"monitorId", "primary"}, {"wallpaperId", "Rainy Day"}, {"fpsCap", 30}});

    call("removeWallpaper", {{"id", "Rainy Day"}});

    EXPECT_EQ(call("getAssignment", {{"monitorId", "primary"}})["result"]["kind"], "none");
    // One rebuild from assignSingle above, plus another for the removal
    // that cleared that assignment.
    EXPECT_EQ(host_->rebuildCount_, 2);
}

TEST_F(UiBridgeTest, RemovingAWallpaperNotAssignedToAnyMonitorSkipsTheRebuild) {
    host_->nextPickResult_ = writeSourceFile("rain.mp4", "fake video").string();
    call("importWallpaper", {{"title", "Rainy Day"}, {"type", "video"}});

    call("removeWallpaper", {{"id", "Rainy Day"}});

    EXPECT_EQ(host_->rebuildCount_, 0);
    EXPECT_GE(host_->persistCount_, 1);
}

TEST_F(UiBridgeTest, RemoveWallpaperDropsItFromAPlaylistWithoutClearingTheWholeAssignment) {
    host_->nextPickResult_ = writeSourceFile("rain.mp4", "fake video").string();
    call("importWallpaper", {{"title", "Rainy Day"}, {"type", "video"}});
    host_->nextPickResult_ = writeSourceFile("snow.mp4", "fake video 2").string();
    call("importWallpaper", {{"title", "Snowy Day"}, {"type", "video"}});
    const json playlist = {{"wallpaperIds", {"Rainy Day", "Snowy Day"}},
                           {"intervalSeconds", 120},
                           {"mode", "sequential"}};
    call("assignPlaylist", {{"monitorId", "primary"}, {"playlist", playlist}, {"fpsCap", 15}});

    call("removeWallpaper", {{"id", "Rainy Day"}});

    const json assignment = call("getAssignment", {{"monitorId", "primary"}})["result"];
    EXPECT_EQ(assignment["kind"], "playlist");
    EXPECT_EQ(assignment["playlist"]["wallpaperIds"], json({"Snowy Day"}));
}

TEST_F(UiBridgeTest, GetSettingsReflectsHostSettings) {
    host_->settings_.pauseOnBattery = true;
    host_->settings_.syncLockScreen = true;
    const json response = call("getSettings")["result"];
    EXPECT_EQ(response["pauseOnBattery"], true);
    EXPECT_EQ(response["syncLockScreen"], true);
}

TEST_F(UiBridgeTest, UpdateSettingsAppliesAPartialPatch) {
    call("updateSettings", {{"pauseOnBattery", true}});
    EXPECT_TRUE(host_->settings_.pauseOnBattery);
    EXPECT_TRUE(host_->settings_.launchOnStartup);  // untouched field keeps its default
    EXPECT_EQ(host_->persistCount_, 1);
    // A settings toggle doesn't touch any monitor's assignment, so it
    // shouldn't pay for restarting playback on every monitor.
    EXPECT_EQ(host_->rebuildCount_, 0);
}

TEST_F(UiBridgeTest, UpdateSettingsAppliesSyncLockScreen) {
    call("updateSettings", {{"syncLockScreen", true}});
    EXPECT_TRUE(host_->settings_.syncLockScreen);
    EXPECT_EQ(host_->persistCount_, 1);
}

TEST_F(UiBridgeTest, GetThemeReturnsWhateverTheHostReports) {
    host_->theme_ = "light";
    EXPECT_EQ(call("getTheme")["result"], "light");
}

TEST_F(UiBridgeTest, UnknownMethodReturnsAnError) {
    const json response = call("bogusMethod");
    EXPECT_TRUE(response.contains("error"));
}

TEST(UiBridgeThemeEvent, BuildsTheExpectedEventEnvelope) {
    const json event = json::parse(UiBridge::buildThemeChangedEvent("dark"));
    EXPECT_EQ(event["event"], "themeChanged");
    EXPECT_EQ(event["payload"], "dark");
}
