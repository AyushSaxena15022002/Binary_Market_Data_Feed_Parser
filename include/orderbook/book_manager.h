#pragma once

#include "common/types.h"
#include "orderbook/order_book.h"
#include <unordered_map>
#include <string>

namespace itch {

struct StockHash {
    size_t operator()(const Stock& s) const {
        size_t h = 0;
        for (int i = 0; i < 8; ++i) {
            h = h * 31 + s.symbol[i];
        }
        return h;
    }
};

struct StockEqual {
    bool operator()(const Stock& a, const Stock& b) const {
        return a == b;
    }
};

class BookManager {
public:
    BookManager() = default;
    ~BookManager();

    OrderBook* get_or_create_book(const Stock& symbol);

    void on_add_order(OrderRef ref, Side side, Shares shares, Price price, const Stock& stock);
    void on_cancel_order(OrderRef ref, Shares shares);
    void on_execute_order(OrderRef ref, Shares shares);
    void on_delete_order(OrderRef ref);
    void on_replace_order(OrderRef old_ref, OrderRef new_ref, Shares new_shares, Price new_price);

    void print_summary() const;
    size_t book_count() const { return books_.size(); }
    size_t total_orders_tracked() const { return order_to_book_.size(); }

private:
    std::unordered_map<Stock, OrderBook*, StockHash, StockEqual> books_;
    std::unordered_map<OrderRef, OrderBook*> order_to_book_;
};

}
