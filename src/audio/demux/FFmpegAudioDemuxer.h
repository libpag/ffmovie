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

#ifdef __cplusplus
}
#endif

#include "ffmovie/movie.h"

namespace ffmovie {

class FFmpegAudioDemuxer : public FFAudioDemuxer {
 public:
  ~FFmpegAudioDemuxer();

  bool selectTrack(unsigned int index) override;

  int getTrackCount() override;

  MediaFormat* getTrackFormat(unsigned int index) override;

  bool seekTo(int64_t timestamp) override;

  bool advance() override;

  SampleData readSampleData() const override;

  int64_t getSampleTime() override;

  int getCurrentTrackIndex() override;

  int open(const std::string& path);

  int open(uint8_t* data, size_t length);

  struct BufferData {
    uint8_t* ptr = nullptr;  // 文件中对应位置指针
    size_t size = 0;         // 文件当前指针到末尾的距离
    uint8_t* originPtr = nullptr;
    size_t fileSize = 0;
  } bufferData;

 private:
  AVIOContext* avioCtx = nullptr;
  int currentStreamIndex = -1;
  int64_t currentTime = -1;
  AVFormatContext* fmtCtx = nullptr;
  AVPacket avPacket{};
  std::unordered_map<unsigned int, MediaFormat*> formats{};

  MediaFormat* getTrackFormatInternal(unsigned int index);
};
}  // namespace ffmovie
