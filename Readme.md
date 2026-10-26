# HFT Project

A C++ implementation of a **Limit Order Book (LOB)** as the foundation for a high-frequency trading (HFT) market-making simulator.

The project is intended as a practical exercise in modern C++ and market microstructure, building from individual orders and order-book management toward a matching engine and, eventually, a market-making simulator.

## Project Goals

- Build a limit order book that maintains buy and sell orders for a financial instrument.
- Implement price-time priority for organizing resting orders.
- Develop a matching engine that executes compatible buy and sell orders.
- Build a market-making simulator on top of the matching engine.
- Learn modern C++ concepts through practical implementation, including ownership, standard containers, and exception safety.

## Current Features

- [x] `Order` class representing individual buy and sell limit orders.
- [x] `Order` state management, including activation, cancellation, rejection, and fills.
- [x] `OrderBook` class for managing orders for a specific instrument.
- [x] Separate bid and ask sides, ordered by price.
- [x] Price-level queues for maintaining time priority among orders at the same price.
- [x] Order ID index for efficient lookup.
- [x] Order insertion and cancellation.
- [x] Best bid and best ask queries.
- [ ] Matching engine with price-time priority.
- [ ] Execution reporting and trade history.
- [ ] Market-making strategy.
- [ ] Simulation and performance benchmarks.

## Architecture

The project is organized around three core components:

### `Order`

Represents an individual limit order. It stores order information such as its ID, instrument, side, price, quantity, and state.
Upon creation an order is in the `New` state. It can transition to `Active`, `PartiallyFilled`, `Cancelled`, `Rejected`, or `Filled` states
based on its lifecycle in the order book.

An order validates its attributes upon construction and throws an exception if any attributes are invalid (e.g., negative price or quantity).
The order class provides methods to update its state and track filled quantities.

### `OrderBook`

Maintains the resting buy and sell orders for a particular instrument.

- **Bids:** Ordered from highest to lowest price.
- **Asks:** Ordered from lowest to highest price.
- **Price levels:** Each price level maintains a queue of orders to preserve time priority.
- **Order index:** Maps order IDs to owned orders for lookup and cancellation.

The order book owns its orders through `std::unique_ptr<Order>`. Price-level queues hold non-owning pointers to those orders.

### `MatchingEngine` — In process

Will handle incoming orders, match compatible buy and sell orders, apply price-time priority, update order states, and report executions. The order book will maintain the remaining resting liquidity.

### `Execution` — In process

Data structure to record trade executions, including the matched orders, execution price, quantity.

## Ownership Model

An Incoming order is first created by the client. It is then passed to the MatchingEngine, if the order is accepted, the MatchingEngine transfers ownership.
The MatchingEngine then tries to match the order against the opposite side of the book.

1. Any trade executions that occur are recorded as 'Execution' objects, which are owned by the MatchingEngine. Once the matching is complete one of the following occurs:
   - If the order is partially filled, the MatchingEngine will hand ownership of the remaining order to the OrderBook.
   - If the order is fully filled, the MatchingEngine will destroy the order.
2. If the order is rejected, the MatchingEngine will destroy the order.

## Development Roadmap

1. **Order model** — Implement order attributes and valid state transitions.
2. **Order book** — Implement insertion, cancellation, best-price queries, and consistent ownership.
3. **Testing** — Add unit tests for order behavior, price levels, cancellation, and ownership.
4. **Matching engine** — Match incoming orders against the opposite side of the book.
5. **Execution reporting** — Record trades and execution details.
6. **Market-making simulator** — Simulate quoting, fills, inventory, and P&L.
7. **Performance analysis** — Benchmark throughput, latency, and data-structure choices.

## Building and Testing

The project uses CMake. Configure and build from the project root:

```bash
cmake -S . -B build
cmake --build build
```

Run the test executables from the build directory or through CTest, depending on the test targets configured in `CMakeLists.txt`.

For example:

```bash
ctest --test-dir build --output-on-failure
```

GoogleTest is used for unit testing if configured in the project.

## Scope

This is an educational project, not a production-ready trading system. The initial focus is correctness, clear ownership semantics, and a sound order-book design before optimizing for low latency.

The longer-term objective is to understand the mechanics of a limit order book and matching engine, then use them as the foundation for a market-making simulator.
