/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2013 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/apu/apu_flags.h"

DEFINE_bool(mute, false, "Mutes all audio output.", "APU")
DEFINE_path(apu_xma_divergence_log, "",
            "Append XMA decoder divergence records (JSONL).", "APU");
DEFINE_path(apu_pcm_hash_log, "",
            "Append rolling PCM mix SHA256 windows for smoke titles (JSONL).",
            "APU");
DEFINE_uint32(apu_pcm_hash_interval_ms, 1000,
              "Minimum milliseconds between PCM hash log lines.", "APU");
