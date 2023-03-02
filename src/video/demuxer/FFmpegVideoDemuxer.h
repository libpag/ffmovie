/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Tencent is pleased to support the open source community by making libpag available.
//
//  Copyright (C) 2021 THL A29 Limited, a Tencent company. All rights reserved.
//
//  Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
//  except in compliance with the License. You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  unless required by applicable law or agreed to in writing, software distributed under the
//  license is distributed on an "as is" basis, without warranties or conditions of any kind,
//  either express or implied. see the license for the specific language governing permissions
//  and limitations under the license.
//
/////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <unordered_map>

#ifdef __cplusplus
extern "C" {
#endif

#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/parseutils.h>

#ifdef __cplusplus
}
#endif

#include "ffmovie/movie.h"

namespace ffmovie {
class PTSDetail {
 public:
  PTSDetail(int64_t duration, std::vector<int64_t> ptsVector, std::vector<int> keyframeIndexVector)
      : duration(duration), ptsVector(std::move(ptsVector)),
        keyframeIndexVector(std::move(keyframeIndexVector)) {
  }
  /**
   * 总时长
   */
  int64_t duration = 0;
  /**
   * 排过序的 pts 列表
   */
  std::vector<int64_t> ptsVector;
  /**
   * 关键帧在 pts 列表中的位置
   * 对于 h264 的视频，关键帧在 pts 列表中的位置和在 dts 列表中的位置是一样的
   * 对于 hevc 的视频，这两个位置不一定一样
   */
  std::vector<int> keyframeIndexVector;

  /**
   * 找出目标时间所在 GOP 的关键帧在 keyframeIndexVector 中的位置
   * @return 0 ~ (keyframeIndexVector.size - 1) 之间
   */
  int findKeyframeIndex(int64_t atTime) const;

  /**
   * 获取关键帧对应的时间戳
   * @param withKeyframeIndex 关键帧在 keyframeIndexVector 中的位置
   * @return withKeyframeIndex < 0，返回 INT_MIN，withKeyframeIndex > size，返回 INT_MAX
   */
  int64_t getKeyframeTime(int withKeyframeIndex) const;

  /**
   * 获取目标时间对应的帧时间戳
   */
  int64_t getSampleTimeAt(int64_t targetTime) const;

  /**
   * 获取目标时间对应的下一帧的时间戳
   */
  int64_t getNextSampleTimeAt(int64_t targetTime);

 private:
  /**
   * 找出目标时间在 ptsVector 中的位置
   * @return 0 ~ (ptsVector.size - 1) 之间
   */
  int findFrameIndex(int64_t targetTime) const;
};

class FFmpegVideoDemuxer : public FFVideoDemuxer {
 public:
  ~FFmpegVideoDemuxer() override;

  int getCurrentTrackIndex() override;

  int64_t getSampleTime() override;

  bool advance() override;

  int getTrackCount() override;

  MediaFormat* getTrackFormat(unsigned int index) override;

  SampleData readSampleData() const override;

  bool seekTo(int64_t timestamp) override;

  bool selectTrack(unsigned int index) override;

  int64_t getSampleTimeAt(int64_t targetTime) override;

  bool needSeeking(int64_t currentSampleTime, int64_t targetSampleTime) override;

  void reset() override;

  FFmpegVideoDemuxer() = default;

  bool open(const std::string& filePath);

 private:
  NALUType naluStartCodeType = NALUType::AVCC;
  PTSDetail* ptsDetail = nullptr;
  int64_t maxPendingTime = INT64_MIN;
  int currentKeyframeIndex = -1;
  int videoStreamIndex = -1;
  AVFormatContext* formatContext = nullptr;
  AVBSFContext* avbsfContext = nullptr;
  AVPacket avPacket = {};
  int64_t sampleTime = INT64_MIN;
  std::unordered_map<int, MediaFormat*> formats;

  PTSDetail* getPTSDetail();
  std::vector<std::shared_ptr<ByteData>> createHeaders(AVStream* avStream);
  friend FFVideoDemuxer;
};
}  // namespace ffmovie