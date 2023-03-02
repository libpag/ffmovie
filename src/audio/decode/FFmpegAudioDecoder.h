/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Tencent is pleased to support the open source community by making libpag
//  available.
//
//  Copyright (C) 2021 THL A29 Limited, a Tencent company. All rights reserved.
//
//  Licensed under the Apache License, Version 2.0 (the "License"); you may not
//  use this file except in compliance with the License. You may obtain a copy
//  of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  unless required by applicable law or agreed to in writing, software
//  distributed under the license is distributed on an "as is" basis, without
//  warranties or conditions of any kind, either express or implied. see the
//  license for the specific language governing permissions and limitations
//  under the license.
//
/////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

#ifdef __cplusplus
}
#endif

#include "audio/AudioUtils.h"
#include "audio/process/AudioFormatConverter.h"
#include "ffmovie/movie.h"

namespace ffmovie {
class FFmpegAudioDecoder : public FFAudioDecoder {
 public:
  ~FFmpegAudioDecoder() override;

  bool onConfigure(FFMediaDemuxer* demuxer, std::shared_ptr<AudioOutputConfig> config);

  pag::DecoderResult onEndOfStream() override;

  pag::DecoderResult onSendBytes(void* bytes, size_t length, int64_t time) override;

  pag::DecoderResult onDecodeFrame() override;

  void onFlush() override;

  SampleData onRenderFrame() override;

  int64_t currentPresentationTime() override;

 private:
  std::shared_ptr<PCMOutputConfig> outputConfig = nullptr;
  AudioFormatConverter* converter = nullptr;
  AVPacket* packet = nullptr;
  AVFrame* frame = nullptr;
  AVCodecContext* avCodecContext = nullptr;
};
}  // namespace pag
