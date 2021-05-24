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

#ifndef ANDROID_PLATFORM_IMAGINATION_H_
#define ANDROID_PLATFORM_IMAGINATION_H_

#include <map>
#include <mutex>

#include "DrmGenericImporter.h"

namespace android {

typedef struct bo_cached_descriptor {
    hwc_drm_bo_t bo;
    // Handle used by some external component(SF)
    bool used;
    // Deallocation needed
    bool needFlush;
} bo_t;


class ImaginationImporter : public DrmGenericImporter {
public:
  ImaginationImporter(DrmDevice *drm) : DrmGenericImporter(drm) {}
  ~ImaginationImporter() override;

  int ImportBuffer(hwc_drm_bo_t *bo) override;
  int ReleaseBuffer(hwc_drm_bo_t *bo) override;
  void HandleHotplug() override;

 private:
  void flushUnusedCachedBuffersUnlocked();
  std::map<unsigned long long, bo_t> cachedBo_;
  std::mutex cachedBoMutex_;
  static constexpr int cMaxCacheSize = 6;

};

}  // namespace android

#endif
