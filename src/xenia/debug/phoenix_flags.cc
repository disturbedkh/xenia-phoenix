/**
 ******************************************************************************
 * Phoenix localhost debug probe — cvars.
 ******************************************************************************
 */

#include "xenia/debug/phoenix_flags.h"

DEFINE_uint32(
    phoenix_debug_port, 0,
    "TCP port for localhost-only Phoenix debug HTTP API (0 = disabled).",
    "Debug");
