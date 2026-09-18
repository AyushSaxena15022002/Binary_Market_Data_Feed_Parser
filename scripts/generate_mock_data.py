#!/usr/bin/env python3
"""
NASDAQ ITCH 5.0 Synthetic Market Data Generator

Generates bit-for-bit spec-compliant binary ITCH 5.0 sample files
for testing and benchmarking the C++ parser.

ITCH 5.0 wire format reminder:
  - Every message is preceded by a 2-byte big-endian LENGTH (of the message body, excluding itself).
  - Timestamps are 6 bytes big-endian (nanoseconds since midnight).
  - All integer fields are big-endian.
  - Stock symbols are 8 bytes, right-padded with spaces.

Usage:
  python generate_mock_data.py                          # 100k messages -> data/sample_100k.itch
  python generate_mock_data.py -n 1000000 -o data/big.itch  # 1M messages
"""

import struct
import random
import argparse
import os


# ── helpers ──────────────────────────────────────────────────────────────────

SYMBOLS = [b"AAPL    ", b"MSFT    ", b"GOOGL   ", b"AMZN    ",
           b"TSLA    ", b"NVDA    ", b"META    "]

BASE_PRICES = {                       # price in 1/10000 dollars
    b"AAPL    ": 1_500_000,           # $150.00
    b"MSFT    ": 3_100_000,           # $310.00
    b"GOOGL   ": 2_800_000,           # $280.00
    b"AMZN    ": 3_300_000,           # $330.00
    b"TSLA    ": 7_000_000,           # $700.00
    b"NVDA    ": 2_200_000,           # $220.00
    b"META    ": 3_400_000,           # $340.00
}


def pack_timestamp(ns: int) -> bytes:
    """Pack a 6-byte big-endian timestamp (nanoseconds since midnight)."""
    return ns.to_bytes(6, byteorder="big")


def write_msg(f, body: bytes):
    """Write a length-prefixed ITCH message (2-byte BE length + body)."""
    f.write(struct.pack(">H", len(body)))
    f.write(body)


# ── message builders ─────────────────────────────────────────────────────────

def build_system_event(ts_ns: int, event_code: bytes) -> bytes:
    """Type 'S' – System Event (12 bytes body)."""
    return struct.pack(">c2xHH", b'S', 0, 0) [:1] + \
           b'\x00\x00' + \
           b'\x00\x00' + \
           pack_timestamp(ts_ns) + \
           event_code
    # Simpler: just manually assemble
    # body = type(1) + stock_locate(2) + tracking(2) + timestamp(6) + event_code(1) = 12
    body  = b'S'
    body += struct.pack(">H", 0)           # stock_locate
    body += struct.pack(">H", 0)           # tracking_number
    body += pack_timestamp(ts_ns)          # timestamp (6 bytes)
    body += event_code                     # event_code (1 byte)
    return body                            # 12 bytes total


def build_add_order(ts_ns, locate, ref, side, shares, symbol, price) -> bytes:
    """
    Type 'A' – Add Order (no MPID attribution) – 36 bytes body.
    Fields: type(1) + locate(2) + tracking(2) + ts(6) + ref(8) + side(1) + shares(4) + stock(8) + price(4)
    """
    body  = b'A'
    body += struct.pack(">H", locate)
    body += struct.pack(">H", 0)           # tracking
    body += pack_timestamp(ts_ns)
    body += struct.pack(">Q", ref)
    body += side                           # b'B' or b'S'
    body += struct.pack(">I", shares)
    body += symbol                         # 8 bytes
    body += struct.pack(">I", price)
    return body                            # 36 bytes


def build_cancel(ts_ns, locate, ref, cancelled_shares) -> bytes:
    """
    Type 'X' – Order Cancel – 23 bytes body.
    Fields: type(1) + locate(2) + tracking(2) + ts(6) + ref(8) + cancelled_shares(4)
    """
    body  = b'X'
    body += struct.pack(">H", locate)
    body += struct.pack(">H", 0)
    body += pack_timestamp(ts_ns)
    body += struct.pack(">Q", ref)
    body += struct.pack(">I", cancelled_shares)
    return body                            # 23 bytes


def build_executed(ts_ns, locate, ref, exec_shares, match_number) -> bytes:
    """
    Type 'E' – Order Executed – 31 bytes body.
    Fields: type(1) + locate(2) + tracking(2) + ts(6) + ref(8) + exec_shares(4) + match_number(8)
    """
    body  = b'E'
    body += struct.pack(">H", locate)
    body += struct.pack(">H", 0)
    body += pack_timestamp(ts_ns)
    body += struct.pack(">Q", ref)
    body += struct.pack(">I", exec_shares)
    body += struct.pack(">Q", match_number)
    return body                            # 31 bytes


def build_delete(ts_ns, locate, ref) -> bytes:
    """
    Type 'D' – Order Delete – 19 bytes body.
    Fields: type(1) + locate(2) + tracking(2) + ts(6) + ref(8)
    """
    body  = b'D'
    body += struct.pack(">H", locate)
    body += struct.pack(">H", 0)
    body += pack_timestamp(ts_ns)
    body += struct.pack(">Q", ref)
    return body                            # 19 bytes


def build_replace(ts_ns, locate, old_ref, new_ref, new_shares, new_price) -> bytes:
    """
    Type 'U' – Order Replace – 35 bytes body.
    Fields: type(1) + locate(2) + tracking(2) + ts(6) + orig_ref(8) + new_ref(8) + shares(4) + price(4)
    """
    body  = b'U'
    body += struct.pack(">H", locate)
    body += struct.pack(">H", 0)
    body += pack_timestamp(ts_ns)
    body += struct.pack(">Q", old_ref)
    body += struct.pack(">Q", new_ref)
    body += struct.pack(">I", new_shares)
    body += struct.pack(">I", new_price)
    return body                            # 35 bytes


def build_trade(ts_ns, locate, ref, side, shares, symbol, price, match_number) -> bytes:
    """
    Type 'P' – Trade (Non-Cross) – 44 bytes body.
    Fields: type(1) + locate(2) + tracking(2) + ts(6) + ref(8) + side(1) + shares(4) + stock(8) + price(4) + match(8)
    """
    body  = b'P'
    body += struct.pack(">H", locate)
    body += struct.pack(">H", 0)
    body += pack_timestamp(ts_ns)
    body += struct.pack(">Q", ref)
    body += side
    body += struct.pack(">I", shares)
    body += symbol
    body += struct.pack(">I", price)
    body += struct.pack(">Q", match_number)
    return body                            # 44 bytes


# ── generator ────────────────────────────────────────────────────────────────

def generate_itch_file(output_path: str, num_messages: int):
    print(f"[*] Generating {num_messages:,} ITCH 5.0 messages -> '{output_path}'")

    live_orders = {}          # ref -> (symbol, side, price, shares, locate)
    next_ref    = 1_000_000
    next_match  = 1
    ts_ns       = 9 * 3600 * 1_000_000_000   # 09:00:00.000000000

    os.makedirs(os.path.dirname(os.path.abspath(output_path)), exist_ok=True)

    stats = {"A": 0, "X": 0, "E": 0, "D": 0, "U": 0, "P": 0}

    with open(output_path, "wb") as f:
        # System Event: Start of Messages
        write_msg(f, build_system_event(ts_ns, b'O'))

        for _ in range(num_messages):
            ts_ns += random.randint(100, 5_000)  # advance 0.1–5 µs
            locate = random.randint(1, 100)
            roll   = random.random()

            # ── Add Order (60%) ──────────────────────────────────────────
            if roll < 0.60 or len(live_orders) < 20:
                next_ref += 1
                ref    = next_ref
                symbol = random.choice(SYMBOLS)
                side   = random.choice([b'B', b'S'])
                base   = BASE_PRICES[symbol]
                if side == b'B':
                    price = max(10_000, base - random.randint(100, 5_000))
                else:
                    price = max(10_000, base + random.randint(100, 5_000))
                shares = random.choice([100, 200, 300, 500, 1_000, 2_500])

                live_orders[ref] = (symbol, side, price, shares, locate)
                write_msg(f, build_add_order(ts_ns, locate, ref, side, shares, symbol, price))
                stats["A"] += 1

            # ── Cancel Order (15%) ───────────────────────────────────────
            elif roll < 0.75 and live_orders:
                ref = random.choice(list(live_orders))
                sym, side, price, cur, loc = live_orders[ref]
                cancel = random.randint(1, cur)

                if cancel >= cur:
                    del live_orders[ref]
                else:
                    live_orders[ref] = (sym, side, price, cur - cancel, loc)

                write_msg(f, build_cancel(ts_ns, loc, ref, cancel))
                stats["X"] += 1

            # ── Execute Order (10%) ──────────────────────────────────────
            elif roll < 0.85 and live_orders:
                ref = random.choice(list(live_orders))
                sym, side, price, cur, loc = live_orders[ref]
                exec_qty = min(cur, random.choice([100, 200, 500]))
                next_match += 1

                if exec_qty >= cur:
                    del live_orders[ref]
                else:
                    live_orders[ref] = (sym, side, price, cur - exec_qty, loc)

                write_msg(f, build_executed(ts_ns, loc, ref, exec_qty, next_match))
                stats["E"] += 1

            # ── Delete Order (5%) ────────────────────────────────────────
            elif roll < 0.90 and live_orders:
                ref = random.choice(list(live_orders))
                _, _, _, _, loc = live_orders.pop(ref)
                write_msg(f, build_delete(ts_ns, loc, ref))
                stats["D"] += 1

            # ── Replace Order (5%) ───────────────────────────────────────
            elif roll < 0.95 and live_orders:
                old_ref = random.choice(list(live_orders))
                sym, side, price, _, loc = live_orders.pop(old_ref)
                next_ref += 1
                new_ref    = next_ref
                new_shares = random.choice([100, 200, 500, 1_000])
                base = BASE_PRICES[sym]
                if side == b'B':
                    new_price = max(10_000, base - random.randint(100, 5_000))
                else:
                    new_price = max(10_000, base + random.randint(100, 5_000))

                live_orders[new_ref] = (sym, side, new_price, new_shares, loc)
                write_msg(f, build_replace(ts_ns, loc, old_ref, new_ref, new_shares, new_price))
                stats["U"] += 1

            # ── Trade (5%) ───────────────────────────────────────────────
            else:
                symbol = random.choice(SYMBOLS)
                base   = BASE_PRICES[symbol]
                price  = max(10_000, base + random.randint(-3_000, 3_000))
                shares = random.choice([100, 200, 500])
                next_match += 1

                write_msg(f, build_trade(ts_ns, locate, 0, b'B', shares, symbol, price, next_match))
                stats["P"] += 1

        # System Event: End of Messages
        ts_ns += 1000
        write_msg(f, build_system_event(ts_ns, b'C'))

    size_mb = os.path.getsize(output_path) / (1024 * 1024)

    print(f"[+] Done! {size_mb:.2f} MB written")
    print(f"    Message breakdown:")
    for k, v in sorted(stats.items()):
        label = {"A": "Add Order", "X": "Cancel", "E": "Execute",
                 "D": "Delete", "U": "Replace", "P": "Trade"}[k]
        print(f"      {k} ({label:>10s}): {v:>8,}")
    print(f"    Total:             {sum(stats.values()):>8,}")


# ── main ─────────────────────────────────────────────────────────────────────

if __name__ == "__main__":
    ap = argparse.ArgumentParser(
        description="Generate synthetic NASDAQ ITCH 5.0 binary test data."
    )
    ap.add_argument("-o", "--output", default="data/sample_100k.itch",
                    help="Output file path (default: data/sample_100k.itch)")
    ap.add_argument("-n", "--count", type=int, default=100_000,
                    help="Number of messages (default: 100,000)")
    args = ap.parse_args()

    generate_itch_file(args.output, args.count)
