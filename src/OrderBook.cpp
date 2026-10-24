#include "OrderBook.hpp"
#include "Order.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <stdexcept>
#include <utility>

namespace od {

OrderBook::OrderBook(std::uint32_t instrument_id)
    : instrument_id_(instrument_id) {};

std::uint32_t OrderBook::instrument_id() const { return instrument_id_; }

bool OrderBook::add_order(std::unique_ptr<Order> order) {
  if (order == NULL) {
    throw std::invalid_argument("Recieved a nullptr as input");
  } 
  if (order->instrument_id() != instrument_id_) {
    throw std::invalid_argument(
        "Order is in the wrong book, the InstrumentID, should match.");
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
  // Note that order is a nullptr after this line
  auto result = order_index_.emplace(id, std::move(order));
  if (!result.second) {
    return false;
  }

  // Depending on weather the order is a buy/sell find the queue
  OrderQueue *queue;
  if (side == OrderSide::Buy) {
    // add a queue if the price has not been seen before
    bid_side_.try_emplace(price);
    queue = &bid_side_.find(price)->second;
  } else {
    ask_side_.try_emplace(price);
    queue = &ask_side_.find(price)->second;
  }
  queue->emplace_back(ptr);

  return true;
}

} // namespace od
