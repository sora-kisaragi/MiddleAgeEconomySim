#pragma once

#include "core/economy_types.h"

namespace economy {

// A committed trade record uses run-scoped identifiers and simulation time.
// Wall-clock time is deliberately excluded so equal runs produce equal records.
class Transaction {
public:
    Transaction(EventId event_id, ActorId seller_id, ActorId buyer_id,
                ProductId product_id, Quantity quantity, Money unit_price,
                GameTime committed_at)
        : event_id_(event_id), seller_id_(seller_id), buyer_id_(buyer_id),
          product_id_(product_id), quantity_(quantity), unit_price_(unit_price),
          committed_day_(committed_at.day), committed_phase_(committed_at.phase),
          committed_sequence_in_day_(committed_at.sequence_in_day) {
        if (seller_id_ == buyer_id_) {
            throw std::invalid_argument("seller and buyer must differ");
        }
        if (quantity_.value() == 0) {
            throw std::invalid_argument("transaction quantity must be positive");
        }
    }

    EventId eventId() const noexcept { return event_id_; }
    ActorId sellerId() const noexcept { return seller_id_; }
    ActorId buyerId() const noexcept { return buyer_id_; }
    ProductId productId() const noexcept { return product_id_; }
    Quantity quantity() const noexcept { return quantity_; }
    Money unitPrice() const noexcept { return unit_price_; }
    GameTime committedAt() const noexcept {
        return GameTime(committed_day_, committed_phase_, committed_sequence_in_day_);
    }

    Money totalValue() const { return unit_price_.multiply(quantity_.value()); }

private:
    EventId event_id_;
    ActorId seller_id_;
    ActorId buyer_id_;
    ProductId product_id_;
    Quantity quantity_;
    Money unit_price_;
    std::uint64_t committed_day_;
    std::uint8_t committed_phase_;
    std::uint64_t committed_sequence_in_day_;
};

} // namespace economy
