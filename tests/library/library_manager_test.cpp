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

#include "library/library_manager.h"

#include <gtest/gtest.h>
#include <zip.h>

#include <algorithm>
#include <fstream>

using umbra::detectWallpaperType;
using umbra::ImportError;
using umbra::LibraryManager;
using umbra::WallpaperType;

namespace fs = std::filesystem;

namespace {

class LibraryManagerTest : public ::testing::Test {
   protected:
    void SetUp() override {
        const std::string testName = testing::UnitTest::GetInstance()->current_test_info()->name();
        root_ = fs::temp_directory_path() / fs::path("umbra_library_test_" + testName);
        fs::remove_all(root_);
        fs::create_directories(root_);
        storage_ = root_ / "storage";
        manager_ = std::make_unique<LibraryManager>(storage_);
    }

    void TearDown() override { fs::remove_all(root_); }

    fs::path writeFile(const std::string& name, const std::string& contents) {
        const fs::path path = root_ / name;
        std::ofstream file(path, std::ios::binary);
        file << contents;
        return path;
    }

    fs::path root_;
    fs::path storage_;
    std::unique_ptr<LibraryManager> manager_;
};

}  // namespace

TEST(DetectWallpaperType, ClassifiesByExtension) {
    EXPECT_EQ(detectWallpaperType("clip.mp4"), WallpaperType::Video);
    EXPECT_EQ(detectWallpaperType("clip.WEBM"), WallpaperType::Video);
    EXPECT_EQ(detectWallpaperType("anim.gif"), WallpaperType::Image);
    EXPECT_EQ(detectWallpaperType("anim.apng"), WallpaperType::Image);
    EXPECT_EQ(detectWallpaperType("photo.png"), WallpaperType::Image);
    EXPECT_EQ(detectWallpaperType("photo.JPG"), WallpaperType::Image);
    EXPECT_EQ(detectWallpaperType("photo.jpeg"), WallpaperType::Image);
    EXPECT_EQ(detectWallpaperType("photo.bmp"), WallpaperType::Image);
    EXPECT_EQ(detectWallpaperType("photo.tif"), WallpaperType::Image);
    EXPECT_EQ(detectWallpaperType("photo.tiff"), WallpaperType::Image);
    EXPECT_EQ(detectWallpaperType("project.zip"), WallpaperType::Web);
}

TEST(DetectWallpaperType, ThrowsOnUnsupportedExtension) {
    EXPECT_THROW(detectWallpaperType("notes.txt"), std::invalid_argument);
}

TEST_F(LibraryManagerTest, ImportsVideoFileWithNormalizedName) {
    const fs::path source = writeFile("rain.mp4", "fake video bytes");

    const auto result = manager_->import("Rainy Day", source);

    ASSERT_TRUE(result.success);
    EXPECT_EQ(result.profile.type, WallpaperType::Video);
    EXPECT_TRUE(fs::exists(manager_->pathForTitle("Rainy Day") / "video.mp4"));
}

TEST_F(LibraryManagerTest, ImportsImageFileWithNormalizedName) {
    const fs::path source = writeFile("loop.gif", "fake gif bytes");

    const auto result = manager_->import("Loop", source);

    ASSERT_TRUE(result.success);
    EXPECT_EQ(result.profile.type, WallpaperType::Image);
    EXPECT_TRUE(fs::exists(manager_->pathForTitle("Loop") / "image.gif"));
}

TEST_F(LibraryManagerTest, ImportsWebFolderAsIs) {
    const fs::path webDir = root_ / "web-project";
    fs::create_directories(webDir / "assets");
    std::ofstream(webDir / "index.html") << "<html></html>";
    std::ofstream(webDir / "assets" / "style.css") << "body{}";

    const auto result = manager_->import("My Scene", webDir);

    ASSERT_TRUE(result.success);
    EXPECT_EQ(result.profile.type, WallpaperType::Web);
    EXPECT_TRUE(fs::exists(manager_->pathForTitle("My Scene") / "index.html"));
    EXPECT_TRUE(fs::exists(manager_->pathForTitle("My Scene") / "assets" / "style.css"));
}

TEST_F(LibraryManagerTest, RejectsWebFolderMissingIndexHtml) {
    const fs::path webDir = root_ / "broken-web-project";
    fs::create_directories(webDir);
    std::ofstream(webDir / "main.js") << "console.log(1)";

    const auto result = manager_->import("Broken", webDir);

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error, ImportError::WebMissingIndexHtml);
    EXPECT_FALSE(fs::exists(manager_->pathForTitle("Broken")));
}

TEST_F(LibraryManagerTest, ImportsWebZipContainingIndexHtml) {
    const fs::path zipPath = root_ / "project.zip";
    int errorCode = 0;
    zip_t* archive = zip_open(zipPath.string().c_str(), ZIP_CREATE | ZIP_EXCL, &errorCode);
    ASSERT_NE(archive, nullptr);

    const std::string indexContents = "<html></html>";
    zip_source_t* indexSource =
        zip_source_buffer(archive, indexContents.data(), indexContents.size(), 0);
    zip_file_add(archive, "index.html", indexSource, ZIP_FL_ENC_UTF_8);

    const std::string cssContents = "body{}";
    zip_source_t* cssSource = zip_source_buffer(archive, cssContents.data(), cssContents.size(), 0);
    zip_file_add(archive, "assets/style.css", cssSource, ZIP_FL_ENC_UTF_8);

    zip_close(archive);

    const auto result = manager_->import("Zipped Scene", zipPath);

    ASSERT_TRUE(result.success);
    EXPECT_EQ(result.profile.type, WallpaperType::Web);
    EXPECT_TRUE(fs::exists(manager_->pathForTitle("Zipped Scene") / "index.html"));
    EXPECT_TRUE(fs::exists(manager_->pathForTitle("Zipped Scene") / "assets" / "style.css"));
}

TEST_F(LibraryManagerTest, RejectsZipEntryEscapingDestinationFolder) {
    const fs::path zipPath = root_ / "malicious.zip";
    int errorCode = 0;
    zip_t* archive = zip_open(zipPath.string().c_str(), ZIP_CREATE | ZIP_EXCL, &errorCode);
    ASSERT_NE(archive, nullptr);

    const std::string indexContents = "<html></html>";
    zip_source_t* indexSource =
        zip_source_buffer(archive, indexContents.data(), indexContents.size(), 0);
    zip_file_add(archive, "index.html", indexSource, ZIP_FL_ENC_UTF_8);

    const std::string evilContents = "pwned";
    zip_source_t* evilSource =
        zip_source_buffer(archive, evilContents.data(), evilContents.size(), 0);
    zip_file_add(archive, "../../escaped.txt", evilSource, ZIP_FL_ENC_UTF_8);

    zip_close(archive);

    const auto result = manager_->import("Malicious Scene", zipPath);

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error, ImportError::CopyFailed);
    EXPECT_FALSE(fs::exists(root_ / "escaped.txt"));
    EXPECT_FALSE(fs::exists(manager_->pathForTitle("Malicious Scene")));
}

TEST_F(LibraryManagerTest, FailsWhenSourceDoesNotExist) {
    const auto result = manager_->import("Ghost", root_ / "does-not-exist.mp4");

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error, ImportError::SourceNotFound);
}

TEST_F(LibraryManagerTest, FailsWhenDestinationAlreadyExists) {
    const fs::path source = writeFile("rain.mp4", "fake video bytes");
    ASSERT_TRUE(manager_->import("Rainy Day", source).success);

    const auto result = manager_->import("Rainy Day", source);

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error, ImportError::DestinationAlreadyExists);
}

TEST_F(LibraryManagerTest, RejectsEmptyTitle) {
    const fs::path source = writeFile("rain.mp4", "fake video bytes");

    const auto result = manager_->import("", source);

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error, ImportError::TitleEmpty);
}

TEST_F(LibraryManagerTest, RejectsTitleContainingPathSeparator) {
    const fs::path source = writeFile("rain.mp4", "fake video bytes");

    const auto result = manager_->import("../escape", source);

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error, ImportError::InvalidTitle);
}

TEST_F(LibraryManagerTest, RenameMovesTheFolder) {
    const fs::path source = writeFile("rain.mp4", "fake video bytes");
    ASSERT_TRUE(manager_->import("Rainy Day", source).success);

    EXPECT_TRUE(manager_->rename("Rainy Day", "Storm"));

    EXPECT_FALSE(fs::exists(manager_->pathForTitle("Rainy Day")));
    EXPECT_TRUE(fs::exists(manager_->pathForTitle("Storm") / "video.mp4"));
}

TEST_F(LibraryManagerTest, RenameFailsWhenTargetAlreadyExists) {
    const fs::path source = writeFile("rain.mp4", "fake video bytes");
    ASSERT_TRUE(manager_->import("Rainy Day", source).success);
    ASSERT_TRUE(manager_->import("Storm", source).success);

    EXPECT_FALSE(manager_->rename("Rainy Day", "Storm"));
}

TEST_F(LibraryManagerTest, RemoveDeletesTheFolder) {
    const fs::path source = writeFile("rain.mp4", "fake video bytes");
    ASSERT_TRUE(manager_->import("Rainy Day", source).success);

    EXPECT_TRUE(manager_->remove("Rainy Day"));

    EXPECT_FALSE(fs::exists(manager_->pathForTitle("Rainy Day")));
}

TEST_F(LibraryManagerTest, RemoveReturnsFalseWhenTitleDoesNotExist) {
    EXPECT_FALSE(manager_->remove("Nonexistent"));
}

TEST_F(LibraryManagerTest, ListReturnsEmptyForAFreshStorageRoot) {
    EXPECT_TRUE(manager_->list().empty());
}

TEST_F(LibraryManagerTest, ListReflectsEveryImportedEntryAndItsType) {
    ASSERT_TRUE(manager_->import("Rainy Day", writeFile("rain.mp4", "fake video bytes")).success);
    ASSERT_TRUE(manager_->import("A Still Life", writeFile("still.gif", "fake gif bytes")).success);

    const fs::path webDir = root_ / "web-src";
    fs::create_directories(webDir);
    writeFile("web-src/index.html", "<html></html>");
    ASSERT_TRUE(manager_->import("Interactive Clock", webDir).success);

    const auto entries = manager_->list();
    ASSERT_EQ(entries.size(), 3u);

    auto findByTitle = [&entries](const std::string& title) {
        return std::find_if(entries.begin(), entries.end(),
                            [&](const auto& entry) { return entry.title == title; });
    };

    const auto rainy = findByTitle("Rainy Day");
    ASSERT_NE(rainy, entries.end());
    EXPECT_EQ(rainy->type, WallpaperType::Video);
    EXPECT_EQ(rainy->path, manager_->pathForTitle("Rainy Day"));

    const auto still = findByTitle("A Still Life");
    ASSERT_NE(still, entries.end());
    EXPECT_EQ(still->type, WallpaperType::Image);

    const auto clock = findByTitle("Interactive Clock");
    ASSERT_NE(clock, entries.end());
    EXPECT_EQ(clock->type, WallpaperType::Web);
}

TEST_F(LibraryManagerTest, ListOmitsAnUnrelatedSubfolderThatWasNotImported) {
    fs::create_directories(storage_ / "not-a-wallpaper");
    EXPECT_TRUE(manager_->list().empty());
}

TEST_F(LibraryManagerTest, ListLeavesThumbnailPathEmptyWhenNoThumbnailWasGenerated) {
    ASSERT_TRUE(manager_->import("Rainy Day", writeFile("rain.mp4", "fake video bytes")).success);

    const auto entries = manager_->list();
    ASSERT_EQ(entries.size(), 1u);
    EXPECT_TRUE(entries.front().thumbnailPath.empty());
}

TEST_F(LibraryManagerTest, ListReportsThumbnailPathWhenThumbnailFileExists) {
    ASSERT_TRUE(manager_->import("Rainy Day", writeFile("rain.mp4", "fake video bytes")).success);

    const fs::path expectedThumbnail = manager_->thumbnailPathForTitle("Rainy Day");
    std::ofstream thumbnail(expectedThumbnail, std::ios::binary);
    thumbnail << "fake png bytes";
    thumbnail.close();

    const auto entries = manager_->list();
    ASSERT_EQ(entries.size(), 1u);
    EXPECT_EQ(entries.front().thumbnailPath, expectedThumbnail);
}
