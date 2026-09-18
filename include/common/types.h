#pragma once

#include <cstdint>

namespace itch {

using Price = uint32_t;
using Shares = uint32_t;
using OrderRef = uint64_t;
using Timestamp = uint64_t;
using StockLocate = uint16_t;
using TrackingNumber = uint16_t;

enum class Side : char {
    Buy = 'B',
    Sell = 'S'
};

struct Stock {
    char symbol[8];

    bool operator==(const Stock& other) const {
        for (int i = 0; i < 8; ++i) {
            if (symbol[i] != other.symbol[i]) return false;
        }
        return true;
    }
};

}
