///////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2023 Tencent. All rights reserved.
//
//  This library is free software; you can redistribute it and/or modify it under the terms of the
//  GNU Lesser General Public License as published by the Free Software Foundation; either
//  version 2.1 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
//  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
//  the GNU Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public License along with this
//  library; if not, write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
//  Boston, MA  02110-1301  USA
//
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <libavformat/avformat.h>
#include <libswresample/swresample.h>

#ifdef __cplusplus
}
#endif

#include "audio/AudioUtils.h"
#include "ffmovie/movie.h"

namespace ffmovie {
class AudioFormatConverter {
 public:
  explicit AudioFormatConverter(std::shared_ptr<PCMOutputConfig> pcmOutputConfig);

  ~AudioFormatConverter();

  SampleData convert(AVFrame* frame);

 private:
  uint8_t* pConvertBuff = nullptr;
  int outputSamples = 0;
  SwrContext* pSwrContext = nullptr;
  std::shared_ptr<PCMOutputConfig> pcmOutputConfig;
  std::shared_ptr<PCMOutputConfig> preFramePCMOutputConfig;
};
}  // namespace ffmovie
