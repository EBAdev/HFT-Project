#include <gtest/gtest.h>

#include "Order.hpp"
#include "OrderBook.hpp"

#include <cstdint>
#include <memory>

namespace od {

class OrderBookTest : public ::testing::Test {
protected:
  static constexpr std::uint32_t instrument_id = 1;

  OrderBook book{instrument_id};

  std::unique_ptr<Order> make_order(std::uint64_t id, OrderSide side,
                                    std::int64_t price,
                                    std::uint32_t quantity = 10) {
    auto order =
        std::make_unique<Order>(id, instrument_id, price, quantity, side);
    order->activate();
    return order;
  }
};

TEST_F(OrderBookTest, InitiallyEmpty) {
  EXPECT_FALSE(book.best_bid().has_value());
  EXPECT_FALSE(book.best_ask().has_value());
}

TEST_F(OrderBookTest, AddBuyOrderUpdatesBestBid) {
  auto order = make_order(1, OrderSide::Buy, 100);

  EXPECT_TRUE(book.add_order(order));
  ASSERT_TRUE(book.best_bid().has_value());
  EXPECT_EQ(*book.best_bid(), 100);
}

TEST_F(OrderBookTest, AddSellOrderUpdatesBestAsk) {
  auto order = make_order(1, OrderSide::Sell, 105);

  EXPECT_TRUE(book.add_order(order));
  ASSERT_TRUE(book.best_ask().has_value());
  EXPECT_EQ(*book.best_ask(), 105);
}

TEST_F(OrderBookTest, BestBidIsHighestBidPrice) {
  auto order1 = make_order(1, OrderSide::Buy, 100);
  auto order2 = make_order(2, OrderSide::Buy, 105);
  auto order3 = make_order(3, OrderSide::Buy, 102);

  ASSERT_TRUE(book.add_order(order1));
  ASSERT_TRUE(book.add_order(order2));
  ASSERT_TRUE(book.add_order(order3));

  ASSERT_TRUE(book.best_bid().has_value());
  EXPECT_EQ(*book.best_bid(), 105);
}

TEST_F(OrderBookTest, BestAskIsLowestAskPrice) {
  auto order1 = make_order(1, OrderSide::Sell, 110);
  auto order2 = make_order(2, OrderSide::Sell, 105);
  auto order3 = make_order(3, OrderSide::Sell, 108);

  ASSERT_TRUE(book.add_order(order1));
  ASSERT_TRUE(book.add_order(order2));
  ASSERT_TRUE(book.add_order(order3));

  ASSERT_TRUE(book.best_ask().has_value());
  EXPECT_EQ(*book.best_ask(), 105);
}

TEST_F(OrderBookTest, BuyAndSellSidesAreIndependent) {
  auto buy = make_order(1, OrderSide::Buy, 100);
  auto sell = make_order(2, OrderSide::Sell, 110);

  ASSERT_TRUE(book.add_order(buy));
  ASSERT_TRUE(book.add_order(sell));

  ASSERT_TRUE(book.best_bid().has_value());
  ASSERT_TRUE(book.best_ask().has_value());

  EXPECT_EQ(*book.best_bid(), 100);
  EXPECT_EQ(*book.best_ask(), 110);
}

TEST_F(OrderBookTest, RejectDuplicateOrderId) {
  auto first = make_order(1, OrderSide::Buy, 100);
  auto duplicate = make_order(1, OrderSide::Buy, 105);

  ASSERT_TRUE(book.add_order(first));
  EXPECT_FALSE(book.add_order(duplicate));

  ASSERT_TRUE(book.best_bid().has_value());
  EXPECT_EQ(*book.best_bid(), 100);

  // With add_order(std::unique_ptr<Order>&), failed insertion
  // should leave ownership with the caller.
  EXPECT_NE(duplicate, nullptr);
}

TEST_F(OrderBookTest, RejectOrderForDifferentInstrument) {
  auto order =
      std::make_unique<Order>(1, instrument_id + 1, 100, 10, OrderSide::Buy);
  order->activate();

  EXPECT_THROW(book.add_order(order), std::invalid_argument);
  EXPECT_NE(order, nullptr);
  EXPECT_FALSE(book.best_bid().has_value());
}

TEST_F(OrderBookTest, RejectNewOrderThatHasNotBeenActivated) {
  auto order =
      std::make_unique<Order>(1, instrument_id, 100, 10, OrderSide::Buy);

  EXPECT_FALSE(book.add_order(order));
  EXPECT_NE(order, nullptr);
  EXPECT_FALSE(book.best_bid().has_value());
}

TEST_F(OrderBookTest, CancelOrderRemovesBestBid) {
  auto order = make_order(1, OrderSide::Buy, 100);

  ASSERT_TRUE(book.add_order(order));
  ASSERT_TRUE(book.best_bid().has_value());

  EXPECT_TRUE(book.cancel_order(1));
  EXPECT_FALSE(book.best_bid().has_value());
}

TEST_F(OrderBookTest, CancelBestBidRevealsNextPriceLevel) {
  auto order1 = make_order(1, OrderSide::Buy, 100);
  auto order2 = make_order(2, OrderSide::Buy, 105);

  ASSERT_TRUE(book.add_order(order1));
  ASSERT_TRUE(book.add_order(order2));

  ASSERT_TRUE(book.best_bid().has_value());
  EXPECT_EQ(*book.best_bid(), 105);

  EXPECT_TRUE(book.cancel_order(2));

  ASSERT_TRUE(book.best_bid().has_value());
  EXPECT_EQ(*book.best_bid(), 100);
}

TEST_F(OrderBookTest, CancelUnknownOrderReturnsFalse) {
  EXPECT_FALSE(book.cancel_order(999));
}

TEST_F(OrderBookTest, CannotCancelSameOrderTwice) {
  auto order = make_order(1, OrderSide::Buy, 100);

  ASSERT_TRUE(book.add_order(order));

  EXPECT_TRUE(book.cancel_order(1));
  EXPECT_FALSE(book.cancel_order(1));
}

TEST_F(OrderBookTest, CancellingOneOrderPreservesOtherOrdersAtSamePrice) {
  auto order1 = make_order(1, OrderSide::Buy, 100);
  auto order2 = make_order(2, OrderSide::Buy, 100);

  ASSERT_TRUE(book.add_order(order1));
  ASSERT_TRUE(book.add_order(order2));

  EXPECT_TRUE(book.cancel_order(1));

  // The remaining order at price 100 should still be present.
  ASSERT_TRUE(book.best_bid().has_value());
  EXPECT_EQ(*book.best_bid(), 100);
}

TEST_F(OrderBookTest, RejectCancelledOrder) {
  auto order = make_order(1, OrderSide::Buy, 100);
  order->cancel();

  EXPECT_FALSE(book.add_order(order));
  EXPECT_NE(order, nullptr);
  EXPECT_FALSE(book.best_bid().has_value());
}
} // namespace od
