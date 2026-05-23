/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Phoenix. All rights reserved.                       *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/app/library/game_library.h"

#include <algorithm>
#include <fstream>
#include <thread>

#include "third_party/fmt/include/fmt/format.h"
#include "third_party/stb/stb_image_write.h"
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wabsolute-value"
#endif
#include "third_party/tomlplusplus/toml.hpp"
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
#include "xenia/base/filesystem.h"
#include "xenia/base/logging.h"
#include "xenia/base/string.h"

namespace xe {
namespace app {
namespace library {

namespace {

constexpr std::string_view kLibraryFilename = "library.toml";
constexpr std::string_view kLibraryVersionKey = "version";
constexpr int kLibraryVersion = 1;

std::string TitleIdHex(uint32_t title_id) {
  return fmt::format("{:08X}", title_id);
}

}  // namespace

GameLibrary::GameLibrary(std::filesystem::path storage_root)
    : storage_root_(std::move(storage_root)),
      library_path_(storage_root_ / kLibraryFilename) {
  std::error_code ec;
  std::filesystem::create_directories(IconsCacheDir(), ec);
}

std::filesystem::path GameLibrary::IconsCacheDir() const {
  return storage_root_ / "cache" / "icons";
}

void GameLibrary::Load() {
  watched_directories_.clear();
  entries_.clear();

  if (!std::filesystem::exists(library_path_)) {
    return;
  }

  try {
    auto parsed = toml::parse_file(xe::path_to_utf8(library_path_));
    if (auto* dirs = parsed["watched_directories"].as_array()) {
      for (const auto& node : *dirs) {
        if (auto* s = node.as_string()) {
          std::filesystem::path p = xe::to_path(s->get());
          std::error_code ec;
          if (std::filesystem::exists(p, ec)) {
            watched_directories_.push_back(std::move(p));
          }
        }
      }
    }
    if (auto* games = parsed["games"].as_table()) {
      for (const auto& [key, entry_node] : *games) {
        if (!entry_node.is_table()) {
          continue;
        }
        const auto* t = entry_node.as_table();
        LibraryEntry e;
        if (auto* v = t->get("title_id")) {
          if (auto* s = v->as_string()) {
            e.title_id = std::stoul(s->get(), nullptr, 16);
          } else if (auto* i = v->as_integer()) {
            e.title_id = static_cast<uint32_t>(i->get());
          }
        }
        e.title_id_hex = TitleIdHex(e.title_id);
        if (auto* v = t->get("title_name")) {
          if (auto* s = v->as_string()) {
            e.title_name = s->get();
          }
        }
        if (auto* v = t->get("path")) {
          if (auto* s = v->as_string()) {
            e.path = xe::to_path(s->get());
          }
        }
        if (auto* v = t->get("icon_path")) {
          if (auto* s = v->as_string()) {
            e.icon_path = xe::to_path(s->get());
          }
        }
        if (auto* v = t->get("last_played")) {
          if (auto* i = v->as_integer()) {
            e.last_played = static_cast<std::time_t>(i->get());
          }
        }
        if (auto* v = t->get("play_seconds")) {
          if (auto* i = v->as_integer()) {
            e.play_seconds = static_cast<uint64_t>(i->get());
          }
        }
        if (auto* v = t->get("media_type")) {
          if (auto* s = v->as_string()) {
            e.media_type = s->get();
          }
        }
        std::error_code ec;
        if (!e.path.empty() && std::filesystem::exists(e.path, ec)) {
          entries_.push_back(std::move(e));
        }
      }
    }
  } catch (const std::exception& ex) {
    XELOGE("GameLibrary: failed to parse {}: {}",
           xe::path_to_utf8(library_path_), ex.what());
  }
}

void GameLibrary::Save() {
  std::lock_guard<std::mutex> lock(save_mu_);
  toml::table root;
  root.insert(kLibraryVersionKey, kLibraryVersion);

  toml::array dirs;
  for (const auto& d : watched_directories_) {
    dirs.push_back(xe::path_to_utf8(d));
  }
  root.insert("watched_directories", dirs);

  toml::table games;
  for (const auto& e : entries_) {
    toml::table entry;
    entry.insert("title_id", e.title_id_hex);
    entry.insert("title_name", e.title_name);
    entry.insert("path", xe::path_to_utf8(e.path));
    if (!e.icon_path.empty()) {
      entry.insert("icon_path", xe::path_to_utf8(e.icon_path));
    }
    entry.insert("last_played", static_cast<int64_t>(e.last_played));
    entry.insert("play_seconds", static_cast<int64_t>(e.play_seconds));
    entry.insert("media_type", e.media_type);
    games.insert(
        e.title_id_hex.empty() ? xe::path_to_utf8(e.path) : e.title_id_hex,
        entry);
  }
  root.insert("games", games);

  xe::filesystem::CreateParentFolder(library_path_);
  std::ofstream file(library_path_, std::ofstream::trunc);
  if (!file) {
    XELOGE("GameLibrary: failed to write {}", xe::path_to_utf8(library_path_));
    return;
  }
  file << root;
}

void GameLibrary::AddWatchedDirectory(const std::filesystem::path& path) {
  std::error_code ec;
  std::filesystem::path canonical = std::filesystem::weakly_canonical(path, ec);
  if (ec) {
    canonical = path;
  }
  for (const auto& existing : watched_directories_) {
    if (existing == canonical) {
      return;
    }
  }
  watched_directories_.push_back(canonical);
  Save();
}

void GameLibrary::RemoveWatchedDirectory(const std::filesystem::path& path) {
  std::error_code ec;
  std::filesystem::path canonical = std::filesystem::weakly_canonical(path, ec);
  if (ec) {
    canonical = path;
  }
  watched_directories_.erase(std::remove(watched_directories_.begin(),
                                         watched_directories_.end(), canonical),
                             watched_directories_.end());
  Save();
}

void GameLibrary::ImportRecentTitles(
    const std::vector<std::pair<std::string, std::filesystem::path>>& recent) {
  for (const auto& [name, path] : recent) {
    if (FindByPath(path)) {
      continue;
    }
    LibraryEntry e;
    e.path = path;
    e.title_name = name.empty() ? xe::path_to_utf8(path.filename()) : name;
    e.media_type = xe::path_to_utf8(path.extension());
    if (!e.media_type.empty() && e.media_type.front() == '.') {
      e.media_type.erase(e.media_type.begin());
    }
    e.last_played = std::time(nullptr);
    entries_.push_back(std::move(e));
  }
  Save();
}

LibraryEntry GameLibrary::FromDiscovered(const DiscoveredGame& g) {
  LibraryEntry e;
  e.path = g.path;
  e.title_id = g.title_id;
  e.title_id_hex = g.title_id_hex;
  e.media_type = g.format;
  const std::string& preferred = PreferredName(g);
  e.title_name =
      preferred.empty() ? xe::path_to_utf8(g.path.filename()) : preferred;
  if (!g.icon_png.empty() && g.title_id) {
    WriteIconPng(g.title_id, g.icon_png);
    e.icon_path = IconsCacheDir() / (TitleIdHex(g.title_id) + ".png");
  } else if (!g.icon_rgba.empty() && g.title_id) {
    WriteIconPngFromRgba(g.title_id, g.icon_width, g.icon_height, g.icon_rgba);
    e.icon_path = IconsCacheDir() / (TitleIdHex(g.title_id) + ".png");
  }
  return e;
}

void GameLibrary::WriteIconPng(uint32_t title_id,
                               const std::vector<uint8_t>& png) {
  auto out = IconsCacheDir() / (TitleIdHex(title_id) + ".png");
  std::ofstream file(out, std::ios::binary | std::ios::trunc);
  if (file) {
    file.write(reinterpret_cast<const char*>(png.data()),
               static_cast<std::streamsize>(png.size()));
  }
}

void GameLibrary::WriteIconPngFromRgba(uint32_t title_id, int width, int height,
                                       const std::vector<uint8_t>& rgba) {
  auto out = IconsCacheDir() / (TitleIdHex(title_id) + ".png");
  stbi_write_png(xe::path_to_utf8(out).c_str(), width, height, 4, rgba.data(),
                 width * 4);
}

void GameLibrary::MergeScanResults(
    const std::vector<DiscoveredGame>& discovered) {
  for (const auto& g : discovered) {
    if (g.title_id == 0 && g.path.empty()) {
      continue;
    }
    LibraryEntry* existing = FindByPath(g.path);
    if (!existing && g.title_id) {
      existing = FindByTitleId(g.title_id);
    }
    LibraryEntry merged = FromDiscovered(g);
    if (existing) {
      merged.last_played = existing->last_played;
      merged.play_seconds = existing->play_seconds;
      if (merged.icon_path.empty()) {
        merged.icon_path = existing->icon_path;
      }
      *existing = std::move(merged);
    } else {
      entries_.push_back(std::move(merged));
    }
  }
  Save();
}

void GameLibrary::StartScan() {
  CancelScan();
  if (watched_directories_.empty()) {
    return;
  }

  scan_active_.store(true);
  auto roots = watched_directories_;

  scan_threads_.emplace_back([this, roots = std::move(roots)]() {
    std::vector<DiscoveredGame> all_results;
    for (const auto& root : roots) {
      if (!scan_active_.load()) {
        break;
      }
      DirectoryScanner scanner;
      scanner.Start(root);
      while (scan_active_.load()) {
        {
          std::lock_guard<std::mutex> lock(scan_mu_);
          last_scan_progress_ = scanner.GetProgress();
        }
        if (last_scan_progress_.done) {
          break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }
      auto partial = scanner.GetAllResults();
      all_results.insert(all_results.end(),
                         std::make_move_iterator(partial.begin()),
                         std::make_move_iterator(partial.end()));
    }
    MergeScanResults(all_results);
    scan_active_.store(false);
  });
}

void GameLibrary::CancelScan() {
  scan_active_.store(false);
  for (auto& t : scan_threads_) {
    if (t.joinable()) {
      t.join();
    }
  }
  scan_threads_.clear();
}

bool GameLibrary::IsScanning() const { return scan_active_.load(); }

DirectoryScanner::Progress GameLibrary::ScanProgress() const {
  std::lock_guard<std::mutex> lock(scan_mu_);
  return last_scan_progress_;
}

std::vector<LibraryEntry> GameLibrary::MergedEntries() const {
  return entries_;
}

void GameLibrary::RemoveEntry(const std::filesystem::path& path) {
  entries_.erase(
      std::remove_if(entries_.begin(), entries_.end(),
                     [&](const LibraryEntry& e) { return e.path == path; }),
      entries_.end());
  Save();
}

void GameLibrary::RecordPlay(const std::filesystem::path& path,
                             uint32_t title_id, const std::string& title_name) {
  LibraryEntry* e = FindByPath(path);
  if (!e && title_id) {
    e = FindByTitleId(title_id);
  }
  if (!e) {
    LibraryEntry ne;
    ne.path = path;
    ne.title_id = title_id;
    ne.title_id_hex = TitleIdHex(title_id);
    ne.title_name = title_name;
    ne.last_played = std::time(nullptr);
    entries_.push_back(std::move(ne));
    e = &entries_.back();
  } else {
    if (title_id) {
      e->title_id = title_id;
      e->title_id_hex = TitleIdHex(title_id);
    }
    if (!title_name.empty()) {
      e->title_name = title_name;
    }
    e->last_played = std::time(nullptr);
  }
  Save();
}

void GameLibrary::CacheIcon(uint32_t title_id,
                            const std::vector<uint8_t>& png_bytes) {
  if (!title_id || png_bytes.empty()) {
    return;
  }
  WriteIconPng(title_id, png_bytes);
  auto icon_path = IconsCacheDir() / (TitleIdHex(title_id) + ".png");
  if (auto* e = FindByTitleId(title_id)) {
    e->icon_path = icon_path;
    Save();
  }
}

void GameLibrary::CacheIconRgba(uint32_t title_id, int width, int height,
                                const std::vector<uint8_t>& rgba) {
  if (!title_id || rgba.empty()) {
    return;
  }
  WriteIconPngFromRgba(title_id, width, height, rgba);
  auto icon_path = IconsCacheDir() / (TitleIdHex(title_id) + ".png");
  if (auto* e = FindByTitleId(title_id)) {
    e->icon_path = icon_path;
    Save();
  }
}

LibraryEntry* GameLibrary::FindByPath(const std::filesystem::path& path) {
  for (auto& e : entries_) {
    if (e.path == path) {
      return &e;
    }
  }
  return nullptr;
}

LibraryEntry* GameLibrary::FindByTitleId(uint32_t title_id) {
  for (auto& e : entries_) {
    if (e.title_id == title_id) {
      return &e;
    }
  }
  return nullptr;
}

}  // namespace library
}  // namespace app
}  // namespace xe
