#include "orderbook/order_book.h"
#include <algorithm>
#include <cmath>

namespace itch {

OrderBook::OrderBook(const Stock& symbol) : symbol_(symbol) {}

OrderBook::~OrderBook() {
    for (auto& pair : orders_) {
        delete pair.second;
    }
}

void OrderBook::add_order(OrderRef ref, Side side, Shares shares, Price price) {
    Order* order = new Order(ref, side, shares, price, symbol_);
    orders_[ref] = order;

    if (side == Side::Buy) {
        bids_[price].add_order(order);
    } else {
        asks_[price].add_order(order);
    }
}

void OrderBook::cancel_order(OrderRef ref, Shares cancelled_shares) {
    auto it = orders_.find(ref);
    if (it == orders_.end()) return;

    Order* order = it->second;
    order->shares -= cancelled_shares;

    if (order->side == Side::Buy) {
        bids_[order->price].reduce_shares(cancelled_shares);
    } else {
        asks_[order->price].reduce_shares(cancelled_shares);
    }

    if (order->shares == 0) {
        remove_order_from_book(order);
        orders_.erase(it);
        delete order;
    }
}

void OrderBook::execute_order(OrderRef ref, Shares executed_shares) {
    cancel_order(ref, executed_shares);
}

void OrderBook::delete_order(OrderRef ref) {
    auto it = orders_.find(ref);
    if (it == orders_.end()) return;

    Order* order = it->second;
    remove_order_from_book(order);
    orders_.erase(it);
    delete order;
}

void OrderBook::replace_order(OrderRef old_ref, OrderRef new_ref, Shares new_shares, Price new_price) {
    auto it = orders_.find(old_ref);
    if (it == orders_.end()) return;

    Order* old_order = it->second;
    Side side = old_order->side;

    delete_order(old_ref);
    add_order(new_ref, side, new_shares, new_price);
}

void OrderBook::remove_order_from_book(Order* order) {
    if (order->side == Side::Buy) {
        auto& level = bids_[order->price];
        level.remove_order(order);
        if (level.orders.empty()) {
            bids_.erase(order->price);
        }
    } else {
        auto& level = asks_[order->price];
        level.remove_order(order);
        if (level.orders.empty()) {
            asks_.erase(order->price);
        }
    }
}

Price OrderBook::best_bid() const {
    return bids_.empty() ? 0 : bids_.begin()->first;
}

Price OrderBook::best_ask() const {
    return asks_.empty() ? 0 : asks_.begin()->first;
}

Shares OrderBook::bid_volume_at_best() const {
    return bids_.empty() ? 0 : bids_.begin()->second.total_shares;
}

Shares OrderBook::ask_volume_at_best() const {
    return asks_.empty() ? 0 : asks_.begin()->second.total_shares;
}

double OrderBook::spread() const {
    if (bids_.empty() || asks_.empty()) return 0.0;
    int64_t diff = static_cast<int64_t>(best_ask()) - static_cast<int64_t>(best_bid());
    return diff / 10000.0;
}

}
