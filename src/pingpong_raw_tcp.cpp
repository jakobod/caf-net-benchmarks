#include <chrono>
#include <thread>

#include "caf/error.hpp"
#include "caf/net/socket_guard.hpp"
#include "caf/net/stream_socket.hpp"
#include "caf/sec.hpp"
#include "caf/settings.hpp"
#include "caf/span.hpp"
#include "utility.hpp"

using namespace caf;
using namespace caf::net;

using timestamp_type = long long;

static constexpr size_t timestamp_size = sizeof(timestamp_type);
static constexpr size_t payload_size = timestamp_size;

timestamp_type get_timestamp() {
  using namespace std::chrono;
  return duration_cast<microseconds>(system_clock::now().time_since_epoch())
    .count();
}

void serialize(byte_buffer& buf, timestamp_type t1, timestamp_type t2 = 0) {
  memcpy(buf.data(), &t1, sizeof(timestamp_type));
  memcpy(buf.data() + sizeof(timestamp_type), &t2, sizeof(timestamp_type));
}

void deserialize(const byte_buffer& buf, timestamp_type& t1,
                 timestamp_type& t2) {
  if (buf.size() < 2 * timestamp_size)
    exit("buffer does not contain enough bytes!");
  memcpy(&t1, buf.data(), sizeof(timestamp_type));
  memcpy(&t2, buf.data() + sizeof(timestamp_type), sizeof(timestamp_type));
}

error send(stream_socket sock, const byte_buffer& buf, size_t amount) {
  auto written = 0;
  do {
    auto ret = write(sock, make_span(buf.data(), payload_size));
    if (auto num_bytes = get_if<size_t>(&ret))
      written += *num_bytes;
    else
      return get<sec>(ret);
  } while (written < amount);
  return none;
}

error receive(stream_socket sock, byte_buffer& buf, size_t amount) {
  size_t received = 0;
  do {
    auto data = buf.data() + received;
    auto missing = amount - received;
    auto ret = read(sock, make_span(data, missing));
    if (auto num_bytes = get_if<size_t>(&ret))
      received += *num_bytes;
    else
      return get<sec>(ret);
  } while (received < amount);
  return none;
}

void echo_server(stream_socket sock) {
  byte_buffer buf(payload_size);
  while (true) {
    if (auto err = receive(sock, buf, payload_size)) {
      if (static_cast<sec>(err.code()) == sec::socket_disconnected)
        break;
      else
        exit(err);
    }
    // Append new timestamp to received data.
    auto ts = get_timestamp();
    memcpy(buf.data(), &ts, timestamp_size);
    if (auto err = send(sock, buf, payload_size * 2)) {
      if (static_cast<sec>(err.code()) == sec::socket_disconnected)
        break;
      else
        exit(err);
    }
  }
  std::cerr << "connection closed by remote node" << std::endl;
}

int main() {
  size_t max = 10'000;
  std::vector<timestamp_type> t1;
  std::vector<timestamp_type> t2;
  std::vector<timestamp_type> t3;
  t1.reserve(max);
  t2.reserve(max);
  t3.reserve(max);
  byte_buffer buf(2 * payload_size);
  size_t received = 0;
  auto sockets = *make_connected_tcp_socket_pair();
  // libcaf_io and libcaf_net both set nodelay options.
  if (auto err = nodelay(sockets.first, true))
    exit(err);
  if (auto err = nodelay(sockets.second, true))
    exit(err);
  auto client_guard = make_socket_guard(sockets.first);
  auto serv_guard = make_socket_guard(sockets.second);
  auto f = [=]() { echo_server(sockets.second); };
  std::thread serv_thread{f};
  while (true) {
    // Serialize timestamp and send it to the remote node
    auto ts = get_timestamp();
    serialize(buf, ts);
    if (auto err = send(client_guard.socket(), buf, payload_size))
      exit(err);
    t1.emplace_back(ts);
    // Receive the remote timestamp and save the result.
    if (auto err = receive(client_guard.socket(), buf, 2 * payload_size))
      exit(err);
    // Save timestamps.
    t3.emplace_back(get_timestamp());
    timestamp_type ts1;
    timestamp_type ts2;
    deserialize(buf, ts1, ts2);
    t2.emplace_back(ts1);
    if (++received >= max)
      break;
  }
  shutdown(client_guard.release());
  serv_thread.join();
  std::cerr << "received " << std::to_string(max) << " number of pings"
            << std::endl;
  std::cout << "what, ";
  for (size_t i = 0; i < t3.size(); ++i)
    std::cout << "value" << std::to_string(i) << ", ";
  std::cout << std::endl;
  std::cout << "request, ";
  for (size_t i = 0; i < t3.size(); ++i)
    std::cout << std::to_string(t2.at(i) - t1.at(i)) << ", ";
  std::cout << std::endl;
  std::cout << "response, ";
  for (size_t i = 0; i < t3.size(); ++i)
    std::cout << std::to_string(t3.at(i) - t2.at(i)) << ", ";
  std::cout << std::endl;
  return 0;
}
