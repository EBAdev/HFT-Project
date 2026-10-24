#pragma once // prevents multiple inclusions
#include "Order.hpp"
#include <cstdint>
#include <list>
#include <map>
#include <memory>
#include <optional>

namespace od {

class OrderBook {
  // stores and organizes resting orders for an instrument.
  // Uses First-In-First-Out (FIFO) principle for each price.

public:
  // Constructor
  explicit OrderBook(std::uint32_t instrument_id);

  // Getters for private values
  std::uint32_t instrument_id() const;

  // add an order to the book, method should check
  // The instrument ID matches the book.
  // The order ID is not already present.
  // The order is in an acceptable state.
  // The order is inserted at the correct price level and in FIFO order.
  bool add_order(std::unique_ptr<Order> order);

  // Cancel an order on the book
  bool cancel_order(std::uint64_t order_id);

  // get best bid/ask price
  std::optional<std::int64_t> best_bid();
  std::optional<std::int64_t> best_ask();

  // get order by id
  Order get_order(std::uint64_t order_id);

private:
  std::uint32_t instrument_id_ ;
  std::unique_ptr<Order> order_index;
  std::map<std::int64_t, std::list<std::unique_ptr<Order>>> bid_side;
  std::map<std::int64_t, std::list<std::unique_ptr<Order>>> ask_side;
};

} // namespace od
