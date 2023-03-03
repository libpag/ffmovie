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

#include <libswresample/swresample.h>

#ifdef __cplusplus
}
#endif

#define DEFAULT_OUTPUT_SAMPLE_COUNT 1024
#include "ffmovie/movie.h"

namespace ffmovie {
struct VolumeRange {
  VolumeRange() = default;

  VolumeRange(const TimeRange timeRange, float startVolume, float endVolume)
      : timeRange(timeRange), startVolume(startVolume), endVolume(endVolume) {
  }

  TimeRange timeRange{-1, -1};
  float startVolume = 1.0f;
  float endVolume = 1.0f;
};

struct PCMOutputConfig {
  // 采样率，默认 44.1kHZ
  int sampleRate = 44100;
  int format = AV_SAMPLE_FMT_S16;
  // 默认按每声道 1024 个采样点往外输出
  int outputSamplesCount = DEFAULT_OUTPUT_SAMPLE_COUNT;
  // 双声道
  int channels = 2;
  // 立体声
  uint64_t channelLayout = AV_CH_LAYOUT_STEREO;
};

inline int64_t SampleLengthToCount(int64_t length, PCMOutputConfig* config) {
  return length / config->channels /
         av_get_bytes_per_sample(static_cast<AVSampleFormat>(config->format));
}

inline int64_t SampleLengthToTime(int64_t length, PCMOutputConfig* config) {
  return ceill(length * 1000000.0 / config->channels /
               av_get_bytes_per_sample(static_cast<AVSampleFormat>(config->format)) /
               config->sampleRate);
}

inline int64_t SampleCountToLength(int64_t sampleCount, PCMOutputConfig* config) {
  return sampleCount * config->channels *
         av_get_bytes_per_sample(static_cast<AVSampleFormat>(config->format));
}

inline int64_t SampleTimeToLength(int64_t time, PCMOutputConfig* config) {
  return SampleCountToLength(ceill(static_cast<double>(time * config->sampleRate) / 1000000.0),
                             config);
}

inline bool IsSampleConfig(const PCMOutputConfig& left, const AVFrame& right) {
  return left.sampleRate == right.sample_rate && left.format == right.format &&
         left.channels == right.channels && left.channelLayout == right.channel_layout;
}
}  // namespace ffmovie
