///////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2021 Tencent. All rights reserved.
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
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>

#ifdef __cplusplus
}
#endif

#include <memory>
#include "audio/AudioUtils.h"
#include "ffmovie/movie.h"

#define AUDIO_OUT_SAMPLES 1024
#define AUDIO_OUT_FORMAT AV_SAMPLE_FMT_S16
#define AUDIO_SAMPLE_BYTE 2
#define AUDIO_CHANNELS 2
#define AUDIO_SAMPLE_RATE 44100
#define AUDIO_BIT_RATE_BPS 128000
#define AUDIO_ENCODE_DEFAULT_SAMPLES 10000

namespace ffmovie {

class FFmpegAudioEncoder : public FFAudioEncoder {
 public:
  explicit FFmpegAudioEncoder(const ExportConfig& config);
  ~FFmpegAudioEncoder() override;
  bool initEncoder() override;
  CodingResult onSendData(uint8_t *data, int64_t length, int sampleCount) override;
  CodingResult onEncodeData(void **packet) override;
  CodingResult onEndOfStream() override;
  std::shared_ptr<MediaFormat> getMediaFormat() override;
  void insertErrorMsgs(std::vector<std::string> *const toMsgs) override;

 private:
  bool initCodecContext();
  bool initResampleContext(AVFrame* avFrame);
  CodingResult sendFrame(AVFrame* audioFrame);
  void initEncodeSampleFormat();
  void initEncodeSampleRate();
  void initEncodeChannelLayout();
  bool createFrame();
  bool createSrcFrame();

  ExportConfig audioEncoderConfig;
  AVCodecID codecID = AVCodecID::AV_CODEC_ID_AAC;
  AVCodecContext* codecContext = nullptr;
  AVCodec* avCodec = nullptr;
  struct SwrContext* swrContext = nullptr;
  PCMOutputConfig lastAudioFrameConfig;
  int lastAudioFormat = AV_SAMPLE_FMT_NONE;
  AVPacket* packet = nullptr;
  AVFrame* frame = nullptr;
  AVFrame* srcFrame = nullptr;
  int encodedSamplesCount = 0;
  bool needResample;
  std::vector<std::string> msgs;
};
}  // namespace pag
