#pragma once
#include "MatchingEngine.hpp"
#include "Execution.hpp"
#include "Order.hpp"
#include "OrderBook.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>

namespace od {

MatchingEngine::MatchingEngine(std::uint32_t instrument_id)
    : instrument_id_(instrument_id), book_(OrderBook(instrument_id)) {}

// Expose the order book and its statistics
const OrderBook *MatchingEngine::order_book() const { return &book_; }
std::optional<std::int64_t> MatchingEngine::best_bid() const {
  return book_.best_bid();
}
std::optional<std::int64_t> MatchingEngine::best_ask() const {
  return book_.best_ask();
}

// expose private values
const std::uint64_t *MatchingEngine::total_executions() const {
  return &total_executions_;
}
const std::uint64_t *MatchingEngine::traded_quantity() const {
  return &traded_quantity_;
}
const std::uint32_t MatchingEngine::instrument_id() const {
  return instrument_id_;
}
const MatchingEngine::ExecutionHistory *
MatchingEngine::execution_history() const {
  return &executions_;
}

void MatchingEngine::validate_incoming_order(Order *order) {
  if (order == nullptr) {
    throw std::invalid_argument("Input pointer is a nullptr");
  } else if (order->state() != OrderState::New) {
    throw std::invalid_argument("Order has wrong state, only OrderState::New "
                                "can be added to Matching Engine.");
  }
}

void MatchingEngine::add_order(std::unique_ptr<Order> order) {

  validate_incoming_order(order.get());

  // If order passes validation we activate it.
  order->activate();

  // Use the incoming order to match against the current OrderBook
  std::uint64_t num_executions = settle_incoming_order(order.get());

  // Once the incoming order has been processed it is moved to the book if it is
  // still active
  if (order->is_active()) {
    book_.add_order(order);
  }
  // Otherwise we delete the order
}

std::uint64_t MatchingEngine::settle_incoming_order(Order *incoming_order) {

  if (incoming_order == nullptr) {
    throw std::invalid_argument("Input pointer is a nullptr");
  }

  // determine if we have a match
  OrderSide side = incoming_order->side();
  std::int64_t inc_price = incoming_order->tick_price();

  Order *counterpart;
  OrderSide counter_side;
  if (side == OrderSide::Buy) {
    counterpart = book_.best_order(OrderSide::Sell);
    counter_side = OrderSide::Sell;
  } else {
    counterpart = book_.best_order(OrderSide::Buy);
    counter_side = OrderSide::Buy;
  }

  // While there are counterparties matching the incoming order
  std::uint64_t num_executions = 0;
  while (
      counterpart != nullptr && incoming_order->remaining_quantity() > 0 &&
      ((counterpart->tick_price() <= inc_price && side == OrderSide::Buy) ||
       (counterpart->tick_price() >= inc_price && side == OrderSide::Sell))) {

    // Create a trade execution
    Order *buyer = (side == OrderSide::Buy) ? incoming_order : counterpart;
    Order *seller = (side == OrderSide::Sell) ? incoming_order : counterpart;
    Execution trade = {
        executions_.size(), // Execution ID
        buyer->id(),
        seller->id(),
        instrument_id(),
        counterpart->tick_price(), // incoming order matches resting prices
        0, // Execution quantity (zero until we know how much is filled)
        num_executions // Sequence Number
    };
    // actually fill the orders and transition states of orders to filled or
    // partially filled
    trade.execution_quantity = fill_order(buyer, seller);
    executions_.emplace_back(trade);

    // If the counterpart was filled
    if (counterpart->state() == OrderState::Filled) {
      // remove the old order from the book
      bool removed = book_.remove_filled_order(counterpart->id());
      if (not removed) {
        throw std::logic_error("An unexpected error occurred during deletion "
                               "of filled order in OrderBook");
      }
      // fetch new a counterpart
      counterpart = book_.best_order(counter_side);
    }

    num_executions++;
  }
  return num_executions;
}

std::uint32_t MatchingEngine::fill_order(Order *bid_order, Order *ask_order) {
  auto fill = std::min(bid_order->remaining_quantity(),
                       ask_order->remaining_quantity());
  bool bid_res = bid_order->fill(fill);
  bool ask_res = ask_order->fill(fill);
  if (not bid_res or not ask_res)
    throw std::logic_error("An error occurred during filling of orders");
  return fill;
}

} // namespace od
