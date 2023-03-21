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

#include "FFmpegVideoDemuxer.h"
#include <list>
#include "h264_sps_parser.h"
#include "hevc_vps_parser.h"
#include "utils/FFmpegUtils.h"

namespace ffmovie {
#define DEFAULT_MAX_NUM_REORDER 4

int GetMaxReorderSize(const std::vector<std::shared_ptr<ByteData>>& headers,
                      const std::string& mimeType) {
  int maxNumReorder = 0;
  if (mimeType == "video/hevc") {
    auto vps = headers[0]->data() + 4;
    size_t vpsSize = headers[0]->length() - 4;
    maxNumReorder = hevc_get_max_num_reorder(vps, vpsSize);
  } else {
    auto sps = headers[0]->data() + 4;
    size_t spsSize = headers[0]->length() - 4;
    maxNumReorder = h264_get_max_num_reorder(sps, spsSize);
  }
  maxNumReorder = maxNumReorder < 0 ? DEFAULT_MAX_NUM_REORDER : maxNumReorder;
  return maxNumReorder;
}

/**
 * satisfy condition: array[?] <= target and the last one
 */
int BinarySearch(int start, int end, const std::function<bool(int)>& condition) {
  while (start <= end) {
    int mid = (start + end) / 2;
    if (condition(mid)) {
      start = mid + 1;
    } else {
      end = mid - 1;
    }
  }
  return start == 0 ? 0 : start - 1;
}

int PTSDetail::findKeyframeIndex(int64_t atTime) const {
  if (ptsVector.empty()) {
    return 0;
  }
  int start = 0;
  auto end = static_cast<int>(keyframeIndexVector.size()) - 1;
  return BinarySearch(start, end, [this, atTime](int mid) {
    return ptsVector[keyframeIndexVector[mid]] <= atTime;
  });
}

int64_t PTSDetail::getKeyframeTime(int withKeyframeIndex) const {
  if (withKeyframeIndex < 0) {
    return INT64_MIN;
  }
  if (withKeyframeIndex >= static_cast<int>(keyframeIndexVector.size())) {
    return INT64_MAX;
  }
  return ptsVector[keyframeIndexVector[withKeyframeIndex]];
}

int64_t PTSDetail::getSampleTimeAt(int64_t targetTime) const {
  if (targetTime < ptsVector.front()) {
    if (targetTime >= 0 && !ptsVector.empty()) {
      return ptsVector[0];
    }
    return INT64_MIN;
  }
  if (targetTime >= duration) {
    return INT64_MAX;
  }
  auto frameIndex = findFrameIndex(targetTime);
  return ptsVector[frameIndex];
}

int64_t PTSDetail::getNextSampleTimeAt(int64_t targetTime) {
  if (targetTime < ptsVector.front()) {
    if (targetTime >= 0 && !ptsVector.empty()) {
      return ptsVector[0];
    }
    return INT64_MAX;
  }
  if (targetTime >= duration) {
    return INT64_MAX;
  }
  size_t frameIndex = findFrameIndex(targetTime) + 1;
  if (frameIndex >= ptsVector.size()) {
    return INT64_MAX;
  }
  return ptsVector[frameIndex];
}

int PTSDetail::findFrameIndex(int64_t targetTime) const {
  auto start = findKeyframeIndex(targetTime);
  auto end = start + 1;
  start = keyframeIndexVector[start];
  if (end >= static_cast<int>(keyframeIndexVector.size())) {
    end = static_cast<int>(ptsVector.size()) - 1;
  } else {
    end = keyframeIndexVector[end];
  }
  return BinarySearch(start, end,
                      [this, targetTime](int mid) { return ptsVector[mid] <= targetTime; });
}

std::unique_ptr<FFVideoDemuxer> FFVideoDemuxer::Make(const std::string& path,
                                                     NALUType startCodeType) {
  auto demuxer = std::unique_ptr<FFmpegVideoDemuxer>(new FFmpegVideoDemuxer());
  if (!demuxer->open(path)) {
    return nullptr;
  }
  demuxer->naluStartCodeType = startCodeType;
  return demuxer;
}

FFmpegVideoDemuxer::~FFmpegVideoDemuxer() {
  av_packet_unref(&avPacket);
  if (formatContext != nullptr) {
    avformat_close_input(&formatContext);
  }
  av_bsf_free(&avbsfContext);
  for (auto& format : formats) {
    delete format.second;
  }
  delete ptsDetail;
}

int64_t FFmpegVideoDemuxer::getSampleTimeAt(int64_t targetTime) {
  return getPTSDetail()->getSampleTimeAt(targetTime);
}

int64_t FFmpegVideoDemuxer::getSampleTime() {
  return sampleTime;
}

bool FFmpegVideoDemuxer::advance() {
  av_packet_unref(&avPacket);
  if (formatContext && videoStreamIndex >= 0) {
    auto avStream = formatContext->streams[videoStreamIndex];
    while (av_read_frame(formatContext, &avPacket) >= 0) {
      if (avPacket.stream_index == videoStreamIndex) {
        sampleTime = av_rescale_q_rnd(avPacket.pts, avStream->time_base,
                                      AVRational{1, AV_TIME_BASE}, AVRounding::AV_ROUND_ZERO);
        if (naluStartCodeType == NALUType::AnnexB) {
          if (av_bsf_send_packet(avbsfContext, &avPacket) != 0) {
          }
          while (av_bsf_receive_packet(avbsfContext, &avPacket) == 0) {
          }
        }
        maxPendingTime = std::max(maxPendingTime, sampleTime);
        if (currentKeyframeIndex < 0) {
          currentKeyframeIndex = getPTSDetail()->findKeyframeIndex(sampleTime);
        } else {
          if (avPacket.flags & AVINDEX_KEYFRAME) {
            currentKeyframeIndex++;
          }
        }
        return true;
      }
      av_packet_unref(&avPacket);
    }
  }
  return false;
}

int FFmpegVideoDemuxer::getTrackCount() {
  if (formatContext == nullptr) {
    return 0;
  }
  return formatContext->nb_streams;
}

MediaFormat* FFmpegVideoDemuxer::getTrackFormat(unsigned int index) {
  if (index < 0 || static_cast<uint32_t>(index) > formatContext->nb_streams - 1) {
    return nullptr;
  }
  if (formats.find(index) != formats.end()) {
    return formats[index];
  }
  AVStream* avStream = formatContext->streams[index];
  AVMediaType type = avStream->codecpar->codec_type;
  if (type != AVMEDIA_TYPE_VIDEO && type != AVMEDIA_TYPE_AUDIO) {
    return nullptr;
  }
  auto trackFormat = new MediaFormat();
  auto duration = 1000000 * avStream->duration * avStream->time_base.num / avStream->time_base.den;
  if (!duration && formatContext->duration) {
    duration = formatContext->duration;
  }
  trackFormat->setLong(KEY_DURATION, duration);
  trackFormat->setInteger(KEY_TRACK_ID, index);
  if (type == AVMEDIA_TYPE_VIDEO) {
    trackFormat->setInteger(KEY_WIDTH, avStream->codecpar->width);
    trackFormat->setInteger(KEY_HEIGHT, avStream->codecpar->height);
    auto sar = av_guess_sample_aspect_ratio(formatContext, avStream, NULL);
    if (sar.num > 0 && sar.den > 0) {
      trackFormat->setFloat(KEY_SAMPLE_ASPECT_RATIO, sar.num * 1.0 / sar.den);
    }
    auto frameRate = 1.0 * avStream->avg_frame_rate.num / avStream->avg_frame_rate.den;
    trackFormat->setFloat(KEY_FRAME_RATE, frameRate);

    if (avStream->codecpar->color_primaries == AVCOL_PRI_BT709) {
      trackFormat->setString(KEY_COLOR_SPACE, COLORSPACE_REC709);
    } else if (avStream->codecpar->color_primaries == AVCOL_PRI_BT2020) {
      trackFormat->setString(KEY_COLOR_SPACE, COLORSPACE_REC2020);
    } else {
      trackFormat->setString(KEY_COLOR_SPACE, COLORSPACE_REC601);
    }
    if (avStream->codecpar->color_range == AVCOL_RANGE_MPEG) {
      trackFormat->setString(KEY_COLOR_RANGE, COLORRANGE_MPEG);
    } else if (avStream->codecpar->color_range == AVCOL_RANGE_JPEG) {
      trackFormat->setString(KEY_COLOR_RANGE, COLORRANGE_JPEG);
    } else {
      trackFormat->setString(KEY_COLOR_RANGE, COLORRANGE_UNKNOWN);
    }
    auto codecID = AVCodecIDToStringVideo(avStream->codecpar->codec_id);
    if (codecID == MIMETYPE_UNKNOWN) {
      delete trackFormat;
      return nullptr;
    }
    trackFormat->setString(KEY_MIME, codecID);
    AVDictionaryEntry* tag = nullptr;
    tag = av_dict_get(avStream->metadata, "rotate", tag, 0);
    if (tag) {
      trackFormat->setInteger(KEY_ROTATION, static_cast<int>(std::strtol(tag->value, nullptr, 10)));
    }
    trackFormat->setCodecPar(avStream->codecpar);
    trackFormat->setHeaders(createHeaders(avStream));
    trackFormat->setInteger(KEY_VIDEO_MAX_REORDER,
                            GetMaxReorderSize(trackFormat->headers(), codecID));
  } else {
    trackFormat->setString(KEY_MIME, "audio");
  }
  formats[index] = trackFormat;
  return trackFormat;
}

SampleData FFmpegVideoDemuxer::readSampleData() const {
  SampleData sample = {};
  sample.data = avPacket.data;
  sample.length = avPacket.size;
  return sample;
}

bool FFmpegVideoDemuxer::needSeeking(int64_t currentTime, int64_t targetTime) {
  // 判断当前解码时间和解码器中缓存的帧的最大时间，如果在这个区间，不需要触发 seek。
  // 主要处理连续 GOP 的连接处不需要 seek。
  if (currentTime <= targetTime && targetTime < maxPendingTime) {
    return false;
  }
  // 请求时间大于等于当前时间且位于同一个关键帧内，不需要 seek。
  auto targetKeyframeIndex = getPTSDetail()->findKeyframeIndex(targetTime);
  if (currentTime <= targetTime && currentKeyframeIndex == targetKeyframeIndex) {
    return false;
  }
  // hevc 格式的视频，如果要 seek 到一个 GOP 的后几帧时，由于后面的几帧可能被编码在后面的一个 GOP，
  // 所以 FFmpeg 会直接 seek 到后面一个 GOP 的关键帧，这里我们直接 seek 到目标的关键帧。
  return true;
}

bool FFmpegVideoDemuxer::seekTo(int64_t targetTime) {
  if (!formatContext || videoStreamIndex < 0) {
    return false;
  }
  auto targetKeyframeIndex = ptsDetail->findKeyframeIndex(targetTime);
  // hevc 格式的视频，如果要 seek 到一个 GOP 的后几帧时，由于后面的几帧可能被编码在后面的一个 GOP，
  // 所以 FFmpeg 会直接 seek 到后面一个 GOP 的关键帧，这里我们直接 seek 到目标的关键帧。
  // 例如测试用例 AsyncDecode.init_ID79850945
  targetTime = ptsDetail->getKeyframeTime(targetKeyframeIndex);
  // flag 是 AVSEEK_FLAG_BACKWARD 时，seek 传入关键帧的时间，得到的时间小于不等于传入的时间。
  // flag 是 0 时，seek 传入关键帧的时间，得到的时间大于等于传入的时间。
  // '+1' 确保可以 seek 到指定的时间
  int64_t pos = av_rescale_q_rnd(targetTime + 1, AVRational{1, AV_TIME_BASE},
                                 formatContext->streams[videoStreamIndex]->time_base,
                                 AVRounding::AV_ROUND_ZERO);
  int ret = av_seek_frame(formatContext, videoStreamIndex, pos, AVSEEK_FLAG_BACKWARD);
  return ret >= 0;
}

bool FFmpegVideoDemuxer::selectTrack(unsigned int index) {
  if (formatContext == nullptr || index >= formatContext->nb_streams) {
    return false;
  }
  videoStreamIndex = static_cast<int>(index);
  return true;
}

void FFmpegVideoDemuxer::reset() {
  maxPendingTime = INT64_MIN;
  currentKeyframeIndex = -1;
}

bool FFmpegVideoDemuxer::open(const std::string& filePath) {
  auto path = static_cast<const char*>(filePath.data());
  if (avformat_open_input(&formatContext, path, nullptr, nullptr) < 0) {
    return false;
  }
  if (avformat_find_stream_info(formatContext, nullptr) < 0) {
    return false;
  }
  auto numStreams = static_cast<int>(formatContext->nb_streams);
  AVStream* avStream = nullptr;
  const AVBitStreamFilter* absFilter = nullptr;
  for (int index = 0; index < numStreams; index++) {
    auto stream = formatContext->streams[index];
    auto codecID = stream->codecpar->codec_id;
    switch (codecID) {
      case AV_CODEC_ID_H264:
        absFilter = av_bsf_get_by_name("h264_mp4toannexb");
        break;
      case AV_CODEC_ID_HEVC:
        absFilter = av_bsf_get_by_name("hevc_mp4toannexb");
        break;
      default:
        break;
    }
    if (absFilter != nullptr) {
      videoStreamIndex = index;
      avStream = stream;
      break;
    }
  }
  if (avStream == nullptr) {
    return false;
  }
  av_bsf_alloc(absFilter, &avbsfContext);
  avcodec_parameters_copy(avbsfContext->par_in, avStream->codecpar);
  av_bsf_init(avbsfContext);
  avPacket.data = nullptr;
  avPacket.size = 0;
  av_new_packet(&avPacket, 0);
  return true;
}

PTSDetail* FFmpegVideoDemuxer::getPTSDetail() {
  if (ptsDetail != nullptr) {
    return ptsDetail;
  }
  std::vector<int> keyframeIndexVector{};
  AVStream* avStream = formatContext->streams[videoStreamIndex];
  std::list<int64_t> ptsList{};
  for (int i = avStream->nb_index_entries - 1; i >= 0; --i) {
    auto entry = avStream->index_entries[i];
    auto pts = av_rescale_q_rnd(entry.pts, avStream->time_base, AVRational{1, AV_TIME_BASE},
                                AVRounding::AV_ROUND_ZERO);
    int index = 0;
    auto it = ptsList.begin();
    for (; it != ptsList.end(); ++it) {
      if (*it < pts) {
        ++index;
        continue;
      }
      break;
    }
    ptsList.insert(it, pts);
    if (entry.flags & AVINDEX_KEYFRAME) {
      index = avStream->nb_index_entries - (static_cast<int>(ptsList.size()) - index);
      keyframeIndexVector.insert(keyframeIndexVector.begin(), index);
    }
  }
  auto duration = 1000000 * avStream->duration * avStream->time_base.num / avStream->time_base.den;
  auto ptsVector = std::vector<int64_t>{std::make_move_iterator(ptsList.begin()),
                                        std::make_move_iterator(ptsList.end())};
  ptsDetail = new PTSDetail(duration, std::move(ptsVector), std::move(keyframeIndexVector));
  return ptsDetail;
}

int FFmpegVideoDemuxer::getCurrentTrackIndex() {
  return videoStreamIndex;
}

void SetH264StartCodeIndex(int i, int* startCodeSPSIndex, int* startCodeFPPSIndex) {
  if (*startCodeSPSIndex == 0) {
    *startCodeSPSIndex = i;
  }
  if (i > *startCodeSPSIndex) {
    *startCodeFPPSIndex = i;
  }
}

void SetH265StartCodeIndex(int i, int* startCodeVPSIndex, int* startCodeSPSIndex,
                           int* startCodeFPPSIndex, int* startCodeRPPSIndex) {
  if (*startCodeVPSIndex == 0) {
    *startCodeVPSIndex = i;
    return;
  }
  if (i > *startCodeVPSIndex && *startCodeSPSIndex == 0) {
    *startCodeSPSIndex = i;
    return;
  }
  if (i > *startCodeSPSIndex && *startCodeFPPSIndex == 0) {
    *startCodeFPPSIndex = i;
    return;
  }
  if (i > *startCodeFPPSIndex && *startCodeRPPSIndex == 0) {
    *startCodeRPPSIndex = i;
  }
}

void SetStartCodeIndex(uint8_t* extradata, int extradataSize, AVCodecID avCodecId,
                       int* startCodeVPSIndex, int* startCodeSPSIndex, int* startCodeFPPSIndex,
                       int* startCodeRPPSIndex) {
  for (int i = 0; i < extradataSize; i++) {
    if (i >= 3) {
      if (extradata[i] == 0x01 && extradata[i - 1] == 0x00 && extradata[i - 2] == 0x00 &&
          extradata[i - 3] == 0x00) {
        if (avCodecId == AV_CODEC_ID_H264) {
          SetH264StartCodeIndex(i, startCodeSPSIndex, startCodeFPPSIndex);
        } else if (avCodecId == AV_CODEC_ID_HEVC) {
          SetH265StartCodeIndex(i, startCodeVPSIndex, startCodeSPSIndex, startCodeFPPSIndex,
                                startCodeRPPSIndex);
        }
      }
    }
  }
}

void SetH264Headers(uint8_t* extradata, int extradataSize,
                    std::vector<std::shared_ptr<ByteData>>& headers, int spsSize,
                    int startCodeSPSIndex, int startCodeFPPSIndex, int naluType) {
  int ppsSize = extradataSize - (startCodeFPPSIndex + 1);
  naluType = (uint8_t)extradata[startCodeSPSIndex + 1] & 0x1Fu;
  if (naluType == 0x07) {
    auto sps = &extradata[startCodeSPSIndex + 1];
    headers.push_back(ByteData::MakeCopy(sps - 4, spsSize + 4));
  }
  naluType = (uint8_t)extradata[startCodeFPPSIndex + 1] & 0x1Fu;
  if (naluType == 0x08) {
    auto pps = &extradata[startCodeFPPSIndex + 1];
    headers.push_back(ByteData::MakeCopy(pps - 4, ppsSize + 4));
  }
}

void SetH265Headers(uint8_t* extradata, int extradataSize,
                    std::vector<std::shared_ptr<ByteData>>& headers, int spsSize,
                    int startCodeVPSIndex, int startCodeSPSIndex, int startCodeFPPSIndex,
                    int startCodeRPPSIndex, int naluType) {
  int vpsSize = startCodeSPSIndex - startCodeVPSIndex - 4;
  int ppsSize;
  if (startCodeRPPSIndex > 0) {
    ppsSize = startCodeRPPSIndex - startCodeFPPSIndex - 4;
  } else {
    ppsSize = extradataSize - (startCodeFPPSIndex + 1);
  }

  naluType = (uint8_t)extradata[startCodeVPSIndex + 1] & 0x4Fu;
  if (naluType == 0x40) {
    auto vps = &extradata[startCodeVPSIndex + 1];
    headers.push_back(ByteData::MakeCopy(vps - 4, vpsSize + 4));
  }

  naluType = (uint8_t)extradata[startCodeSPSIndex + 1] & 0x4Fu;
  if (naluType == 0x42) {
    auto sps = &extradata[startCodeSPSIndex + 1];
    headers.push_back(ByteData::MakeCopy(sps - 4, spsSize + 4));
  }

  naluType = (uint8_t)extradata[startCodeFPPSIndex + 1] & 0x4Fu;
  if (naluType == 0x44) {
    auto pps = &extradata[startCodeFPPSIndex + 1];
    headers.push_back(ByteData::MakeCopy(pps - 4, ppsSize + 4));
  }
  if (startCodeRPPSIndex > 0) {
    int rppsSize = extradataSize - (startCodeRPPSIndex + 1);
    naluType = (uint8_t)extradata[startCodeRPPSIndex + 1] & 0x4Fu;
    if (naluType == 0x44) {
      uint8_t* rpps = &extradata[startCodeRPPSIndex + 1];
      headers.push_back(ByteData::MakeCopy(rpps - 4, rppsSize + 4));
    }
  }
}

std::vector<std::shared_ptr<ByteData>> FFmpegVideoDemuxer::createHeaders(AVStream* avStream) {
  AVPacket pkt;
  int size = av_read_frame(formatContext, &pkt);
  if (size < 0 || pkt.size < 0) {
    av_packet_unref(&pkt);
    return {};
  }

  auto avCodecId = avStream->codecpar->codec_id;
  uint8_t* extradata = avbsfContext->par_out->extradata;
  int extradataSize = avbsfContext->par_out->extradata_size;

  int startCodeVPSIndex = 0;
  int startCodeSPSIndex = 0;
  int startCodeFPPSIndex = 0;

  int startCodeRPPSIndex = 0;
  int naluType = 0;

  SetStartCodeIndex(extradata, extradataSize, avCodecId, &startCodeVPSIndex, &startCodeSPSIndex,
                    &startCodeFPPSIndex, &startCodeRPPSIndex);

  std::vector<std::shared_ptr<ByteData>> headers{};
  int spsSize = startCodeFPPSIndex - startCodeSPSIndex - 4;

  if (avCodecId == AV_CODEC_ID_H264) {
    SetH264Headers(extradata, extradataSize, headers, spsSize, startCodeSPSIndex,
                   startCodeFPPSIndex, naluType);
  } else if (avCodecId == AV_CODEC_ID_HEVC) {
    SetH265Headers(extradata, extradataSize, headers, spsSize, startCodeVPSIndex, startCodeSPSIndex,
                   startCodeFPPSIndex, startCodeRPPSIndex, naluType);
  }
  av_packet_unref(&pkt);
  return headers;
}
}  // namespace ffmovie
