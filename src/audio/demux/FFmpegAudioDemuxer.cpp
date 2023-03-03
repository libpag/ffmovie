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

#include "FFmpegAudioDemuxer.h"
#include "utils/FFmpegUtils.h"

namespace ffmovie {

static const AVRational TimeBaseQ = {1, AV_TIME_BASE};

std::unique_ptr<FFAudioDemuxer> FFAudioDemuxer::Make(const std::string& path) {
  auto demuxer = std::unique_ptr<FFmpegAudioDemuxer>(new FFmpegAudioDemuxer());
  if (demuxer->open(path) < 0) {
    return nullptr;
  }
  return demuxer;
}

std::unique_ptr<FFAudioDemuxer> FFAudioDemuxer::Make(uint8_t* data, size_t length) {
  auto demuxer = std::unique_ptr<FFmpegAudioDemuxer>(new FFmpegAudioDemuxer());
  if (demuxer->open(data, length) < 0) {
    return nullptr;
  }
  return demuxer;
}

FFmpegAudioDemuxer::~FFmpegAudioDemuxer() {
  av_packet_unref(&avPacket);
  avformat_close_input(&fmtCtx);
  if (avioCtx) {
    // note: the internal buffer could have changed, and be != buffer
    av_freep(&avioCtx->buffer);
    avio_context_free(&avioCtx);
  }
  for (const auto& iter : formats) {
    delete iter.second;
  }
}

bool FFmpegAudioDemuxer::selectTrack(unsigned int index) {
  if (index >= fmtCtx->nb_streams) {
    return false;
  }
  currentStreamIndex = static_cast<int>(index);
  return true;
}

int FFmpegAudioDemuxer::getTrackCount() {
  return fmtCtx->nb_streams;
}

MediaFormat* FFmpegAudioDemuxer::getTrackFormat(unsigned int index) {
  auto iter = formats.find(index);
  if (iter != formats.end()) {
    return iter->second;
  }
  auto format = getTrackFormatInternal(index);
  if (format) {
    formats.insert(std::make_pair(index, format));
  }
  return format;
}

bool FFmpegAudioDemuxer::seekTo(int64_t timestamp) {
  if (currentStreamIndex < 0) {
    return false;
  }
  auto time_base = fmtCtx->streams[currentStreamIndex]->time_base;
  auto pos = av_rescale_q(timestamp, TimeBaseQ, time_base);
  auto ret = av_seek_frame(fmtCtx, currentStreamIndex, pos, AVSEEK_FLAG_ANY | AVSEEK_FLAG_BACKWARD);
  if (ret < 0) {
    return false;
  }
  return true;
}

bool FFmpegAudioDemuxer::advance() {
  if (currentStreamIndex < 0) {
    return false;
  }
  av_packet_unref(&avPacket);
  while (av_read_frame(fmtCtx, &avPacket) >= 0) {
    if (avPacket.stream_index != currentStreamIndex) {
      av_packet_unref(&avPacket);
      continue;
    }
    auto time_base = fmtCtx->streams[currentStreamIndex]->time_base;
    currentTime = av_rescale_q(avPacket.pts, time_base, TimeBaseQ);
    return true;
  }
  return false;
}

SampleData FFmpegAudioDemuxer::readSampleData() const {
  if (avPacket.data == nullptr) {
    return {};
  }
  return {avPacket.data, avPacket.size};
}

int64_t FFmpegAudioDemuxer::getSampleTime() {
  return currentTime;
}

int readPacket(void* opaque, uint8_t* buf, int bufSize) {
  auto bufferData = (FFmpegAudioDemuxer::BufferData*)opaque;
  bufSize = FFMIN(bufSize, static_cast<int>(bufferData->size));
  if (bufSize <= 0) {
    return AVERROR_EOF;
  }
  memcpy(buf, bufferData->ptr, bufSize);
  bufferData->ptr += bufSize;
  bufferData->size -= bufSize;
  return bufSize;
}

int64_t seekInBuffer(void* opaque, int64_t offset, int whence) {
  auto bufferData = static_cast<FFmpegAudioDemuxer::BufferData*>(opaque);
  int64_t ret = -1;
  switch (whence) {
    case AVSEEK_SIZE:
      ret = bufferData->fileSize;
      break;
    case SEEK_SET:
      bufferData->ptr = bufferData->originPtr + offset;
      bufferData->size = bufferData->fileSize - offset;
      ret = reinterpret_cast<int64_t>(bufferData->ptr);
      break;
    default:
      break;
  }
  return ret;
}

MediaFormat* FFmpegAudioDemuxer::getTrackFormatInternal(unsigned int index) {
  auto avStream = fmtCtx->streams[index];
  auto mediaType = avStream->codecpar->codec_type;
  if (mediaType != AVMEDIA_TYPE_AUDIO) {
    return nullptr;
  }
  auto trackFormat = new MediaFormat();
  trackFormat->setInteger(KEY_TRACK_ID, static_cast<int>(index));
  auto duration = av_rescale_q(avStream->duration, avStream->time_base, TimeBaseQ);
  trackFormat->setLong(KEY_DURATION, duration);
  trackFormat->setString(KEY_MIME, AVCodecIDToStringAudio(avStream->codecpar->codec_id));
  trackFormat->setCodecPar(avStream->codecpar);
  return trackFormat;
}

int FFmpegAudioDemuxer::open(const std::string& path) {
  auto ret = avformat_open_input(&fmtCtx, path.c_str(), nullptr, nullptr);
  if (ret < 0) {
    // LOGE("Could not open source file %s, %s\n", path, av_err2str(ret));
    return -2;
  }
  if (avformat_find_stream_info(fmtCtx, nullptr) < 0) {
    return -3;
  }
  return 0;
}

int FFmpegAudioDemuxer::open(uint8_t* data, size_t length) {
  bufferData.originPtr = data;
  bufferData.fileSize = length;
  bufferData.ptr = data;
  bufferData.size = length;
  size_t bufferSize = 32768;
  auto buffer = static_cast<unsigned char*>(av_malloc(bufferSize));
  avioCtx = avio_alloc_context(buffer, static_cast<int>(bufferSize), 0, &bufferData, readPacket,
                               nullptr, seekInBuffer);
  if (avioCtx == nullptr) {
    av_freep(&buffer);
    return -1;
  }

  fmtCtx = avformat_alloc_context();
  fmtCtx->pb = avioCtx;
  fmtCtx->avio_flags = AVFMT_FLAG_CUSTOM_IO;
  if (avformat_open_input(&fmtCtx, "", nullptr, nullptr) < 0) {
    return -2;
  }
  if (avformat_find_stream_info(fmtCtx, nullptr) < 0) {
    return -3;
  }
  return 0;
}

int FFmpegAudioDemuxer::getCurrentTrackIndex() {
  return currentStreamIndex;
}
}  // namespace ffmovie