#include "Execution.hpp"
#include "MatchingEngine.hpp"
#include "Order.hpp"
#include "OrderBook.hpp"

#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <memory>

constexpr std::uint32_t kInstrumentId = 42;

namespace od {

std::unique_ptr<Order> make_order(std::uint64_t id, OrderSide side,
                                  std::int64_t price, std::uint32_t quantity) {
  return std::make_unique<Order>(id, kInstrumentId, price, quantity, side);
}

void print_separator() {
  std::cout << "----------------------------------------\n";
}

void print_market_state(const MatchingEngine &engine) {
  auto bid = engine.best_bid();
  auto ask = engine.best_ask();
  auto spread = engine.bid_ask_spread();

  std::cout << "Best bid: ";
  if (bid.has_value()) {
    std::cout << *bid;
  } else {
    std::cout << "None";
  }

  std::cout << "\nBest ask: ";
  if (ask.has_value()) {
    std::cout << *ask;
  } else {
    std::cout << "None";
  }

  std::cout << "\nSpread: ";
  if (spread.has_value()) {
    std::cout << *spread;
  } else {
    std::cout << "Unavailable";
  }

  std::cout << "\n";
}

void print_statistics(const MatchingEngine &engine) {
  std::cout << "Total executions: " << *engine.total_executions() << "\n";

  std::cout << "Total traded quantity: " << *engine.traded_quantity() << "\n";
}

void print_latest_execution(const MatchingEngine &engine) {
  const Execution *execution = engine.latest_execution();

  if (execution == nullptr) {
    std::cout << "No executions have taken place yet.\n";
    return;
  }

  std::cout << "Latest execution:\n";
  std::cout << "  Execution ID: " << execution->id << "\n";
  std::cout << "  Buyer ID: " << execution->buy_order_id << "\n";
  std::cout << "  Seller ID: " << execution->sell_order_id << "\n";
  std::cout << "  Quantity: " << execution->execution_quantity << "\n";
  std::cout << "  Price: " << execution->execution_price << "\n";
}

void print_execution_history(const MatchingEngine &engine) {
  const auto *executions = engine.execution_history();

  std::cout << "\nComplete execution history\n";
  print_separator();

  if (executions->empty()) {
    std::cout << "No executions.\n";
    return;
  }

  std::cout << std::left << std::setw(12) << "Trade ID" << std::setw(12)
            << "Buyer" << std::setw(12) << "Seller" << std::setw(12)
            << "Quantity" << std::setw(12) << "Price"
            << "\n";

  print_separator();

  for (const auto &execution : *executions) {
    std::cout << std::left << std::setw(12) << execution.id << std::setw(12)
              << execution.buy_order_id << std::setw(12)
              << execution.sell_order_id << std::setw(12)
              << execution.execution_quantity << std::setw(12)
              << execution.execution_price << "\n";
  }
}

void print_order_status(const MatchingEngine &engine, std::uint64_t order_id) {
  const OrderBook *book = engine.order_book();
  const Order *order = book->get_order(order_id);

  std::cout << "Order " << order_id << ": ";

  if (order == nullptr) {
    std::cout << "not present in the order book\n";
    return;
  }

  std::cout << "price = " << order->tick_price()
            << ", remaining quantity = " << order->remaining_quantity()
            << ", state = ";

  switch (order->state()) {
  case OrderState::New:
    std::cout << "New";
    break;
  case OrderState::Active:
    std::cout << "Active";
    break;
  case OrderState::PartiallyFilled:
    std::cout << "PartiallyFilled";
    break;
  case OrderState::Filled:
    std::cout << "Filled";
    break;
  case OrderState::Cancelled:
    std::cout << "Cancelled";
    break;
  case OrderState::Rejected:
    std::cout << "Rejected";
    break;
  }

  std::cout << "\n";
}

} // namespace od

int main() {
  od::MatchingEngine engine(kInstrumentId);

  // --------------------------------------------------
  // Phase 1: Populate the order book
  // --------------------------------------------------

  std::cout << "\nPHASE 1: INITIAL MARKET LIQUIDITY\n";
  od::print_separator();

  // Sell orders
  engine.add_order(od::make_order(101, od::OrderSide::Sell, 101, 40));
  engine.add_order(od::make_order(102, od::OrderSide::Sell, 101, 60));
  engine.add_order(od::make_order(103, od::OrderSide::Sell, 102, 80));
  engine.add_order(od::make_order(104, od::OrderSide::Sell, 104, 50));

  // Buy orders
  engine.add_order(od::make_order(201, od::OrderSide::Buy, 99, 70));
  engine.add_order(od::make_order(202, od::OrderSide::Buy, 98, 100));

  std::cout << "Initial market state:\n";
  od::print_market_state(engine);
  od::print_statistics(engine);

  // --------------------------------------------------
  // Phase 2: Incoming buy order
  // --------------------------------------------------

  std::cout << "\nPHASE 2: INCOMING BUY ORDER\n";
  od::print_separator();

  std::cout << "Submitting BUY order 301: 130 units @ 102\n";

  engine.add_order(od::make_order(301, od::OrderSide::Buy, 102, 130));

  od::print_latest_execution(engine);
  std::cout << "\nMarket state after incoming buy:\n";
  od::print_market_state(engine);

  std::cout << "\nResting order status:\n";
  od::print_order_status(engine, 103);

  // --------------------------------------------------
  // Phase 3: Incoming sell order
  // --------------------------------------------------

  std::cout << "\nPHASE 3: INCOMING SELL ORDER\n";
  od::print_separator();

  std::cout << "Submitting SELL order 302: 90 units @ 98\n";

  engine.add_order(od::make_order(302, od::OrderSide::Sell, 98, 90));

  od::print_latest_execution(engine);
  std::cout << "\nMarket state after incoming sell:\n";
  od::print_market_state(engine);

  std::cout << "\nResting order status:\n";
  od::print_order_status(engine, 202);

  // --------------------------------------------------
  // Phase 4: Cancel partially filled order
  // --------------------------------------------------

  std::cout << "\nPHASE 4: CANCEL RESTING ORDER\n";
  od::print_separator();

  std::cout << "Before cancellation:\n";
  od::print_order_status(engine, 103);

  bool cancelled = engine.cancel_order(103);

  if (cancelled) {
    std::cout << "Order 103 successfully cancelled.\n";
  } else {
    std::cout << "Failed to cancel order 103.\n";
  }

  std::cout << "\nMarket state after cancellation:\n";
  od::print_market_state(engine);

  // --------------------------------------------------
  // Final report
  // --------------------------------------------------

  std::cout << "\nFINAL EXECUTION REPORT\n";
  od::print_separator();

  od::print_statistics(engine);
  od::print_execution_history(engine);

  std::cout << "\nFINAL MARKET STATE\n";
  od::print_separator();
  od::print_market_state(engine);

  return 0;
}
