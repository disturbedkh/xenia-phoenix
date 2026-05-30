/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Phoenix. All rights reserved.                       *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_APP_GAME_METADATA_H_
#define XENIA_APP_GAME_METADATA_H_

#include <cstdint>
#include <filesystem>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

namespace xe {
namespace app {

struct GameMetadata {
  uint32_t title_id = 0;
  std::string title_name;
  std::string developer;
  std::string publisher;
  std::string genre;
  std::string release_date;
  std::string compatibility_status;
  std::string description;
};

// Lazy-loaded x360db cache (network fetch with local disk cache).
class GameMetadataService {
 public:
  explicit GameMetadataService(std::filesystem::path storage_root);

  std::optional<GameMetadata> Lookup(uint32_t title_id);
  std::optional<GameMetadata> LookupByName(const std::string& title_name);
  void PrefetchAsync(uint32_t title_id);

  bool IsLoaded() const { return loaded_; }
  void EnsureLoaded();

 private:
  void LoadFromCache();
  void SaveToCache();
  bool FetchFromNetwork();
  void ParseGameEntry(uint32_t title_id, const std::string& json_fragment);

  std::filesystem::path storage_root_;
  std::filesystem::path cache_path_;
  std::unordered_map<uint32_t, GameMetadata> by_id_;
  std::mutex mu_;
  bool loaded_ = false;
  bool load_attempted_ = false;
};

}  // namespace app
}  // namespace xe

#endif  // XENIA_APP_GAME_METADATA_H_
