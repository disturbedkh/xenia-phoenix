/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Phoenix. All rights reserved.                       *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/app/nxe/nxe_strings.h"

#include <fstream>

#include "third_party/rapidjson/include/rapidjson/document.h"
#include "xenia/app/nxe/nxe_assets.h"
#include "xenia/base/logging.h"

namespace xe {
namespace app {
namespace nxe {

NxeStrings& NxeStrings::Instance() {
  static NxeStrings instance;
  return instance;
}

void NxeStrings::LoadDefaults() {
  en_["about"] = "ABOUT";
  en_["my_xenia"] = "MY XENIA";
  en_["open_tray"] = "OPEN TRAY";
  en_["achievements"] = "ACHIEVEMENTS";
  en_["game_library"] = "GAME LIBRARY";
  en_["system_settings"] = "SYSTEM SETTINGS";
  en_["select"] = "Select";
  en_["back"] = "Back";
  en_["change_profile"] = "Change Profile";
  en_["menu"] = "Menu";
  en_["welcome"] = "Welcome to Xenia Phoenix";
  en_["no_games"] = "No games found. Add a folder in Settings.";
  en_["settings"] = "Settings";
  en_["colors"] = "Colors";
  en_["audio"] = "Audio";
  en_["display"] = "Display";
  en_["patches"] = "Patches";
  en_["content"] = "Content";
  en_["friends"] = "Friends";
  en_["core"] = "Core";
  en_["system"] = "System";
  en_["config"] = "Config";
  en_["guide"] = "Xbox Guide";
}

void NxeStrings::Load(const std::filesystem::path& storage_root) {
  LoadDefaults();
  NxeAssets::Instance().Initialize(storage_root);
  const auto locales_path = NxeAssets::Instance().DataFile("locales.json");
  std::error_code ec;
  if (!std::filesystem::exists(locales_path, ec)) {
    return;
  }
  std::ifstream in(locales_path);
  if (!in) {
    return;
  }
  std::string json((std::istreambuf_iterator<char>(in)),
                   std::istreambuf_iterator<char>());
  rapidjson::Document doc;
  doc.Parse(json.c_str());
  if (doc.HasParseError() || !doc.IsObject()) {
    return;
  }
  if (doc.HasMember("en") && doc["en"].IsObject()) {
    for (auto it = doc["en"].MemberBegin(); it != doc["en"].MemberEnd(); ++it) {
      if (it->value.IsString()) {
        en_[it->name.GetString()] = it->value.GetString();
      }
    }
  }
  if (doc.HasMember("ar") && doc["ar"].IsObject()) {
    for (auto it = doc["ar"].MemberBegin(); it != doc["ar"].MemberEnd(); ++it) {
      if (it->value.IsString()) {
        ar_[it->name.GetString()] = it->value.GetString();
      }
    }
  }
}

const char* NxeStrings::Tr(const char* key) const {
  const auto& table = locale_ == NxeLocale::kArabic ? ar_ : en_;
  auto it = table.find(key);
  if (it != table.end()) {
    return it->second.c_str();
  }
  return key;
}

const char* NxeStrings::Tr(const char* key_en, const char* key_ar) const {
  return Tr(locale_ == NxeLocale::kArabic ? key_ar : key_en);
}

}  // namespace nxe
}  // namespace app
}  // namespace xe
