#include "regimeflow/engine/order.h"

namespace regimeflow::engine {

Order Order::market(SymbolId symbol, OrderSide side, Quantity qty) {
    Order order;
    order.symbol = symbol;
    order.side = side;
    order.type = OrderType::Market;
    order.quantity = qty;
    return order;
}

Order Order::limit(SymbolId symbol, OrderSide side, Quantity qty, Price price) {
    Order order;
    order.symbol = symbol;
    order.side = side;
    order.type = OrderType::Limit;
    order.quantity = qty;
    order.limit_price = price;
    return order;
}

Order Order::stop(SymbolId symbol, OrderSide side, Quantity qty, Price stop) {
    Order order;
    order.symbol = symbol;
    order.side = side;
    order.type = OrderType::Stop;
    order.quantity = qty;
    order.stop_price = stop;
    return order;
}

}  // namespace regimeflow::engine
