/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Project Phoenix contributors. All rights reserved.          *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 *
 * Minimal POSIX implementation of host Socket helpers. Guest networking uses
 * BSD sockets in kernel/xsocket.cc. Nothing in the main emulator currently
 * requires host-side xe::Socket on Linux; these stubs satisfy linkage if used.
 */

#include "xenia/base/socket.h"

#include "xenia/base/logging.h"

namespace xe {

std::unique_ptr<Socket> Socket::Connect(std::string hostname, uint16_t port) {
  XELOGW("xe::Socket::Connect is not implemented on this platform ({}, {})",
         hostname, port);
  return nullptr;
}

std::unique_ptr<SocketServer> SocketServer::Create(
    uint16_t port,
    std::function<void(std::unique_ptr<Socket> client)> accept_callback) {
  XELOGW("xe::SocketServer::Create is not implemented on this platform (port {})",
         port);
  (void)accept_callback;
  return nullptr;
}

}  // namespace xe
