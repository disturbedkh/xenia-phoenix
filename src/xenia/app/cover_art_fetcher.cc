/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Phoenix. All rights reserved.                       *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/app/cover_art_fetcher.h"

#include <cctype>
#include <fstream>
#include <thread>

#include "third_party/fmt/include/fmt/format.h"
#include "third_party/libcurl/include/curl/curl.h"
#include "third_party/rapidjson/include/rapidjson/document.h"
#include "xenia/app/theme_manager.h"
#include "xenia/base/logging.h"
#include "xenia/base/platform.h"

namespace xe {
namespace app {
namespace {

constexpr const char* kCoversSubdir = "cache/covers";
constexpr const char* kHeroesSubdir = "cache/heroes";
constexpr const char* kLogosSubdir = "cache/logos";
constexpr const char* kIconsSubdir = "cache/icons";

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
  const size_t total = size * nmemb;
  auto* buffer = static_cast<std::vector<uint8_t>*>(userp);
  const auto* data = static_cast<const uint8_t*>(contents);
  buffer->insert(buffer->end(), data, data + total);
  return total;
}

std::string HttpGet(const std::string& url, const std::string& api_key) {
  std::vector<uint8_t> buffer;
  CURL* curl = curl_easy_init();
  if (!curl) {
    return {};
  }
  struct curl_slist* headers = nullptr;
  const std::string auth = "Authorization: Bearer " + api_key;
  headers = curl_slist_append(headers, auth.c_str());
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "xenia-phoenix");
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  const CURLcode result = curl_easy_perform(curl);
  long code = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);
  if (result != CURLE_OK || code != 200 || buffer.empty()) {
    return {};
  }
  return std::string(buffer.begin(), buffer.end());
}

}  // namespace

CoverArtFetcher::CoverArtFetcher(std::filesystem::path storage_root)
    : storage_root_(std::move(storage_root)) {
  api_key_ = ThemeManager::Instance().config().steamgriddb_api_key;
}

std::filesystem::path CoverArtFetcher::CoversCacheDir() const {
  return storage_root_ / kCoversSubdir;
}

std::filesystem::path CoverArtFetcher::CoverPathForTitle(
    uint32_t title_id) const {
  return CoversCacheDir() /
         fmt::format("{:08X}.png", static_cast<uint32_t>(title_id));
}

bool CoverArtFetcher::HasCachedCover(uint32_t title_id) const {
  std::error_code ec;
  return std::filesystem::exists(CoverPathForTitle(title_id), ec);
}

bool CoverArtFetcher::DownloadUrlToFile(const std::string& url,
                                        const std::filesystem::path& dest) {
  std::vector<uint8_t> buffer;
  CURL* curl = curl_easy_init();
  if (!curl) {
    return false;
  }
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
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
  std::filesystem::create_directories(dest.parent_path(), ec);
  std::ofstream out(dest, std::ios::binary);
  if (!out) {
    return false;
  }
  out.write(reinterpret_cast<const char*>(buffer.data()),
            static_cast<std::streamsize>(buffer.size()));
  return true;
}

bool CoverArtFetcher::FetchFromSteamGridDB(uint32_t title_id,
                                           const std::string& search_name,
                                           const std::filesystem::path& dest) {
  if (api_key_.empty()) {
    api_key_ = ThemeManager::Instance().config().steamgriddb_api_key;
  }
  if (api_key_.empty() || search_name.empty()) {
    return false;
  }

  const std::string encoded = [&search_name]() {
    std::string out;
    for (char c : search_name) {
      if (std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_') {
        out += c;
      } else if (c == ' ') {
        out += "%20";
      }
    }
    return out;
  }();
  const std::string search_url =
      "https://www.steamgriddb.com/api/v2/search/autocomplete/" + encoded;
  const std::string search_json = HttpGet(search_url, api_key_);
  if (search_json.empty()) {
    return false;
  }
  rapidjson::Document search_doc;
  search_doc.Parse(search_json.c_str());
  if (search_doc.HasParseError() || !search_doc.HasMember("data") ||
      !search_doc["data"].IsArray() || search_doc["data"].Empty()) {
    return false;
  }
  const auto& first = search_doc["data"][0];
  if (!first.HasMember("id") || !first["id"].IsInt()) {
    return false;
  }
  const int game_id = first["id"].GetInt();

  const std::string grids_url =
      fmt::format("https://www.steamgriddb.com/api/v2/grids/game/{}", game_id);
  const std::string grids_json = HttpGet(grids_url, api_key_);
  if (grids_json.empty()) {
    return false;
  }
  rapidjson::Document grids_doc;
  grids_doc.Parse(grids_json.c_str());
  if (grids_doc.HasParseError() || !grids_doc.HasMember("data") ||
      !grids_doc["data"].IsArray() || grids_doc["data"].Empty()) {
    return false;
  }
  const auto& grid = grids_doc["data"][0];
  if (!grid.HasMember("url") || !grid["url"].IsString()) {
    return false;
  }
  (void)title_id;
  return DownloadUrlToFile(grid["url"].GetString(), dest);
}

void CoverArtFetcher::FetchCoverAsync(uint32_t title_id,
                                      const std::string& search_name,
                                      FetchCallback callback) {
  if (HasCachedCover(title_id)) {
    if (callback) {
      callback(title_id, true);
    }
    return;
  }
  std::thread([this, title_id, search_name, callback]() {
    const auto dest = CoverPathForTitle(title_id);
    bool ok = FetchFromSteamGridDB(title_id, search_name, dest);
    if (callback) {
      callback(title_id, ok);
    }
  }).detach();
}

}  // namespace app
}  // namespace xe
