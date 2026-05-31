/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2025 Xenia Canary. All rights reserved.                          *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/kernel/netplay/network_adapter_manager.h"

#include "xenia/base/cvar.h"
#include "xenia/base/logging.h"
#include "xenia/base/platform.h"
#include "xenia/base/string_util.h"

#if XE_PLATFORM_WIN32
#include <Iphlpapi.h>
#include <WinSock2.h>
#else
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)
typedef int SOCKET;
inline int closesocket(SOCKET s) { return close(s); }
#endif

DEFINE_string(network_guid, "", "Network Interface GUID", "Live");

namespace xe {
namespace kernel {

NetworkAdapterManager::NetworkAdapterManager() {}

void NetworkAdapterManager::Initialize() {
  adapter_addresses_ = DiscoverNetworkAdapters();

  if (!adapter_addresses_.empty()) {
    best_interface_IfIndex_ = GetBestInterfaceIfIndex();
    AutoSelectNetworkAdapter(best_interface_IfIndex_);
  }
}

void NetworkAdapterManager::SelectBestInterface() {
  ResetSelectedAdapter();
  Initialize();
}

void NetworkAdapterManager::SetSelectedAdapterGUID(const std::string guid) {
  ResetSelectedAdapter();

  const auto adapter = GetAdapterFromGUID(guid);

  if (adapter.has_value()) {
    UpdateNetworkInterface(adapter.value());
  }

  XELOGI(GetSelectedAdapterDesciption());
}

std::string NetworkAdapterManager::GetSelectedAdapaterGUID() const {
  return cvars::network_guid;
}

std::string NetworkAdapterManager::GetSelectedAdapterDesciption() const {
  return fmt::format("Network Interface: {} {} {}", GetSelectedAdapterName(),
                     GetSelectedAdapterLocalIPString(),
                     is_WAN_routing_ ? "(WAN Routing)" : "(Non-WAN Routing)");
}

std::optional<NetworkAdapterInfo> NetworkAdapterManager::GetAdapterFromGUID(
    const std::string guid) const {
  const auto found_adapter =
      std::find_if(adapter_addresses_.cbegin(), adapter_addresses_.cend(),
                   [&guid](const NetworkAdapterInfo& adapter) {
                     return guid == adapter.adapter_name;
                   });

  if (found_adapter != adapter_addresses_.cend()) {
    return *found_adapter;
  }

  return std::nullopt;
}

std::optional<NetworkAdapterInfo> NetworkAdapterManager::GetAdapterFromIfIndex(
    const int32_t IfIndex) const {
  const auto found_adapter =
      std::find_if(adapter_addresses_.cbegin(), adapter_addresses_.cend(),
                   [IfIndex](const NetworkAdapterInfo& adapter) {
                     return adapter.if_index == IfIndex;
                   });

  if (found_adapter != adapter_addresses_.cend()) {
    return *found_adapter;
  }

  return std::nullopt;
}

std::string NetworkAdapterManager::GetAdapterFriendlyName(
    const NetworkAdapterInfo& adapter) const {
  return adapter.friendly_name;
}

std::vector<std::string> NetworkAdapterManager::GetAdaptersNames() const {
  std::vector<std::string> adapter_names;
  adapter_names.reserve(adapter_addresses_.size());

  for (const auto& adapter : adapter_addresses_) {
    adapter_names.push_back(GetAdapterFriendlyName(adapter));
  }

  return adapter_names;
}

MacAddress NetworkAdapterManager::GetAdapterMacAddressFromGUID(
    const std::string guid) const {
  const auto adapter = GetAdapterFromGUID(guid);

  if (adapter.has_value() && adapter->mac.has_value()) {
    return adapter->mac.value();
  }

  return GenerateMacAddress();
}

std::optional<NetworkAdapterInfo> NetworkAdapterManager::GetSelectedAdapter()
    const {
  return GetAdapterFromGUID(cvars::network_guid);
}

std::string NetworkAdapterManager::GetSelectedAdapterName() const {
  const auto adapter = GetSelectedAdapter();
  return adapter.has_value() ? GetAdapterFriendlyName(adapter.value()) : "";
}

std::string NetworkAdapterManager::GetSelectedAdapterLocalIPString() const {
  return ip_to_string(local_ip_);
}

bool NetworkAdapterManager::IsInterfaceSelected() const {
  return local_ip_.sin_addr.s_addr != 0;
}

void NetworkAdapterManager::ResetSelectedAdapter() {
  local_ip_ = {};
  is_WAN_routing_ = false;
  OVERRIDE_string(network_guid, "");
}

#if XE_PLATFORM_WIN32

std::vector<NetworkAdapterInfo>
NetworkAdapterManager::DiscoverNetworkAdapters() {
  XELOGI("Discovering network interfaces...");

  std::vector<NetworkAdapterInfo> adapter_addresses;

  ULONG buffer_length = 0;
  const uint32_t flags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST |
                         GAA_FLAG_SKIP_DNS_SERVER;

  if (GetAdaptersAddresses(AF_INET, flags, 0, nullptr, &buffer_length) !=
      ERROR_BUFFER_OVERFLOW) {
    return adapter_addresses;
  }

  adapter_addresses_data_.resize(buffer_length);
  auto* adapters_ptr =
      reinterpret_cast<IP_ADAPTER_ADDRESSES*>(adapter_addresses_data_.data());

  if (GetAdaptersAddresses(AF_INET, flags, 0, adapters_ptr, &buffer_length) !=
      NO_ERROR) {
    return adapter_addresses;
  }

  std::string networks = "Network Interfaces:\n";

  for (IP_ADAPTER_ADDRESSES* adapter_ptr = adapters_ptr; adapter_ptr != nullptr;
       adapter_ptr = adapter_ptr->Next) {
    if (adapter_ptr->OperStatus != IfOperStatusUp) {
      continue;
    }

    if (adapter_ptr->IfType != IF_TYPE_IEEE80211 &&
        adapter_ptr->IfType != IF_TYPE_ETHERNET_CSMACD &&
        adapter_ptr->IfType != IF_TYPE_PROP_VIRTUAL &&
        adapter_ptr->IfType != IF_TYPE_TUNNEL) {
      continue;
    }

    for (PIP_ADAPTER_UNICAST_ADDRESS_LH adapter_address =
             adapter_ptr->FirstUnicastAddress;
         adapter_address != nullptr; adapter_address = adapter_address->Next) {
      auto* addr_ptr =
          reinterpret_cast<sockaddr_in*>(adapter_address->Address.lpSockaddr);
      if (addr_ptr->sin_family != AF_INET) {
        continue;
      }

      NetworkAdapterInfo info;
      info.adapter_name = adapter_ptr->AdapterName;
      info.friendly_name =
          xe::to_utf8(reinterpret_cast<char16_t*>(adapter_ptr->FriendlyName));
      info.if_index = static_cast<int32_t>(adapter_ptr->IfIndex);
      info.if_type = adapter_ptr->IfType;
      info.oper_status = adapter_ptr->OperStatus;
      info.ipv4 = *addr_ptr;

      if (adapter_ptr->PhysicalAddressLength == MacAddress::MacAddressSize) {
        info.mac = MacAddress(adapter_ptr->PhysicalAddress);
      }

      networks += fmt::format("{} {}: {}\n", info.friendly_name,
                              info.adapter_name, ip_to_string(info.ipv4));
      adapter_addresses.push_back(std::move(info));
      break;
    }
  }

  if (adapter_addresses.empty()) {
    XELOGI("No network interfaces detected!");
  } else {
    XELOGI("Found {} network interfaces!", adapter_addresses.size());
    XELOGI("{}", xe::string_util::trim(networks));
  }

  return adapter_addresses;
}

int32_t NetworkAdapterManager::GetBestInterfaceIfIndex() {
  const in_addr destAddr = ip_to_in_addr("8.8.8.8");

  DWORD bestIfIndex = static_cast<DWORD>(-1);
  const DWORD result = GetBestInterface(destAddr.S_un.S_addr, &bestIfIndex);

  if (result != NO_ERROR) {
    XELOGI("Error finding best interface: {}", result);
  }

  return static_cast<int32_t>(bestIfIndex);
}

#else

std::vector<NetworkAdapterInfo>
NetworkAdapterManager::DiscoverNetworkAdapters() {
  XELOGI("Discovering network interfaces...");

  std::vector<NetworkAdapterInfo> adapter_addresses;
  ifaddrs* ifaddr = nullptr;
  if (getifaddrs(&ifaddr) != 0) {
    XELOGI("No network interfaces detected!");
    return adapter_addresses;
  }

  std::string networks = "Network Interfaces:\n";

  for (ifaddrs* ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
    if (!ifa->ifa_addr || ifa->ifa_addr->sa_family != AF_INET) {
      continue;
    }

    if (!(ifa->ifa_flags & IFF_UP) || (ifa->ifa_flags & IFF_LOOPBACK)) {
      continue;
    }

    NetworkAdapterInfo info;
    info.adapter_name = ifa->ifa_name ? ifa->ifa_name : "";
    info.friendly_name = info.adapter_name;
    info.if_index = if_nametoindex(info.adapter_name.c_str());
    info.if_type = ifa->ifa_flags;
    info.oper_status = (ifa->ifa_flags & IFF_UP) ? 1 : 0;
    info.ipv4 = *reinterpret_cast<sockaddr_in*>(ifa->ifa_addr);

    adapter_addresses.push_back(std::move(info));
    networks += fmt::format("{} {}: {}\n", info.friendly_name,
                            info.adapter_name, ip_to_string(info.ipv4));
  }

  freeifaddrs(ifaddr);

  if (adapter_addresses.empty()) {
    XELOGI("No network interfaces detected!");
  } else {
    XELOGI("Found {} network interfaces!", adapter_addresses.size());
    XELOGI("{}", xe::string_util::trim(networks));
  }

  return adapter_addresses;
}

int32_t NetworkAdapterManager::GetBestInterfaceIfIndex() {
  for (const auto& adapter : adapter_addresses_) {
    if (IsInterfaceWANRouting(adapter.ipv4)) {
      return adapter.if_index;
    }
  }

  if (!adapter_addresses_.empty()) {
    return adapter_addresses_.front().if_index;
  }

  return -1;
}

#endif

bool NetworkAdapterManager::UpdateNetworkInterface(
    const NetworkAdapterInfo& adapter) {
  const std::optional<sockaddr_in> adapter_addr =
      GetInterfaceIPFromIfIndex(adapter.if_index);

  if (!adapter_addr.has_value() && adapter.ipv4.sin_addr.s_addr) {
    local_ip_ = adapter.ipv4;
    is_WAN_routing_ = IsInterfaceWANRouting(local_ip_);
    OVERRIDE_string(network_guid, adapter.adapter_name);
    return true;
  }

  if (adapter_addr.has_value()) {
    local_ip_ = adapter_addr.value();
    is_WAN_routing_ = IsInterfaceWANRouting(local_ip_);
    OVERRIDE_string(network_guid, adapter.adapter_name);
    return true;
  }

  return false;
}

bool NetworkAdapterManager::IsInterfaceWANRouting(sockaddr_in interface_addr) {
  SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

  if (sock == INVALID_SOCKET) {
    return false;
  }

  if (bind(sock, reinterpret_cast<sockaddr*>(&interface_addr),
           sizeof(sockaddr)) == SOCKET_ERROR) {
    closesocket(sock);
    return false;
  }

  int timeout = 3000;
  setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO,
             reinterpret_cast<const char*>(&timeout), sizeof(timeout));

  sockaddr_in remoteAddr{};
  remoteAddr.sin_family = AF_INET;
  remoteAddr.sin_port = htons(53);
  remoteAddr.sin_addr = ip_to_in_addr("8.8.8.8");

  const bool success = connect(sock, reinterpret_cast<sockaddr*>(&remoteAddr),
                               sizeof(remoteAddr)) != SOCKET_ERROR;

  closesocket(sock);

  return success;
}

std::optional<sockaddr_in> NetworkAdapterManager::GetInterfaceIPFromIfIndex(
    int32_t IfIndex) const {
  if (IfIndex < 0) {
    return std::nullopt;
  }

  for (const auto& adapter : adapter_addresses_) {
    if (adapter.if_index == IfIndex && adapter.ipv4.sin_addr.s_addr) {
      return adapter.ipv4;
    }
  }

  return std::nullopt;
}

bool NetworkAdapterManager::SelectSavedNetworkAdapter() {
  const std::optional<NetworkAdapterInfo> adapter = GetSelectedAdapter();

  if (adapter.has_value()) {
    return UpdateNetworkInterface(adapter.value());
  }

  XELOGI("Interface GUID: {} not found!", cvars::network_guid);
  return false;
}

void NetworkAdapterManager::AutoSelectNetworkAdapter(
    int32_t best_interface_IfIndex) {
  bool selected = SelectSavedNetworkAdapter();

  if (!selected) {
    const auto adapter = GetAdapterFromIfIndex(best_interface_IfIndex);

    if (adapter.has_value()) {
      selected = UpdateNetworkInterface(adapter.value());
    }
  }

  if (selected) {
    XELOGI(GetSelectedAdapterDesciption());
  } else {
    XELOGI("Unspecified Network Interface!");
  }
}

}  // namespace kernel
}  // namespace xe
