import datetime as dt

import regimeflow as rf


def test_order_expire_at_roundtrip():
    expire_at = dt.datetime(2025, 1, 1, 12, 0, 0)
    order = rf.Order(
        symbol="AAPL",
        side=rf.OrderSide.BUY,
        type=rf.OrderType.MARKET,
        quantity=1.0,
        tif=rf.TimeInForce.GTD,
        expire_at=expire_at,
    )
    assert order.expire_at is not None
    assert order.expire_at.year == 2025
