#pragma once

#include <cstddef>

namespace basebox {

static const int packet_data_len = 1528;

struct packet {
  char data[packet_data_len]; ///< total allocated buffer
  std::size_t len;            ///< actual lenght written into data
};
} // namespace basebox
