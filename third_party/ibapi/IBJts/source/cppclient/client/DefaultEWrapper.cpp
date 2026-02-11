/* Copyright (C) 2025 Interactive Brokers LLC. All rights reserved. This code is subject to the terms
 * and conditions of the IB API Non-Commercial License or the IB API Commercial License, as applicable. */

#include "StdAfx.h"
#include "DefaultEWrapper.h"

void DefaultEWrapper::tickPrice( TickerId tickerId, TickType field, double price, const TickAttrib& attribs) { }
void DefaultEWrapper::tickSize( TickerId tickerId, TickType field, Decimal size) { }
void DefaultEWrapper::tickOptionComputation( TickerId tickerId, TickType tickType, int tickAttrib, double impliedVol, double delta,
	   double optPrice, double pvDividend, double gamma, double vega, double theta, double undPrice) { }
void DefaultEWrapper::tickGeneric(TickerId tickerId, TickType tickType, double value) { }
void DefaultEWrapper::tickString(TickerId tickerId, TickType tickType, const std::string& value) { }
void DefaultEWrapper::tickEFP(TickerId tickerId, TickType tickType, double basisPoints, const std::string& formattedBasisPoints,
	   double totalDividends, int holdDays, const std::string& futureLastTradeDate, double dividendImpact, double dividendsToLastTradeDate) { }
void DefaultEWrapper::orderStatus( OrderId orderId, const std::string& status, Decimal filled,
	   Decimal remaining, double avgFillPrice, long long permId, int parentId,
	   double lastFillPrice, int clientId, const std::string& whyHeld, double mktCapPrice) { }
void DefaultEWrapper::openOrder( OrderId orderId, const Contract&, const Order&, const OrderState&) { }
void DefaultEWrapper::openOrderEnd() { }
void DefaultEWrapper::winError( const std::string& str, int lastError) { }
void DefaultEWrapper::connectionClosed() { }
void DefaultEWrapper::updateAccountValue(const std::string& key, const std::string& val,
   const std::string& currency, const std::string& accountName) { }
void DefaultEWrapper::updatePortfolio( const Contract& contract, Decimal position,
      double marketPrice, double marketValue, double averageCost,
      double unrealizedPNL, double realizedPNL, const std::string& accountName) { }
void DefaultEWrapper::updateAccountTime(const std::string& timeStamp) { }
void DefaultEWrapper::accountDownloadEnd(const std::string& accountName) { }
void DefaultEWrapper::nextValidId( OrderId orderId) { }
void DefaultEWrapper::contractDetails( int reqId, const ContractDetails& contractDetails) { }
void DefaultEWrapper::bondContractDetails( int reqId, const ContractDetails& contractDetails) { }
void DefaultEWrapper::contractDetailsEnd( int reqId) { }
void DefaultEWrapper::execDetails( int reqId, const Contract& contract, const Execution& execution) { }
void DefaultEWrapper::execDetailsEnd( int reqId) { }
void DefaultEWrapper::error(int id, time_t errorTime, int errorCode, const std::string& errorString, const std::string& advancedOrderRejectJson) { }
void DefaultEWrapper::updateMktDepth(TickerId id, int position, int operation, int side,
	double price, Decimal size) { }
void DefaultEWrapper::updateMktDepthL2(TickerId id, int position, const std::string& marketMaker, int operation,
      int side, double price, Decimal size, bool isSmartDepth) { }
void DefaultEWrapper::updateNewsBulletin(int msgId, int msgType, const std::string& newsMessage, const std::string& originExch) { }
void DefaultEWrapper::managedAccounts( const std::string& accountsList) { }
void DefaultEWrapper::receiveFA(faDataType pFaDataType, const std::string& cxml) { }
void DefaultEWrapper::historicalData(TickerId reqId, const Bar& bar) { }
void DefaultEWrapper::historicalDataEnd(int reqId, const std::string& startDateStr, const std::string& endDateStr) { }
void DefaultEWrapper::scannerParameters(const std::string& xml) { }
void DefaultEWrapper::scannerData(int reqId, int rank, const ContractDetails& contractDetails,
	   const std::string& distance, const std::string& benchmark, const std::string& projection,
	   const std::string& legsStr) { }
void DefaultEWrapper::scannerDataEnd(int reqId) { }
void DefaultEWrapper::realtimeBar(TickerId reqId, long time, double open, double high, double low, double close,
	   Decimal volume, Decimal wap, int count) { }
void DefaultEWrapper::currentTime(long time) { }
void DefaultEWrapper::fundamentalData(TickerId reqId, const std::string& data) { }
void DefaultEWrapper::deltaNeutralValidation(int reqId, const DeltaNeutralContract& deltaNeutralContract) { }
void DefaultEWrapper::tickSnapshotEnd( int reqId) { }
void DefaultEWrapper::marketDataType( TickerId reqId, int marketDataType) { }
void DefaultEWrapper::commissionAndFeesReport( const CommissionAndFeesReport& commissionAndFeesReport) { }
void DefaultEWrapper::position( const std::string& account, const Contract& contract, Decimal position, double avgCost) { }
void DefaultEWrapper::positionEnd() { }
void DefaultEWrapper::accountSummary( int reqId, const std::string& account, const std::string& tag, const std::string& value, const std::string& currency) { }
void DefaultEWrapper::accountSummaryEnd( int reqId) { }
void DefaultEWrapper::verifyMessageAPI( const std::string& apiData) { }
void DefaultEWrapper::verifyCompleted( bool isSuccessful, const std::string& errorText) { }
void DefaultEWrapper::displayGroupList( int reqId, const std::string& groups) { }
void DefaultEWrapper::displayGroupUpdated( int reqId, const std::string& contractInfo) { }
void DefaultEWrapper::verifyAndAuthMessageAPI( const std::string& apiData, const std::string& xyzChallange) { }
void DefaultEWrapper::verifyAndAuthCompleted( bool isSuccessful, const std::string& errorText) { }
void DefaultEWrapper::connectAck() { }
void DefaultEWrapper::positionMulti( int reqId, const std::string& account,const std::string& modelCode, const Contract& contract, Decimal pos, double avgCost) { }
void DefaultEWrapper::positionMultiEnd( int reqId) { }
void DefaultEWrapper::accountUpdateMulti( int reqId, const std::string& account, const std::string& modelCode, const std::string& key, const std::string& value, const std::string& currency) { }
void DefaultEWrapper::accountUpdateMultiEnd( int reqId) { }
void DefaultEWrapper::securityDefinitionOptionalParameter(int reqId, const std::string& exchange, int underlyingConId, const std::string& tradingClass,
	const std::string& multiplier, const std::set<std::string>& expirations, const std::set<double>& strikes) { }
void DefaultEWrapper::securityDefinitionOptionalParameterEnd(int reqId) { }
void DefaultEWrapper::softDollarTiers(int reqId, const std::vector<SoftDollarTier> &tiers) { }
void DefaultEWrapper::familyCodes(const std::vector<FamilyCode> &familyCodes) { }
void DefaultEWrapper::symbolSamples(int reqId, const std::vector<ContractDescription> &contractDescriptions) { }
void DefaultEWrapper::mktDepthExchanges(const std::vector<DepthMktDataDescription> &depthMktDataDescriptions) { }
void DefaultEWrapper::tickNews(int tickerId, time_t timeStamp, const std::string& providerCode, const std::string& articleId, const std::string& headline, const std::string& extraData) { }
void DefaultEWrapper::smartComponents(int reqId, const SmartComponentsMap& theMap) { }
void DefaultEWrapper::tickReqParams(int tickerId, double minTick, const std::string& bboExchange, int snapshotPermissions) { }
void DefaultEWrapper::newsProviders(const std::vector<NewsProvider> &newsProviders) { }
void DefaultEWrapper::newsArticle(int requestId, int articleType, const std::string& articleText) { }
void DefaultEWrapper::historicalNews(int requestId, const std::string& time, const std::string& providerCode, const std::string& articleId, const std::string& headline) { }
void DefaultEWrapper::historicalNewsEnd(int requestId, bool hasMore) { }
void DefaultEWrapper::headTimestamp(int reqId, const std::string& headTimestamp) { }
void DefaultEWrapper::histogramData(int reqId, const HistogramDataVector& data) { }
void DefaultEWrapper::historicalDataUpdate(TickerId reqId, const Bar& bar) { }
void DefaultEWrapper::rerouteMktDataReq(int reqId, int conid, const std::string& exchange) { }
void DefaultEWrapper::rerouteMktDepthReq(int reqId, int conid, const std::string& exchange) { }
void DefaultEWrapper::marketRule(int marketRuleId, const std::vector<PriceIncrement> &priceIncrements) { }
void DefaultEWrapper::pnl(int reqId, double dailyPnL, double unrealizedPnL, double realizedPnL) { }
void DefaultEWrapper::pnlSingle(int reqId, Decimal pos, double dailyPnL, double unrealizedPnL, double realizedPnL, double value) { }
void DefaultEWrapper::historicalTicks(int reqId, const std::vector<HistoricalTick>& ticks, bool done) { }
void DefaultEWrapper::historicalTicksBidAsk(int reqId, const std::vector<HistoricalTickBidAsk>& ticks, bool done) { }
void DefaultEWrapper::historicalTicksLast(int reqId, const std::vector<HistoricalTickLast>& ticks, bool done) { }
void DefaultEWrapper::tickByTickAllLast(int reqId, int tickType, time_t time, double price, Decimal size, const TickAttribLast& tickAttribLast, const std::string& exchange, const std::string& specialConditions) { }
void DefaultEWrapper::tickByTickBidAsk(int reqId, time_t time, double bidPrice, double askPrice, Decimal bidSize, Decimal askSize, const TickAttribBidAsk& tickAttribBidAsk) { }
void DefaultEWrapper::tickByTickMidPoint(int reqId, time_t time, double midPoint) { }
void DefaultEWrapper::orderBound(long long permId, int clientId, int orderId) { }
void DefaultEWrapper::completedOrder(const Contract& contract, const Order& order, const OrderState& orderState) { }
void DefaultEWrapper::completedOrdersEnd() { }
void DefaultEWrapper::replaceFAEnd(int reqId, const std::string& text) { }
void DefaultEWrapper::wshMetaData(int reqId, const std::string& dataJson) { }
void DefaultEWrapper::wshEventData(int reqId, const std::string& dataJson) { }
void DefaultEWrapper::historicalSchedule(int reqId, const std::string& startDateTime, const std::string& endDateTime, const std::string& timeZone, const std::vector<HistoricalSession>& sessions) { }
void DefaultEWrapper::userInfo(int reqId, const std::string& whiteBrandingId) { }
void DefaultEWrapper::currentTimeInMillis(time_t timeInMillis) { }

// protobuf
#if !defined(USE_WIN_DLL)
void DefaultEWrapper::execDetailsProtoBuf(const protobuf::ExecutionDetails& executionDetailsProto) { }
void DefaultEWrapper::execDetailsEndProtoBuf(const protobuf::ExecutionDetailsEnd& executionDetailsEndProto) { }
void DefaultEWrapper::orderStatusProtoBuf(const protobuf::OrderStatus& orderStatusProto) { }
void DefaultEWrapper::openOrderProtoBuf(const protobuf::OpenOrder& openOrderProto) { }
void DefaultEWrapper::openOrdersEndProtoBuf(const protobuf::OpenOrdersEnd& openOrderEndProto) { }
void DefaultEWrapper::errorProtoBuf(const protobuf::ErrorMessage& errorProto) { }
void DefaultEWrapper::completedOrderProtoBuf(const protobuf::CompletedOrder& completedOrderProto) { }
void DefaultEWrapper::completedOrdersEndProtoBuf(const protobuf::CompletedOrdersEnd& completedOrdersEndProto) { }
void DefaultEWrapper::orderBoundProtoBuf(const protobuf::OrderBound& orderBoundProto) { }
void DefaultEWrapper::contractDataProtoBuf(const protobuf::ContractData& contractDataProto) { }
void DefaultEWrapper::bondContractDataProtoBuf(const protobuf::ContractData& contractDataProto) { }
void DefaultEWrapper::contractDataEndProtoBuf(const protobuf::ContractDataEnd& contractDataEndProto) { }
void DefaultEWrapper::tickPriceProtoBuf(const protobuf::TickPrice& tickPriceProto) { }
void DefaultEWrapper::tickSizeProtoBuf(const protobuf::TickSize& tickSizeProto) { }
void DefaultEWrapper::tickOptionComputationProtoBuf(const protobuf::TickOptionComputation& tickOptionComputationProto) { }
void DefaultEWrapper::tickGenericProtoBuf(const protobuf::TickGeneric& tickGenericProto) { }
void DefaultEWrapper::tickStringProtoBuf(const protobuf::TickString& tickStringProto) { }
void DefaultEWrapper::tickSnapshotEndProtoBuf(const protobuf::TickSnapshotEnd& tickSnapshotEndProto) { }
void DefaultEWrapper::updateMarketDepthProtoBuf(const protobuf::MarketDepth& marketDepthProto) { }
void DefaultEWrapper::updateMarketDepthL2ProtoBuf(const protobuf::MarketDepthL2& marketDepthL2Proto) { }
void DefaultEWrapper::marketDataTypeProtoBuf(const protobuf::MarketDataType& marketDataTypeProto) { }
void DefaultEWrapper::tickReqParamsProtoBuf(const protobuf::TickReqParams& tickReqParamnsProto) { }
void DefaultEWrapper::updateAccountValueProtoBuf(const protobuf::AccountValue& accountValueProto) { }
void DefaultEWrapper::updatePortfolioProtoBuf(const protobuf::PortfolioValue& portfolioValueProto) { }
void DefaultEWrapper::updateAccountTimeProtoBuf(const protobuf::AccountUpdateTime& accountUpdateTimeProto) { }
void DefaultEWrapper::accountDataEndProtoBuf(const protobuf::AccountDataEnd& accountDataEndProto) { }
void DefaultEWrapper::managedAccountsProtoBuf(const protobuf::ManagedAccounts& managedAccountsProto) { }
void DefaultEWrapper::positionProtoBuf(const protobuf::Position& positionProto) { }
void DefaultEWrapper::positionEndProtoBuf(const protobuf::PositionEnd& positionEndProto) { }
void DefaultEWrapper::accountSummaryProtoBuf(const protobuf::AccountSummary& accountSummaryProto) { }
void DefaultEWrapper::accountSummaryEndProtoBuf(const protobuf::AccountSummaryEnd& accountSummaryEndProto) { }
void DefaultEWrapper::positionMultiProtoBuf(const protobuf::PositionMulti& positionMultiProto) { }
void DefaultEWrapper::positionMultiEndProtoBuf(const protobuf::PositionMultiEnd& positionMultiEndProto) { }
void DefaultEWrapper::accountUpdateMultiProtoBuf(const protobuf::AccountUpdateMulti& accountUpdateMultiProto) { }
void DefaultEWrapper::accountUpdateMultiEndProtoBuf(const protobuf::AccountUpdateMultiEnd& accountUpdateMultiEndProto) { }
void DefaultEWrapper::historicalDataProtoBuf(const protobuf::HistoricalData& historicalDataProto) { }
void DefaultEWrapper::historicalDataUpdateProtoBuf(const protobuf::HistoricalDataUpdate& historicalDataUpdateProto) { }
void DefaultEWrapper::historicalDataEndProtoBuf(const protobuf::HistoricalDataEnd& historicalDataEndProto) { }
void DefaultEWrapper::realTimeBarTickProtoBuf(const protobuf::RealTimeBarTick& realTimeBarTickProto) { }
void DefaultEWrapper::headTimestampProtoBuf(const protobuf::HeadTimestamp& headTimestampProto) { }
void DefaultEWrapper::histogramDataProtoBuf(const protobuf::HistogramData& histogramDataProto) { }
void DefaultEWrapper::historicalTicksProtoBuf(const protobuf::HistoricalTicks& historicalTicksProto) { }
void DefaultEWrapper::historicalTicksBidAskProtoBuf(const protobuf::HistoricalTicksBidAsk& historicalTicksBidAskProto) { }
void DefaultEWrapper::historicalTicksLastProtoBuf(const protobuf::HistoricalTicksLast& historicalTicksLastProto) { }
void DefaultEWrapper::tickByTickDataProtoBuf(const protobuf::TickByTickData& tickByTickDataProto) { }
void DefaultEWrapper::updateNewsBulletinProtoBuf(const protobuf::NewsBulletin& newsBulletinProto) { }
void DefaultEWrapper::newsArticleProtoBuf(const protobuf::NewsArticle& newsArticleProto) { }
void DefaultEWrapper::newsProvidersProtoBuf(const protobuf::NewsProviders& newsProvidersProto) { }
void DefaultEWrapper::historicalNewsProtoBuf(const protobuf::HistoricalNews& historicalNewsProto) { }
void DefaultEWrapper::historicalNewsEndProtoBuf(const protobuf::HistoricalNewsEnd& historicalNewsEndProto) { }
void DefaultEWrapper::wshMetaDataProtoBuf(const protobuf::WshMetaData& wshMetaDataProto) { }
void DefaultEWrapper::wshEventDataProtoBuf(const protobuf::WshEventData& wshEventDataProto) { }
void DefaultEWrapper::tickNewsProtoBuf(const protobuf::TickNews& tickNewsProto) { }
void DefaultEWrapper::scannerParametersProtoBuf(const protobuf::ScannerParameters& scannerParametersProto) { }
void DefaultEWrapper::scannerDataProtoBuf(const protobuf::ScannerData& scannerDataProto) { }
void DefaultEWrapper::fundamentalsDataProtoBuf(const protobuf::FundamentalsData& fundamentalsDataProto) { }
void DefaultEWrapper::pnlProtoBuf(const protobuf::PnL& pnlProto) { }
void DefaultEWrapper::pnlSingleProtoBuf(const protobuf::PnLSingle& pnlSingleProto) { }
void DefaultEWrapper::receiveFAProtoBuf(const protobuf::ReceiveFA& receiveFAProto) { }
void DefaultEWrapper::replaceFAEndProtoBuf(const protobuf::ReplaceFAEnd& replaceFAEndProto) { }
void DefaultEWrapper::commissionAndFeesReportProtoBuf(const protobuf::CommissionAndFeesReport& commissionAndFeesReportProto) { }
void DefaultEWrapper::historicalScheduleProtoBuf(const protobuf::HistoricalSchedule& historicalScheduleProto) { }
void DefaultEWrapper::rerouteMarketDataRequestProtoBuf(const protobuf::RerouteMarketDataRequest& rerouteMarketDataRequestProto) { }
void DefaultEWrapper::rerouteMarketDepthRequestProtoBuf(const protobuf::RerouteMarketDepthRequest& rerouteMarketDepthRequestProto) { }
void DefaultEWrapper::secDefOptParameterProtoBuf(const protobuf::SecDefOptParameter& secDefOptParameterProto) { }
void DefaultEWrapper::secDefOptParameterEndProtoBuf(const protobuf::SecDefOptParameterEnd& secDefOptParameterEndProto) { }
void DefaultEWrapper::softDollarTiersProtoBuf(const protobuf::SoftDollarTiers& softDollarTiersProto) { }
void DefaultEWrapper::familyCodesProtoBuf(const protobuf::FamilyCodes& familyCodesProto) { }
void DefaultEWrapper::symbolSamplesProtoBuf(const protobuf::SymbolSamples& symbolSamplesProto) { }
void DefaultEWrapper::smartComponentsProtoBuf(const protobuf::SmartComponents& smartComponentsProto) { }
void DefaultEWrapper::marketRuleProtoBuf(const protobuf::MarketRule& marketRuleProto) { }
void DefaultEWrapper::userInfoProtoBuf(const protobuf::UserInfo& userInfoProto) {}
void DefaultEWrapper::nextValidIdProtoBuf(const protobuf::NextValidId& nextValidIdProto) {}
void DefaultEWrapper::currentTimeProtoBuf(const protobuf::CurrentTime& currentTimeProto) {}
void DefaultEWrapper::currentTimeInMillisProtoBuf(const protobuf::CurrentTimeInMillis& currentTimeInMillisProto) {}
void DefaultEWrapper::verifyMessageApiProtoBuf(const protobuf::VerifyMessageApi& verifyMessageApiProto) {}
void DefaultEWrapper::verifyCompletedProtoBuf(const protobuf::VerifyCompleted& verifyCompletedProto) {}
void DefaultEWrapper::displayGroupListProtoBuf(const protobuf::DisplayGroupList& displayGroupListProto) {}
void DefaultEWrapper::displayGroupUpdatedProtoBuf(const protobuf::DisplayGroupUpdated& displayGroupUpdatedProto) {}
void DefaultEWrapper::marketDepthExchangesProtoBuf(const protobuf::MarketDepthExchanges& marketDepthExchangesProto) {}
#endif