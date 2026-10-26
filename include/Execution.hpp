#pragma once
#include <cstdint>

namespace od {

struct Execution {
  std::uint64_t id;            // Uniquely identifies this trade
  std::uint64_t buy_order_id;  // Identifies the buyer's order
  std::uint64_t sell_order_id; // Identifies the seller's order
  std::uint32_t instrument_id; // Identifies the traded instrument
  std::int64_t
      execution_price; // The price at which the trade occurred (in ticks)
  std::uint32_t execution_quantity; // The number of units traded
  std::uint64_t sequence_number;    // Orders executions chronologically
};

} // namespace od
