#pragma once
#include "MatchingEngine.hpp"
#include "Execution.hpp"
#include "Order.hpp"
#include "OrderBook.hpp"
#include <algorithm>
#include <cstdint>
#include <optional>

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
const MatchingEngine::ExecutionHistory *
MatchingEngine::execution_history() const {
  return &executions_;
}


} // namespace od
