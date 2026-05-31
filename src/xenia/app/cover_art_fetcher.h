/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Phoenix. All rights reserved.                       *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_APP_COVER_ART_FETCHER_H_
#define XENIA_APP_COVER_ART_FETCHER_H_

#include <cstdint>
#include <filesystem>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

namespace xe {
namespace app {

// Fetches grid/cover art from SteamGridDB (optional API key) with local cache.
class CoverArtFetcher {
 public:
  explicit CoverArtFetcher(std::filesystem::path storage_root);

  std::filesystem::path CoversCacheDir() const;
  std::filesystem::path CoverPathForTitle(uint32_t title_id) const;

  bool HasCachedCover(uint32_t title_id) const;
  void SetApiKey(const std::string& api_key) { api_key_ = api_key; }

  using FetchCallback = std::function<void(uint32_t title_id, bool success)>;
  void FetchCoverAsync(uint32_t title_id, const std::string& search_name,
                       FetchCallback callback);

 private:
  bool DownloadUrlToFile(const std::string& url,
                         const std::filesystem::path& dest);
  bool FetchFromSteamGridDB(uint32_t title_id, const std::string& search_name,
                            const std::filesystem::path& dest);

  std::filesystem::path storage_root_;
  std::string api_key_;
  std::mutex mu_;
};

}  // namespace app
}  // namespace xe

#endif  // XENIA_APP_COVER_ART_FETCHER_H_
