/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2025 Xenia Canary. All rights reserved.                          *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_KERNEL_NETPLAY_NETWORK_ADAPTER_MANAGER_H_
#define XENIA_KERNEL_NETPLAY_NETWORK_ADAPTER_MANAGER_H_

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "xenia/kernel/netplay/net_utils.h"

namespace xe {
namespace kernel {

struct NetworkAdapterInfo {
  std::string adapter_name;
  std::string friendly_name;
  int32_t if_index = -1;
  uint32_t if_type = 0;
  uint32_t oper_status = 0;
  std::optional<MacAddress> mac;
  sockaddr_in ipv4{};
};

class NetworkAdapterManager {
 public:
  NetworkAdapterManager();

  void Initialize();

  void SelectBestInterface();

  void SetSelectedAdapterGUID(const std::string guid);

  std::string GetSelectedAdapaterGUID() const;

  std::string GetSelectedAdapterDesciption() const;

  std::optional<NetworkAdapterInfo> GetAdapterFromGUID(
      const std::string guid) const;

  std::optional<NetworkAdapterInfo> GetAdapterFromIfIndex(
      int32_t IfIndex) const;

  std::string GetAdapterFriendlyName(const NetworkAdapterInfo& adapter) const;

  std::vector<std::string> GetAdaptersNames() const;

  MacAddress GetAdapterMacAddressFromGUID(const std::string guid) const;

  std::optional<NetworkAdapterInfo> GetSelectedAdapter() const;

  std::string GetSelectedAdapterName() const;

  sockaddr_in GetSelectedAdapterLocalIP() const { return local_ip_; }

  std::string GetSelectedAdapterLocalIPString() const;

  const std::vector<NetworkAdapterInfo>& GetAdapters() const {
    return adapter_addresses_;
  }

  bool IsSelectedAdapterWANRouting() const { return is_WAN_routing_; }

  bool IsInterfaceSelected() const;

 private:
  void ResetSelectedAdapter();

  std::vector<NetworkAdapterInfo> DiscoverNetworkAdapters();

  bool UpdateNetworkInterface(const NetworkAdapterInfo& adapter);

  int32_t GetBestInterfaceIfIndex();

  bool IsInterfaceWANRouting(sockaddr_in interface_addr);

  std::optional<sockaddr_in> GetInterfaceIPFromIfIndex(int32_t IfIndex) const;

  bool SelectSavedNetworkAdapter();

  void AutoSelectNetworkAdapter(int32_t best_interface_IfIndex);

  std::vector<NetworkAdapterInfo> adapter_addresses_;
  int32_t best_interface_IfIndex_ = -1;
  sockaddr_in local_ip_{};
  bool is_WAN_routing_ = false;

#if XE_PLATFORM_WIN32
  std::vector<uint8_t> adapter_addresses_data_;
#endif
};

}  // namespace kernel
}  // namespace xe

#endif  // XENIA_KERNEL_NETPLAY_NETWORK_ADAPTER_MANAGER_H_
