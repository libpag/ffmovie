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

#include "FFmpegAudioDecoder.h"

namespace ffmovie {
std::unique_ptr<FFAudioDecoder> FFAudioDecoder::Make(FFMediaDemuxer* demuxer,
                                                     std::shared_ptr<AudioOutputConfig> config) {
  if (demuxer == nullptr || config == nullptr) {
    return nullptr;
  }
  auto decoder = std::unique_ptr<FFmpegAudioDecoder>(new FFmpegAudioDecoder());
  if (!decoder->onConfigure(demuxer, std::move(config))) {
    return nullptr;
  }
  return decoder;
}

FFmpegAudioDecoder::~FFmpegAudioDecoder() {
  delete converter;
  avcodec_free_context(&avCodecContext);
  av_frame_free(&frame);
  av_packet_free(&packet);
}

pag::DecoderResult FFmpegAudioDecoder::onEndOfStream() {
  return onSendBytes(nullptr, 0, -1);
}

pag::DecoderResult FFmpegAudioDecoder::onSendBytes(void* bytes, size_t length, int64_t time) {
  packet->data = static_cast<uint8_t*>(bytes);
  packet->size = static_cast<int>(length);
  packet->pts = time;
  int result = avcodec_send_packet(avCodecContext, packet);
  if (result >= 0 || result == AVERROR_EOF) {
    return pag::DecoderResult::Success;
  } else if (result == AVERROR(EAGAIN)) {
    return pag::DecoderResult::TryAgainLater;
  } else {
    return pag::DecoderResult::Error;
  }
}

pag::DecoderResult FFmpegAudioDecoder::onDecodeFrame() {
  auto result = avcodec_receive_frame(avCodecContext, frame);
  if (result == 0 && frame->data[0] != nullptr) {
    return pag::DecoderResult::Success;
  } else if (result == AVERROR(EAGAIN)) {
    return pag::DecoderResult::TryAgainLater;
  } else if (result == AVERROR_EOF) {
    return pag::DecoderResult::Success;
  } else {
    return pag::DecoderResult::Error;
  }
}

void FFmpegAudioDecoder::onFlush() {
  avcodec_flush_buffers(avCodecContext);
}

SampleData FFmpegAudioDecoder::onRenderFrame() {
  if (IsSampleConfig(*outputConfig, *frame)) {
    return SampleData(frame->data[0], frame->linesize[0]);
  } else {
    if (converter == nullptr) {
      converter = new AudioFormatConverter(outputConfig);
    }
    return converter->convert(frame);
  }
}

int64_t FFmpegAudioDecoder::currentPresentationTime() {
  if (frame == nullptr) {
    return -1;
  }
  return frame->pts;
}

bool FFmpegAudioDecoder::onConfigure(FFMediaDemuxer* demuxer,
                                     std::shared_ptr<AudioOutputConfig> config) {
  auto trackIndex = demuxer->getCurrentTrackIndex();
  auto mediaFormat = demuxer->getTrackFormat(trackIndex);
  auto dec =
      avcodec_find_decoder(static_cast<AVCodecParameters*>(mediaFormat->getCodecPar())->codec_id);
  if (!dec) {
    return false;
  }
  avCodecContext = avcodec_alloc_context3(dec);
  if (!avCodecContext) {
    return false;
  }
  if (avcodec_parameters_to_context(
          avCodecContext, static_cast<AVCodecParameters*>(mediaFormat->getCodecPar())) < 0) {
    return false;
  }
  AVDictionary* opts = nullptr;
  av_dict_set(&opts, "refcounted_frames", "0", 0);
  if (avcodec_open2(avCodecContext, dec, &opts) < 0) {
    return false;
  }
  frame = av_frame_alloc();
  if (!frame) {
    return false;
  }
  packet = av_packet_alloc();
  if (!packet) {
    return -1;
  }
  outputConfig = std::make_shared<PCMOutputConfig>();
  outputConfig->channels = config->channels;
  outputConfig->sampleRate = config->sampleRate;
  outputConfig->outputSamplesCount = config->outputSamplesCount;
  outputConfig->channelLayout = config->channels == 2 ? AV_CH_LAYOUT_STEREO : AV_CH_LAYOUT_MONO;
  return true;
}

}  // namespace ffmovie
