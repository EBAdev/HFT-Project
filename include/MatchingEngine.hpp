#pragma once
#include "Execution.hpp"
#include "Order.hpp"
#include "OrderBook.hpp"
#include <cstdint>
#include <optional>
#include <vector>

namespace od {

class MatchingEngine {
  // Will handle incoming orders, match compatible buy and sell orders,
  // apply price-time priority, update order states, and report executions.
  // The order book will maintain the remaining resting liquidity.

public:
  // Constructor
  explicit MatchingEngine(OrderBook book);

  // Adds a new limit order to the Market.
  // Only accepts Orders with state new.
  // On success the Order was activated and handed to the OrderBook.
  // On failure the method throws an error
  // and does not transition the order.
  // Successful order submission triggers liquidty matching,
  // this continues until the book has settled in a state where no match is
  // possible.
  bool add_order(Order order);

  // Cancels an active order in the Market.
  // On sucess the order was able to cancel and was removed from the OrderBook.
  // On failure the order cannot cancel.
  bool cancel_order(std::uint64_t order_id);

  // Find an order by ID.
  // Returns nullptr if the order is not found.
  // The returned pointer does not transfer ownership.
  Order *get_order(std::uint64_t order_id);
  const Order *get_order(std::uint64_t order_id) const;

  // Market statistics
  std::optional<std::int64_t> best_bid() const;
  std::optional<std::int64_t> best_ask() const;
  std::optional<std::int64_t> bid_ask_spread() const;
  std::int64_t total_executions() const;
  std::int64_t traded_quantity() const;
  Execution *latest_execution() const;

  // OrderID1 and OrderID2
  using ExecutionHistory = std::vector<Execution>;

  // Returns a history of Executions
  ExecutionHistory match_history() const;

private:
  OrderBook *book_;
  ExecutionHistory executions_;

  // Matches the incoming order with the current bid/ask.
  // An exchange of the instrument happens at the resting orders price.
  // The return is how many executions that happens due to the incoming order.
  // If the return i 0 no bids matched an ask.
  std::uint64_t settle_incoming_liquidity(Order incoming_order);

  // Fills the ask order with the bid volume.
  // The order with the least quantity becomes filled
  // The order with the most quantity becomes partially filled
  // if the quantities are equal both orders become filled
  bool fill_order(Order *bid_order, Order *ask_order);
};
} // namespace od
