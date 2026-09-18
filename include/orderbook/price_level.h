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

    std::list<Order*>::iterator add_order(Order* order) {
        orders.push_back(order);
        total_shares += order->shares;
        auto it = orders.end();
        --it;
        return it;
    }

    void remove_order(std::list<Order*>::iterator it) {
        total_shares -= (*it)->shares;
        orders.erase(it);
    }

    void reduce_shares(Shares amount) {
        total_shares -= amount;
    }
};

}
