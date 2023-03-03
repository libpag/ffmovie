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

#include <memory>
#include <mutex>

namespace ffmovie {

class LockGuard {
 public:
  explicit LockGuard(std::shared_ptr<std::mutex> locker) : mutex(std::move(locker)) {
    if (mutex) {
      mutex->lock();
    }
  }

  ~LockGuard() {
    if (mutex) {
      mutex->unlock();
    }
  }

 private:
  std::shared_ptr<std::mutex> mutex;
};

}  // namespace ffmovie
