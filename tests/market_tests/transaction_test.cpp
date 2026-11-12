#include "market/transaction.h"

#include <gtest/gtest.h>

#include <limits>
#include <type_traits>
#include <vector>

using namespace economy;

TEST(TransactionTest, StoresTypedReferencesAndSimulationTime) {
    const Transaction transaction(EventId(1), ActorId(2), ActorId(3), ProductId(4),
                                  Quantity(5), Money(100), GameTime(8, 7, 6));

    EXPECT_EQ(transaction.eventId().value(), 1U);
    EXPECT_EQ(transaction.sellerId().value(), 2U);
    EXPECT_EQ(transaction.buyerId().value(), 3U);
    EXPECT_EQ(transaction.productId().value(), 4U);
    EXPECT_EQ(transaction.quantity().value(), 5);
    EXPECT_EQ(transaction.unitPrice().value(), 100);
    EXPECT_EQ(transaction.committedAt(), GameTime(8, 7, 6));
    EXPECT_EQ(transaction.totalValue().value(), 500);
}

TEST(TransactionTest, HasNoAbsentOrPartiallyInitializedRecord) {
    static_assert(!std::is_default_constructible_v<Transaction>);
    SUCCEED();
}

TEST(TransactionTest, SupportsHistoryCompactionAsAValueRecord) {
    std::vector<Transaction> history;
    history.emplace_back(EventId(1), ActorId(2), ActorId(3), ProductId(4),
                         Quantity(5), Money(100), GameTime(8, 7, 6));
    history.emplace_back(EventId(2), ActorId(3), ActorId(4), ProductId(5),
                         Quantity(6), Money(200), GameTime(8, 7, 7));

    history.erase(history.begin());

    ASSERT_EQ(history.size(), 1U);
    EXPECT_EQ(history.front().eventId(), EventId(2));
    EXPECT_EQ(history.front().committedAt(), GameTime(8, 7, 7));
}

TEST(TransactionTest, RejectsSelfTradeAndZeroQuantity) {
    EXPECT_THROW((Transaction(EventId(1), ActorId(2), ActorId(2), ProductId(4),
                              Quantity(1), Money(100), GameTime(8, 7, 6))),
                 std::invalid_argument);
    EXPECT_THROW((Transaction(EventId(1), ActorId(2), ActorId(3), ProductId(4),
                              Quantity(0), Money(100), GameTime(8, 7, 6))),
                 std::invalid_argument);
}

TEST(TransactionTest, AllowsZeroPriceAndRejectsTotalOverflow) {
    const Transaction free_trade(EventId(1), ActorId(2), ActorId(3), ProductId(4),
                                 Quantity(5), Money(0), GameTime(8, 7, 6));
    EXPECT_EQ(free_trade.totalValue().value(), 0);

    const Transaction overflowing(
        EventId(2), ActorId(2), ActorId(3), ProductId(4), Quantity(2),
        Money(std::numeric_limits<std::int64_t>::max()), GameTime(8, 7, 7));
    EXPECT_THROW(overflowing.totalValue(), std::overflow_error);
}

TEST(TransactionTest, EqualInputsProduceEqualPersistentFields) {
    const auto make_transaction = [] {
        return Transaction(EventId(9), ActorId(2), ActorId(3), ProductId(4),
                           Quantity(5), Money(100), GameTime(8, 7, 6));
    };
    const auto first = make_transaction();
    const auto second = make_transaction();

    EXPECT_EQ(first.eventId(), second.eventId());
    EXPECT_EQ(first.sellerId(), second.sellerId());
    EXPECT_EQ(first.buyerId(), second.buyerId());
    EXPECT_EQ(first.productId(), second.productId());
    EXPECT_EQ(first.quantity(), second.quantity());
    EXPECT_EQ(first.unitPrice(), second.unitPrice());
    EXPECT_EQ(first.committedAt(), second.committedAt());
}
