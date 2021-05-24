/*
 * Copyright (C) 2021 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "hwc-platform-drm-generic"

#include "ImaginationImporter.h"

#include <gralloc_handle.h>
#include <hardware/gralloc.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include <cinttypes>

#include "utils/log.h"
#include "utils/properties.h"

#include "img_gralloc1_public.h"

namespace android {

ImaginationImporter::~ImaginationImporter() {
  std::lock_guard<std::mutex> lock(cachedBoMutex_);
  for (auto &pair : cachedBo_) {
    DrmGenericImporter::ReleaseBuffer(&pair.second.bo);
  }
  cachedBo_.clear();
}

void ImaginationImporter::flushUnusedCachedBuffersUnlocked() {
  for (auto it = cachedBo_.begin(); it != cachedBo_.end();) {
    it->second.needFlush = true;
    if (!it->second.used) {
      DrmGenericImporter::ReleaseBuffer(&it->second.bo);
      it = cachedBo_.erase(it);
    } else {
      ++it;
    }
  }
}

int ImaginationImporter::ImportBuffer(hwc_drm_bo_t *bo) {
  IMG_native_handle_t *img_hnd = (IMG_native_handle_t *)bo->priv;
  int ret = 0;

  if (!img_hnd)
    return -EINVAL;

  {
    std::lock_guard<std::mutex> lock(cachedBoMutex_);
    auto it = cachedBo_.find(img_hnd->ui64Stamp);
    if (it != cachedBo_.end()) {
      it->second.used = true;
      *bo = it->second.bo;
      return 0;
    }
  }
  ret = DrmGenericImporter::ImportBuffer(bo);
  if (ret) {
    ALOGE("could not create drm fb %d", ret);
    return ret;
  }
  {
    std::lock_guard<std::mutex> lock(cachedBoMutex_);

    bo->priv = reinterpret_cast<void *>(img_hnd->ui64Stamp);
    bo_t buffer;
    buffer.bo = *bo;
    buffer.needFlush = false;
    buffer.used = true;

    cachedBo_.insert(std::make_pair(img_hnd->ui64Stamp, buffer));
  }
  return ret;
}

int ImaginationImporter::ReleaseBuffer(hwc_drm_bo_t *bo) {
  std::lock_guard<std::mutex> lock(cachedBoMutex_);
  unsigned long long key = reinterpret_cast<unsigned long long>(bo->priv);
  auto &buffer = cachedBo_[key];
  buffer.used = false;
  if (buffer.needFlush) {
    DrmGenericImporter::ReleaseBuffer(&buffer.bo);
    buffer.needFlush = false;
    cachedBo_.erase(key);
  }

  if (cachedBo_.size() > cMaxCacheSize)
    flushUnusedCachedBuffersUnlocked();
  return 0;
}

void ImaginationImporter::HandleHotplug() {
  std::lock_guard<std::mutex> lock(cachedBoMutex_);
  flushUnusedCachedBuffersUnlocked();
} 
}