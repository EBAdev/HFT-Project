#include "OrderBook.hpp"
#include "Order.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>

namespace od {

OrderBook::OrderBook(std::uint32_t instrument_id)
    : instrument_id_(instrument_id) {};

std::uint32_t OrderBook::instrument_id() const { return instrument_id_; }

bool OrderBook::add_order(std::unique_ptr<Order> &order) {
  if (order == nullptr) {
    throw std::invalid_argument("Recieved a nullptr as input");
  }
  if (order->instrument_id() != instrument_id_) {
    throw std::invalid_argument(
        "Order is in the wrong book, the InstrumentID, should match.");
  }

  // only allow orders which can be filled to rest
  if (not((order->state() == OrderState::Active or
           order->state() == OrderState::PartiallyFilled) and
          order->remaining_quantity() > 0)) {
    return false;
  }

  // Check whether the order ID already exists
  if (order_index_.find(order->id()) != order_index_.end()) {
    return false;
  }

  // We will need these later once owenership has been transferred.
  std::int64_t price = order->tick_price();
  std::uint64_t id = order->id();
  OrderSide side = order->side();
  Order *ptr = order.get();

  // add the order to the index
  auto result = order_index_.try_emplace(id, nullptr);
  if (!result.second) {
    return false; // Duplicate ID; order is still owned by caller.
  }

  // Depending on weather the order is a buy/sell find the queue

  OrderQueue *queue;
  bool created = false;
  try {
    auto tmp = find_or_create_queue(side, price);
    queue = tmp.first;
    created = tmp.second;
    queue->emplace_back(ptr);
  } catch (...) {
    order_index_.erase(id);
    // Also remove the price level if this attempt created it
    // and it is still empty.
    if (created && queue->empty()) {
      if (side == OrderSide::Buy) {
        bid_side_.erase(price);
      } else {
        ask_side_.erase(price);
      }
    }
    throw;
  }

  result.first->second = std::move(order);

  return true;
}

Order *OrderBook::get_order(std::uint64_t order_id) {
  auto result = order_index_.find(order_id);
  if (result != order_index_.end()) {
    return result->second.get();
  }
  return nullptr;
}
const Order *OrderBook::get_order(std::uint64_t order_id) const {
  auto result = order_index_.find(order_id);
  if (result != order_index_.end()) {
    return result->second.get();
  }
  return nullptr;
}

std::pair<OrderBook::OrderQueue *, bool>
OrderBook::find_or_create_queue(OrderSide side, std::int64_t price) {
  if (side == OrderSide::Buy) {
    auto [it, inserted] = bid_side_.try_emplace(price);
    return {&it->second, inserted};
  } else {
    auto [it, inserted] = ask_side_.try_emplace(price);
    return {&it->second, inserted};
  }
}

OrderBook::OrderQueue *OrderBook::find_queue(OrderSide side,
                                             std::int64_t price) {
  // Depending on weather the order is a buy/sell find the queue
  OrderQueue *queue;
  if (side == OrderSide::Buy) {
    auto result = bid_side_.find(price);
    if (result == bid_side_.end()) {
      return nullptr;
    }
    queue = &result->second;
  } else {
    auto result = ask_side_.find(price);
    if (result == ask_side_.end()) {
      return nullptr;
    }
    queue = &result->second;
  }

  return queue;
}

bool OrderBook::cancel_order(std::uint64_t order_id) {
  Order *order = get_order(order_id);
  if (order == nullptr) {
    return false;
  }

  // Depending on whether the order is a buy/sell find the queue
  OrderQueue *queue = find_queue(order->side(), order->tick_price());
  if (queue == nullptr) {
    return false;
  }

  // remove the order from the queue
  auto it = std::find(queue->begin(), queue->end(), order);
  if (it == queue->end()) {
    return false;
  }
  // Cancel order if possible
  if (not order->cancel()) {
    return false;
  }
  queue->erase(it);

  if (queue->empty()) {
    // if order was the last at its price level, we remove the price level from
    // the bid/ask
    if (order->side() == OrderSide::Buy) {
      bid_side_.erase(order->tick_price());
    } else {
      ask_side_.erase(order->tick_price());
    }
  }

  // remove the order from the order_index, this destroys the order.
  order_index_.erase(order_id);
  return true;
}

std::optional<std::int64_t> OrderBook::best_bid() const {
  if (bid_side_.empty()) {
    return std::nullopt;
  }
  return bid_side_.begin()->first;
}
std::optional<std::int64_t> OrderBook::best_ask() const {
  if (ask_side_.empty()) {
    return std::nullopt;
  }
  return ask_side_.begin()->first;
}

} // namespace od
