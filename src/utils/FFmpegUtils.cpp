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

#include "FFmpegUtils.h"
#include "ffmovie/movie.h"
extern "C" {
#include <libavutil/log.h>
}

namespace ffmovie {

const char* tav_default_item_name(void* ptr) {
  return av_default_item_name(ptr);
}

void tav_log_set_callback(void (*callback)(void*, int, const char*, va_list)) {
  av_log_set_callback(callback);
}

std::vector<std::string> FFMediaDemuxer::SupportDemuxers() {
  // 打印ffmpeg配置
  const AVInputFormat* fmt = NULL;
  void* demuxerIndex = 0;
  std::vector<std::string> names;
  while ((fmt = av_demuxer_iterate(&demuxerIndex))) {
    names.emplace_back(fmt->name);
  }
  return names;
}

std::vector<std::string> FFMediaDecoder::SupportDecoders() {
  // 打印ffmpeg配置
  void* codecIndex = 0;
  std::vector<std::string> names;
  const AVCodec* codec = NULL;
  while ((codec = av_codec_iterate(&codecIndex))) {
    names.emplace_back(codec->name);
  }
  return names;
}

std::string AVCodecIDToStringVideo(AVCodecID codecId) {
  std::string codecIDString;
  switch (codecId) {
    case AVCodecID::AV_CODEC_ID_H264:
      codecIDString = MIMETYPE_VIDEO_AVC;
      break;
    case AVCodecID::AV_CODEC_ID_HEVC:
      codecIDString = MIMETYPE_VIDEO_HEVC;
      break;
    case AVCodecID::AV_CODEC_ID_VP8:
      codecIDString = MIMETYPE_VIDEO_VP8;
      break;
    case AVCodecID::AV_CODEC_ID_VP9:
      codecIDString = MIMETYPE_VIDEO_VP9;
      break;
    case AVCodecID::AV_CODEC_ID_GIF:
      codecIDString = MIMETYPE_VIDEO_GIF;
      break;
    default:
      codecIDString = MIMETYPE_UNKNOWN;
      break;
  }
  return codecIDString;
}

std::string AVCodecIDToStringAudio(AVCodecID codecId) {
  std::string codecIDString;
  switch (codecId) {
    case AVCodecID::AV_CODEC_ID_AAC:
      codecIDString = MIMETYPE_AUDIO_AAC;
      break;
    case AVCodecID::AV_CODEC_ID_MP3:
      codecIDString = MIMETYPE_AUDIO_MP3;
      break;
    case AVCodecID::AV_CODEC_ID_VORBIS:
      codecIDString = MIMETYPE_AUDIO_VORBIS;
      break;
    case AVCodecID::AV_CODEC_ID_OPUS:
      codecIDString = MIMETYPE_AUDIO_OPUS;
      break;
    case AVCodecID::AV_CODEC_ID_PCM_S16LE:
      codecIDString = MIMETYPE_AUDIO_PCM_S16LE;
      break;
    case AVCodecID::AV_CODEC_ID_PCM_S16BE:
      codecIDString = MIMETYPE_AUDIO_PCM_S16BE;
      break;
    case AVCodecID::AV_CODEC_ID_PCM_F32LE:
      codecIDString = MIMETYPE_AUDIO_PCM_F32LE;
      break;
    case AVCodecID::AV_CODEC_ID_PCM_F32BE:
      codecIDString = MIMETYPE_AUDIO_PCM_F32BE;
      break;
    case AVCodecID::AV_CODEC_ID_AC3:
      codecIDString = MIMETYPE_AUDIO_AC3;
      break;
    case AVCodecID::AV_CODEC_ID_AMR_NB:
      codecIDString = MIMETYPE_AUDIO_AMR_NB;
      break;
    case AVCodecID::AV_CODEC_ID_AMR_WB:
      codecIDString = MIMETYPE_AUDIO_AMR_WB;
      break;
    case AVCodecID::AV_CODEC_ID_PCM_MULAW:
      codecIDString = MIMETYPE_AUDIO_PCM_MULAW;
      break;
    case AVCodecID::AV_CODEC_ID_PCM_U8:
      codecIDString = MIMETYPE_AUDIO_PCM_U8;
      break;
    default:
      codecIDString = MIMETYPE_UNKNOWN;
      break;
  }
  return codecIDString;
}

AVCodecID MineStringToVideoAVCodecID(const std::string& mime) {
  AVCodecID codecId = AVCodecID::AV_CODEC_ID_NONE;
  if (mime.empty()) {
    return codecId;
  }
  if (mime == MIMETYPE_VIDEO_AVC) {
    codecId = AVCodecID::AV_CODEC_ID_H264;
  } else if (mime == MIMETYPE_VIDEO_HEVC) {
    codecId = AVCodecID::AV_CODEC_ID_HEVC;
  } else if (mime == MIMETYPE_VIDEO_VP8) {
    codecId = AVCodecID::AV_CODEC_ID_VP8;
  } else if (mime == MIMETYPE_VIDEO_VP9) {
    codecId = AVCodecID::AV_CODEC_ID_VP9;
  } else if (mime == MIMETYPE_VIDEO_GIF) {
    codecId = AVCodecID::AV_CODEC_ID_GIF;
  }
  return codecId;
}

AVCodecID MineStringToAudioAVCodecID(const std::string& mime) {
  AVCodecID codecId = AVCodecID::AV_CODEC_ID_NONE;
  if (mime.empty()) {
    return codecId;
  }
  if (mime == MIMETYPE_AUDIO_AAC) {
    codecId = AVCodecID::AV_CODEC_ID_AAC;
  } else if (mime == MIMETYPE_AUDIO_MP3) {
    codecId = AVCodecID::AV_CODEC_ID_MP3;
  } else if (mime == MIMETYPE_AUDIO_VORBIS) {
    codecId = AVCodecID::AV_CODEC_ID_VORBIS;
  } else if (mime == MIMETYPE_AUDIO_OPUS) {
    codecId = AVCodecID::AV_CODEC_ID_OPUS;
  } else if (mime == MIMETYPE_AUDIO_PCM_S16LE) {
    codecId = AVCodecID::AV_CODEC_ID_PCM_S16LE;
  } else if (mime == MIMETYPE_AUDIO_PCM_S16BE) {
    codecId = AVCodecID::AV_CODEC_ID_PCM_S16BE;
  } else if (mime == MIMETYPE_AUDIO_PCM_F32LE) {
    codecId = AVCodecID::AV_CODEC_ID_PCM_F32LE;
  } else if (mime == MIMETYPE_AUDIO_PCM_F32BE) {
    codecId = AVCodecID::AV_CODEC_ID_PCM_F32BE;
  } else if (mime == MIMETYPE_AUDIO_AC3) {
    codecId = AVCodecID::AV_CODEC_ID_AC3;
  } else if (mime == MIMETYPE_AUDIO_AMR_NB) {
    codecId = AVCodecID::AV_CODEC_ID_AMR_NB;
  } else if (mime == MIMETYPE_AUDIO_AMR_WB) {
    codecId = AVCodecID::AV_CODEC_ID_AMR_WB;
  } else if (mime == MIMETYPE_AUDIO_PCM_MULAW) {
    codecId = AVCodecID::AV_CODEC_ID_PCM_MULAW;
  } else if (mime == MIMETYPE_AUDIO_PCM_U8) {
    codecId = AVCodecID::AV_CODEC_ID_PCM_U8;
  }
  return codecId;
}

AVPacket* CreateAVPacket(EncodePacket* encodePacket) {
  AVPacket* packet = av_packet_alloc();
  int error = av_new_packet(packet, encodePacket->data->length());
  if (error != 0) {
    av_packet_free(&packet);
    return nullptr;
  }
  packet->flags = encodePacket->isKeyFrame;
  packet->pts = encodePacket->pts;
  packet->dts = encodePacket->dts;
  memcpy(packet->data, encodePacket->data->data(), encodePacket->data->length());
  return packet;
}
}  // namespace ffmovie