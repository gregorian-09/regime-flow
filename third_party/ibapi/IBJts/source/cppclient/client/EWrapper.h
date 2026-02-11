/* Copyright (C) 2025 Interactive Brokers LLC. All rights reserved. This code is subject to the terms
 * and conditions of the IB API Non-Commercial License or the IB API Commercial License, as applicable. */

#pragma once
#ifndef TWS_API_CLIENT_EWRAPPER_H
#define TWS_API_CLIENT_EWRAPPER_H

#include <string>
#include <set>
#include <map>
#include <tuple>
#include <vector>
#include "CommonDefs.h"
#include "SoftDollarTier.h"
#include "DepthMktDataDescription.h"
#include "FamilyCode.h"
#include "NewsProvider.h"
#include "TickAttrib.h"
#include "HistogramEntry.h"
#include "bar.h"
#include "PriceIncrement.h"
#include "HistoricalTick.h"
#include "HistoricalTickBidAsk.h"
#include "HistoricalTickLast.h"
#include "Decimal.h"
#include "HistoricalSession.h"

#include "ExecutionDetails.pb.h"
#include "ExecutionDetailsEnd.pb.h"
#include "ErrorMessage.pb.h"
#include "OpenOrder.pb.h"
#include "OpenOrdersEnd.pb.h"
#include "OrderStatus.pb.h"
#include "CompletedOrder.pb.h"
#include "CompletedOrdersEnd.pb.h"
#include "OrderBound.pb.h"
#include "ContractData.pb.h"
#include "ContractDataEnd.pb.h"
#include "TickPrice.pb.h"
#include "TickSize.pb.h"
#include "TickOptionComputation.pb.h"
#include "TickGeneric.pb.h"
#include "TickString.pb.h"
#include "TickSnapshotEnd.pb.h"
#include "MarketDepth.pb.h"
#include "MarketDepthL2.pb.h"
#include "MarketDataType.pb.h"
#include "TickReqParams.pb.h"
#include "AccountValue.pb.h"
#include "PortfolioValue.pb.h"
#include "AccountUpdateTime.pb.h"
#include "AccountDataEnd.pb.h"
#include "ManagedAccounts.pb.h"
#include "Position.pb.h"
#include "PositionEnd.pb.h"
#include "AccountSummary.pb.h"
#include "AccountSummaryEnd.pb.h"
#include "PositionMulti.pb.h"
#include "PositionMultiEnd.pb.h"
#include "AccountUpdateMulti.pb.h"
#include "AccountUpdateMultiEnd.pb.h"
#include "HistoricalData.pb.h"
#include "HistoricalDataUpdate.pb.h"
#include "HistoricalDataEnd.pb.h"
#include "RealTimeBarTick.pb.h"
#include "HeadTimestamp.pb.h"
#include "HistogramData.pb.h"
#include "HistoricalTicks.pb.h"
#include "HistoricalTicksBidAsk.pb.h"
#include "HistoricalTicksLast.pb.h"
#include "TickByTickData.pb.h"
#include "NewsBulletin.pb.h"
#include "NewsArticle.pb.h"
#include "NewsProviders.pb.h"
#include "HistoricalNews.pb.h"
#include "HistoricalNewsEnd.pb.h"
#include "WshMetaData.pb.h"
#include "WshEventData.pb.h"
#include "TickNews.pb.h"
#include "ScannerParameters.pb.h"
#include "ScannerData.pb.h"
#include "FundamentalsData.pb.h"
#include "PnL.pb.h"
#include "PnLSingle.pb.h"
#include "ReceiveFA.pb.h"
#include "ReplaceFAEnd.pb.h"
#include "CommissionAndFeesReport.pb.h"
#include "HistoricalSchedule.pb.h"
#include "RerouteMarketDataRequest.pb.h"
#include "RerouteMarketDepthRequest.pb.h"
#include "SecDefOptParameter.pb.h"
#include "SecDefOptParameterEnd.pb.h"
#include "SoftDollarTiers.pb.h"
#include "FamilyCodes.pb.h"
#include "SymbolSamples.pb.h"
#include "SmartComponents.pb.h"
#include "MarketRule.pb.h"
#include "UserInfo.pb.h"
#include "NextValidId.pb.h"
#include "CurrentTime.pb.h"
#include "CurrentTimeInMillis.pb.h"
#include "VerifyMessageApi.pb.h"
#include "VerifyCompleted.pb.h"
#include "DisplayGroupList.pb.h"
#include "DisplayGroupUpdated.pb.h"
#include "MarketDepthExchanges.pb.h"

enum TickType { BID_SIZE, BID, ASK, ASK_SIZE, LAST, LAST_SIZE,
				HIGH, LOW, VOLUME, CLOSE,
				BID_OPTION_COMPUTATION,
				ASK_OPTION_COMPUTATION,
				LAST_OPTION_COMPUTATION,
				MODEL_OPTION,
				OPEN,
				LOW_13_WEEK,
				HIGH_13_WEEK,
				LOW_26_WEEK,
				HIGH_26_WEEK,
				LOW_52_WEEK,
				HIGH_52_WEEK,
				AVG_VOLUME,
				OPEN_INTEREST,
				OPTION_HISTORICAL_VOL,
				OPTION_IMPLIED_VOL,
				OPTION_BID_EXCH,
				OPTION_ASK_EXCH,
				OPTION_CALL_OPEN_INTEREST,
				OPTION_PUT_OPEN_INTEREST,
				OPTION_CALL_VOLUME,
				OPTION_PUT_VOLUME,
				INDEX_FUTURE_PREMIUM,
				BID_EXCH,
				ASK_EXCH,
				AUCTION_VOLUME,
				AUCTION_PRICE,
				AUCTION_IMBALANCE,
				MARK_PRICE,
				BID_EFP_COMPUTATION,
				ASK_EFP_COMPUTATION,
				LAST_EFP_COMPUTATION,
				OPEN_EFP_COMPUTATION,
				HIGH_EFP_COMPUTATION,
				LOW_EFP_COMPUTATION,
				CLOSE_EFP_COMPUTATION,
				LAST_TIMESTAMP,
				SHORTABLE,
				FUNDAMENTAL_RATIOS,
				RT_VOLUME,
				HALTED,
				BID_YIELD,
				ASK_YIELD,
				LAST_YIELD,
				CUST_OPTION_COMPUTATION,
				TRADE_COUNT,
				TRADE_RATE,
				VOLUME_RATE,
				LAST_RTH_TRADE,
				RT_HISTORICAL_VOL,
				IB_DIVIDENDS,
				BOND_FACTOR_MULTIPLIER,
				REGULATORY_IMBALANCE,
				NEWS_TICK,
				SHORT_TERM_VOLUME_3_MIN,
				SHORT_TERM_VOLUME_5_MIN,
				SHORT_TERM_VOLUME_10_MIN,
				DELAYED_BID,
				DELAYED_ASK,
				DELAYED_LAST,
				DELAYED_BID_SIZE,
				DELAYED_ASK_SIZE,
				DELAYED_LAST_SIZE,
				DELAYED_HIGH,
				DELAYED_LOW,
				DELAYED_VOLUME,
				DELAYED_CLOSE,
				DELAYED_OPEN,
				RT_TRD_VOLUME,
				CREDITMAN_MARK_PRICE,
				CREDITMAN_SLOW_MARK_PRICE,
				DELAYED_BID_OPTION_COMPUTATION,
				DELAYED_ASK_OPTION_COMPUTATION,
				DELAYED_LAST_OPTION_COMPUTATION,
				DELAYED_MODEL_OPTION_COMPUTATION,
				LAST_EXCH,
				LAST_REG_TIME,
				FUTURES_OPEN_INTEREST,
				AVG_OPT_VOLUME,
				DELAYED_LAST_TIMESTAMP,
				SHORTABLE_SHARES,
				DELAYED_HALTED,
				REUTERS_2_MUTUAL_FUNDS,
				ETF_NAV_CLOSE,
				ETF_NAV_PRIOR_CLOSE,
				ETF_NAV_BID,
				ETF_NAV_ASK,
				ETF_NAV_LAST,
				ETF_FROZEN_NAV_LAST,
				ETF_NAV_HIGH,
				ETF_NAV_LOW,
				SOCIAL_MARKET_ANALYTICS,
				ESTIMATED_IPO_MIDPOINT,
				FINAL_IPO_LAST,
	            DELAYED_YIELD_BID,
	            DELAYED_YIELD_ASK,
				NOT_SET };

typedef std::map<int, std::tuple<std::string, char>> SmartComponentsMap;
typedef std::vector<HistogramEntry> HistogramDataVector;


inline bool isPrice( TickType tickType) {
	return tickType == BID || tickType == ASK || tickType == LAST || tickType == DELAYED_BID || tickType == DELAYED_ASK || tickType == DELAYED_LAST;
}

struct Contract;
struct ContractDetails;
struct ContractDescription;
struct Order;
struct OrderState;
struct Execution;
struct DeltaNeutralContract;
struct CommissionAndFeesReport;

class EWrapper
{
public:
   virtual ~EWrapper() {};

	#define EWRAPPER_VIRTUAL_IMPL =0
	#include "EWrapper_prototypes.h"
};


#endif
