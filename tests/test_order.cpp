#include "Order.hpp"

#include <gtest/gtest.h>
#include <stdexcept>

namespace od {

class OrderTest : public ::testing::Test {
protected:
  Order order{1, 1, 12345, 100, OrderSide::Buy};
};

// Construction

TEST_F(OrderTest, InitializesFieldsCorrectly) {
  EXPECT_EQ(order.id(), 1);
  EXPECT_EQ(order.instrument_id(), 1);
  EXPECT_EQ(order.tick_price(), 12345);
  EXPECT_EQ(order.quantity(), 100);
  EXPECT_EQ(order.side(), OrderSide::Buy);
  EXPECT_EQ(order.state(), OrderState::New);
  EXPECT_EQ(order.remaining_quantity(), 100);
}

TEST(OrderConstructionTest, RejectsZeroPrice) {
  EXPECT_THROW(Order(1, 1, 0, 100, OrderSide::Buy), std::invalid_argument);
}

TEST(OrderConstructionTest, RejectsNegativePrice) {
  EXPECT_THROW(Order(1, 1, -100, 100, OrderSide::Buy), std::invalid_argument);
}

TEST(OrderConstructionTest, RejectsZeroQuantity) {
  EXPECT_THROW(Order(1, 1, 12345, 0, OrderSide::Buy), std::invalid_argument);
}

// Activation and rejection

TEST_F(OrderTest, ActivatesNewOrder) {
  EXPECT_TRUE(order.activate());
  EXPECT_EQ(order.state(), OrderState::Active);
  EXPECT_TRUE(order.is_active());
}

TEST_F(OrderTest, CannotActivateOrderTwice) {
  ASSERT_TRUE(order.activate());

  EXPECT_FALSE(order.activate());
  EXPECT_EQ(order.state(), OrderState::Active);
}

TEST_F(OrderTest, CanRejectNewOrder) {
  EXPECT_TRUE(order.reject());
  EXPECT_EQ(order.state(), OrderState::Rejected);
}

TEST_F(OrderTest, CannotRejectActiveOrder) {
  ASSERT_TRUE(order.activate());

  EXPECT_FALSE(order.reject());
  EXPECT_EQ(order.state(), OrderState::Active);
}

// Fills

TEST_F(OrderTest, PartiallyFillsActiveOrder) {
  ASSERT_TRUE(order.activate());

  EXPECT_TRUE(order.fill(30));
  EXPECT_EQ(order.state(), OrderState::PartiallyFilled);
  EXPECT_EQ(order.remaining_quantity(), 70);
  EXPECT_TRUE(order.is_active());
}

TEST_F(OrderTest, FullyFillsActiveOrder) {
  ASSERT_TRUE(order.activate());

  EXPECT_TRUE(order.fill(100));
  EXPECT_EQ(order.state(), OrderState::Filled);
  EXPECT_EQ(order.remaining_quantity(), 0);
  EXPECT_FALSE(order.is_active());
}

TEST_F(OrderTest, CanFillPartiallyFilledOrderAgain) {
  ASSERT_TRUE(order.activate());
  ASSERT_TRUE(order.fill(30));

  EXPECT_TRUE(order.fill(20));
  EXPECT_EQ(order.state(), OrderState::PartiallyFilled);
  EXPECT_EQ(order.remaining_quantity(), 50);
}

TEST_F(OrderTest, FinalFillCompletesPartiallyFilledOrder) {
  ASSERT_TRUE(order.activate());
  ASSERT_TRUE(order.fill(30));

  EXPECT_TRUE(order.fill(70));
  EXPECT_EQ(order.state(), OrderState::Filled);
  EXPECT_EQ(order.remaining_quantity(), 0);
}

TEST_F(OrderTest, RejectsZeroFill) {
  ASSERT_TRUE(order.activate());

  EXPECT_FALSE(order.fill(0));
  EXPECT_EQ(order.state(), OrderState::Active);
  EXPECT_EQ(order.remaining_quantity(), 100);
}

TEST_F(OrderTest, RejectsFillExceedingRemainingQuantity) {
  ASSERT_TRUE(order.activate());
  ASSERT_TRUE(order.fill(60));

  EXPECT_FALSE(order.fill(41));
  EXPECT_EQ(order.state(), OrderState::PartiallyFilled);
  EXPECT_EQ(order.remaining_quantity(), 40);
}

TEST_F(OrderTest, CannotFillNewOrder) {
  EXPECT_FALSE(order.fill(10));
  EXPECT_EQ(order.state(), OrderState::New);
  EXPECT_EQ(order.remaining_quantity(), 100);
}

TEST_F(OrderTest, CannotFillFilledOrder) {
  ASSERT_TRUE(order.activate());
  ASSERT_TRUE(order.fill(100));

  EXPECT_FALSE(order.fill(1));
  EXPECT_EQ(order.state(), OrderState::Filled);
  EXPECT_EQ(order.remaining_quantity(), 0);
}

// Cancellation

TEST_F(OrderTest, CanCancelActiveOrder) {
  ASSERT_TRUE(order.activate());

  EXPECT_TRUE(order.cancel());
  EXPECT_EQ(order.state(), OrderState::Cancelled);
  EXPECT_EQ(order.remaining_quantity(), 100);
}

TEST_F(OrderTest, CanCancelPartiallyFilledOrder) {
  ASSERT_TRUE(order.activate());
  ASSERT_TRUE(order.fill(30));

  EXPECT_TRUE(order.cancel());
  EXPECT_EQ(order.state(), OrderState::Cancelled);
  EXPECT_EQ(order.remaining_quantity(), 70);
}

TEST_F(OrderTest, CannotCancelNewOrder) {
  EXPECT_FALSE(order.cancel());
  EXPECT_EQ(order.state(), OrderState::New);
}

TEST_F(OrderTest, CannotCancelFilledOrder) {
  ASSERT_TRUE(order.activate());
  ASSERT_TRUE(order.fill(100));

  EXPECT_FALSE(order.cancel());
  EXPECT_EQ(order.state(), OrderState::Filled);
}

TEST_F(OrderTest, CannotCancelRejectedOrder) {
  ASSERT_TRUE(order.reject());

  EXPECT_FALSE(order.cancel());
  EXPECT_EQ(order.state(), OrderState::Rejected);
}

TEST_F(OrderTest, CannotCancelAlreadyCancelledOrder) {
  ASSERT_TRUE(order.activate());
  ASSERT_TRUE(order.cancel());

  EXPECT_FALSE(order.cancel());
  EXPECT_EQ(order.state(), OrderState::Cancelled);
}

// State queries

TEST_F(OrderTest, IsActiveOnlyForLiveOrders) {
  EXPECT_FALSE(order.is_active());

  ASSERT_TRUE(order.activate());
  EXPECT_TRUE(order.is_active());

  ASSERT_TRUE(order.fill(10));
  EXPECT_TRUE(order.is_active());

  ASSERT_TRUE(order.cancel());
  EXPECT_FALSE(order.is_active());
}

} // namespace od
