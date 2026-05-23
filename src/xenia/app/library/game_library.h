/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Phoenix. All rights reserved.                       *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_APP_LIBRARY_GAME_LIBRARY_H_
#define XENIA_APP_LIBRARY_GAME_LIBRARY_H_

#include <atomic>
#include <cstdint>
#include <ctime>
#include <filesystem>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "xenia/app/library/directory_scanner.h"

namespace xe {
namespace app {
namespace library {

struct LibraryEntry {
  uint32_t title_id = 0;
  std::string title_name;
  std::filesystem::path path;
  std::filesystem::path icon_path;
  std::time_t last_played = 0;
  uint64_t play_seconds = 0;
  std::string media_type;
  std::string title_id_hex;
};

// Persistent game library: watched folders, scanned entries, icon cache.
class GameLibrary {
 public:
  explicit GameLibrary(std::filesystem::path storage_root);

  const std::filesystem::path& storage_root() const { return storage_root_; }
  const std::vector<std::filesystem::path>& watched_directories() const {
    return watched_directories_;
  }
  const std::vector<LibraryEntry>& entries() const { return entries_; }

  void Load();
  void Save();

  void AddWatchedDirectory(const std::filesystem::path& path);
  void RemoveWatchedDirectory(const std::filesystem::path& path);

  void ImportRecentTitles(
      const std::vector<std::pair<std::string, std::filesystem::path>>& recent);

  void StartScan();
  void CancelScan();
  bool IsScanning() const;
  DirectoryScanner::Progress ScanProgress() const;
  std::vector<LibraryEntry> MergedEntries() const;

  void RemoveEntry(const std::filesystem::path& path);
  void RecordPlay(const std::filesystem::path& path, uint32_t title_id,
                  const std::string& title_name);
  void CacheIcon(uint32_t title_id, const std::vector<uint8_t>& png_bytes);
  void CacheIconRgba(uint32_t title_id, int width, int height,
                     const std::vector<uint8_t>& rgba);

  LibraryEntry* FindByPath(const std::filesystem::path& path);
  LibraryEntry* FindByTitleId(uint32_t title_id);

  std::filesystem::path IconsCacheDir() const;

 private:
  void MergeScanResults(const std::vector<DiscoveredGame>& discovered);
  LibraryEntry FromDiscovered(const DiscoveredGame& g);
  void WriteIconPng(uint32_t title_id, const std::vector<uint8_t>& png);
  void WriteIconPngFromRgba(uint32_t title_id, int width, int height,
                            const std::vector<uint8_t>& rgba);

  std::filesystem::path storage_root_;
  std::filesystem::path library_path_;

  std::vector<std::filesystem::path> watched_directories_;
  std::vector<LibraryEntry> entries_;

  mutable std::mutex scan_mu_;
  std::vector<std::thread> scan_threads_;
  std::atomic<bool> scan_active_{false};
  DirectoryScanner::Progress last_scan_progress_;
};

}  // namespace library
}  // namespace app
}  // namespace xe

#endif  // XENIA_APP_LIBRARY_GAME_LIBRARY_H_
