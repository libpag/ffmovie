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
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>

#ifdef __cplusplus
}
#endif

#include <memory>
#include "ffmovie/movie.h"

#define VIDEO_BIT_RATE_BPS 8000000
#define VIDEO_FRAME_RATE 30
#define VIDEO_WIDTH 1280
#define VIDEO_HEIGHT 720
#define VIDEO_ENCODE_THREAD_COUNT 5
#define STREAM_PIX_FMT AV_PIX_FMT_YUV420P
#define VIDEO_GOP_SIZE 120

namespace ffmovie {

class FFmpegVideoEncoder : public FFVideoEncoder {
 public:
  explicit FFmpegVideoEncoder(const ExportConfig& config);
  ~FFmpegVideoEncoder() override;
  bool initEncoder() override;
  CodingResult onSendData(std::unique_ptr<ByteData> rgbaData, int width, int height, int rowBytes,
                          int64_t pts) override;
  CodingResult onEndOfStream() override;
  CodingResult onEncodeData(void** packet) override;
  std::shared_ptr<MediaFormat> getMediaFormat() override;
  void collectErrorMsgs(std::vector<std::string>* const toMsgs) override;

 private:
  bool initCodec();
  bool initCodecContext();

  CodingResult sendFrame(AVFrame* videoFrame);
  ExportConfig videoEncoderConfig;
  AVCodecID codecId = AVCodecID::AV_CODEC_ID_H264;
  AVCodecContext* codecContext = nullptr;
  AVCodec* avCodec = nullptr;
  AVPacket* packet = nullptr;
  std::vector<std::string> msgs;

  friend class FFmpegVideoExport;
};

}  // namespace ffmovie
