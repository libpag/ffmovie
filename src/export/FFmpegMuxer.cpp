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

#include "FFmpegMuxer.h"
#include "utils/FFmpegUtils.h"
#include "utils/LockGuard.h"
#include "utils/StringUtils.h"

namespace ffmovie {

#define FF_DISABLE_DEPRECATION_WARNINGS \
  _Pragma("GCC diagnostic push") _Pragma("GCC diagnostic ignored \"-Wdeprecated-declarations\"")

std::shared_ptr<FFMediaMuxer> FFMediaMuxer::Make() {
  return std::make_shared<FFmpegMuxer>();
}

FFmpegMuxer::~FFmpegMuxer() {
  if (avFormatContext) {
    unInitMuxer();
  }
}

bool FFmpegMuxer::writeFrame(int streamIndex, void* frame) {
  LockGuard autoLock(locker);
  if (!frame) {
    msgs.emplace_back("packet error");
    return false;
  }

  auto iter = streamMap.find(streamIndex);
  if (iter == streamMap.end()) {
    return false;
  }
  auto localStream = iter->second;

  AVPacket* avPacket = static_cast<AVPacket*>(frame);

  auto ctxIter = ctxTimeBaseMap.find(streamIndex);
  if (ctxIter != ctxTimeBaseMap.end()) {
    auto timeBase = ctxIter->second;
    av_packet_rescale_ts(avPacket, timeBase, localStream->time_base);
  }
  avPacket->stream_index = streamIndex;
  /* Write the compressed frame to the media file. */
  int ret = av_interleaved_write_frame(avFormatContext, avPacket);

  return ret >= 0;
}

bool FFmpegMuxer::writeFrame(int streamIndex, std::shared_ptr<EncodePacket> packet) {
  AVPacket* avPacket = CreateAVPacket(packet.get());
  if (avPacket == nullptr) {
    return false;
  }
  auto result = writeFrame(streamIndex, avPacket);
  av_packet_free(&avPacket);
  return result;
}

bool FFmpegMuxer::start() {
  LockGuard autoLock(locker);
  /* open the output file, if needed */
  int ret = avOutputFormat->flags & AVFMT_NOFILE;
  if (ret == 0) {
    ret = avio_open(&avFormatContext->pb, movieOutputPath.c_str(), AVIO_FLAG_WRITE);
    if (ret < 0) {
      msgs.emplace_back(string_format("Could not open '%s'!!!", movieOutputPath.c_str()));
      return false;
    }
  }

  /* Write the stream header, if any. */
  ret = avformat_write_header(avFormatContext, &avDict);
  if (ret < 0) {
    msgs.emplace_back("Error occurred when opening output file");
    return false;
  }

  return true;
}

bool FFmpegMuxer::stop() {
  LockGuard autoLock(locker);
  int ret = av_write_trailer(avFormatContext);
  if (ret < 0) {
    msgs.emplace_back(string_format("Error occurred when write trailer, ret: %d", ret));
    return false;
  }
  return true;
}

bool FFmpegMuxer::initMuxer(const std::string& filePath) {
  LockGuard lockGuard(locker);
  movieOutputPath = filePath;
  av_dict_set(&avDict, "movflags", "faststart", 0);
  /* allocate the output media context */
  avformat_alloc_output_context2(&avFormatContext, nullptr, nullptr, movieOutputPath.c_str());
  if (!avFormatContext) {
    msgs.emplace_back("Could not deduce output format from file extension: using MPEG.");
    avformat_alloc_output_context2(&avFormatContext, nullptr, "mpeg", movieOutputPath.c_str());
  }
  if (!avFormatContext) {
    msgs.emplace_back("Could not deduce output format.");
    return false;
  }

  avOutputFormat = avFormatContext->oformat;
  av_dump_format(avFormatContext, 0, movieOutputPath.c_str(), 1);
  return true;
}

void FFmpegMuxer::unInitMuxer() {
  LockGuard autoLock(locker);

  av_dict_free(&avDict);
  if (avFormatContext && !(avOutputFormat->flags & AVFMT_NOFILE)) {
    avio_closep(&avFormatContext->pb);
  }
  avformat_free_context(avFormatContext);
  avFormatContext = nullptr;
}

int FFmpegMuxer::addTrack(std::shared_ptr<MediaFormat> mediaFormat) {
  LockGuard autoLock(locker);

  auto trackType = mediaFormat->getInteger(KEY_TRACK_TYPE);
  if (trackType != VIDEO_TRACK && trackType != AUDIO_TRACK) {
    msgs.emplace_back(string_format("UnKnow trackType: %d", trackType));
    return -1;
  }

  AVStream* stream = avformat_new_stream(avFormatContext, nullptr);
  if (!stream) {
    msgs.emplace_back("Could not allocate stream");
    return -1;
  }
  stream->id = avFormatContext->nb_streams - 1;

  int num = mediaFormat->getInteger(KEY_TIME_BASE_NUM);
  int den = mediaFormat->getInteger(KEY_TIME_BASE_DEN);
  auto codecContextTimeBase = AVRational{num, den};

  if (trackType == VIDEO_TRACK) {
    stream->codecpar->codec_id = AV_CODEC_ID_H264;
    stream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
    //    stream->codecpar->codec_tag = mediaFormat->getLong(KEY_VIDEO_CODEC_TAG);
    stream->codecpar->width = mediaFormat->getInteger(KEY_WIDTH);
    stream->codecpar->height = mediaFormat->getInteger(KEY_HEIGHT);
    stream->codecpar->format = AV_PIX_FMT_YUV420P;
    stream->codecpar->profile = FF_PROFILE_H264_BASELINE;
    stream->codecpar->bit_rate = mediaFormat->getInteger(KEY_VIDEO_BITRATE);

    std::vector<std::shared_ptr<ByteData>> headersByteData{};
    auto headers = mediaFormat->headers();
    for (const auto& item : headers) {
      headersByteData.push_back(
          std::shared_ptr<ByteData>(ByteData::MakeCopy(item->data(), item->length())));
    }
    updateExtraData(stream, headersByteData);
    stream->time_base = {1, mediaFormat->getInteger(KEY_FRAME_RATE)};

  } else {
    stream->codecpar->codec_id = AV_CODEC_ID_AAC;
    stream->codecpar->codec_type = AVMEDIA_TYPE_AUDIO;
    stream->codecpar->format = mediaFormat->getInteger(KEY_AUDIO_FORMAT);
    stream->codecpar->bit_rate = mediaFormat->getInteger(KEY_AUDIO_BITRATE);
    stream->codecpar->sample_rate = mediaFormat->getInteger(KEY_AUDIO_SAMPLE_RATE);
    stream->codecpar->channels = mediaFormat->getInteger(KEY_AUDIO_CHANNELS);
    stream->codecpar->frame_size = mediaFormat->getInteger(KEY_AUDIO_FRAME_SIZE);

    std::vector<std::shared_ptr<ByteData>> headersByteData{};
    auto headers = mediaFormat->headers();
    for (const auto& item : headers) {
      headersByteData.push_back(
          std::shared_ptr<ByteData>(ByteData::MakeCopy(item->data(), item->length())));
    }
    updateExtraData(stream, headersByteData);
    stream->time_base = codecContextTimeBase;
  }

  ctxTimeBaseMap[stream->id] = codecContextTimeBase;
  streamMap[stream->id] = stream;
  FF_DISABLE_DEPRECATION_WARNINGS
  if (avFormatContext->oformat->flags & AVFMT_GLOBALHEADER) {
    stream->codec->flags = stream->codec->flags | AV_CODEC_FLAG_GLOBAL_HEADER;
  }
  FF_DISABLE_DEPRECATION_WARNINGS
  return stream->id;
}

void FFmpegMuxer::refreshExtraData(int streamIndex,
                                   const std::vector<std::shared_ptr<ByteData>>& header) {
  LockGuard lockGuard(locker);
  auto stream = streamMap[streamIndex];
  if (stream) {
    updateExtraData(stream, header);
  }
}

void FFmpegMuxer::collectErrorMsgs(std::vector<std::string>* const toMsgs) {
  LockGuard lockGuard(locker);
  if (toMsgs == nullptr) {
    return;
  }
  toMsgs->insert(toMsgs->end(), msgs.begin(), msgs.end());
}

int FFmpegMuxer::calculateExtraDataLength(const std::vector<std::shared_ptr<ByteData>>& headers) {
  size_t extraDataSize = 0;
  for (auto& header : headers) {
    extraDataSize += header->length();
  }
  return static_cast<int>(extraDataSize);
}

void FFmpegMuxer::headersToExtraData(const std::vector<std::shared_ptr<ByteData>>& headers,
                                     uint8_t* extraData) {
  int pos = 0;
  for (auto& header : headers) {
    memcpy(extraData + pos, header->data(), header->length());
    pos += static_cast<int>(header->length());
  }
}

void FFmpegMuxer::updateExtraData(AVStream* stream,
                                  const std::vector<std::shared_ptr<ByteData>>& headers) {
  int extraDataSize = calculateExtraDataLength(headers);
  if (extraDataSize != 0) {
    stream->codecpar->extradata =
        static_cast<uint8_t*>(av_mallocz(extraDataSize + AV_INPUT_BUFFER_PADDING_SIZE));
    headersToExtraData(headers, stream->codecpar->extradata);
    stream->codecpar->extradata_size = extraDataSize;
  }
}

}  // namespace ffmovie