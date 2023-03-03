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
std::unique_ptr<ByteData> ByteData::FromPath(const std::string& filePath) {
  auto file = fopen(filePath.c_str(), "rb");
  if (file == nullptr) {
    return nullptr;
  }
  fseek(file, 0, SEEK_END);
  auto length = ftell(file);
  if (length <= 0) {
    fclose(file);
    return nullptr;
  }
  fseek(file, 0, SEEK_SET);
  auto data = ByteData::Make(length);
  if (data->length() != (size_t)length) {
    fclose(file);
    return nullptr;
  }
  fread(data->data(), 1, data->length(), file);
  return data;
}

std::unique_ptr<ByteData> ByteData::MakeCopy(const void* bytes, size_t length) {
  if (length == 0) {
    return Make(0);
  }
  auto data = new (std::nothrow) uint8_t[length];
  if (data == nullptr) {
    return nullptr;
  }
  memcpy(data, bytes, length);
  auto byteData = new ByteData(data, length);
  return std::unique_ptr<ByteData>(byteData);
}

std::unique_ptr<ByteData> ByteData::MakeWithoutCopy(void* data, size_t length) {
  if (length == 0) {
    return Make(0);
  }
  auto byteData = new ByteData(reinterpret_cast<uint8_t*>(data), length, nullptr);
  return std::unique_ptr<ByteData>(byteData);
}

std::unique_ptr<ByteData> ByteData::MakeAdopted(uint8_t* data, size_t length,
                                                std::function<void(uint8_t*)> releaseCallback) {
  if (length == 0) {
    data = nullptr;
  }
  auto byteData = new ByteData(data, length, releaseCallback);
  return std::unique_ptr<ByteData>(byteData);
}

std::unique_ptr<ByteData> ByteData::Make(size_t length) {
  auto data = length > 0 ? new (std::nothrow) uint8_t[length] : nullptr;
  if (length > 0 && data == nullptr) {
    length = 0;
  }
  auto byteData = new ByteData(data, length);
  return std::unique_ptr<ByteData>(byteData);
}

}  // namespace ffmovie
