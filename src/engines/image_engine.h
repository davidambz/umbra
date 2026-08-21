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
#include <wincodec.h>
#include <wrl/client.h>

#include <string>
#include <vector>

#include "engines/frame_scheduler.h"
#include "engines/wallpaper_engine.h"
#include "render/fit_rect.h"

namespace umbra {

// Decodes a still image or animated GIF/APNG via WIC, up front, into one
// D3D11 texture per frame — animated images in practice have few enough
// frames (tens, not thousands) that decoding them all at load time and
// picking one per advance() is far simpler than a streaming decoder, and
// avoids re-decoding the same loop over and over. Windows-only, verified
// manually against a live desktop session (see TESTING.md) — which frame
// to show at a given time is delegated to FrameScheduler, which is
// unit-tested on its own.
class ImageEngine : public IWallpaperEngine {
   public:
    // device must outlive this ImageEngine. Throws std::runtime_error if
    // the file can't be decoded.
    ImageEngine(ID3D11Device* device, const std::string& path);

    ImageEngine(const ImageEngine&) = delete;
    ImageEngine& operator=(const ImageEngine&) = delete;

    void advance(double deltaSeconds) override;
    ID3D11ShaderResourceView* currentFrame() const override;
    Size frameSize() const override { return frameSize_; }

   private:
    void decodeFrames(const std::string& path);

    Microsoft::WRL::ComPtr<ID3D11Device> device_;
    std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> frames_;
    Size frameSize_;
    FrameScheduler scheduler_{{}};
};

}  // namespace umbra
