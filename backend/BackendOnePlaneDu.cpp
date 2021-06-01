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

#include "BackendOnePlaneDu.h"

#include "BackendManager.h"
#include "bufferinfo/BufferInfoGetter.h"
#include "drm_fourcc.h"
#include "utils/log.h"

namespace android {

HWC2::Error BackendOnePlaneDu::ValidateDisplay(DrmHwcTwo::HwcDisplay *display,
                                               uint32_t *num_types,
                                               uint32_t *num_requests) {
  *num_types = 0;
  *num_requests = 0;

  auto layers = display->GetOrderLayersByZPos();

#ifdef USE_ONE_LAYER_ENHANCEMENT
  if ((layers.size() == 1) && (!IsClientLayer(display, layers[0]))) {
    layers[0]->set_validated_type(HWC2::Composition::Device);
    ++*num_types;
    return HWC2::Error::None;
  }
#endif

  for (auto &l : layers) {
    l->set_validated_type(HWC2::Composition::Client);
    ++*num_types;
  }

  return HWC2::Error::HasChanges;
}

bool BackendOnePlaneDu::IsClientLayer(DrmHwcTwo::HwcDisplay *display,
                                      DrmHwcTwo::HwcLayer *layer) {
  hwc_drm_bo_t bo;

  int ret = BufferInfoGetter::GetInstance()->ConvertBoInfo(layer->buffer(),
                                                           &bo);
  if (ret)
    return true;

  return Backend::IsClientLayer(display, layer);
}

// clang-format off
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables, cert-err58-cpp)
REGISTER_BACKEND("oneplane-du", BackendOnePlaneDu);
// clang-format on

}  // namespace android