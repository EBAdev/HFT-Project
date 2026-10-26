#include "Order.hpp"
#include <cstdint>
#include <stdexcept>

namespace od {

// Constructor
Order::Order(std::uint64_t order_id, std::uint32_t instrument_id,
             std::int64_t tick_price, std::uint32_t quantity, OrderSide side)
    : order_id_(order_id), instrument_id_(instrument_id),
      tick_price_(tick_price), quantity_(quantity), side_(side),
      state_(OrderState::New), filled_quantity_(0) {

  if (not valid_price()) {
    throw std::invalid_argument("The input price is invalid.");
  }

  if (quantity_ == 0) {
    throw std::invalid_argument("The input quantity is invalid.");
  }
}

std::uint64_t Order::id() const { return order_id_; }
std::uint32_t Order::instrument_id() const { return instrument_id_; }
std::int64_t Order::tick_price() const { return tick_price_; }
std::uint32_t Order::quantity() const { return quantity_; }
OrderSide Order::side() const { return side_; }
OrderState Order::state() const { return state_; }
std::uint32_t Order::remaining_quantity() const {
  return quantity_ - filled_quantity_;
}

bool Order::valid_price() const { return tick_price_ > 0; }

bool Order::is_active() const {
  return (state_ == OrderState::Active ||
          state_ == OrderState::PartiallyFilled);
}

bool Order::valid_fill(std::uint32_t fill_quantity) const {
  return (is_active() && fill_quantity > 0 &&
          fill_quantity <= remaining_quantity());
}

bool Order::fill(std::uint32_t fill_quantity) {
  if (valid_fill(fill_quantity)) {
    filled_quantity_ += fill_quantity;
    if (remaining_quantity() > 0) {
      state_ = OrderState::PartiallyFilled;
    } else {
      state_ = OrderState::Filled;
    }
    return true;
  }
  return false;
}

bool Order::cancel() {
  if (is_active()) {
    state_ = OrderState::Cancelled;
    return 1;
  }
  return 0;
}

bool Order::activate() {
  if (state_ == OrderState::New) {
    state_ = OrderState::Active;
    return 1;
  }
  return 0;
}

bool Order::reject() {
  if (state_ == OrderState::New) {
    state_ = OrderState::Rejected;
    return 1;
  }
  return 0;
}

} // namespace od
