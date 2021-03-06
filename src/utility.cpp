#include "utility.hpp"

#include <chrono>
#include <cstdlib>
#include <string>
#include <utility>

#include "caf/error.hpp"
#include "caf/expected.hpp"
#include "caf/ip_endpoint.hpp"
#include "caf/net/ip.hpp"
#include "caf/net/socket_guard.hpp"
#include "caf/net/stream_socket.hpp"
#include "caf/net/tcp_accept_socket.hpp"
#include "caf/net/tcp_stream_socket.hpp"
#include "caf/uri.hpp"

bench_mode convert(const std::string& str) {
  if (str == "netBench")
    return bench_mode::net;
  else if (str == "ioBench")
    return bench_mode::io;
  else if (str == "localBench")
    return bench_mode::local;
  else
    return bench_mode::invalid;
}

caf::expected<std::pair<caf::net::stream_socket, caf::net::stream_socket>>
make_connected_tcp_socket_pair() {
  using namespace std;
  using namespace caf;
  using namespace caf::net;
  tcp_accept_socket acceptor;
  uri::authority_type auth;
  auth.port = 0;
  auth.host = "0.0.0.0"s;
  if (auto res = net::make_tcp_accept_socket(auth, false))
    acceptor = *res;
  else
    return res.error();
  auto guard = make_socket_guard(acceptor);
  uri::authority_type dst;
  dst.host = "localhost"s;
  if (auto port = local_port(socket_cast<network_socket>(guard.socket())))
    dst.port = *port;
  else
    return port.error();
  tcp_stream_socket sock1;
  if (auto res = make_connected_tcp_stream_socket(dst))
    sock1 = *res;
  else
    return res.error();
  if (auto res = accept(guard.socket()))
    return make_pair(sock1, *res);
  else
    return res.error();
}

caf::net::socket_guard<caf::net::tcp_stream_socket> accept() {
  using namespace caf::net;
  caf::uri::authority_type auth;
  auto acceptor = make_socket_guard(*make_tcp_accept_socket(auth));
  std::cout << "*** Acceptor spawned on port " << local_port(acceptor.socket())
            << std::endl;
  if (auto res = accept(acceptor.socket()))
    return make_socket_guard(*res);
  return make_socket_guard(tcp_stream_socket(invalid_socket_id));
}

caf::net::socket_guard<caf::net::tcp_stream_socket> connect(std::string host,
                                                            uint16_t port) {
  using namespace caf::net;
  if (host == "" || port == 0)
    exit("host AND port have to be specified!");
  for (const auto& addr : ip::resolve(host)) {
    caf::ip_endpoint ep{addr, port};
    if (auto ret = make_connected_tcp_stream_socket(ep))
      return make_socket_guard(*ret);
  }
  return make_socket_guard(tcp_stream_socket(invalid_socket_id));
}

void exit(const std::string& msg, const caf::error& err) {
  std::cerr << "ERROR: ";
  if (msg != "")
    std::cerr << msg;
  if (err)
    std::cerr << ": " << to_string(err) << std::endl;
  std::abort();
}

void exit(const caf::error& err) {
  exit(caf::to_string(err));
}
