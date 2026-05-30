/**
 ******************************************************************************
 * JSON builders for Phoenix Probe /netplay/* endpoints.
 ******************************************************************************
 */

#include "xenia/kernel/netplay/netplay_probe_util.h"

#include <sstream>

#include "xenia/base/cvar.h"
#include "xenia/emulator.h"
#include "xenia/kernel/netplay/xlive_api.h"
#include "xenia/kernel/netplay/xnet_types.h"

DECLARE_int32(network_mode);
DECLARE_bool(upnp);

namespace xe {
namespace kernel {
namespace {

const char* NetworkModeName(uint32_t mode) {
  switch (mode) {
    case NETWORK_MODE::OFFLINE:
      return "offline";
    case NETWORK_MODE::LAN:
      return "lan";
    case NETWORK_MODE::XBOXLIVE:
      return "xboxlive";
    default:
      return "unknown";
  }
}

}  // namespace

std::string BuildNetplayStatusJson(Emulator* emulator) {
  if (!emulator) {
    return "{\"error\":\"no_emulator\"}";
  }

  auto* xlive = emulator->GetXboxLiveAPI();
  const auto* adapter_manager = emulator->GetNetworkAdapterManager();
  std::ostringstream os;
  os << '{';
  os << "\"network_mode\":\"" << NetworkModeName(cvars::network_mode) << '"';
  os << ",\"title_id\":" << emulator->title_id();
  os << ",\"connected\":"
     << (xlive && xlive->IsConnectedToServer() ? "true" : "false");
  os << ",\"nat_type\":" << (xlive ? xlive->GetNatType() : 0);
  os << ",\"player_port\":" << (xlive ? xlive->GetPlayerPort() : 0);
  os << ",\"api_address\":\"" << XLiveAPI::GetApiAddress() << '"';
  os << ",\"online_ip\":\"" << (xlive ? xlive->OnlineIP_str() : "") << '"';
  os << ",\"adapter\":\""
     << (adapter_manager ? adapter_manager->GetSelectedAdapterName() : "")
     << '"';
  os << ",\"adapter_ip\":\""
     << (adapter_manager ? adapter_manager->GetSelectedAdapterLocalIPString()
                         : "")
     << '"';
  os << ",\"wan_routing\":"
     << (adapter_manager && adapter_manager->IsSelectedAdapterWANRouting()
             ? "true"
             : "false");
  os << ",\"upnp_enabled\":" << (cvars::upnp ? "true" : "false");
  os << '}';
  return os.str();
}

std::string BuildNetplaySessionsJson(Emulator* emulator) {
  if (!emulator) {
    return "{\"error\":\"no_emulator\"}";
  }

  auto* xlive = emulator->GetXboxLiveAPI();
  if (!xlive) {
    return "{\"sessions\":[]}";
  }

  const uint32_t title_id = emulator->title_id();
  auto sessions = xlive->GetTitleSessions(title_id);

  std::ostringstream os;
  os << "{\"title_id\":" << title_id << ",\"sessions\":[";
  bool first = true;
  for (const auto& session : sessions) {
    if (!session) {
      continue;
    }
    if (!first) {
      os << ',';
    }
    first = false;
    os << '{';
    os << "\"session_id\":" << session->SessionID_UInt();
    os << ",\"host_address\":\"" << session->HostAddress() << '"';
    os << ",\"port\":" << static_cast<uint32_t>(session->Port());
    os << ",\"open_public_slots\":"
       << static_cast<uint32_t>(session->OpenPublicSlotsCount());
    os << ",\"open_private_slots\":"
       << static_cast<uint32_t>(session->OpenPrivateSlotsCount());
    os << ",\"filled_public_slots\":"
       << static_cast<uint32_t>(session->FilledPublicSlotsCount());
    os << ",\"filled_private_slots\":"
       << static_cast<uint32_t>(session->FilledPrivateSlotsCount());
    os << '}';
  }
  os << "]}";
  return os.str();
}

}  // namespace kernel
}  // namespace xe
