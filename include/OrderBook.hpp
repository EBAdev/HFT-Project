#pragma once
#include "Order.hpp"

#include <cstdint>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <unordered_map>
#include <utility>

namespace od {

class OrderBook {
  // Stores and organizes resting orders for a single instrument.
  // Orders at each price level follow FIFO priority.

public:
  // Constructor
  explicit OrderBook(std::uint32_t instrument_id);

  // Getters
  std::uint32_t instrument_id() const;

  // Add an order to the book.
  // Ownership transfers to the OrderBook on successful insertion.
  // If insertion fails, the caller retains ownership of the order.
  bool add_order(std::unique_ptr<Order> &order);

  // Cancel an order by its ID.
  bool cancel_order(std::uint64_t order_id);

  // A resting order may be fully filled by incoming orders
  // this method cleans the order_index if this happens.
  bool remove_filled_order(std::uint64_t order_id);

  // Get best bid/ask prices.
  std::optional<std::int64_t> best_bid() const;
  std::optional<std::int64_t> best_ask() const;

  // Find best order for a given side
  // Returns nullptr if the order is not found.
  // The returned pointer does not transfer ownership.
  const Order *best_order(OrderSide side) const;
  Order *best_order(OrderSide side);

  // Find an order by ID.
  // Returns nullptr if the order is not found.
  // The returned pointer does not transfer ownership.
  Order *get_order(std::uint64_t order_id);
  const Order *get_order(std::uint64_t order_id) const;

private:
  // A list of orders
  using OrderQueue = std::list<Order *>;
  // for each price is an OrderQueue
  using BidSide =
      std::map<std::int64_t, OrderQueue, std::greater<std::int64_t>>;
  using AskSide = std::map<std::int64_t, OrderQueue>;

  std::uint32_t instrument_id_;

  // Sole owners of Order objects, indexed by order ID.
  std::unordered_map<std::uint64_t, std::unique_ptr<Order>> order_index_;

  // Non-owning pointers preserve price-time priority.
  BidSide bid_side_;
  AskSide ask_side_;

  OrderQueue *find_queue(OrderSide side, std::int64_t price);
  std::pair<OrderQueue *, bool> find_or_create_queue(OrderSide side,
                                                     std::int64_t price);
};

} // namespace od
