#include "MatchingEngine.hpp"
#include "Order.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <stdexcept>

namespace od {
namespace {

constexpr std::uint32_t kInstrumentId = 42;

std::unique_ptr<Order> make_order(std::uint64_t id, OrderSide side,
                                  std::int64_t price, std::uint32_t quantity,
                                  std::uint32_t instrument_id = kInstrumentId) {
  return std::make_unique<Order>(id, instrument_id, price, quantity, side);
}

class MatchingEngineTest : public ::testing::Test {
protected:
  MatchingEngineTest() : engine(kInstrumentId) {}

  MatchingEngine engine;
};

// -----------------------------------------------------------------------------
// Initial state and orders that do not cross
// -----------------------------------------------------------------------------

TEST_F(MatchingEngineTest, StartsWithAnEmptyBookAndZeroStatistics) {
  EXPECT_FALSE(engine.best_bid().has_value());
  EXPECT_FALSE(engine.best_ask().has_value());
  EXPECT_FALSE(engine.bid_ask_spread().has_value());

  ASSERT_NE(engine.total_executions(), nullptr);
  ASSERT_NE(engine.traded_quantity(), nullptr);
  EXPECT_EQ(*engine.total_executions(), 0u);
  EXPECT_EQ(*engine.traded_quantity(), 0u);

  EXPECT_EQ(engine.latest_execution(), nullptr);

  ASSERT_NE(engine.execution_history(), nullptr);
  EXPECT_TRUE(engine.execution_history()->empty());
}

TEST_F(MatchingEngineTest, RestingBuyOrderSetsBestBid) {
  engine.add_order(make_order(1, OrderSide::Buy, 99, 10));

  ASSERT_TRUE(engine.best_bid().has_value());
  EXPECT_EQ(*engine.best_bid(), 99);
  EXPECT_FALSE(engine.best_ask().has_value());
  EXPECT_FALSE(engine.bid_ask_spread().has_value());

  EXPECT_EQ(*engine.total_executions(), 0u);
  EXPECT_EQ(*engine.traded_quantity(), 0u);
}

TEST_F(MatchingEngineTest, RestingSellOrderSetsBestAsk) {
  engine.add_order(make_order(1, OrderSide::Sell, 101, 10));

  ASSERT_TRUE(engine.best_ask().has_value());
  EXPECT_EQ(*engine.best_ask(), 101);
  EXPECT_FALSE(engine.best_bid().has_value());
  EXPECT_FALSE(engine.bid_ask_spread().has_value());
}

TEST_F(MatchingEngineTest, NonCrossingOrdersRemainInTheBook) {
  engine.add_order(make_order(1, OrderSide::Buy, 99, 10));
  engine.add_order(make_order(2, OrderSide::Sell, 101, 10));

  ASSERT_TRUE(engine.best_bid().has_value());
  ASSERT_TRUE(engine.best_ask().has_value());
  EXPECT_EQ(*engine.best_bid(), 99);
  EXPECT_EQ(*engine.best_ask(), 101);
  EXPECT_EQ(*engine.bid_ask_spread(), 2);

  EXPECT_EQ(*engine.total_executions(), 0u);
  EXPECT_EQ(*engine.traded_quantity(), 0u);
}

// -----------------------------------------------------------------------------
// Matching and execution accounting
// -----------------------------------------------------------------------------

TEST_F(MatchingEngineTest, EqualPriceOrdersExecuteCompletely) {
  engine.add_order(make_order(1, OrderSide::Sell, 100, 10));
  engine.add_order(make_order(2, OrderSide::Buy, 100, 10));

  EXPECT_FALSE(engine.best_bid().has_value());
  EXPECT_FALSE(engine.best_ask().has_value());

  EXPECT_EQ(*engine.total_executions(), 1u);
  EXPECT_EQ(*engine.traded_quantity(), 10u);
  ASSERT_NE(engine.latest_execution(), nullptr);
  ASSERT_NE(engine.execution_history(), nullptr);
  EXPECT_EQ(engine.execution_history()->size(), 1u);
}

TEST_F(MatchingEngineTest, AggressiveBuyExecutesAtRestingAskPrice) {
  engine.add_order(make_order(1, OrderSide::Sell, 100, 10));
  engine.add_order(make_order(2, OrderSide::Buy, 105, 10));

  EXPECT_EQ(*engine.total_executions(), 1u);
  EXPECT_EQ(*engine.traded_quantity(), 10u);
  EXPECT_FALSE(engine.best_ask().has_value());
  EXPECT_FALSE(engine.best_bid().has_value());
}

TEST_F(MatchingEngineTest, AggressiveSellExecutesAtRestingBidPrice) {
  engine.add_order(make_order(1, OrderSide::Buy, 105, 10));
  engine.add_order(make_order(2, OrderSide::Sell, 100, 10));

  EXPECT_EQ(*engine.total_executions(), 1u);
  EXPECT_EQ(*engine.traded_quantity(), 10u);
  EXPECT_FALSE(engine.best_bid().has_value());
  EXPECT_FALSE(engine.best_ask().has_value());
}

TEST_F(MatchingEngineTest,
       IncomingBuyMatchesMultipleAskPriceLevelsInPriceOrder) {
  engine.add_order(make_order(1, OrderSide::Sell, 103, 10));
  engine.add_order(make_order(2, OrderSide::Sell, 101, 10));
  engine.add_order(make_order(3, OrderSide::Sell, 102, 10));

  engine.add_order(make_order(4, OrderSide::Buy, 103, 30));

  EXPECT_EQ(*engine.total_executions(), 3u);
  EXPECT_EQ(*engine.traded_quantity(), 30u);
  EXPECT_FALSE(engine.best_ask().has_value());
  EXPECT_FALSE(engine.best_bid().has_value());

  ASSERT_NE(engine.execution_history(), nullptr);
  EXPECT_EQ(engine.execution_history()->size(), 3u);
}

TEST_F(MatchingEngineTest,
       IncomingSellMatchesMultipleBidPriceLevelsInPriceOrder) {
  engine.add_order(make_order(1, OrderSide::Buy, 98, 10));
  engine.add_order(make_order(2, OrderSide::Buy, 100, 10));
  engine.add_order(make_order(3, OrderSide::Buy, 99, 10));

  engine.add_order(make_order(4, OrderSide::Sell, 98, 30));

  EXPECT_EQ(*engine.total_executions(), 3u);
  EXPECT_EQ(*engine.traded_quantity(), 30u);
  EXPECT_FALSE(engine.best_bid().has_value());
  EXPECT_FALSE(engine.best_ask().has_value());
}

// -----------------------------------------------------------------------------
// Partial fills and residual incoming quantity
// -----------------------------------------------------------------------------

TEST_F(MatchingEngineTest, SmallerIncomingBuyPartiallyFillsRestingSell) {
  engine.add_order(make_order(1, OrderSide::Sell, 100, 100));
  engine.add_order(make_order(2, OrderSide::Buy, 100, 40));

  EXPECT_EQ(*engine.total_executions(), 1u);
  EXPECT_EQ(*engine.traded_quantity(), 40u);

  ASSERT_TRUE(engine.best_ask().has_value());
  EXPECT_EQ(*engine.best_ask(), 100);
  EXPECT_FALSE(engine.best_bid().has_value());

  const Order *resting_order = engine.order_book()->get_order(1);
  ASSERT_NE(resting_order, nullptr);
  EXPECT_EQ(resting_order->remaining_quantity(), 60u);
  EXPECT_EQ(resting_order->state(), OrderState::PartiallyFilled);
}

TEST_F(MatchingEngineTest, SmallerIncomingSellPartiallyFillsRestingBuy) {
  engine.add_order(make_order(1, OrderSide::Buy, 100, 100));
  engine.add_order(make_order(2, OrderSide::Sell, 100, 40));

  EXPECT_EQ(*engine.total_executions(), 1u);
  EXPECT_EQ(*engine.traded_quantity(), 40u);

  ASSERT_TRUE(engine.best_bid().has_value());
  EXPECT_EQ(*engine.best_bid(), 100);
  EXPECT_FALSE(engine.best_ask().has_value());

  const Order *resting_order = engine.order_book()->get_order(1);
  ASSERT_NE(resting_order, nullptr);
  EXPECT_EQ(resting_order->remaining_quantity(), 60u);
  EXPECT_EQ(resting_order->state(), OrderState::PartiallyFilled);
}

TEST_F(MatchingEngineTest, ResidualIncomingBuyBecomesRestingBid) {
  engine.add_order(make_order(1, OrderSide::Sell, 100, 50));
  engine.add_order(make_order(2, OrderSide::Buy, 102, 80));

  EXPECT_EQ(*engine.total_executions(), 1u);
  EXPECT_EQ(*engine.traded_quantity(), 50u);

  ASSERT_TRUE(engine.best_bid().has_value());
  EXPECT_EQ(*engine.best_bid(), 102);
  EXPECT_FALSE(engine.best_ask().has_value());

  const Order *residual = engine.order_book()->get_order(2);
  ASSERT_NE(residual, nullptr);
  EXPECT_EQ(residual->remaining_quantity(), 30u);
  EXPECT_EQ(residual->state(), OrderState::PartiallyFilled);
}

TEST_F(MatchingEngineTest, ResidualIncomingSellBecomesRestingAsk) {
  engine.add_order(make_order(1, OrderSide::Buy, 102, 50));
  engine.add_order(make_order(2, OrderSide::Sell, 100, 80));

  EXPECT_EQ(*engine.total_executions(), 1u);
  EXPECT_EQ(*engine.traded_quantity(), 50u);

  ASSERT_TRUE(engine.best_ask().has_value());
  EXPECT_EQ(*engine.best_ask(), 100);
  EXPECT_FALSE(engine.best_bid().has_value());

  const Order *residual = engine.order_book()->get_order(2);
  ASSERT_NE(residual, nullptr);
  EXPECT_EQ(residual->remaining_quantity(), 30u);
  EXPECT_EQ(residual->state(), OrderState::PartiallyFilled);
}

TEST_F(MatchingEngineTest, OneRestingOrderCanBeFilledBySeveralIncomingOrders) {
  engine.add_order(make_order(1, OrderSide::Sell, 100, 200));

  engine.add_order(make_order(2, OrderSide::Buy, 100, 50));
  engine.add_order(make_order(3, OrderSide::Buy, 100, 75));
  engine.add_order(make_order(4, OrderSide::Buy, 100, 75));

  EXPECT_EQ(*engine.total_executions(), 3u);
  EXPECT_EQ(*engine.traded_quantity(), 200u);
  EXPECT_FALSE(engine.best_ask().has_value());
  EXPECT_FALSE(engine.best_bid().has_value());
  EXPECT_EQ(engine.order_book()->get_order(1), nullptr);
}

// -----------------------------------------------------------------------------
// Price-time priority
// -----------------------------------------------------------------------------

TEST_F(MatchingEngineTest, BestPricedAskExecutesBeforeWorsePricedAsks) {
  engine.add_order(make_order(1, OrderSide::Sell, 102, 10));
  engine.add_order(make_order(2, OrderSide::Sell, 101, 10));

  engine.add_order(make_order(3, OrderSide::Buy, 105, 10));

  EXPECT_EQ(*engine.total_executions(), 1u);
  EXPECT_EQ(*engine.traded_quantity(), 10u);

  // The order at 101 should have been consumed first.
  EXPECT_EQ(engine.order_book()->get_order(2), nullptr);
  ASSERT_NE(engine.order_book()->get_order(1), nullptr);
  EXPECT_EQ(*engine.best_ask(), 102);
}

TEST_F(MatchingEngineTest, BestPricedBidExecutesBeforeWorsePricedBids) {
  engine.add_order(make_order(1, OrderSide::Buy, 99, 10));
  engine.add_order(make_order(2, OrderSide::Buy, 100, 10));

  engine.add_order(make_order(3, OrderSide::Sell, 95, 10));

  EXPECT_EQ(*engine.total_executions(), 1u);
  EXPECT_EQ(*engine.traded_quantity(), 10u);

  // The order at 100 should have been consumed first.
  EXPECT_EQ(engine.order_book()->get_order(2), nullptr);
  ASSERT_NE(engine.order_book()->get_order(1), nullptr);
  EXPECT_EQ(*engine.best_bid(), 99);
}

TEST_F(MatchingEngineTest, SamePriceAsksAreMatchedInArrivalOrder) {
  engine.add_order(make_order(1, OrderSide::Sell, 100, 50));
  engine.add_order(make_order(2, OrderSide::Sell, 100, 100));

  engine.add_order(make_order(3, OrderSide::Buy, 100, 75));

  // FIFO means order 1 is completely filled before order 2 is touched.
  EXPECT_EQ(engine.order_book()->get_order(1), nullptr);

  const Order *second = engine.order_book()->get_order(2);
  ASSERT_NE(second, nullptr);
  EXPECT_EQ(second->remaining_quantity(), 75u);
  EXPECT_EQ(*engine.total_executions(), 2u);
  EXPECT_EQ(*engine.traded_quantity(), 75u);
}

TEST_F(MatchingEngineTest, SamePriceBidsAreMatchedInArrivalOrder) {
  engine.add_order(make_order(1, OrderSide::Buy, 100, 50));
  engine.add_order(make_order(2, OrderSide::Buy, 100, 100));

  engine.add_order(make_order(3, OrderSide::Sell, 100, 75));

  EXPECT_EQ(engine.order_book()->get_order(1), nullptr);

  const Order *second = engine.order_book()->get_order(2);
  ASSERT_NE(second, nullptr);
  EXPECT_EQ(second->remaining_quantity(), 75u);
  EXPECT_EQ(*engine.total_executions(), 2u);
  EXPECT_EQ(*engine.traded_quantity(), 75u);
}

TEST_F(MatchingEngineTest, PartiallyFilledOrderRetainsItsTimePriority) {
  engine.add_order(make_order(1, OrderSide::Sell, 100, 100));
  engine.add_order(make_order(2, OrderSide::Sell, 100, 100));

  engine.add_order(make_order(3, OrderSide::Buy, 100, 40));

  const Order *first = engine.order_book()->get_order(1);
  ASSERT_NE(first, nullptr);
  EXPECT_EQ(first->remaining_quantity(), 60u);

  engine.add_order(make_order(4, OrderSide::Buy, 100, 80));

  EXPECT_EQ(engine.order_book()->get_order(1), nullptr);

  const Order *second = engine.order_book()->get_order(2);
  ASSERT_NE(second, nullptr);
  EXPECT_EQ(second->remaining_quantity(), 80u);
  EXPECT_EQ(*engine.traded_quantity(), 120u);
}

// -----------------------------------------------------------------------------
// Cancellation
// -----------------------------------------------------------------------------

TEST_F(MatchingEngineTest, CancelsRestingBuyOrder) {
  engine.add_order(make_order(1, OrderSide::Buy, 99, 10));

  EXPECT_TRUE(engine.cancel_order(1));
  EXPECT_FALSE(engine.cancel_order(1));
  EXPECT_EQ(engine.order_book()->get_order(1), nullptr);
  EXPECT_FALSE(engine.best_bid().has_value());
  EXPECT_EQ(*engine.total_executions(), 0u);
  EXPECT_EQ(*engine.traded_quantity(), 0u);
}

TEST_F(MatchingEngineTest, CancelsRestingSellOrder) {
  engine.add_order(make_order(1, OrderSide::Sell, 101, 10));

  EXPECT_TRUE(engine.cancel_order(1));
  EXPECT_EQ(engine.order_book()->get_order(1), nullptr);
  EXPECT_FALSE(engine.best_ask().has_value());
}

TEST_F(MatchingEngineTest, CancellingUnknownOrderReturnsFalse) {
  EXPECT_FALSE(engine.cancel_order(999));
}

TEST_F(MatchingEngineTest,
       CancellingOneOrderDoesNotRemoveOtherOrdersAtSamePrice) {
  engine.add_order(make_order(1, OrderSide::Sell, 101, 10));
  engine.add_order(make_order(2, OrderSide::Sell, 101, 20));
  engine.add_order(make_order(3, OrderSide::Sell, 101, 30));

  EXPECT_TRUE(engine.cancel_order(2));

  EXPECT_EQ(engine.order_book()->get_order(2), nullptr);
  ASSERT_NE(engine.order_book()->get_order(1), nullptr);
  ASSERT_NE(engine.order_book()->get_order(3), nullptr);
  EXPECT_EQ(*engine.best_ask(), 101);
}

TEST_F(MatchingEngineTest, CancellingBestPriceMovesBestPriceToNextLevel) {
  engine.add_order(make_order(1, OrderSide::Buy, 100, 10));
  engine.add_order(make_order(2, OrderSide::Buy, 99, 10));

  EXPECT_TRUE(engine.cancel_order(1));

  ASSERT_TRUE(engine.best_bid().has_value());
  EXPECT_EQ(*engine.best_bid(), 99);
}

// -----------------------------------------------------------------------------
// Market statistics and execution history
// -----------------------------------------------------------------------------

TEST_F(MatchingEngineTest, SpreadIsAskMinusBid) {
  engine.add_order(make_order(1, OrderSide::Buy, 99, 10));
  engine.add_order(make_order(2, OrderSide::Sell, 104, 10));

  ASSERT_TRUE(engine.bid_ask_spread().has_value());
  EXPECT_EQ(*engine.bid_ask_spread(), 5);
}

TEST_F(MatchingEngineTest, PartialFillDoesNotChangeBestPrice) {
  engine.add_order(make_order(1, OrderSide::Sell, 100, 100));
  engine.add_order(make_order(2, OrderSide::Buy, 100, 40));

  ASSERT_TRUE(engine.best_ask().has_value());
  EXPECT_EQ(*engine.best_ask(), 100);
}

TEST_F(MatchingEngineTest, ExecutionHistoryAndCountersStayConsistent) {
  engine.add_order(make_order(1, OrderSide::Sell, 100, 10));
  engine.add_order(make_order(2, OrderSide::Sell, 101, 20));
  engine.add_order(make_order(3, OrderSide::Buy, 101, 30));

  ASSERT_NE(engine.execution_history(), nullptr);
  EXPECT_EQ(engine.execution_history()->size(), *engine.total_executions());
  EXPECT_EQ(*engine.total_executions(), 2u);
  EXPECT_EQ(*engine.traded_quantity(), 30u);
  ASSERT_NE(engine.latest_execution(), nullptr);
}

TEST_F(MatchingEngineTest, CancellationDoesNotChangeExecutionStatistics) {
  engine.add_order(make_order(1, OrderSide::Buy, 99, 10));
  engine.add_order(make_order(2, OrderSide::Sell, 101, 10));

  EXPECT_TRUE(engine.cancel_order(1));

  EXPECT_EQ(*engine.total_executions(), 0u);
  EXPECT_EQ(*engine.traded_quantity(), 0u);
}

// -----------------------------------------------------------------------------
// Validation and instrument isolation
// -----------------------------------------------------------------------------

TEST_F(MatchingEngineTest, RejectsNullOrder) {
  std::unique_ptr<Order> empty_order;

  EXPECT_THROW(engine.add_order(std::move(empty_order)), std::invalid_argument);
}

TEST_F(MatchingEngineTest, RejectsOrderForDifferentInstrument) {
  EXPECT_THROW(engine.add_order(make_order(1, OrderSide::Buy, 100, 10, 999)),
               std::invalid_argument);
}

TEST_F(MatchingEngineTest, RejectsOrderThatIsNotNew) {
  auto order = make_order(1, OrderSide::Buy, 100, 10);
  ASSERT_TRUE(order->activate());

  EXPECT_THROW(engine.add_order(std::move(order)), std::invalid_argument);
}

TEST_F(MatchingEngineTest, DifferentEnginesMaintainIndependentBooks) {
  MatchingEngine another_engine(kInstrumentId + 1);

  engine.add_order(make_order(1, OrderSide::Buy, 99, 10));
  another_engine.add_order(
      make_order(1, OrderSide::Sell, 101, 10, kInstrumentId + 1));

  EXPECT_EQ(*engine.best_bid(), 99);
  EXPECT_FALSE(engine.best_ask().has_value());

  EXPECT_FALSE(another_engine.best_bid().has_value());
  EXPECT_EQ(*another_engine.best_ask(), 101);

  EXPECT_EQ(*engine.total_executions(), 0u);
  EXPECT_EQ(*another_engine.total_executions(), 0u);
}

// -----------------------------------------------------------------------------
// Integration scenario: multiple price levels, FIFO, partial residual
// -----------------------------------------------------------------------------

TEST_F(MatchingEngineTest, MultiLevelFifoScenarioProducesExpectedResidualBook) {
  // At 101, order 2 arrived before order 3.
  engine.add_order(make_order(1, OrderSide::Sell, 102, 100));
  engine.add_order(make_order(2, OrderSide::Sell, 101, 100));
  engine.add_order(make_order(3, OrderSide::Sell, 101, 50));
  engine.add_order(make_order(4, OrderSide::Sell, 103, 100));

  engine.add_order(make_order(5, OrderSide::Buy, 103, 180));

  // 100 from order 2, 50 from order 3, and 30 from order 1.
  EXPECT_EQ(*engine.total_executions(), 3u);
  EXPECT_EQ(*engine.traded_quantity(), 180u);

  EXPECT_EQ(engine.order_book()->get_order(2), nullptr);
  EXPECT_EQ(engine.order_book()->get_order(3), nullptr);

  const Order *order_at_102 = engine.order_book()->get_order(1);
  ASSERT_NE(order_at_102, nullptr);
  EXPECT_EQ(order_at_102->remaining_quantity(), 70u);

  const Order *order_at_103 = engine.order_book()->get_order(4);
  ASSERT_NE(order_at_103, nullptr);
  EXPECT_EQ(order_at_103->remaining_quantity(), 100u);

  EXPECT_EQ(*engine.best_ask(), 102);
  EXPECT_FALSE(engine.best_bid().has_value());
  ASSERT_NE(engine.latest_execution(), nullptr);
}

TEST_F(MatchingEngineTest, ExecutionRecordContainsCorrectBuyAndSellOrderIds) {
  auto sell = make_order(1, OrderSide::Sell, 100, 10);
  auto buy = make_order(2, OrderSide::Buy, 100, 10);

  engine.add_order(std::move(sell));
  engine.add_order(std::move(buy));

  ASSERT_EQ(engine.execution_history()->size(), 1);

  const auto &execution = engine.execution_history()->front();

  EXPECT_EQ(execution.buy_order_id, 2);
  EXPECT_EQ(execution.sell_order_id, 1);
  EXPECT_EQ(execution.instrument_id, kInstrumentId);
  EXPECT_EQ(execution.execution_price, 100);
  EXPECT_EQ(execution.execution_quantity, 10);
}

TEST_F(MatchingEngineTest, ExecutionRecordUsesRestingOrderPrice) {
  auto sell = make_order(1, OrderSide::Sell, 105, 10);
  auto buy = make_order(2, OrderSide::Buy, 110, 10);

  engine.add_order(std::move(sell));
  engine.add_order(std::move(buy));

  ASSERT_EQ(engine.execution_history()->size(), 1);

  EXPECT_EQ(engine.execution_history()->front().execution_price, 105);
}

TEST_F(MatchingEngineTest, ExecutionIdsAreConsecutive) {
  auto sell1 = make_order(1, OrderSide::Sell, 100, 10);
  auto sell2 = make_order(2, OrderSide::Sell, 101, 10);
  auto buy = make_order(3, OrderSide::Buy, 101, 20);

  engine.add_order(std::move(sell1));
  engine.add_order(std::move(sell2));
  engine.add_order(std::move(buy));

  const auto *history = engine.execution_history();

  ASSERT_EQ(history->size(), 2);
  EXPECT_EQ((*history)[0].id, 0);
  EXPECT_EQ((*history)[1].id, 1);
}

TEST_F(MatchingEngineTest, SequenceNumberResetsForEachIncomingOrder) {
  auto sell1 = make_order(1, OrderSide::Sell, 100, 10);
  auto sell2 = make_order(2, OrderSide::Sell, 101, 10);
  auto buy1 = make_order(3, OrderSide::Buy, 101, 20);

  engine.add_order(std::move(sell1));
  engine.add_order(std::move(sell2));
  engine.add_order(std::move(buy1));

  auto sell3 = make_order(4, OrderSide::Sell, 99, 10);
  auto buy2 = make_order(5, OrderSide::Buy, 99, 10);

  engine.add_order(std::move(sell3));
  engine.add_order(std::move(buy2));

  const auto *history = engine.execution_history();

  ASSERT_EQ(history->size(), 3);

  EXPECT_EQ((*history)[0].sequence_number, 0);
  EXPECT_EQ((*history)[1].sequence_number, 1);
  EXPECT_EQ((*history)[2].sequence_number, 0);
}

TEST_F(MatchingEngineTest, ExecutionStatisticsMatchExecutionHistory) {
  auto sell1 = make_order(1, OrderSide::Sell, 100, 5);
  auto sell2 = make_order(2, OrderSide::Sell, 101, 10);
  auto buy = make_order(3, OrderSide::Buy, 101, 12);

  engine.add_order(std::move(sell1));
  engine.add_order(std::move(sell2));
  engine.add_order(std::move(buy));

  const auto *history = engine.execution_history();

  std::size_t total_quantity = 0;

  for (const auto &execution : *history) {
    total_quantity += execution.execution_quantity;
  }

  EXPECT_EQ(*engine.total_executions(), history->size());
  EXPECT_EQ(*engine.traded_quantity(), total_quantity);
  EXPECT_EQ(*engine.traded_quantity(), 12);
}

TEST_F(MatchingEngineTest, LatestExecutionReturnsLastExecution) {
  auto sell1 = make_order(1, OrderSide::Sell, 100, 5);
  auto sell2 = make_order(2, OrderSide::Sell, 101, 5);
  auto buy = make_order(3, OrderSide::Buy, 101, 10);

  engine.add_order(std::move(sell1));
  engine.add_order(std::move(sell2));
  engine.add_order(std::move(buy));

  const auto *history = engine.execution_history();
  const auto *latest = engine.latest_execution();

  ASSERT_EQ(history->size(), 2);
  ASSERT_NE(latest, nullptr);

  EXPECT_EQ(latest->id, history->back().id);
  EXPECT_EQ(latest->buy_order_id, history->back().buy_order_id);
  EXPECT_EQ(latest->sell_order_id, history->back().sell_order_id);
}

TEST_F(MatchingEngineTest,
       IncomingBuyMatchesMultipleAskLevelsAtEachRestingPrice) {
  auto sell1 = make_order(1, OrderSide::Sell, 100, 5);
  auto sell2 = make_order(2, OrderSide::Sell, 101, 5);
  auto buy = make_order(3, OrderSide::Buy, 101, 10);

  engine.add_order(std::move(sell1));
  engine.add_order(std::move(sell2));
  engine.add_order(std::move(buy));

  const auto *history = engine.execution_history();

  ASSERT_EQ(history->size(), 2);

  EXPECT_EQ((*history)[0].execution_price, 100);
  EXPECT_EQ((*history)[0].execution_quantity, 5);

  EXPECT_EQ((*history)[1].execution_price, 101);
  EXPECT_EQ((*history)[1].execution_quantity, 5);
}

TEST_F(MatchingEngineTest,
       IncomingSellMatchesMultipleBidLevelsAtEachRestingPrice) {
  auto buy1 = make_order(1, OrderSide::Buy, 101, 5);
  auto buy2 = make_order(2, OrderSide::Buy, 100, 5);
  auto sell = make_order(3, OrderSide::Sell, 100, 10);

  engine.add_order(std::move(buy1));
  engine.add_order(std::move(buy2));
  engine.add_order(std::move(sell));

  const auto *history = engine.execution_history();

  ASSERT_EQ(history->size(), 2);

  EXPECT_EQ((*history)[0].execution_price, 101);
  EXPECT_EQ((*history)[0].execution_quantity, 5);

  EXPECT_EQ((*history)[1].execution_price, 100);
  EXPECT_EQ((*history)[1].execution_quantity, 5);
}

TEST_F(MatchingEngineTest,
       PartialFillAcrossMultipleOrdersProducesCorrectQuantities) {
  auto sell1 = make_order(1, OrderSide::Sell, 100, 4);
  auto sell2 = make_order(2, OrderSide::Sell, 100, 6);
  auto buy = make_order(3, OrderSide::Buy, 100, 7);

  engine.add_order(std::move(sell1));
  engine.add_order(std::move(sell2));
  engine.add_order(std::move(buy));

  const auto *history = engine.execution_history();

  ASSERT_EQ(history->size(), 2);

  EXPECT_EQ((*history)[0].execution_quantity, 4);
  EXPECT_EQ((*history)[1].execution_quantity, 3);

  EXPECT_EQ(*engine.traded_quantity(), 7);
}
} // namespace
} // namespace od
