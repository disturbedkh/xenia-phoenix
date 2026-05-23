/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2013 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_APU_APU_FLAGS_H_
#define XENIA_APU_APU_FLAGS_H_

#include "xenia/base/cvar.h"
DECLARE_bool(mute) DECLARE_path(apu_xma_divergence_log);
DECLARE_path(apu_pcm_hash_log);
DECLARE_uint32(apu_pcm_hash_interval_ms);

#endif  // XENIA_APU_APU_FLAGS_H_
