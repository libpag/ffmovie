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

#include "FFmpegVideoEncoder.h"
#include "libyuv/convert.h"
#include "utils/StringUtils.h"

namespace ffmovie {

std::unique_ptr<FFVideoEncoder> FFVideoEncoder::Make(const ExportConfig& config) {
  return std::unique_ptr<FFmpegVideoEncoder>(new FFmpegVideoEncoder(config));
}

FFmpegVideoEncoder::~FFmpegVideoEncoder() {
  if (codecContext) {
    avcodec_close(codecContext);
    avcodec_free_context(&codecContext);
  }

  if (packet) {
    av_packet_free(&packet);
  }
}

bool FFmpegVideoEncoder::initEncoder() {
  if (!initCodec()) {
    msgs.emplace_back("FFmpegVideoEncoder: initCodec failed.");
    return false;
  }

  if (!initCodecContext()) {
    msgs.emplace_back("FFmpegVideoEncoder: initailed.");
    return false;
  }

  packet = av_packet_alloc();
  if (!packet) {
    msgs.emplace_back("FFmpegVideoEncoder: create packet failed.");
    return false;
  }
  int ret = avcodec_open2(codecContext, avCodec, nullptr);
  if (ret < 0) {
    char errorbuf[1024] = {0};
    av_strerror(ret, errorbuf, sizeof(errorbuf));
    msgs.emplace_back(
        string_format("FFmpegVideoEncoder: open codec failed, ret = %d, msg = %s", ret, errorbuf));
    return false;
  }
  return true;
}

CodingResult FFmpegVideoEncoder::onSendData(std::unique_ptr<ByteData> rgbaData, int width,
                                            int height, int rowBytes, int64_t pts) {
  AVFrame* encodeFrame = av_frame_alloc();
  encodeFrame->width = width;
  encodeFrame->height = height;
  encodeFrame->format = AV_PIX_FMT_YUV420P;
  int ret = av_frame_get_buffer(encodeFrame, 0);
  if (ret < 0) {
    return CodingResult::CodingError;
  }
  // convert
  // RGBAToI420, 内存顺序是RGBA,所以用方法得反过来ARGB
  libyuv::ABGRToI420(rgbaData->data(), rowBytes, encodeFrame->data[0], encodeFrame->linesize[0],
                     encodeFrame->data[1], encodeFrame->linesize[1], encodeFrame->data[2],
                     encodeFrame->linesize[2], width, height);
  ret = av_frame_make_writable(encodeFrame);
  if (ret < 0) {
    return CodingResult::CodingError;
  }
  encodeFrame->pts = pts;
  return sendFrame(encodeFrame);
}

CodingResult FFmpegVideoEncoder::onEndOfStream() {
  return sendFrame(nullptr);
}

CodingResult FFmpegVideoEncoder::onEncodeData(void** encodedPacket) {
  auto result = avcodec_receive_packet(codecContext, packet);
  if (result == 0) {
    *encodedPacket = packet;
    return CodingResult::CodingSuccess;
  } else if (result == AVERROR(EAGAIN)) {
    return CodingResult::TryAgainLater;
  } else if (result == AVERROR_EOF) {
    return CodingResult::EndOfStream;
  } else {
    return CodingResult::CodingError;
  }
}

FFmpegVideoEncoder::FFmpegVideoEncoder(const ExportConfig& config)
    : videoEncoderConfig(std::move(config)) {
}

bool FFmpegVideoEncoder::initCodec() {
  avCodec = avcodec_find_encoder(codecId);
  if (!avCodec) {
    msgs.emplace_back(string_format("Could not find encoder for '%s'", avcodec_get_name(codecId)));
    return false;
  }
  return true;
}

bool FFmpegVideoEncoder::initCodecContext() {
  codecContext = avcodec_alloc_context3(avCodec);
  if (!codecContext) {
    msgs.emplace_back("Could not alloc an encoding context");
    return false;
  }
  codecContext->width = videoEncoderConfig.width;
  codecContext->height = videoEncoderConfig.height;
  codecContext->bit_rate = videoEncoderConfig.videoBitrate;
  codecContext->thread_count = VIDEO_ENCODE_THREAD_COUNT;
  codecContext->pix_fmt = STREAM_PIX_FMT;
  codecContext->codec_id = codecId;
  codecContext->time_base = {1, static_cast<int>(videoEncoderConfig.frameRate)};
  codecContext->framerate = {static_cast<int>(videoEncoderConfig.frameRate), 1};
  codecContext->gop_size = VIDEO_GOP_SIZE;
  codecContext->colorspace = AVCOL_SPC_BT470BG;
  codecContext->max_b_frames = 0;
  codecContext->profile = videoEncoderConfig.profile == VideoProfile::BASELINE
                              ? FF_PROFILE_H264_BASELINE
                              : FF_PROFILE_H264_HIGH;
  return true;
}

CodingResult FFmpegVideoEncoder::sendFrame(AVFrame* videoFrame) {
  int ret = avcodec_send_frame(codecContext, videoFrame);
  av_frame_free(&videoFrame);
  if (ret >= 0) {
    return CodingResult::CodingSuccess;
  } else if (ret == AVERROR(EAGAIN)) {
    return CodingResult::TryAgainLater;
  } else {
    msgs.emplace_back("Error encoding video frame!!!");
    return CodingResult::CodingError;
  }
}

std::shared_ptr<MediaFormat> FFmpegVideoEncoder::getMediaFormat() {
  auto trackFormat = std::make_shared<MediaFormat>();
  trackFormat->setInteger(KEY_TRACK_TYPE, VIDEO_TRACK);
  trackFormat->setInteger(KEY_WIDTH, codecContext->width);
  trackFormat->setInteger(KEY_HEIGHT, codecContext->height);
  trackFormat->setFloat(KEY_FRAME_RATE, codecContext->framerate.num / codecContext->framerate.den);
  trackFormat->setInteger(KEY_TIME_BASE_NUM, codecContext->time_base.num);
  trackFormat->setInteger(KEY_TIME_BASE_DEN, codecContext->time_base.den);
  trackFormat->setInteger(KEY_VIDEO_BITRATE, codecContext->bit_rate);

  std::vector<std::shared_ptr<ByteData>> headers{};
  headers.push_back(ByteData::MakeCopy(codecContext->extradata, codecContext->extradata_size));
  trackFormat->setHeaders(headers);
  return trackFormat;
}

void FFmpegVideoEncoder::collectErrorMsgs(std::vector<std::string>* const toMsgs) {
  if (toMsgs == nullptr) {
    return;
  }
  toMsgs->insert(toMsgs->end(), msgs.begin(), msgs.end());
}

}  // namespace ffmovie
