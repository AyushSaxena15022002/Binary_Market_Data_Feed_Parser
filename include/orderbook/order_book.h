#pragma once

#include "common/types.h"
#include "common/memory_pool.h"
#include "orderbook/order.h"
#include "orderbook/price_level.h"
#include <map>
#include <unordered_map>
#include <memory>

namespace itch {

class OrderBook {
public:
    explicit OrderBook(const Stock& symbol);
    ~OrderBook();

    void add_order(OrderRef ref, Side side, Shares shares, Price price);
    void cancel_order(OrderRef ref, Shares cancelled_shares);
    void execute_order(OrderRef ref, Shares executed_shares);
    void delete_order(OrderRef ref);
    void replace_order(OrderRef old_ref, OrderRef new_ref, Shares new_shares, Price new_price);

    Price best_bid() const;
    Price best_ask() const;
    Shares bid_volume_at_best() const;
    Shares ask_volume_at_best() const;
    double spread() const;

    size_t total_orders() const { return orders_.size(); }
    const Stock& symbol() const { return symbol_; }

private:
    Stock symbol_;
    std::map<Price, PriceLevel, std::greater<Price>> bids_;
    std::map<Price, PriceLevel, std::less<Price>> asks_;
    std::unordered_map<OrderRef, Order*> orders_;
    MemoryPool<Order, 4096> order_pool_;

    void remove_order_from_book(Order* order);
};

}
