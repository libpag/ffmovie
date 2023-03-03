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

#include "ffmovie/movie.h"

namespace ffmovie {
MediaFormat::~MediaFormat() {
  trackFormatMap.clear();
}

int MediaFormat::getInteger(const std::string& name) {
  auto iter = trackFormatMap.find(name);
  if (iter != trackFormatMap.end()) {
    return static_cast<int>(std::strtol(iter->second.data(), nullptr, 10));
  }
  return 0;
}

void MediaFormat::setInteger(std::string name, int value) {
  if (!name.empty()) {
    trackFormatMap[name] = std::to_string(value);
  }
}

int64_t MediaFormat::getLong(const std::string& name) {
  auto iter = trackFormatMap.find(name);
  if (iter != trackFormatMap.end()) {
    return strtoll(iter->second.data(), nullptr, 10);
  }
  return 0;
}

void MediaFormat::setLong(std::string name, int64_t value) {
  if (!name.empty()) {
    trackFormatMap[name] = std::to_string(value);
  }
}

float MediaFormat::getFloat(const std::string& name) {
  auto iter = trackFormatMap.find(name);
  if (iter != trackFormatMap.end()) {
    return static_cast<float>(std::strtod(iter->second.data(), nullptr));
  }
  return 0;
}

void MediaFormat::setFloat(std::string name, float value) {
  if (!name.empty()) {
    trackFormatMap[name] = std::to_string(value);
  }
}

std::string MediaFormat::getString(const std::string& name) {
  auto iter = trackFormatMap.find(name);
  if (iter != trackFormatMap.end()) {
    return iter->second;
  }
  return "";
}

void MediaFormat::setString(std::string name, std::string value) {
  if (name.empty() || value.empty()) {
    return;
  }
  trackFormatMap[name] = value;
}

void* MediaFormat::getCodecPar() {
  return _codecPar;
}

void MediaFormat::setCodecPar(void* value) {
  _codecPar = value;
}

std::vector<std::shared_ptr<ByteData>> MediaFormat::headers() {
  return _headers;
}

void MediaFormat::setHeaders(std::vector<std::shared_ptr<ByteData>> headers) {
  _headers = std::move(headers);
}
}  // namespace ffmovie