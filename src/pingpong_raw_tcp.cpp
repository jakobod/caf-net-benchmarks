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

error send(stream_socket sock, const_byte_span payload) {
  while (!payload.empty()) {
    auto ret = write(sock, payload);
    if (ret > 0)
      payload = payload.subspan(ret);
    else if (ret == 0)
      return sec::socket_disconnected;
    else if (!last_socket_error_is_temporary())
      return sec::runtime_error;
  }
  return none;
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
  const auto message_size = read_size_t(sock);
  byte_buffer p(message_size);
  auto receive_amount = detail::serialized_size(p);
  byte_buffer send_buf;
  byte_buffer recv_buf;
  while (true) {
    recv_buf.resize(receive_amount);
    if (auto err = receive(sock, recv_buf)) {
      if (err == sec::socket_disconnected)
        break;
      exit("receive failed", err);
    }
    binary_deserializer source{nullptr, recv_buf};
    byte_buffer buf;
    if (!source.apply_object(buf))
      exit("deserializing failed", source.get_error());
    // serialize data before sending
    binary_serializer sink{nullptr, send_buf};
    if (!sink.apply_object(p))
      exit("serializing failed", sink.get_error());
    if (auto err = send(sock, send_buf))
      exit("send failed", err);
    send_buf.clear();
  }
}

void run_client(stream_socket sock, size_t amount, size_t message_size) {
  send_size_t(sock, message_size);
  payload p(message_size);
  auto receive_amount = detail::serialized_size(p);
  size_t rounds = 0;

  byte_buffer send_buf;
  byte_buffer recv_buf;
  do {
    binary_serializer sink{nullptr, send_buf};
    if (!sink.apply_object(p))
      exit("serializing failed", sink.get_error());
    if (auto err = send(sock, send_buf))
      exit("send failed", err);
    send_buf.clear();
    // receive message
    recv_buf.resize(receive_amount);
    if (auto err = receive(sock, recv_buf))
      exit("send failed", err);
    binary_deserializer source{nullptr, recv_buf};
    if (!source.apply_object(p))
      exit("deserializing failed", sink.get_error());

  } while (++rounds < amount);
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
        exit(EXIT_FAILURE);
    }
  }

  if (is_server) {
    auto sock = accept();
    if (sock.socket() == invalid_socket)
      exit("accept failed");
    if (auto err = nodelay(sock.socket(), true))
      exit("nodelay failed", err);
    run_server(sock.socket());
  } else if (is_client) {
    if (port == 0)
      exit("port has to be set explicitly");
    auto sock = connect(host, port);
    if (sock.socket() == invalid_socket)
      exit("connect failed");
    if (auto err = nodelay(sock.socket(), true))
      exit("nodelay failed", err);
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
      shutdown(client_guard.release());
      server_t.join();
    } else {
      exit("make_connected_tcp_socket_pair failed", socks.error());
    }
  }
  return 0;
}
