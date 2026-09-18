#pragma once

#include "common/types.h"
#include <list>

namespace itch {

struct Order {
    OrderRef ref;
    Side side;
    Shares shares;
    Price price;
    Stock stock;
    std::list<Order*>::iterator level_iter;

    Order() : ref(0), side(Side::Buy), shares(0), price(0), stock{} {}

    Order(OrderRef r, Side s, Shares sh, Price p, const Stock& st)
        : ref(r), side(s), shares(sh), price(p), stock(st) {}
};

}
