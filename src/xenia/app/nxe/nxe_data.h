/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia Phoenix. All rights reserved.                       *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_APP_NXE_NXE_DATA_H_
#define XENIA_APP_NXE_NXE_DATA_H_

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace xe {
class Emulator;
namespace kernel {
namespace xam {
class UserProfile;
}  // namespace xam
}  // namespace kernel
}  // namespace xe

namespace xe {
namespace app {
namespace nxe {

struct ProfileSummary {
  uint8_t slot = 0;
  uint64_t xuid = 0;
  std::string gamertag;
  uint32_t gamerscore = 0;
  std::vector<uint8_t> avatar_rgba;
  int avatar_w = 0;
  int avatar_h = 0;
  bool signed_in = false;
};

struct AchievementEntry {
  uint32_t id = 0;
  std::string label;
  std::string description;
  std::string unachieved_description;
  uint32_t gamerscore = 0;
  bool unlocked = false;
  uint64_t unlock_time = 0;
  std::vector<uint8_t> tile_rgba;
  int tile_w = 0;
  int tile_h = 0;
};

struct TitleAchievementSummary {
  uint32_t title_id = 0;
  std::string title_name;
  uint32_t earned_gs = 0;
  uint32_t total_gs = 0;
  uint32_t unlocked = 0;
  uint32_t total = 0;
};

struct FriendEntry {
  uint64_t xuid = 0;
  std::string gamertag;
  std::string presence;
  bool online = false;
};

class NxeDataAdapter {
 public:
  explicit NxeDataAdapter(Emulator* emulator);

  std::vector<ProfileSummary> ListProfiles() const;
  ProfileSummary ActiveProfile() const;
  uint32_t TotalGamerscore(uint64_t xuid) const;

  std::vector<TitleAchievementSummary> ListPlayedTitles(uint64_t xuid) const;
  std::vector<AchievementEntry> AchievementsForTitle(uint64_t xuid,
                                                     uint32_t title_id) const;

  std::vector<FriendEntry> ListFriends(uint64_t xuid) const;

  void SetActiveProfileSlot(uint8_t slot);
  uint8_t active_profile_slot() const { return active_profile_slot_; }

 private:
  kernel::xam::UserProfile* ProfileForXuid(uint64_t xuid) const;
  kernel::xam::UserProfile* ProfileForSlot(uint8_t slot) const;

  Emulator* emulator_ = nullptr;
  uint8_t active_profile_slot_ = 0;
};

}  // namespace nxe
}  // namespace app
}  // namespace xe

#endif  // XENIA_APP_NXE_NXE_DATA_H_
