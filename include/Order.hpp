#pragma once // prevents multiple inclusions
#include <cstdint>

namespace od {
enum class OrderSide { Buy, Sell };

enum class OrderState {
  New,
  Active,
  PartiallyFilled,
  Filled,
  Cancelled,
  Rejected,
};

class Order {
  // Lifecyle rules:
  // An order is allowed to cancel when its state is active or partially filled.
  // A new order can be activated or rejected.
  // An active order can be partially filled, filled, or cancelled.
  // A partially filled order can receive further fills or be cancelled.
  // A filled, cancelled, or rejected order is terminal in this market
  //
  // An order controls its state transitions according to the lifecycle above,
  // therefore we do not expose a general state setter.

public:
  // Constructor
  explicit Order(std::uint64_t order_id, std::uint32_t instrument_id,
                 std::int64_t tick_price, std::uint32_t quantity,
                 OrderSide side);

  std::uint64_t id() const;
  std::uint32_t instrument_id() const;
  std::int64_t tick_price() const;
  std::uint32_t quantity() const;
  OrderSide side() const;
  OrderState state() const;
  std::uint32_t remaining_quantity() const;

  // Fills an amount of the order and updates the state.
  //  On success, the filled quantity increases by the requested amount.
  //    The state becomes PARTIALLY_FILLED if some quantity remains.
  //    The state becomes FILLED if no quantity remains.
  // On failure,
  //  the order's data remains unchanged.
  bool fill(std::uint32_t fill_quantity);

  // Cancel an order according to its lifecycle.
  // if cancellation is invalid for the current state,
  // we return failure without modifying the order.
  bool cancel();

  // Changes the state of an order to Active.
  // Will only succeed if the current state is New.
  bool activate();

  // Changes the state of an order to Rejected.
  // Will only succeed if the current state is New.
  bool reject();

  // Returns true if the order is active
  bool is_active() const;

private:
  // internal storage
  std::uint64_t order_id_;
  std::uint32_t instrument_id_;
  std::int64_t tick_price_;
  std::uint32_t quantity_;
  OrderSide side_;

  OrderState state_;
  std::uint32_t filled_quantity_;

  // Check that price is strictly positive
  bool valid_price() const;
  // Check that:
  //  The fill quantity is greater than zero.
  //  The fill quantity does not exceed the remaining quantity.
  //  The order is in a state that permits fills.
  bool valid_fill(std::uint32_t fill_quantity) const;
};
} // namespace od
