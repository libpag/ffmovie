/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Tencent is pleased to support the open source community by making libpag available.
//
//  Copyright (C) 2021 THL A29 Limited, a Tencent company. All rights reserved.
//
//  Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
//  except in compliance with the License. You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  unless required by applicable law or agreed to in writing, software distributed under the
//  license is distributed on an "as is" basis, without warranties or conditions of any kind,
//  either express or implied. see the license for the specific language governing permissions
//  and limitations under the license.
//
/////////////////////////////////////////////////////////////////////////////////////////////////

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
