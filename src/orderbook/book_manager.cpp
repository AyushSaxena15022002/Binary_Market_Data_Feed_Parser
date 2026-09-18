#include "orderbook/book_manager.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <algorithm>

namespace itch {

BookManager::~BookManager() {
    for (auto& pair : books_) {
        delete pair.second;
    }
}

OrderBook* BookManager::get_or_create_book(const Stock& symbol) {
    auto it = books_.find(symbol);
    if (it != books_.end()) {
        return it->second;
    }

    OrderBook* book = new OrderBook(symbol);
    books_[symbol] = book;

    if (books_.size() == 1) {
        books_.reserve(8);
    }

    return book;
}

void BookManager::on_add_order(OrderRef ref, Side side, Shares shares, Price price, const Stock& stock) {
    OrderBook* book = get_or_create_book(stock);
    book->add_order(ref, side, shares, price);
    order_to_book_[ref] = book;
}

void BookManager::on_cancel_order(OrderRef ref, Shares shares) {
    auto it = order_to_book_.find(ref);
    if (it == order_to_book_.end()) return;

    OrderBook* book = it->second;
    book->cancel_order(ref, shares);
    if (book->total_orders() == 0 || it->first == ref) {
        // Book handles removal
    }
}

void BookManager::on_execute_order(OrderRef ref, Shares shares) {
    auto it = order_to_book_.find(ref);
    if (it == order_to_book_.end()) return;

    OrderBook* book = it->second;
    book->execute_order(ref, shares);
}

void BookManager::on_delete_order(OrderRef ref) {
    auto it = order_to_book_.find(ref);
    if (it == order_to_book_.end()) return;

    OrderBook* book = it->second;
    book->delete_order(ref);
    order_to_book_.erase(it);
}

void BookManager::on_replace_order(OrderRef old_ref, OrderRef new_ref, Shares new_shares, Price new_price) {
    auto it = order_to_book_.find(old_ref);
    if (it == order_to_book_.end()) return;

    OrderBook* book = it->second;
    book->replace_order(old_ref, new_ref, new_shares, new_price);
    order_to_book_.erase(it);
    order_to_book_[new_ref] = book;
}

void BookManager::print_summary() const {
    if (books_.empty()) {
        std::cout << "No order books created." << std::endl;
        return;
    }

    std::cout << "\n============================== ORDER BOOK SNAPSHOT ==============================" << std::endl;
    std::cout << std::left << std::setw(10) << "Symbol"
              << std::right << std::setw(10) << "Orders"
              << std::setw(16) << "Best Bid"
              << std::setw(16) << "Best Ask"
              << std::setw(14) << "Spread ($)"
              << std::setw(16) << "Mid Price ($)" << std::endl;
    std::cout << std::string(82, '-') << std::endl;

    std::vector<const OrderBook*> sorted_books;
    for (const auto& pair : books_) {
        sorted_books.push_back(pair.second);
    }

    std::sort(sorted_books.begin(), sorted_books.end(),
              [](const OrderBook* a, const OrderBook* b) {
                  return a->total_orders() > b->total_orders();
              });

    for (const auto* book : sorted_books) {
        std::cout << std::left << std::setw(10);
        std::cout.write(book->symbol().symbol, 8);
        std::cout << std::right << std::setw(10) << book->total_orders();

        if (book->best_bid() > 0) {
            std::cout << std::setw(16) << std::fixed << std::setprecision(4)
                      << (book->best_bid() / 10000.0);
        } else {
            std::cout << std::setw(16) << "N/A";
        }

        if (book->best_ask() > 0) {
            std::cout << std::setw(16) << std::fixed << std::setprecision(4)
                      << (book->best_ask() / 10000.0);
        } else {
            std::cout << std::setw(16) << "N/A";
        }

        if (book->best_bid() > 0 && book->best_ask() > 0) {
            std::cout << std::setw(14) << std::fixed << std::setprecision(4)
                      << book->spread();
            double mid = ((book->best_bid() + book->best_ask()) / 2.0) / 10000.0;
            std::cout << std::setw(16) << std::fixed << std::setprecision(4)
                      << mid;
        } else {
            std::cout << std::setw(14) << "N/A";
            std::cout << std::setw(16) << "N/A";
        }

        std::cout << std::endl;
    }
    std::cout << std::string(82, '=') << std::endl;
}

}
