/**
 ******************************************************************************
 * JSON builders for Phoenix Probe /netplay/* endpoints.
 ******************************************************************************
 */
#pragma once

#include <string>

namespace xe {
class Emulator;

namespace kernel {

std::string BuildNetplayStatusJson(Emulator* emulator);
std::string BuildNetplaySessionsJson(Emulator* emulator);

}  // namespace kernel
}  // namespace xe
