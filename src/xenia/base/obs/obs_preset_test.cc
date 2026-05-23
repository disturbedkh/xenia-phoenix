/**
 ******************************************************************************
 * Observability preset unit tests (no full emulator init).
 ******************************************************************************
 */

#include <cassert>
#include <iostream>

#include "xenia/base/obs/obs.h"

int main() {
  using xe::obs::ParsePreset;
  using xe::obs::Preset;
  using xe::obs::PresetName;

  assert(ParsePreset("play") == Preset::kPlay);
  assert(ParsePreset("support") == Preset::kSupport);
  assert(ParsePreset("homebrew") == Preset::kHomebrew);
  assert(ParsePreset("develop") == Preset::kDevelop);
  assert(ParsePreset("forensic") == Preset::kForensic);
  assert(ParsePreset("invalid") == Preset::kUnknown);
  assert(PresetName(Preset::kDevelop) == "develop");
  std::cout << "xenia-obs-preset-test: ok\n";
  return 0;
}
