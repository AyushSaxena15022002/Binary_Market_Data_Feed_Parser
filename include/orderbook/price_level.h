#pragma once

#include "common/types.h"
#include "orderbook/order.h"
#include <list>

namespace itch {

struct PriceLevel {
    Price price;
    Shares total_shares;
    std::list<Order*> orders;

    PriceLevel() : price(0), total_shares(0) {}
    explicit PriceLevel(Price p) : price(p), total_shares(0) {}

    void add_order(Order* order) {
        orders.push_back(order);
        total_shares += order->shares;
    }

    void remove_order(Order* order) {
        orders.remove(order);
        total_shares -= order->shares;
    }

    void reduce_shares(Shares amount) {
        total_shares -= amount;
    }
};

}
