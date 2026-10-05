#include <gtest/gtest.h>
#include "core/types.hpp"

using namespace quanta;

TEST(Types, RupeesConvertToExactTicks) {
    EXPECT_EQ(rupees_to_ticks(100.10), 10010);
    EXPECT_EQ(rupees_to_ticks(100.05), 10005);
    EXPECT_EQ(rupees_to_ticks(0.01), 1);
}

TEST(Types, DoubleArithmeticIsInexactButTicksAreExact) {
    volatile double a = 0.1, b = 0.2, c = 0.3;
    EXPECT_NE(a + b, c);  // doubles fail this
    EXPECT_EQ(rupees_to_ticks(0.1) + rupees_to_ticks(0.2),
              rupees_to_ticks(0.3));  // ticks pass
}

TEST(Types, TicksRoundTripBackToRupees) {
    EXPECT_DOUBLE_EQ(ticks_to_rupees(10010), 100.10);
}

TEST(Types, OppositeSide) {
    EXPECT_EQ(opposite(Side::Buy), Side::Sell);
    EXPECT_EQ(opposite(Side::Sell), Side::Buy);
}

TEST(Types, SideToString) {
    EXPECT_STREQ(to_string(Side::Buy), "BUY");
    EXPECT_STREQ(to_string(Side::Sell), "SELL");
}