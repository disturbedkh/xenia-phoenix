/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Phoenix. All rights reserved.                       *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_APP_NXE_NXE_STRINGS_H_
#define XENIA_APP_NXE_NXE_STRINGS_H_

#include <filesystem>
#include <string>
#include <unordered_map>

namespace xe {
namespace app {
namespace nxe {

enum class NxeLocale { kEnglish, kArabic };

class NxeStrings {
 public:
  static NxeStrings& Instance();

  void Load(const std::filesystem::path& storage_root);
  void SetLocale(NxeLocale locale) { locale_ = locale; }
  NxeLocale locale() const { return locale_; }

  const char* Tr(const char* key) const;
  const char* Tr(const char* key_en, const char* key_ar) const;

 private:
  NxeStrings() = default;
  void LoadDefaults();

  NxeLocale locale_ = NxeLocale::kEnglish;
  std::unordered_map<std::string, std::string> en_;
  std::unordered_map<std::string, std::string> ar_;
};

inline const char* tr(const char* key) {
  return NxeStrings::Instance().Tr(key);
}

}  // namespace nxe
}  // namespace app
}  // namespace xe

#endif  // XENIA_APP_NXE_NXE_STRINGS_H_
