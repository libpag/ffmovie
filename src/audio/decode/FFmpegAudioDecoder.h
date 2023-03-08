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

  DecoderResult onEndOfStream() override;

  DecoderResult onSendBytes(void* bytes, size_t length, int64_t time) override;

  DecoderResult onDecodeFrame() override;

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
