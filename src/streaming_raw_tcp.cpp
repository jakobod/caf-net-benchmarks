#include <chrono>
#include <string>
#include <thread>
#include <unistd.h>

#include "caf/binary_deserializer.hpp"
#include "caf/binary_serializer.hpp"
#include "caf/detail/serialized_size.hpp"
#include "caf/detail/socket_sys_includes.hpp"
#include "caf/error.hpp"
#include "caf/net/socket_guard.hpp"
#include "caf/net/stream_socket.hpp"
#include "caf/net/tcp_stream_socket.hpp"
#include "caf/sec.hpp"
#include "caf/settings.hpp"
#include "caf/span.hpp"
#include "utility.hpp"

using namespace caf;
using namespace caf::net;

using payload = std::vector<byte>;

ptrdiff_t send(stream_socket sock, const_byte_span payload) {
  while (!payload.empty()) {
    auto ret = write(sock, payload);
    if (ret > 0)
      payload = payload.subspan(ret);
    else if (ret == 0)
      return 0;
    else if (!last_socket_error_is_temporary())
      return -1;
  }
  return 1;
}

error receive(stream_socket sock, byte_span buf) {
  auto data = buf.data();
  auto size = buf.size();
  auto received = 0;
  while (received < buf.size()) {
    auto ret = read(sock, make_span(data, size));
    if (ret > 0)
      received += ret;
    else if (ret == 0)
      return sec::socket_disconnected;
    else if (!last_socket_error_is_temporary())
      return sec::runtime_error;
  }
  return none;
}

void send_size_t(stream_socket sock, size_t value) {
  value = htonl(value);
  if (write(sock, make_span(reinterpret_cast<byte*>(&value), sizeof(size_t)))
      <= 0)
    exit("send_size_t failed");
}

size_t read_size_t(stream_socket sock) {
  size_t amount = 0;
  if (read(sock, make_span(reinterpret_cast<byte*>(&amount), sizeof(size_t)))
      <= 0)
    exit("read_size_t failed");
  return ntohl(amount);
}

void run_server(stream_socket sock) {
  const auto amount = read_size_t(sock);
  const auto message_size = read_size_t(sock);
  payload p(message_size);
  auto receive_amount = detail::serialized_size(p);
  byte_buffer recv_buf(receive_amount);
  size_t num_bytes = 0;
  // now we know how many bytes to receive before disconnecting.
  while (num_bytes < amount) {
    if (auto err = receive(sock, recv_buf)) {
      if (err == sec::socket_disconnected)
        break;
      exit("receive failed", err);
    }
    binary_deserializer source{nullptr, recv_buf};
    if (!source.apply_object(p))
      exit("deserializing failed", source.get_error());
    num_bytes += p.size();
    p.clear();
  }
  send(sock, make_span(recv_buf.data(), 1));
}

void run_client(stream_socket sock, size_t amount, size_t message_size) {
  send_size_t(sock, amount);
  send_size_t(sock, message_size);
  payload p(message_size);
  size_t sent = 0;
  byte_buffer send_buf;
  while (sent < amount) {
    binary_serializer sink{nullptr, send_buf};
    if (!sink.apply_object(p))
      exit("serializing failed", sink.get_error());
    auto ret = send(sock, send_buf);
    if (ret <= 0)
      exit("write failed");
    send_buf.clear();
    sent += p.size();
  }
  std::array<byte, 1> dummy;
  ptrdiff_t res = 0;
  do {
    res = read(sock, make_span(dummy.data(), dummy.size()));
  } while (res != 1);
}

int main(int argc, char* argv[]) {
  std::string host = "localhost";
  uint16_t port = 0;
  bool is_client = false;
  bool is_server = false;
  size_t amount = 1024;
  size_t message_size = 1024;

  int opt;
  while ((opt = getopt(argc, argv, "h::p::sca::m::")) != -1) {
    switch (opt) {
      case 'h':
        host = std::string(optarg);
        break;
      case 'p':
        port = atoi(optarg);
        break;
      case 's':
        is_server = true;
        break;
      case 'c':
        is_client = true;
        break;
      case 'a':
        amount = atoi(optarg);
        break;
      case 'm':
        message_size = atoi(optarg);
        break;
      default:
        fprintf(stderr, "Usage: %s [hp] [file...]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
  }

  if (is_server) {
    auto sock = accept();
    if (sock.socket() == invalid_socket)
      exit("accept failed");
    if (auto err = nodelay(sock.socket(), true))
      exit("nodelay failed", err);
    std::cerr << "accepted! Starting benchmark now." << std::endl;
    run_server(sock.socket());
  } else if (is_client) {
    if (port == 0)
      exit("port has to be set explicitly");
    auto sock = connect(host, port);
    if (sock.socket() == invalid_socket)
      exit("connect failed");
    if (auto err = nodelay(sock.socket(), true))
      exit("nodelay failed", err);
    std::cerr << "connected! Starting benchmark now." << std::endl;
    auto start = now();
    run_client(sock.socket(), amount, message_size);
    end(start);
  } else {
    if (auto socks = make_connected_tcp_socket_pair()) {
      if (auto err = nodelay(socks->first, true))
        exit("nodelay failed", err);
      if (auto err = nodelay(socks->second, true))
        exit("nodelay failed", err);
      auto client_guard = make_socket_guard(socks->first);
      auto serv_guard = make_socket_guard(socks->second);
      auto f = [&]() { run_server(serv_guard.socket()); };
      std::thread server_t{f};
      auto start = now();
      run_client(client_guard.socket(), amount, message_size);
      end(start);
      server_t.join();
    } else {
      exit("make_connected_tcp_socket_pair failed", socks.error());
    }
  }
  return 0;
}
