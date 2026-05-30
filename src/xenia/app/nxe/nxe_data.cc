/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Phoenix. All rights reserved.                       *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/app/nxe/nxe_data.h"

#include "xenia/base/string_util.h"
#include "xenia/emulator.h"
#include "xenia/kernel/kernel_state.h"
#include "xenia/kernel/netplay/xnet_types.h"
#include "xenia/kernel/xam/achievement_manager.h"
#include "xenia/kernel/xam/user_profile.h"
#include "xenia/kernel/xam/user_tracker.h"
#include "xenia/kernel/xam/xam_state.h"

namespace xe {
namespace app {
namespace nxe {

NxeDataAdapter::NxeDataAdapter(Emulator* emulator) : emulator_(emulator) {}

kernel::xam::UserProfile* NxeDataAdapter::ProfileForSlot(uint8_t slot) const {
  if (!emulator_ || !emulator_->kernel_state() ||
      !emulator_->kernel_state()->xam_state()) {
    return nullptr;
  }
  return emulator_->kernel_state()->xam_state()->GetUserProfile(
      static_cast<uint32_t>(slot));
}

kernel::xam::UserProfile* NxeDataAdapter::ProfileForXuid(uint64_t xuid) const {
  if (!emulator_ || !emulator_->kernel_state() ||
      !emulator_->kernel_state()->xam_state()) {
    return nullptr;
  }
  return emulator_->kernel_state()->xam_state()->GetUserProfileAny(xuid);
}

std::vector<ProfileSummary> NxeDataAdapter::ListProfiles() const {
  std::vector<ProfileSummary> out;
  if (!emulator_ || !emulator_->kernel_state() ||
      !emulator_->kernel_state()->xam_state()) {
    return out;
  }
  auto* xam = emulator_->kernel_state()->xam_state();
  for (uint8_t i = 0; i < 4; ++i) {
    if (!xam->IsUserSignedIn(static_cast<uint32_t>(i))) {
      continue;
    }
    auto* profile = xam->GetUserProfile(static_cast<uint32_t>(i));
    if (!profile) {
      continue;
    }
    ProfileSummary s;
    s.slot = i;
    s.xuid = profile->xuid();
    s.gamertag = profile->name();
    s.signed_in = true;
    s.gamerscore = TotalGamerscore(s.xuid);
    auto icon = profile->GetProfileIcon(kernel::xam::XTileType::kGamerTile);
    if (!icon.empty()) {
      s.avatar_rgba.assign(icon.begin(), icon.end());
      s.avatar_w = kernel::xam::kProfileIconSize.first;
      s.avatar_h = kernel::xam::kProfileIconSize.second;
    }
    out.push_back(std::move(s));
  }
  return out;
}

ProfileSummary NxeDataAdapter::ActiveProfile() const {
  ProfileSummary s;
  if (auto* profile = ProfileForSlot(active_profile_slot_)) {
    s.slot = active_profile_slot_;
    s.xuid = profile->xuid();
    s.gamertag = profile->name();
    s.signed_in = true;
    s.gamerscore = TotalGamerscore(s.xuid);
    auto icon = profile->GetProfileIcon(kernel::xam::XTileType::kGamerTile);
    if (!icon.empty()) {
      s.avatar_rgba.assign(icon.begin(), icon.end());
      s.avatar_w = kernel::xam::kProfileIconSize.first;
      s.avatar_h = kernel::xam::kProfileIconSize.second;
    }
    return s;
  }
  auto profiles = ListProfiles();
  if (!profiles.empty()) {
    return profiles.front();
  }
  s.gamertag = "Player";
  return s;
}

void NxeDataAdapter::SetActiveProfileSlot(uint8_t slot) {
  active_profile_slot_ = slot;
}

uint32_t NxeDataAdapter::TotalGamerscore(uint64_t xuid) const {
  if (!emulator_ || !emulator_->kernel_state() ||
      !emulator_->kernel_state()->xam_state()) {
    return 0;
  }
  auto* tracker = emulator_->kernel_state()->xam_state()->user_tracker();
  if (!tracker) {
    return 0;
  }
  uint32_t total = 0;
  for (const auto& title : tracker->GetPlayedTitles(xuid)) {
    total += title.title_earned_gamerscore;
  }
  return total;
}

std::vector<TitleAchievementSummary> NxeDataAdapter::ListPlayedTitles(
    uint64_t xuid) const {
  std::vector<TitleAchievementSummary> out;
  if (!emulator_ || !emulator_->kernel_state() ||
      !emulator_->kernel_state()->xam_state()) {
    return out;
  }
  auto* tracker = emulator_->kernel_state()->xam_state()->user_tracker();
  if (!tracker) {
    return out;
  }
  for (const auto& t : tracker->GetPlayedTitles(xuid)) {
    TitleAchievementSummary s;
    s.title_id = t.id;
    s.title_name = to_utf8(t.title_name);
    s.earned_gs = t.title_earned_gamerscore;
    s.total_gs = t.gamerscore_amount;
    s.unlocked = t.unlocked_achievements_count;
    s.total = t.achievements_count;
    out.push_back(std::move(s));
  }
  return out;
}

std::vector<AchievementEntry> NxeDataAdapter::AchievementsForTitle(
    uint64_t xuid, uint32_t title_id) const {
  std::vector<AchievementEntry> out;
  if (!emulator_ || !emulator_->kernel_state() ||
      !emulator_->kernel_state()->xam_state()) {
    return out;
  }
  auto* tracker = emulator_->kernel_state()->xam_state()->user_tracker();
  if (!tracker) {
    return out;
  }
  for (const auto& ach : tracker->GetUserTitleAchievements(xuid, title_id)) {
    AchievementEntry a;
    a.id = ach.achievement_id;
    a.label = to_utf8(ach.achievement_name);
    a.description = to_utf8(ach.unlocked_description);
    a.unachieved_description = to_utf8(ach.locked_description);
    a.gamerscore = ach.gamerscore;
    a.unlocked = ach.IsUnlocked();
    auto tile = tracker->GetAchievementIcon(xuid, title_id, ach.achievement_id);
    if (!tile.empty()) {
      a.tile_rgba.assign(tile.begin(), tile.end());
      a.tile_w = 64;
      a.tile_h = 64;
    }
    out.push_back(std::move(a));
  }
  return out;
}

std::vector<FriendEntry> NxeDataAdapter::ListFriends(uint64_t xuid) const {
  std::vector<FriendEntry> out;
  auto* profile = ProfileForXuid(xuid);
  if (!profile) {
    return out;
  }
  for (const auto& f : profile->GetFriends()) {
    FriendEntry e;
    e.xuid = static_cast<uint64_t>(f.xuid);
    e.gamertag = f.Gamertag;
    e.online = (static_cast<uint32_t>(f.state) &
                X_ONLINE_FRIENDSTATE_FLAG_ONLINE) != 0;
    e.presence = e.online ? "Online" : "Offline";
    out.push_back(std::move(e));
  }
  return out;
}

}  // namespace nxe
}  // namespace app
}  // namespace xe
