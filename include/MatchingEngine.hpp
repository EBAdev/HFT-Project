#pragma once
#include "Execution.hpp"
#include "Order.hpp"
#include "OrderBook.hpp"
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

namespace od {

class MatchingEngine {
  // Will handle incoming orders, match compatible buy and sell orders,
  // apply price-time priority, update order states, and report executions.
  // The order book will maintain the remaining resting liquidity.

public:
  // Constructor, a Matching Engine per traded instrument.
  explicit MatchingEngine(std::uint32_t instrument_id);

  // Adds a new limit order to the Market, only accepts Orders with state New.
  // On failure the method throws an error and the order is destroyed.
  //
  // If the order is accepted it is activated and, ownership is transferred to
  // the MatchingEngine The MatchingEngine will then try to match the incoming
  // order based on the current orderbook If the order is completely filled, the
  // executions are recorded and the order is destroyed. If the order has
  // remaning quantity and no match is possible, ownership is transferred to the
  // order book.
  void add_order(std::unique_ptr<Order> order);

  // Cancels an active order in the Market.
  // On sucess the order was able to cancel and was removed from the OrderBook.
  // On failure the order cannot cancel.
  bool cancel_order(std::uint64_t order_id);

  // Market statistics
  std::optional<std::int64_t> best_bid() const;
  std::optional<std::int64_t> best_ask() const;
  std::optional<std::int64_t> bid_ask_spread() const;

  // Cumulative counters
  const std::uint64_t *total_executions() const;
  const std::uint64_t *traded_quantity() const;

  // returns a nullptr if no executions has happened
  const Execution *latest_execution() const;

  // OrderID1 and OrderID2
  using ExecutionHistory = std::vector<Execution>;

  // Returns a pointer to the history of Executions
  const ExecutionHistory *execution_history() const;
  // Returns a pointer to the OrderBook
  const OrderBook *order_book() const;

  const std::uint32_t instrument_id() const;
  // Throws an error if the order is invalid and would not be accepted by the
  // add_order method
  void validate_incoming_order(Order *order);

private:
  std::int32_t instrument_id_;

  // A MatchingEngine owns its OrderBook
  OrderBook book_;
  ExecutionHistory executions_;
  std::uint64_t total_executions_;
  std::uint64_t traded_quantity_;

  // Matches the incoming order with the current bid/ask.
  // An exchange of the instrument happens at the resting orders price.
  // The return is how many executions that happens due to the incoming order.
  // If the return i 0 no bids matched an ask.
  std::uint64_t settle_incoming_order(Order *incoming_order);

  // Fills the ask order with the bid volume.
  // The order with the least quantity becomes filled
  // The order with the most quantity becomes partially filled
  // if the quantities are equal both orders become filled
  // Return is how much was filled
  std::uint32_t fill_order(Order *bid_order, Order *ask_order);
};
} // namespace od
