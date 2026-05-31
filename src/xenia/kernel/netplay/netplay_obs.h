/**
 ******************************************************************************
 * Netplay observability helpers for Phoenix obs events.
 ******************************************************************************
 */
#pragma once

#include <string_view>

#include "xenia/base/obs/obs.h"

namespace xe {
namespace kernel {
namespace netplay_obs {

inline void Emit(std::string_view code, std::string_view detail) {
  if (!obs::EventsEnabled()) {
    return;
  }
  obs::Event event{};
  event.kind = obs::EventKind::kProbe;
  event.severity = obs::EventSeverity::kInfo;
  event.domain = "netplay";
  event.channel = "Network";
  event.code = code;
  event.detail = detail;
  obs::EmitEvent(event);
}

}  // namespace netplay_obs
}  // namespace kernel
}  // namespace xe
