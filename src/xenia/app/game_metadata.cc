/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Phoenix. All rights reserved.                       *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/app/game_metadata.h"

#include <fstream>
#include <thread>

#include "third_party/fmt/include/fmt/format.h"
#include "third_party/libcurl/include/curl/curl.h"
#include "third_party/rapidjson/include/rapidjson/document.h"
#include "xenia/base/logging.h"
#include "xenia/base/platform.h"

namespace xe {
namespace app {
namespace {

constexpr const char* kGamesJsonUrl =
    "https://xenia-manager.github.io/x360db/games.json";
constexpr const char* kCacheFile = "cache/x360db_games.json";

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
  const size_t total = size * nmemb;
  auto* buffer = static_cast<std::vector<uint8_t>*>(userp);
  const auto* data = static_cast<const uint8_t*>(contents);
  buffer->insert(buffer->end(), data, data + total);
  return total;
}

std::string JsonString(const rapidjson::Value& obj, const char* key) {
  if (!obj.HasMember(key) || !obj[key].IsString()) {
    return {};
  }
  return obj[key].GetString();
}

}  // namespace

GameMetadataService::GameMetadataService(std::filesystem::path storage_root)
    : storage_root_(std::move(storage_root)),
      cache_path_(storage_root_ / kCacheFile) {}

void GameMetadataService::LoadFromCache() {
  std::error_code ec;
  const std::filesystem::path bundled = storage_root_ / "assets" / "dashboard" /
                                        "data" / "x360db_titles_merged.json";
  const std::filesystem::path alt_bundled = std::filesystem::path(
      "src/xenia/app/assets/dashboard/data/"
      "x360db_titles_merged.json");
  std::filesystem::path source = cache_path_;
  if (!std::filesystem::exists(source, ec)) {
    if (std::filesystem::exists(bundled, ec)) {
      source = bundled;
    } else if (std::filesystem::exists(alt_bundled, ec)) {
      source = alt_bundled;
    } else {
      return;
    }
  }
  std::ifstream in(source);
  if (!in) {
    return;
  }
  std::string json((std::istreambuf_iterator<char>(in)),
                   std::istreambuf_iterator<char>());
  rapidjson::Document doc;
  doc.Parse(json.c_str());
  if (doc.HasParseError()) {
    return;
  }
  if (doc.IsObject()) {
    for (auto it = doc.MemberBegin(); it != doc.MemberEnd(); ++it) {
      uint32_t id = 0;
      try {
        id = static_cast<uint32_t>(
            std::stoul(it->name.GetString(), nullptr, 16));
      } catch (...) {
        continue;
      }
      if (!it->value.IsObject()) {
        continue;
      }
      GameMetadata meta;
      meta.title_id = id;
      meta.title_name = JsonString(it->value, "title");
      if (meta.title_name.empty()) {
        meta.title_name = JsonString(it->value, "name");
      }
      meta.developer = JsonString(it->value, "developer");
      meta.publisher = JsonString(it->value, "publisher");
      meta.genre = JsonString(it->value, "genre");
      meta.release_date = JsonString(it->value, "release_date");
      meta.description = JsonString(it->value, "description");
      by_id_[id] = std::move(meta);
    }
  }
  loaded_ = !by_id_.empty();
}

void GameMetadataService::SaveToCache() {
  std::error_code ec;
  std::filesystem::create_directories(cache_path_.parent_path(), ec);
  // Cache is written by network fetch as raw JSON blob in FetchFromNetwork.
}

bool GameMetadataService::FetchFromNetwork() {
  std::vector<uint8_t> buffer;
  CURL* curl = curl_easy_init();
  if (!curl) {
    return false;
  }
  curl_easy_setopt(curl, CURLOPT_URL, kGamesJsonUrl);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "xenia-phoenix");
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  const CURLcode result = curl_easy_perform(curl);
  long code = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
  curl_easy_cleanup(curl);
  if (result != CURLE_OK || code != 200 || buffer.empty()) {
    return false;
  }
  std::error_code ec;
  std::filesystem::create_directories(cache_path_.parent_path(), ec);
  {
    std::ofstream out(cache_path_, std::ios::binary);
    out.write(reinterpret_cast<const char*>(buffer.data()),
              static_cast<std::streamsize>(buffer.size()));
  }
  rapidjson::Document doc;
  doc.Parse(reinterpret_cast<const char*>(buffer.data()), buffer.size());
  if (doc.HasParseError() || !doc.IsObject()) {
    return false;
  }
  by_id_.clear();
  for (auto it = doc.MemberBegin(); it != doc.MemberEnd(); ++it) {
    uint32_t id = 0;
    try {
      id = static_cast<uint32_t>(std::stoul(it->name.GetString(), nullptr, 16));
    } catch (...) {
      continue;
    }
    if (!it->value.IsObject()) {
      continue;
    }
    GameMetadata meta;
    meta.title_id = id;
    meta.title_name = JsonString(it->value, "title");
    meta.developer = JsonString(it->value, "developer");
    meta.publisher = JsonString(it->value, "publisher");
    meta.genre = JsonString(it->value, "genre");
    meta.release_date = JsonString(it->value, "release_date");
    meta.description = JsonString(it->value, "description");
    by_id_[id] = std::move(meta);
  }
  loaded_ = !by_id_.empty();
  return loaded_;
}

void GameMetadataService::EnsureLoaded() {
  std::lock_guard lock(mu_);
  if (load_attempted_) {
    return;
  }
  load_attempted_ = true;
  LoadFromCache();
  if (!loaded_) {
    FetchFromNetwork();
  }
}

std::optional<GameMetadata> GameMetadataService::Lookup(uint32_t title_id) {
  EnsureLoaded();
  std::lock_guard lock(mu_);
  auto it = by_id_.find(title_id);
  if (it == by_id_.end()) {
    return std::nullopt;
  }
  return it->second;
}

std::optional<GameMetadata> GameMetadataService::LookupByName(
    const std::string& title_name) {
  EnsureLoaded();
  std::lock_guard lock(mu_);
  for (const auto& [id, meta] : by_id_) {
    if (meta.title_name == title_name) {
      return meta;
    }
    (void)id;
  }
  return std::nullopt;
}

void GameMetadataService::PrefetchAsync(uint32_t title_id) {
  std::thread([this, title_id]() { (void)Lookup(title_id); }).detach();
}

}  // namespace app
}  // namespace xe
