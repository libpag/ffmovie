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
#include <mutex>
#include <string>
#include <unordered_map>
#include "ffmovie/movie.h"
#ifdef __cplusplus
extern "C" {
#endif

#include <libavformat/avformat.h>
#include <libavutil/avassert.h>
#include <libavutil/channel_layout.h>
#include <libavutil/error.h>
#include <libavutil/mathematics.h>
#include <libavutil/opt.h>
#include <libavutil/timestamp.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>

#ifdef __cplusplus
}
#endif

namespace ffmovie {

class FFmpegMuxer : public FFMediaMuxer {
 public:
  FFmpegMuxer(ExportCallback* callback) : _callback(callback){};

  ~FFmpegMuxer() override;

  bool initMuxer(const std::string& filePath) override;
  bool start() override;
  bool stop() override;
  int addTrack(std::shared_ptr<MediaFormat> mediaFormat) override;
  bool writeFrame(int streamIndex, void* frame) override;
  void refreshExtraData(int streamIndex,
                        const std::vector<std::shared_ptr<ByteData>>& header) override;

  void collectErrorMsgs(std::vector<std::string>* const toMsgs) override;

 private:
  void unInitMuxer();

  int calculateExtraDataLength(const std::vector<std::shared_ptr<ByteData>>&);

  void headersToExtraData(const std::vector<std::shared_ptr<ByteData>>& headers,
                          uint8_t* extraData);

  void updateExtraData(AVStream* stream, const std::vector<std::shared_ptr<ByteData>>& header);

  ExportCallback* _callback = nullptr;
  std::string movieOutputPath;
  AVOutputFormat* avOutputFormat = nullptr;
  AVFormatContext* avFormatContext = nullptr;
  AVDictionary* avDict = nullptr;
  std::shared_ptr<std::mutex> locker = std::make_shared<std::mutex>();
  std::unordered_map<int, AVRational> ctxTimeBaseMap = {};
  std::unordered_map<int, AVStream*> streamMap = {};
  std::vector<std::string> msgs;
};
}  // namespace ffmovie
