/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2013 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "src/xenia/kernel/xsocket.h"

#include <cstring>
#include <thread>

#include "xenia/base/logging.h"
#include "xenia/base/platform.h"
#include "xenia/base/threading.h"
#include "xenia/kernel/kernel_state.h"
#include "xenia/kernel/xam/xam_module.h"
#include "xenia/kernel/xevent.h"

#ifdef XE_PLATFORM_WIN32
// clang-format off
#include "xenia/base/platform_win.h"
#include <WS2tcpip.h>
#include <WinSock2.h>
// clang-format on
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace xe {
namespace kernel {

namespace {

// Drives async_wait completions on a worker thread for the process lifetime.
class IoContextRunner {
 public:
  IoContextRunner() : work_(asio::make_work_guard(io_context_)) {
    thread_ = std::thread([this]() {
      xe::threading::set_name("Xenia Socket I/O");
      io_context_.run();
    });
  }
  ~IoContextRunner() {
    work_.reset();
    io_context_.stop();
    if (thread_.joinable()) {
      thread_.join();
    }
  }
  asio::io_context& get() { return io_context_; }

 private:
  asio::io_context io_context_;
  asio::executor_work_guard<asio::io_context::executor_type> work_;
  std::thread thread_;
};

}  // namespace

// Shared io_context for all sockets
static asio::io_context& GetIoContext() {
  static IoContextRunner runner;
  return runner.get();
}

// Translate socket options to native
// Note:
// SO_DONTLINGER = ~SO_LINGER
// SO_EXCLUSIVEADDRUSE = ~SO_REUSEADDR
// TODO: Check SO_DONTLINGER and SO_EXCLUSIVEADDRUSE usage on linux
const std::map<uint32_t, uint32_t> supported_socket_options = {
    {0x0004, SO_REUSEADDR}, {0x0020, SO_BROADCAST}, {0x0080, SO_LINGER},
    {0x1001, SO_SNDBUF},    {0x1002, SO_RCVBUF},    {0x1005, SO_SNDTIMEO},
    {0x1006, SO_RCVTIMEO},  {~0x0080, ~SO_LINGER},  {~0x0004, ~SO_REUSEADDR}};

// Translate socket TCP options to native
const std::map<uint32_t, uint32_t> supported_tcp_options = {
    {0x0001, TCP_NODELAY}};

// Translate socket levels to native
const std::map<uint32_t, uint32_t> supported_levels = {{0xFFFF, SOL_SOCKET},
                                                       {0x6, IPPROTO_TCP}};

// Translate ioctl commands to native
const std::map<uint32_t, uint32_t> supported_controls = {
    {0x8004667E, FIONBIO}, {0x4004667F, FIONREAD}};

// asio error_code -> Winsock WSAE* code. Guests look up by Winsock value;
// returning raw POSIX errno makes recoverable errors look fatal (e.g. COD4
// MP treats unrecognized recvfrom error as a hard init failure).
uint32_t AsioErrorToWSAError(const asio::error_code& ec) {
  if (!ec) {
    return 0;
  }
  if (ec == asio::error::would_block || ec == asio::error::try_again) {
    return 10035;  // WSAEWOULDBLOCK
  }
  if (ec == asio::error::in_progress) {
    return 10036;  // WSAEINPROGRESS
  }
  if (ec == asio::error::already_started) {
    return 10037;  // WSAEALREADY
  }
  if (ec == asio::error::not_socket) {
    return 10038;  // WSAENOTSOCK
  }
  if (ec == asio::error::message_size) {
    return 10040;  // WSAEMSGSIZE
  }
  if (ec == asio::error::no_protocol_option) {
    return 10042;  // WSAENOPROTOOPT
  }
  if (ec == asio::error::address_family_not_supported) {
    return 10047;  // WSAEAFNOSUPPORT
  }
  if (ec == asio::error::address_in_use) {
    return 10048;  // WSAEADDRINUSE
  }
  if (ec == asio::error::network_down) {
    return 10050;  // WSAENETDOWN
  }
  if (ec == asio::error::network_unreachable) {
    return 10051;  // WSAENETUNREACH
  }
  if (ec == asio::error::network_reset) {
    return 10052;  // WSAENETRESET
  }
  if (ec == asio::error::connection_aborted) {
    return 10053;  // WSAECONNABORTED
  }
  if (ec == asio::error::connection_reset) {
    return 10054;  // WSAECONNRESET
  }
  if (ec == asio::error::no_buffer_space) {
    return 10055;  // WSAENOBUFS
  }
  if (ec == asio::error::already_connected) {
    return 10056;  // WSAEISCONN
  }
  if (ec == asio::error::not_connected) {
    return 10057;  // WSAENOTCONN
  }
  if (ec == asio::error::shut_down) {
    return 10058;  // WSAESHUTDOWN
  }
  if (ec == asio::error::timed_out) {
    return 10060;  // WSAETIMEDOUT
  }
  if (ec == asio::error::connection_refused) {
    return 10061;  // WSAECONNREFUSED
  }
  if (ec == asio::error::host_unreachable) {
    return 10065;  // WSAEHOSTUNREACH
  }
  if (ec == asio::error::access_denied) {
    return 10013;  // WSAEACCES
  }
  if (ec == asio::error::fault) {
    return 10014;  // WSAEFAULT
  }
  if (ec == asio::error::invalid_argument) {
    return 10022;  // WSAEINVAL
  }
  if (ec == asio::error::operation_aborted) {
    return 995;  // WSA_OPERATION_ABORTED
  }
  if (ec == asio::error::interrupted) {
    return 10004;  // WSAEINTR
  }
  return static_cast<uint32_t>(ec.value());
}

XSocket::XSocket(KernelState* kernel_state)
    : XObject(kernel_state, kObjectType) {}

XSocket::XSocket(KernelState* kernel_state, uint64_t native_handle)
    : XObject(kernel_state, kObjectType), native_handle_(native_handle) {}

XSocket::~XSocket() { Close(); }

X_STATUS XSocket::Initialize(AddressFamily af, Type type, Protocol proto) {
  af_ = af;
  type_ = type;
  proto_ = proto;

  if (proto == Protocol::XE_IPPROTO_VDP) {
    // VDP is a layer on top of UDP.
    proto = Protocol::XE_IPPROTO_UDP;
  }

  native_handle_ = socket(af, type, proto);
  if (native_handle_ == -1) {
    return X_STATUS_UNSUCCESSFUL;
  }

  return X_STATUS_SUCCESS;
}

X_STATUS XSocket::Close() {
#if XE_PLATFORM_WIN32
  int ret = closesocket(native_handle_);
#else
  int ret = close(native_handle_);
#endif

  if (ret != 0) {
    return X_STATUS_UNSUCCESSFUL;
  }

  return X_STATUS_SUCCESS;
}

X_STATUS XSocket::GetOption(uint32_t level, uint32_t optname, void* optval_ptr,
                            uint32_t* optlen) {
  int ret =
      getsockopt(native_handle_, level, optname, static_cast<char*>(optval_ptr),
                 reinterpret_cast<socklen_t*>(optlen));
  if (ret < 0) {
    // TODO: WSAGetLastError()
    return X_STATUS_UNSUCCESSFUL;
  }
  return X_STATUS_SUCCESS;
}
X_STATUS XSocket::SetOption(uint32_t level, uint32_t optname, void* optval_ptr,
                            uint32_t optlen) {
  if (level == 0xFFFF && (optname == 0x5801 || optname == 0x5802)) {
    // Disable socket encryption
    secure_ = false;
    return X_STATUS_SUCCESS;
  }

  int native_level = level;

  assert_false(!supported_levels.contains(level));

  if (supported_levels.contains(level)) {
    native_level = supported_levels.at(level);
  }

  int native_optname = optname;

  if (level == 0xFFFF) {
    assert_false(!supported_socket_options.contains(optname));

    if (supported_socket_options.contains(optname)) {
      native_optname = supported_socket_options.at(optname);
    }
  }

  if (level == IPPROTO_TCP) {
    assert_false(!supported_tcp_options.contains(optname));

    if (supported_tcp_options.contains(optname)) {
      native_optname = supported_tcp_options.at(optname);
    }
  }

  int ret = setsockopt(native_handle_, native_level, native_optname,
                       static_cast<char*>(optval_ptr), optlen);
  if (ret < 0) {
    // TODO: WSAGetLastError()
    XELOGE("XSocket::SetOption: failed with error {:08X}", GetLastWSAError());
    return X_STATUS_UNSUCCESSFUL;
  }

  // SO_BROADCAST
  if (level == 0xFFFF && optname == 0x0020) {
    broadcast_socket_ = true;
  }

  return X_STATUS_SUCCESS;
}

X_STATUS XSocket::IOControl(uint32_t cmd, uint8_t* arg_ptr) {
#ifdef XE_PLATFORM_WIN32
  int ret = ioctlsocket(native_handle_, cmd, (u_long*)arg_ptr);
  if (ret < 0) {
    // TODO: Get last error
    return X_STATUS_UNSUCCESSFUL;
  }
  return X_STATUS_SUCCESS;
#else
  int native_cmd = cmd;

  assert_false(!supported_controls.contains(cmd));

  if (supported_controls.contains(cmd)) {
    native_cmd = supported_controls.at(cmd);
  }

  int ret = ioctl(native_handle_, native_cmd, arg_ptr);

  if (ret < 0) {
    return X_STATUS_UNSUCCESSFUL;
  }

  return X_STATUS_SUCCESS;
#endif
}

X_STATUS XSocket::Connect(N_XSOCKADDR* name, int name_len) {
  int ret = connect(native_handle_, (sockaddr*)name, name_len);
  if (ret < 0) {
    return X_STATUS_UNSUCCESSFUL;
  }

  return X_STATUS_SUCCESS;
}

X_STATUS XSocket::Bind(N_XSOCKADDR_IN* name, int name_len) {
  // On Linux and Windows (when running under Wine), ports < 1024 require root
  // privileges. Remap to port + 10000 to avoid privilege issues.
  // Note: sin_port is xe::be<uint16_t> which automatically handles endianness,
  // so we use it directly without ntohs/htons.
  const uint16_t original_port = uint16_t(name->sin_port);
  if (original_port < 1024) {
    uint16_t new_port = original_port + 10000;
    name->sin_port = new_port;
    XELOGW("XSocket::Bind: port {} requires privileges, remapping to port {}",
           original_port, new_port);
  }

  int ret = bind(native_handle_, (sockaddr*)name, name_len);

  if (ret < 0) {
    return X_STATUS_UNSUCCESSFUL;
  }

  bound_ = true;
  bound_port_ = name->sin_port;

  return X_STATUS_SUCCESS;
}

X_STATUS XSocket::Listen(int backlog) {
  int ret = listen(native_handle_, backlog);
  if (ret < 0) {
    return X_STATUS_UNSUCCESSFUL;
  }

  return X_STATUS_SUCCESS;
}

object_ref<XSocket> XSocket::Accept(N_XSOCKADDR* name, int* name_len) {
  sockaddr n_sockaddr;
  socklen_t n_name_len = sizeof(sockaddr);
  uintptr_t ret = accept(native_handle_, &n_sockaddr, &n_name_len);
  if (ret == -1) {
    std::memset(name, 0, *name_len);
    *name_len = 0;
    return nullptr;
  }

  std::memcpy(name, &n_sockaddr, n_name_len);
  *name_len = n_name_len;

  // Create a kernel object to represent the new socket, and copy parameters
  // over.
  auto socket = object_ref<XSocket>(new XSocket(kernel_state_, ret));
  socket->af_ = af_;
  socket->type_ = type_;
  socket->proto_ = proto_;

  return socket;
}

int XSocket::Shutdown(int how) { return shutdown(native_handle_, how); }

int XSocket::Recv(uint8_t* buf, uint32_t buf_len, uint32_t flags) {
  return recv(native_handle_, reinterpret_cast<char*>(buf), buf_len, flags);
}

int XSocket::RecvFrom(uint8_t* buf, uint32_t buf_len, uint32_t flags,
                      N_XSOCKADDR_IN* from, uint32_t* from_len) {
  // Pop from secure packets first
  // TODO(DrChat): Enable when I commit XNet
  /*
  {
    std::lock_guard<std::mutex> lock(incoming_packet_mutex_);
    if (incoming_packets_.size()) {
      packet* pkt = (packet*)incoming_packets_.front();
      int data_len = pkt->data_len;
      std::memcpy(buf, pkt->data, std::min((uint32_t)pkt->data_len, buf_len));

      from->sin_family = 2;
      from->sin_addr = pkt->src_ip;
      from->sin_port = pkt->src_port;

      incoming_packets_.pop();
      uint8_t* pkt_ui8 = (uint8_t*)pkt;
      delete[] pkt_ui8;

      return data_len;
    }
  }
  */

  sockaddr_in nfrom;
  socklen_t nfromlen = sizeof(sockaddr_in);
  int ret = recvfrom(native_handle_, reinterpret_cast<char*>(buf), buf_len,
                     flags, (sockaddr*)&nfrom, &nfromlen);
  if (from) {
    from->sin_family = nfrom.sin_family;
    from->sin_addr = ntohl(nfrom.sin_addr.s_addr);  // BE <- BE
    from->sin_port = nfrom.sin_port;
    std::memset(from->x_sin_zero, 0, sizeof(from->x_sin_zero));
  }

  if (from_len) {
    *from_len = nfromlen;
  }

  return ret;
}

int XSocket::Send(const uint8_t* buf, uint32_t buf_len, uint32_t flags) {
  return send(native_handle_, reinterpret_cast<const char*>(buf), buf_len,
              flags);
}

int XSocket::SendTo(uint8_t* buf, uint32_t buf_len, uint32_t flags,
                    N_XSOCKADDR_IN* to, uint32_t to_len) {
  // Send 2 copies of the packet: One to XNet (for network security) and an
  // unencrypted copy for other Xenia hosts.
  // TODO(DrChat): Enable when I commit XNet.
  /*
  auto xam = kernel_state()->GetKernelModule<xam::XamModule>("xam.xex");
  auto xnet = xam->xnet();
  if (xnet) {
    xnet->SendPacket(this, to, buf, buf_len);
  }
  */

  sockaddr_in nto;
  if (to) {
    nto.sin_addr.s_addr = to->sin_addr;
    nto.sin_family = to->sin_family;
    nto.sin_port = to->sin_port;
  }

  return sendto(native_handle_, reinterpret_cast<char*>(buf), buf_len, flags,
                to ? (sockaddr*)&nto : nullptr, to_len);
}

// Winsock FD_* flags split into asio wait_read / wait_write groups.
namespace {
constexpr uint32_t kFD_READ = 1;
constexpr uint32_t kFD_WRITE = 2;
constexpr uint32_t kFD_OOB = 4;
constexpr uint32_t kFD_ACCEPT = 8;
constexpr uint32_t kFD_CONNECT = 16;
constexpr uint32_t kFD_CLOSE = 32;
constexpr uint32_t kReadEvents = kFD_READ | kFD_ACCEPT | kFD_CLOSE | kFD_OOB;
constexpr uint32_t kWriteEvents = kFD_WRITE | kFD_CONNECT;
}  // namespace

int XSocket::WSAEventSelect(object_ref<XEvent> event, uint32_t flags) {
  if (!tcp_socket_ && !udp_socket_ && !acceptor_) {
    last_error_ = uint32_t(X_WSAError::X_WSAENOTSOCK);
    return -1;
  }

  asio::error_code ec;

  // WSAEventSelect implicitly sets the socket to non-blocking.
  if (acceptor_) {
    acceptor_->non_blocking(true, ec);
  } else if (tcp_socket_) {
    tcp_socket_->non_blocking(true, ec);
  } else if (udp_socket_) {
    udp_socket_->non_blocking(true, ec);
  }

  std::lock_guard<std::mutex> lock(select_mutex_);

  // Cancel pending waits from any prior selection.
  if (selected_event_) {
    asio::error_code cancel_ec;
    if (acceptor_) {
      acceptor_->cancel(cancel_ec);
    } else if (tcp_socket_) {
      tcp_socket_->cancel(cancel_ec);
    } else if (udp_socket_) {
      udp_socket_->cancel(cancel_ec);
    }
  }

  selected_event_ = std::move(event);
  selected_event_flags_ = flags;

  if (flags == 0 || !selected_event_) {
    return 0;
  }

  const bool want_read = (flags & kReadEvents) != 0;
  const bool want_write = (flags & kWriteEvents) != 0;

  // Capture strong refs so the socket and event outlive any pending wait.
  auto handler = [self = retain_object(this),
                  ev = selected_event_](const asio::error_code& wait_ec) {
    if (wait_ec) {
      return;  // cancelled or socket closed
    }
    ev->Set(0, false);
  };

  if (want_read) {
    if (acceptor_) {
      acceptor_->async_wait(asio::socket_base::wait_read, handler);
    } else if (tcp_socket_) {
      tcp_socket_->async_wait(asio::socket_base::wait_read, handler);
    } else if (udp_socket_) {
      udp_socket_->async_wait(asio::socket_base::wait_read, handler);
    }
  }
  if (want_write) {
    if (acceptor_) {
      acceptor_->async_wait(asio::socket_base::wait_write, handler);
    } else if (tcp_socket_) {
      tcp_socket_->async_wait(asio::socket_base::wait_write, handler);
    } else if (udp_socket_) {
      udp_socket_->async_wait(asio::socket_base::wait_write, handler);
    }
  }

  return 0;
}

bool XSocket::QueuePacket(uint32_t src_ip, uint16_t src_port,
                          const uint8_t* buf, size_t len) {
  packet* pkt = reinterpret_cast<packet*>(new uint8_t[sizeof(packet) + len]);
  pkt->src_ip = src_ip;
  pkt->src_port = src_port;

  pkt->data_len = (uint16_t)len;
  std::memcpy(pkt->data, buf, len);

  std::lock_guard<std::mutex> lock(incoming_packet_mutex_);
  incoming_packets_.push((uint8_t*)pkt);

  // TODO: Limit on number of incoming packets?
  return true;
}

X_STATUS XSocket::GetSockName(uint8_t* buf, int* buf_len) {
  struct sockaddr sa = {};

  int ret = getsockname(native_handle_, &sa, (socklen_t*)buf_len);
  if (ret < 0) {
    return X_STATUS_UNSUCCESSFUL;
  }

  std::memcpy(buf, &sa, *buf_len);
  return X_STATUS_SUCCESS;
}

uint32_t XSocket::GetLastWSAError() const {
  // Todo(Gliniak): Provide error mapping table
  // Xbox error codes might not match with what we receive from OS
#ifdef XE_PLATFORM_WIN32
  return WSAGetLastError();
#endif
  return errno;
}

}  // namespace kernel
}  // namespace xe
