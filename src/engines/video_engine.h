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

#pragma once

#include <d3d11.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <wrl/client.h>

#include <string>

#include "engines/playback_clock.h"
#include "engines/wallpaper_engine.h"
#include "render/fit_rect.h"

namespace umbra {

// Decodes a video file via Media Foundation and uploads each decoded frame
// into a D3D11 texture for Compositor::draw(). Loops back to the start on
// end-of-stream (per the PRD: video wallpapers play on an infinite loop).
// Windows-only, verified manually against a live desktop session (see
// TESTING.md) — the frame-pacing/looping decision itself is delegated to
// PlaybackClock, which is unit-tested on its own.
class VideoEngine : public IWallpaperEngine {
   public:
    // device must outlive this VideoEngine — the decoded texture and its
    // shader resource view are created on it and recreated on it whenever
    // the source's frame size is (re)detected. Throws std::runtime_error
    // if the file can't be opened or has no video stream.
    VideoEngine(ID3D11Device* device, const std::string& path, int fpsCap = 60);
    ~VideoEngine() override;

    VideoEngine(const VideoEngine&) = delete;
    VideoEngine& operator=(const VideoEngine&) = delete;

    void advance(double deltaSeconds) override;
    ID3D11ShaderResourceView* currentFrame() const override { return frameView_.Get(); }
    Size frameSize() const override { return frameSize_; }

   private:
    void openSource(const std::string& path, int fpsCap);
    void createTexture();
    void decodeNextFrame();
    void uploadFrame(IMFSample* sample);
    void seekToStart();

    Microsoft::WRL::ComPtr<ID3D11Device> device_;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context_;
    Microsoft::WRL::ComPtr<IMFSourceReader> reader_;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> texture_;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> frameView_;

    Size frameSize_;
    PlaybackClock clock_;
};

}  // namespace umbra
