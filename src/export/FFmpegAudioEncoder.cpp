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

#include "FFmpegAudioEncoder.h"
#include "utils/StringUtils.h"

namespace ffmovie {
std::unique_ptr<FFAudioEncoder> FFAudioEncoder::Make(const ExportConfig& config) {
  return std::unique_ptr<FFmpegAudioEncoder>(new FFmpegAudioEncoder(config));
}

FFmpegAudioEncoder::~FFmpegAudioEncoder() {
  if (codecContext) {
    avcodec_free_context(&codecContext);
  }
  if (swrContext) {
    swr_free(&swrContext);
  }
  if (frame) {
    av_frame_free(&frame);
  }
  if (packet) {
    av_packet_free(&packet);
  }
  if (srcFrame) {
    av_frame_free(&srcFrame);
  }
}

bool FFmpegAudioEncoder::initEncoder() {
  avCodec = avcodec_find_encoder(codecID);
  if (!avCodec) {
    msgs.emplace_back(string_format("Could not find encoder for '%s'", avcodec_get_name(codecID)));
    return false;
  }

  if (!initCodecContext()) {
    msgs.emplace_back("AudioEncoder: initCodecContext failed.");
    return false;
  }

  if (avcodec_open2(codecContext, avCodec, NULL) < 0) {
    msgs.emplace_back("Could not open codec");
    return false;
  }

  packet = av_packet_alloc();
  if (!packet) {
    msgs.emplace_back("AudioEncoder: alloc packet failed.");
    return false;
  }

  if (!createFrame()) {
    return false;
  }

  if (!createSrcFrame()) {
    return false;
  }
  return true;
}

CodingResult FFmpegAudioEncoder::onSendData(uint8_t* data, int64_t length, int sampleCount) {
  srcFrame->data[0] = data;
  srcFrame->linesize[0] = length;
  srcFrame->nb_samples = sampleCount;
  if (srcFrame != nullptr && srcFrame->sample_rate != codecContext->sample_rate) {
    msgs.emplace_back("AudioEncoder: src Frame invalid.");
    return CodingResult::CodingError;
  }

  needResample = (srcFrame->channels != codecContext->channels) ||
                 (srcFrame->sample_rate != codecContext->sample_rate) ||
                 (srcFrame->format != codecContext->sample_fmt);
  if (needResample && !initResampleContext(srcFrame)) {
    msgs.emplace_back("AudioEncoder: initResampleContext failed.");
    return CodingResult::CodingError;
  }

  if (needResample) {
    /* convert to destination format */
    int convertedSamples = swr_convert(swrContext, frame->data, frame->nb_samples,
                                       (const uint8_t**)(srcFrame->data), srcFrame->nb_samples);
    if (convertedSamples < 0) {
      msgs.emplace_back("Error while converting");
      return CodingResult::CodingError;
    }

    return sendFrame(frame);
  } else {
    return sendFrame(srcFrame);
  }
}

CodingResult FFmpegAudioEncoder::onEndOfStream() {
  return sendFrame(nullptr);
}

CodingResult FFmpegAudioEncoder::sendFrame(AVFrame* audioFrame) {
  int encodeNbSamples = 0;
  if (audioFrame) {
    encodeNbSamples = audioFrame->nb_samples;
    audioFrame->pts =
        av_rescale_q(encodedSamplesCount, {1, codecContext->sample_rate}, codecContext->time_base);
  }

  int ret = avcodec_send_frame(codecContext, audioFrame);
  if (ret < 0) {
    msgs.emplace_back("Error encoding audio frame!!!");
    return CodingResult::CodingError;
  }

  encodedSamplesCount += encodeNbSamples;
  return CodingResult::CodingSuccess;
}

CodingResult FFmpegAudioEncoder::onEncodeData(void** encodedPacket) {
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

FFmpegAudioEncoder::FFmpegAudioEncoder(const ExportConfig& config)
    : audioEncoderConfig(std::move(config)) {
  lastAudioFrameConfig.channels = -1;
  lastAudioFrameConfig.sampleRate = -1;
  lastAudioFormat = AV_SAMPLE_FMT_NONE;
}

bool FFmpegAudioEncoder::initCodecContext() {
  codecContext = avcodec_alloc_context3(avCodec);
  if (!codecContext) {
    msgs.emplace_back("Could not alloc an encoding context");
    return false;
  }

  initEncodeSampleFormat();
  initEncodeSampleRate();
  initEncodeChannelLayout();
  codecContext->bit_rate = audioEncoderConfig.audioBitrate;
  return true;
}

void FFmpegAudioEncoder::initEncodeSampleFormat() {
  codecContext->sample_fmt = AVSampleFormat::AV_SAMPLE_FMT_S16;
  if (!avCodec->sample_fmts || avCodec->sample_fmts[0] <= AV_SAMPLE_FMT_NONE) {
    msgs.emplace_back("no supported sample format");
    return;
  }

  codecContext->sample_fmt = avCodec->sample_fmts[0];
  for (int index = 0; avCodec->sample_fmts[index] > AV_SAMPLE_FMT_NONE; index++) {
    if (avCodec->sample_fmts[index] == static_cast<int>(AVSampleFormat::AV_SAMPLE_FMT_S16)) {
      codecContext->sample_fmt = avCodec->sample_fmts[index];
      break;
    }
  }

  return;
}

void FFmpegAudioEncoder::initEncodeSampleRate() {
  codecContext->sample_rate = audioEncoderConfig.sampleRate;
  if (!avCodec->supported_samplerates || avCodec->supported_samplerates[0] <= 0) {
    msgs.emplace_back("no supported sample rate");
    return;
  }

  codecContext->sample_rate = avCodec->supported_samplerates[0];
  for (int index = 0; avCodec->supported_samplerates[index] > 0; index++) {
    if (avCodec->supported_samplerates[index] == audioEncoderConfig.sampleRate) {
      codecContext->sample_rate = avCodec->supported_samplerates[index];
      break;
    }
  }
}

void FFmpegAudioEncoder::initEncodeChannelLayout() {
  codecContext->channels = audioEncoderConfig.channels;
  codecContext->channel_layout = av_get_default_channel_layout(audioEncoderConfig.channels);
  if (!avCodec->channel_layouts || avCodec->channel_layouts[0] == 0) {
    msgs.emplace_back("FFmpegAudioEncoder: no supported channel layout");
    return;
  }

  codecContext->channel_layout = avCodec->channel_layouts[0];
  for (int index = 0; avCodec->channel_layouts[index] > 0; index++) {
    if (static_cast<int>(avCodec->channel_layouts[index]) ==
        av_get_channel_layout_nb_channels(audioEncoderConfig.channels)) {
      codecContext->channel_layout = avCodec->channel_layouts[index];
      break;
    }
  }

  codecContext->channels = av_get_channel_layout_nb_channels(codecContext->channel_layout);
}

bool FFmpegAudioEncoder::initResampleContext(AVFrame* avFrame) {
  bool isSameConfigWithLastAudioFrame = (lastAudioFrameConfig.channels == avFrame->channels) &&
                                        (lastAudioFrameConfig.sampleRate == avFrame->sample_rate) &&
                                        (lastAudioFormat == avFrame->format);
  if (isSameConfigWithLastAudioFrame && swrContext) {
    return true;
  }

  if (swrContext) {
    swr_free(&swrContext);
  }
  swrContext = swr_alloc();
  if (!swrContext) {
    msgs.emplace_back("Could not allocate resampler context");
    return false;
  }

  av_opt_set_int(swrContext, "in_channel_count", avFrame->channels, 0);
  av_opt_set_int(swrContext, "in_sample_rate", avFrame->sample_rate, 0);
  av_opt_set_sample_fmt(swrContext, "in_sample_fmt", static_cast<AVSampleFormat>(avFrame->format),
                        0);
  av_opt_set_int(swrContext, "out_channel_count", codecContext->channels, 0);
  av_opt_set_int(swrContext, "out_sample_rate", codecContext->sample_rate, 0);
  av_opt_set_sample_fmt(swrContext, "out_sample_fmt", codecContext->sample_fmt, 0);

  int ret = swr_init(swrContext);
  if (ret < 0) {
    msgs.emplace_back("Failed to initialize the resampling context");
    if (swrContext) {
      swr_free(&swrContext);
    }
    return false;
  }

  lastAudioFormat = avFrame->format;
  lastAudioFrameConfig.sampleRate = avFrame->sample_rate;
  lastAudioFrameConfig.channels = avFrame->channels;
  return true;
}

bool FFmpegAudioEncoder::createFrame() {
  frame = av_frame_alloc();
  if (!frame) {
    msgs.emplace_back("Error allocating an audio frame");
    return false;
  }

  frame->nb_samples = (codecContext->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE) == 0
                          ? codecContext->frame_size
                          : AUDIO_ENCODE_DEFAULT_SAMPLES;
  frame->format = codecContext->sample_fmt;
  frame->channel_layout = codecContext->channel_layout;
  frame->sample_rate = codecContext->sample_rate;
  frame->channels = codecContext->channels;

  int ret = av_frame_get_buffer(frame, 0);
  if (ret < 0) {
    msgs.emplace_back("Error allocating an audio buffer");
    return false;
  }

  return true;
}

bool FFmpegAudioEncoder::createSrcFrame() {
  srcFrame = av_frame_alloc();
  if (!srcFrame) {
    msgs.emplace_back("Error allocating an audio frame");
    return false;
  }
  srcFrame->nb_samples = DEFAULT_OUTPUT_SAMPLE_COUNT;
  srcFrame->channel_layout = av_get_default_channel_layout(audioEncoderConfig.channels);
  srcFrame->sample_rate = audioEncoderConfig.sampleRate;
  srcFrame->channels = audioEncoderConfig.channels;
  srcFrame->format = AUDIO_OUT_FORMAT;
  int ret = av_frame_get_buffer(srcFrame, 0);
  if (ret < 0) {
    msgs.emplace_back(string_format("Error allocating an audio buffer, ret is %d", ret));
    return false;
  }

  return true;
}

std::shared_ptr<MediaFormat> FFmpegAudioEncoder::getMediaFormat() {
  auto trackFormat = std::make_shared<MediaFormat>();
  trackFormat->setInteger(KEY_TRACK_TYPE, AUDIO_TRACK);
  trackFormat->setInteger(KEY_AUDIO_FORMAT, codecContext->sample_fmt);
  trackFormat->setInteger(KEY_AUDIO_BITRATE, codecContext->bit_rate);
  trackFormat->setInteger(KEY_AUDIO_SAMPLE_RATE, codecContext->sample_rate);
  trackFormat->setInteger(KEY_AUDIO_CHANNELS, codecContext->channels);
  trackFormat->setInteger(KEY_AUDIO_FRAME_SIZE, codecContext->frame_size);
  //  trackFormat->setCodecContext(KEY_CODECCONTEXT, codecContext);
  trackFormat->setInteger(KEY_TIME_BASE_NUM, codecContext->time_base.num);
  trackFormat->setInteger(KEY_TIME_BASE_DEN, codecContext->time_base.den);

  std::vector<std::shared_ptr<ByteData>> headers{};
  auto extraDataSize = codecContext->extradata_size;
  auto extraData = codecContext->extradata;
  auto data = new uint8_t[extraDataSize];
  memcpy(data, extraData, extraDataSize);
  headers.push_back(std::shared_ptr<ByteData>(ByteData::MakeAdopted(data, extraDataSize)));
  trackFormat->setHeaders(headers);
  return trackFormat;
}

void FFmpegAudioEncoder::insertErrorMsgs(std::vector<std::string>* const toMsgs) {
  if (toMsgs == nullptr) {
    return;
  }
  toMsgs->insert(toMsgs->end(), msgs.begin(), msgs.end());
}

}  // namespace ffmovie
