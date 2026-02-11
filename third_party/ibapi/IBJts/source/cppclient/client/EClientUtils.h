/* Copyright (C) 2025 Interactive Brokers LLC. All rights reserved. This code is subject to the terms
 * and conditions of the IB API Non-Commercial License or the IB API Commercial License, as applicable. */

#pragma once
#ifndef TWS_API_CLIENT_ECLIENT_UTILS_H
#define TWS_API_CLIENT_ECLIENT_UTILS_H

#include "Contract.h"
#include "Execution.h"
#include "Order.h"
#include "OrderCancel.h"
#include "ScannerSubscription.h"
#include "WshEventData.h"
#include "Order.pb.h"
#include "CancelOrderRequest.pb.h"
#include "GlobalCancelRequest.pb.h"
#include "ExecutionRequest.pb.h"
#include "OrderCancel.pb.h"
#include "PlaceOrderRequest.pb.h"
#include "AllOpenOrdersRequest.pb.h"
#include "AutoOpenOrdersRequest.pb.h"
#include "OpenOrdersRequest.pb.h"
#include "CompletedOrdersRequest.pb.h"
#include "ContractDataRequest.pb.h"
#include "MarketDataRequest.pb.h"
#include "MarketDepthRequest.pb.h"
#include "MarketDataTypeRequest.pb.h"
#include "CancelMarketData.pb.h"
#include "CancelMarketDepth.pb.h"
#include "AccountDataRequest.pb.h"
#include "ManagedAccountsRequest.pb.h"
#include "PositionsRequest.pb.h"
#include "CancelPositions.pb.h"
#include "AccountSummaryRequest.pb.h"
#include "CancelAccountSummary.pb.h"
#include "PositionsMultiRequest.pb.h"
#include "CancelPositionsMulti.pb.h"
#include "AccountUpdatesMultiRequest.pb.h"
#include "CancelAccountUpdatesMulti.pb.h"
#include "HistoricalDataRequest.pb.h"
#include "RealTimeBarsRequest.pb.h"
#include "HeadTimestampRequest.pb.h"
#include "HistogramDataRequest.pb.h"
#include "HistoricalTicksRequest.pb.h"
#include "TickByTickRequest.pb.h"
#include "CancelHistoricalData.pb.h"
#include "CancelRealTimeBars.pb.h"
#include "CancelHeadTimestamp.pb.h"
#include "CancelHistogramData.pb.h"
#include "CancelTickByTick.pb.h"
#include "NewsBulletinsRequest.pb.h"
#include "CancelNewsBulletins.pb.h"
#include "NewsArticleRequest.pb.h"
#include "NewsProvidersRequest.pb.h"
#include "HistoricalNewsRequest.pb.h"
#include "WshMetaDataRequest.pb.h"
#include "CancelWshMetaData.pb.h"
#include "WshEventDataRequest.pb.h"
#include "CancelWshEventData.pb.h"
#include "ScannerParametersRequest.pb.h"
#include "ScannerSubscriptionRequest.pb.h"
#include "FundamentalsDataRequest.pb.h"
#include "PnLRequest.pb.h"
#include "PnLSingleRequest.pb.h"
#include "CancelScannerSubscription.pb.h"
#include "CancelFundamentalsData.pb.h"
#include "CancelPnL.pb.h"
#include "CancelPnLSingle.pb.h"
#include "FARequest.pb.h"
#include "FAReplace.pb.h"
#include "ExerciseOptionsRequest.pb.h"
#include "CalculateImpliedVolatilityRequest.pb.h"
#include "CancelCalculateImpliedVolatility.pb.h"
#include "CalculateOptionPriceRequest.pb.h"
#include "CancelCalculateOptionPrice.pb.h"
#include "SecDefOptParamsRequest.pb.h"
#include "SoftDollarTiersRequest.pb.h"
#include "FamilyCodesRequest.pb.h"
#include "MatchingSymbolsRequest.pb.h"
#include "SmartComponentsRequest.pb.h"
#include "MarketRuleRequest.pb.h"
#include "UserInfoRequest.pb.h"
#include "IdsRequest.pb.h"
#include "CurrentTimeRequest.pb.h"
#include "CurrentTimeInMillisRequest.pb.h"
#include "StartApiRequest.pb.h"
#include "SetServerLogLevelRequest.pb.h"
#include "VerifyRequest.pb.h"
#include "VerifyMessageRequest.pb.h"
#include "QueryDisplayGroupsRequest.pb.h"
#include "SubscribeToGroupEventsRequest.pb.h"
#include "UpdateDisplayGroupRequest.pb.h"
#include "UnsubscribeFromGroupEventsRequest.pb.h"
#include "MarketDepthExchangesRequest.pb.h"
#include "CancelContractData.pb.h"
#include "CancelHistoricalTicks.pb.h"
#include "AttachedOrders.pb.h"

class EClientUtils {

public:
	static protobuf::ExecutionRequest createExecutionRequestProto(int reqId, const ExecutionFilter& filter);
	static protobuf::PlaceOrderRequest createPlaceOrderRequestProto(OrderId id, const Contract& contract, const Order& order);
	static protobuf::Order createOrderProto(const Order& order);
	static protobuf::AttachedOrders createAttachedOrdersProto(const Order& order);
	static std::list<protobuf::OrderCondition> createConditionsProto(Order order);
	static protobuf::OrderCondition createOrderConditionProto(OrderCondition& condition);
	static protobuf::OrderCondition createOperatorConditionProto(OrderCondition& condition);
	static protobuf::OrderCondition createContractConditionProto(OrderCondition& condition);
	static protobuf::OrderCondition createPriceConditionProto(OrderCondition& condition);
	static protobuf::OrderCondition createTimeConditionProto(OrderCondition& condition);
	static protobuf::OrderCondition createMarginConditionProto(OrderCondition& condition);
	static protobuf::OrderCondition createExecutionConditionProto(OrderCondition& condition);
	static protobuf::OrderCondition createVolumeConditionProto(OrderCondition& condition);
	static protobuf::OrderCondition createPercentChangeConditionProto(OrderCondition& condition);
	static protobuf::SoftDollarTier createSoftDollarTierProto(Order order);
	static std::map<std::string, std::string> createStringStringMap(TagValueListSPtr tagValueListSPtr);
	static protobuf::Contract createContractProto(const Contract& contract, const Order& order);
	static protobuf::DeltaNeutralContract* createDeltaNeutralContractProto(const Contract& contract);
	static std::list<protobuf::ComboLeg> createComboLegProtoList(const Contract& contract, const Order& order);
	static protobuf::ComboLeg createComboLegProto(const ComboLeg& comboLeg, double perLegPrice);
	static protobuf::CancelOrderRequest createCancelOrderRequestProto(OrderId id, const OrderCancel& orderCancel);
	static protobuf::GlobalCancelRequest createGlobalCancelRequestProto(const OrderCancel& orderCancel);
	static protobuf::OrderCancel createOrderCancelProto(const OrderCancel& orderCancel);
	static protobuf::AllOpenOrdersRequest createAllOpenOrdersRequestProto();
	static protobuf::AutoOpenOrdersRequest createAutoOpenOrdersRequestProto(bool autoBind);
	static protobuf::OpenOrdersRequest createOpenOrdersRequestProto();
	static protobuf::CompletedOrdersRequest createCompletedOrdersRequestProto(bool apiOnly);
	static protobuf::ContractDataRequest createContractDataRequestProto(int reqId, const Contract& contract);
	static protobuf::MarketDataRequest createMarketDataRequestProto(int reqId, const Contract& contract, const std::string& genericTickList, bool snapshot, bool regulatorySnapshot, const TagValueListSPtr marketDataOptionsList);
	static protobuf::MarketDepthRequest createMarketDepthRequestProto(int reqId, const Contract& contract, int numRows, bool isSmartDepth, const TagValueListSPtr marketDepthOptionsList);
	static protobuf::MarketDataTypeRequest createMarketDataTypeRequestProto(int marketDataType);
	static protobuf::CancelMarketData createCancelMarketDataProto(int reqId);
	static protobuf::CancelMarketDepth createCancelMarketDepthProto(int reqId, bool isSmartDepth);
	static protobuf::AccountDataRequest createAccountDataRequestProto(bool subscribe, const std::string& acctCode);
	static protobuf::ManagedAccountsRequest createManagedAccountsRequestProto();
	static protobuf::PositionsRequest createPositionsRequestProto();
	static protobuf::CancelPositions createCancelPositionsRequestProto();
	static protobuf::AccountSummaryRequest createAccountSummaryRequestProto(int reqId, const std::string& group, const std::string& tags);
	static protobuf::CancelAccountSummary createCancelAccountSummaryRequestProto(int reqId);
	static protobuf::PositionsMultiRequest createPositionsMultiRequestProto(int reqId, const std::string& account, const std::string& modelCode);
	static protobuf::CancelPositionsMulti createCancelPositionsMultiRequestProto(int reqId);
	static protobuf::AccountUpdatesMultiRequest createAccountUpdatesMultiRequestProto(int reqId, const std::string& account, const std::string& modelCode, bool ledgerAndNLV);
	static protobuf::CancelAccountUpdatesMulti createCancelAccountUpdatesMultiRequestProto(int reqId);
	static protobuf::HistoricalDataRequest createHistoricalDataRequestProto(int reqId, const Contract& contract, const std::string& endDateTime, const std::string& duration, 
		const std::string& barSizeSetting, const std::string& whatToShow, bool useRTH, int formatDate, bool keepUpToDate, 	const TagValueListSPtr& chartOptionsList);
	static protobuf::RealTimeBarsRequest createRealTimeBarsRequestProto(int reqId, const Contract& contract, int barSize, const std::string& whatToShow, bool useRTH, const TagValueListSPtr& realTimeBarsOptionsList);
	static protobuf::HeadTimestampRequest createHeadTimestampRequestProto(int reqId, const Contract& contract, const std::string& whatToShow, bool useRTH, int formatDate);
	static protobuf::HistogramDataRequest createHistogramDataRequestProto(int reqId, const Contract& contract, bool useRTH, const std::string& timePeriod);
	static protobuf::HistoricalTicksRequest createHistoricalTicksRequestProto(int reqId, const Contract& contract, const std::string& startDateTime,
		const std::string& endDateTime, int numberOfTicks, const std::string& whatToShow, bool useRTH, bool ignoreSize, const TagValueListSPtr& miscOptionsList);
	static protobuf::TickByTickRequest createTickByTickRequestProto(int reqId, const Contract& contract, const std::string& tickType, int numberOfTicks, bool ignoreSize);
	static protobuf::CancelHistoricalData createCancelHistoricalDataProto(int reqId);
	static protobuf::CancelRealTimeBars createCancelRealTimeBarsProto(int reqId);
	static protobuf::CancelHeadTimestamp createCancelHeadTimestampProto(int reqId);
	static protobuf::CancelHistogramData createCancelHistogramDataProto(int reqId);
	static protobuf::CancelTickByTick createCancelTickByTickProto(int reqId);
	static protobuf::NewsBulletinsRequest createNewsBulletinsRequestProto(bool allMessages);
	static protobuf::CancelNewsBulletins createCancelNewsBulletinsProto();
	static protobuf::NewsArticleRequest createNewsArticleRequestProto(int reqId, const std::string& providerCode, const std::string& articleId, const TagValueListSPtr newsArticleOptionsList);
	static protobuf::NewsProvidersRequest createNewsProvidersRequestProto();
	static protobuf::HistoricalNewsRequest createHistoricalNewsRequestProto(int reqId, int conId, const std::string& providerCodes, const std::string& startDateTime, const std::string& endDateTime, int totalResults, const TagValueListSPtr historicalNewsOptionsList);
	static protobuf::WshMetaDataRequest createWshMetaDataRequestProto(int reqId);
	static protobuf::CancelWshMetaData createCancelWshMetaDataProto(int reqId);
	static protobuf::WshEventDataRequest createWshEventDataRequestProto(int reqId, const WshEventData& wshEventData);
	static protobuf::CancelWshEventData createCancelWshEventDataProto(int reqId);
	static protobuf::ScannerParametersRequest createScannerParametersRequestProto();
	static protobuf::ScannerSubscriptionRequest createScannerSubscriptionRequestProto(int reqId, const ScannerSubscription& subscription, 
		const TagValueListSPtr& scannerSubscriptionOptionsList, const TagValueListSPtr& scannerSubscriptionFilterOptionsList);
	static protobuf::ScannerSubscription createScannerSubscriptionProto(const ScannerSubscription& subscription,
		const TagValueListSPtr& scannerSubscriptionOptionsList, const TagValueListSPtr& scannerSubscriptionFilterOptionsList);
	static protobuf::FundamentalsDataRequest createFundamentalsDataRequestProto(int reqId, const Contract& contract, const std::string& reportType, const TagValueListSPtr fundamentalsDataOptionsList);
	static protobuf::PnLRequest createPnLRequestProto(int reqId, const std::string& account, const std::string& modelCode);
	static protobuf::PnLSingleRequest createPnLSingleRequestProto(int reqId, const std::string& account, const std::string& modelCode, int conId);
	static protobuf::CancelScannerSubscription createCancelScannerSubscriptionProto(int reqId);
	static protobuf::CancelFundamentalsData createCancelFundamentalsDataProto(int reqId);
	static protobuf::CancelPnL createCancelPnLProto(int reqId);
	static protobuf::CancelPnLSingle createCancelPnLSingleProto(int reqId);
	static protobuf::FARequest createFARequestProto(int faDataType);
	static protobuf::FAReplace createFAReplaceProto(int reqId, int faDataType, const std::string& xml);
	static protobuf::ExerciseOptionsRequest createExerciseOptionsRequestProto(int orderId, const Contract& contract, int exerciseAction, int exerciseQuantity, const std::string& account, bool override, const std::string& manualOrderTime, const std::string& customerAccount, bool professionalCustomer);
	static protobuf::CalculateImpliedVolatilityRequest createCalculateImpliedVolatilityRequestProto(int reqId, const Contract& contract, double optionPrice, double underPrice, const TagValueListSPtr impliedVolatilityOptionsList);
	static protobuf::CancelCalculateImpliedVolatility createCancelCalculateImpliedVolatilityProto(int reqId);
	static protobuf::CalculateOptionPriceRequest createCalculateOptionPriceRequestProto(int reqId, const Contract& contract, double volatility, double underPrice, const TagValueListSPtr optionPriceOptionsList);
	static protobuf::CancelCalculateOptionPrice createCancelCalculateOptionPriceProto(int reqId);
	static protobuf::SecDefOptParamsRequest createSecDefOptParamsRequestProto(int reqId, const std::string& underlyingSymbol, const std::string& futFopExchange, const std::string& underlyingSecType, int underlyingConId);
	static protobuf::SoftDollarTiersRequest createSoftDollarTiersRequestProto(int reqId);
	static protobuf::FamilyCodesRequest createFamilyCodesRequestProto();
	static protobuf::MatchingSymbolsRequest createMatchingSymbolsRequestProto(int reqId, const std::string& pattern);
	static protobuf::SmartComponentsRequest createSmartComponentsRequestProto(int reqId, const std::string& bboExchange);
	static protobuf::MarketRuleRequest createMarketRuleRequestProto(int marketRuleId);
	static protobuf::UserInfoRequest createUserInfoRequestProto(int reqId);
	static protobuf::IdsRequest createIdsRequestProto(int numIds);
	static protobuf::CurrentTimeRequest createCurrentTimeRequestProto();
	static protobuf::CurrentTimeInMillisRequest createCurrentTimeInMillisRequestProto();
	static protobuf::StartApiRequest createStartApiRequestProto(int clientId, const std::string& optionalCapabilities);
	static protobuf::SetServerLogLevelRequest createSetServerLogLevelRequestProto(int logLevel);
	static protobuf::VerifyRequest createVerifyRequestProto(const std::string& apiName, const std::string& apiVersion);
	static protobuf::VerifyMessageRequest createVerifyMessageRequestProto(const std::string& apiData);
	static protobuf::QueryDisplayGroupsRequest createQueryDisplayGroupsRequestProto(int reqId);
	static protobuf::SubscribeToGroupEventsRequest createSubscribeToGroupEventsRequestProto(int reqId, int groupId);
	static protobuf::UpdateDisplayGroupRequest createUpdateDisplayGroupRequestProto(int reqId, const std::string& contractInfo);
	static protobuf::UnsubscribeFromGroupEventsRequest createUnsubscribeFromGroupEventsRequestProto(int reqId);
	static protobuf::MarketDepthExchangesRequest createMarketDepthExchangesRequestProto();
	static protobuf::CancelContractData createCancelContractDataProto(int reqId);
	static protobuf::CancelHistoricalTicks createCancelHistoricalTicksProto(int reqId);
};

#endif

