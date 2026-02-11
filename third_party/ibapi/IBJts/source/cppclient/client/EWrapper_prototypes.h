/* Copyright (C) 2025 Interactive Brokers LLC. All rights reserved. This code is subject to the terms
 * and conditions of the IB API Non-Commercial License or the IB API Commercial License, as applicable. */

/* not using "#pragma once" on purpose! */

#if ! defined(EWRAPPER_VIRTUAL_IMPL)
# define EWRAPPER_VIRTUAL_IMPL
#endif

virtual void tickPrice( TickerId tickerId, TickType field, double price, const TickAttrib& attrib) EWRAPPER_VIRTUAL_IMPL;
virtual void tickSize(TickerId tickerId, TickType field, Decimal size) EWRAPPER_VIRTUAL_IMPL;
virtual void tickOptionComputation( TickerId tickerId, TickType tickType, int tickAttrib, double impliedVol, double delta,
	double optPrice, double pvDividend, double gamma, double vega, double theta, double undPrice) EWRAPPER_VIRTUAL_IMPL;
virtual void tickGeneric(TickerId tickerId, TickType tickType, double value) EWRAPPER_VIRTUAL_IMPL;
virtual void tickString(TickerId tickerId, TickType tickType, const std::string& value) EWRAPPER_VIRTUAL_IMPL;
virtual void tickEFP(TickerId tickerId, TickType tickType, double basisPoints, const std::string& formattedBasisPoints,
	double totalDividends, int holdDays, const std::string& futureLastTradeDate, double dividendImpact, double dividendsToLastTradeDate) EWRAPPER_VIRTUAL_IMPL;
virtual void orderStatus( OrderId orderId, const std::string& status, Decimal filled,
	Decimal remaining, double avgFillPrice, long long permId, int parentId,
	double lastFillPrice, int clientId, const std::string& whyHeld, double mktCapPrice) EWRAPPER_VIRTUAL_IMPL;
virtual void openOrder( OrderId orderId, const Contract&, const Order&, const OrderState&) EWRAPPER_VIRTUAL_IMPL;
virtual void openOrderEnd() EWRAPPER_VIRTUAL_IMPL;
virtual void winError( const std::string& str, int lastError) EWRAPPER_VIRTUAL_IMPL;
virtual void connectionClosed() EWRAPPER_VIRTUAL_IMPL;
virtual void updateAccountValue(const std::string& key, const std::string& val,
const std::string& currency, const std::string& accountName) EWRAPPER_VIRTUAL_IMPL;
virtual void updatePortfolio( const Contract& contract, Decimal position,
	double marketPrice, double marketValue, double averageCost,
	double unrealizedPNL, double realizedPNL, const std::string& accountName) EWRAPPER_VIRTUAL_IMPL;
virtual void updateAccountTime(const std::string& timeStamp) EWRAPPER_VIRTUAL_IMPL;
virtual void accountDownloadEnd(const std::string& accountName) EWRAPPER_VIRTUAL_IMPL;
virtual void nextValidId( OrderId orderId) EWRAPPER_VIRTUAL_IMPL;
virtual void contractDetails( int reqId, const ContractDetails& contractDetails) EWRAPPER_VIRTUAL_IMPL;
virtual void bondContractDetails( int reqId, const ContractDetails& contractDetails) EWRAPPER_VIRTUAL_IMPL;
virtual void contractDetailsEnd( int reqId) EWRAPPER_VIRTUAL_IMPL;
virtual void execDetails( int reqId, const Contract& contract, const Execution& execution) EWRAPPER_VIRTUAL_IMPL;
virtual void execDetailsEnd( int reqId) EWRAPPER_VIRTUAL_IMPL;
virtual void error(int id, time_t errorTime, int errorCode,const std::string& errorString, const std::string& advancedOrderRejectJson) EWRAPPER_VIRTUAL_IMPL;
virtual void updateMktDepth(TickerId id, int position, int operation, int side,
	double price, Decimal size) EWRAPPER_VIRTUAL_IMPL;
virtual void updateMktDepthL2(TickerId id, int position, const std::string& marketMaker, int operation,
	int side, double price, Decimal size, bool isSmartDepth) EWRAPPER_VIRTUAL_IMPL;
virtual void updateNewsBulletin(int msgId, int msgType, const std::string& newsMessage, const std::string& originExch) EWRAPPER_VIRTUAL_IMPL;
virtual void managedAccounts( const std::string& accountsList) EWRAPPER_VIRTUAL_IMPL;
virtual void receiveFA(faDataType pFaDataType, const std::string& cxml) EWRAPPER_VIRTUAL_IMPL;
virtual void historicalData(TickerId reqId, const Bar& bar) EWRAPPER_VIRTUAL_IMPL;
virtual void historicalDataEnd(int reqId, const std::string& startDateStr, const std::string& endDateStr) EWRAPPER_VIRTUAL_IMPL;
virtual void scannerParameters(const std::string& xml) EWRAPPER_VIRTUAL_IMPL;
virtual void scannerData(int reqId, int rank, const ContractDetails& contractDetails,
	const std::string& distance, const std::string& benchmark, const std::string& projection,
	const std::string& legsStr) EWRAPPER_VIRTUAL_IMPL;
virtual void scannerDataEnd(int reqId) EWRAPPER_VIRTUAL_IMPL;
virtual void realtimeBar(TickerId reqId, long time, double open, double high, double low, double close,
	Decimal volume, Decimal wap, int count) EWRAPPER_VIRTUAL_IMPL;
virtual void currentTime(long time) EWRAPPER_VIRTUAL_IMPL;
virtual void fundamentalData(TickerId reqId, const std::string& data) EWRAPPER_VIRTUAL_IMPL;
virtual void deltaNeutralValidation(int reqId, const DeltaNeutralContract& deltaNeutralContract) EWRAPPER_VIRTUAL_IMPL;
virtual void tickSnapshotEnd( int reqId) EWRAPPER_VIRTUAL_IMPL;
virtual void marketDataType( TickerId reqId, int marketDataType) EWRAPPER_VIRTUAL_IMPL;
virtual void commissionAndFeesReport( const CommissionAndFeesReport& commissionAndFeesReport) EWRAPPER_VIRTUAL_IMPL;
virtual void position( const std::string& account, const Contract& contract, Decimal position, double avgCost) EWRAPPER_VIRTUAL_IMPL;
virtual void positionEnd() EWRAPPER_VIRTUAL_IMPL;
virtual void accountSummary( int reqId, const std::string& account, const std::string& tag, const std::string& value, const std::string& currency) EWRAPPER_VIRTUAL_IMPL;
virtual void accountSummaryEnd( int reqId) EWRAPPER_VIRTUAL_IMPL;
virtual void verifyMessageAPI( const std::string& apiData) EWRAPPER_VIRTUAL_IMPL;
virtual void verifyCompleted( bool isSuccessful, const std::string& errorText) EWRAPPER_VIRTUAL_IMPL;
virtual void displayGroupList( int reqId, const std::string& groups) EWRAPPER_VIRTUAL_IMPL;
virtual void displayGroupUpdated( int reqId, const std::string& contractInfo) EWRAPPER_VIRTUAL_IMPL;
virtual void verifyAndAuthMessageAPI( const std::string& apiData, const std::string& xyzChallange) EWRAPPER_VIRTUAL_IMPL;
virtual void verifyAndAuthCompleted( bool isSuccessful, const std::string& errorText) EWRAPPER_VIRTUAL_IMPL;
virtual void connectAck() EWRAPPER_VIRTUAL_IMPL;
virtual void positionMulti( int reqId, const std::string& account,const std::string& modelCode, const Contract& contract, Decimal pos, double avgCost) EWRAPPER_VIRTUAL_IMPL;
virtual void positionMultiEnd( int reqId) EWRAPPER_VIRTUAL_IMPL;
virtual void accountUpdateMulti( int reqId, const std::string& account, const std::string& modelCode, const std::string& key, const std::string& value, const std::string& currency) EWRAPPER_VIRTUAL_IMPL;
virtual void accountUpdateMultiEnd( int reqId) EWRAPPER_VIRTUAL_IMPL;
virtual void securityDefinitionOptionalParameter(int reqId, const std::string& exchange, int underlyingConId, const std::string& tradingClass,
	const std::string& multiplier, const std::set<std::string>& expirations, const std::set<double>& strikes) EWRAPPER_VIRTUAL_IMPL;
virtual void securityDefinitionOptionalParameterEnd(int reqId) EWRAPPER_VIRTUAL_IMPL;
virtual void softDollarTiers(int reqId, const std::vector<SoftDollarTier> &tiers) EWRAPPER_VIRTUAL_IMPL;
virtual void familyCodes(const std::vector<FamilyCode> &familyCodes) EWRAPPER_VIRTUAL_IMPL;
virtual void symbolSamples(int reqId, const std::vector<ContractDescription> &contractDescriptions) EWRAPPER_VIRTUAL_IMPL;
virtual void mktDepthExchanges(const std::vector<DepthMktDataDescription> &depthMktDataDescriptions) EWRAPPER_VIRTUAL_IMPL;
virtual void tickNews(int tickerId, time_t timeStamp, const std::string& providerCode, const std::string& articleId, const std::string& headline, const std::string& extraData) EWRAPPER_VIRTUAL_IMPL;
virtual void smartComponents(int reqId, const SmartComponentsMap& theMap) EWRAPPER_VIRTUAL_IMPL;
virtual void tickReqParams(int tickerId, double minTick, const std::string& bboExchange, int snapshotPermissions) EWRAPPER_VIRTUAL_IMPL;
virtual void newsProviders(const std::vector<NewsProvider> &newsProviders) EWRAPPER_VIRTUAL_IMPL;
virtual void newsArticle(int requestId, int articleType, const std::string& articleText) EWRAPPER_VIRTUAL_IMPL;
virtual void historicalNews(int requestId, const std::string& time, const std::string& providerCode, const std::string& articleId, const std::string& headline) EWRAPPER_VIRTUAL_IMPL;
virtual void historicalNewsEnd(int requestId, bool hasMore) EWRAPPER_VIRTUAL_IMPL;
virtual void headTimestamp(int reqId, const std::string& headTimestamp) EWRAPPER_VIRTUAL_IMPL;
virtual void histogramData(int reqId, const HistogramDataVector& data) EWRAPPER_VIRTUAL_IMPL;
virtual void historicalDataUpdate(TickerId reqId, const Bar& bar) EWRAPPER_VIRTUAL_IMPL;
virtual void rerouteMktDataReq(int reqId, int conid, const std::string& exchange) EWRAPPER_VIRTUAL_IMPL;
virtual void rerouteMktDepthReq(int reqId, int conid, const std::string& exchange) EWRAPPER_VIRTUAL_IMPL;
virtual void marketRule(int marketRuleId, const std::vector<PriceIncrement> &priceIncrements) EWRAPPER_VIRTUAL_IMPL;
virtual void pnl(int reqId, double dailyPnL, double unrealizedPnL, double realizedPnL) EWRAPPER_VIRTUAL_IMPL;
virtual void pnlSingle(int reqId, Decimal pos, double dailyPnL, double unrealizedPnL, double realizedPnL, double value) EWRAPPER_VIRTUAL_IMPL;
virtual void historicalTicks(int reqId, const std::vector<HistoricalTick> &ticks, bool done) EWRAPPER_VIRTUAL_IMPL;
virtual void historicalTicksBidAsk(int reqId, const std::vector<HistoricalTickBidAsk> &ticks, bool done) EWRAPPER_VIRTUAL_IMPL;
virtual void historicalTicksLast(int reqId, const std::vector<HistoricalTickLast> &ticks, bool done) EWRAPPER_VIRTUAL_IMPL;
virtual void tickByTickAllLast(int reqId, int tickType, time_t time, double price, Decimal size, const TickAttribLast& tickAttribLast, const std::string& exchange, const std::string& specialConditions) EWRAPPER_VIRTUAL_IMPL;
virtual void tickByTickBidAsk(int reqId, time_t time, double bidPrice, double askPrice, Decimal bidSize, Decimal askSize, const TickAttribBidAsk& tickAttribBidAsk) EWRAPPER_VIRTUAL_IMPL;
virtual void tickByTickMidPoint(int reqId, time_t time, double midPoint) EWRAPPER_VIRTUAL_IMPL;
virtual void orderBound(long long permId, int clientId, int orderId) EWRAPPER_VIRTUAL_IMPL;
virtual void completedOrder(const Contract& contract, const Order& order, const OrderState& orderState) EWRAPPER_VIRTUAL_IMPL;
virtual void completedOrdersEnd() EWRAPPER_VIRTUAL_IMPL;
virtual void replaceFAEnd(int reqId, const std::string& text) EWRAPPER_VIRTUAL_IMPL;
virtual void wshMetaData(int reqId, const std::string& dataJson) EWRAPPER_VIRTUAL_IMPL;
virtual void wshEventData(int reqId, const std::string& dataJson) EWRAPPER_VIRTUAL_IMPL;
virtual void historicalSchedule(int reqId, const std::string& startDateTime, const std::string& endDateTime, const std::string& timeZone, const std::vector<HistoricalSession>& sessions) EWRAPPER_VIRTUAL_IMPL;
virtual void userInfo(int reqId, const std::string& whiteBrandingId) EWRAPPER_VIRTUAL_IMPL;
virtual void currentTimeInMillis(time_t timeInMillis) EWRAPPER_VIRTUAL_IMPL;

// protobuf
#if !defined(USE_WIN_DLL)
virtual void execDetailsProtoBuf(const protobuf::ExecutionDetails& executionDetailsProto) EWRAPPER_VIRTUAL_IMPL;
virtual void execDetailsEndProtoBuf(const protobuf::ExecutionDetailsEnd& executionDetailsEndProto) EWRAPPER_VIRTUAL_IMPL;
virtual void orderStatusProtoBuf(const protobuf::OrderStatus& orderStatusProto) EWRAPPER_VIRTUAL_IMPL;
virtual void openOrderProtoBuf(const protobuf::OpenOrder& openOrderProto) EWRAPPER_VIRTUAL_IMPL;
virtual void openOrdersEndProtoBuf(const protobuf::OpenOrdersEnd& openOrderEndProto) EWRAPPER_VIRTUAL_IMPL;
virtual void errorProtoBuf(const protobuf::ErrorMessage& errorProto) EWRAPPER_VIRTUAL_IMPL;
virtual void completedOrderProtoBuf(const protobuf::CompletedOrder& completedOrderProto) EWRAPPER_VIRTUAL_IMPL;
virtual void completedOrdersEndProtoBuf(const protobuf::CompletedOrdersEnd& completedOrdersEndProto) EWRAPPER_VIRTUAL_IMPL;
virtual void orderBoundProtoBuf(const protobuf::OrderBound& orderBoundProto) EWRAPPER_VIRTUAL_IMPL;
virtual void contractDataProtoBuf(const protobuf::ContractData& contractDataProto) EWRAPPER_VIRTUAL_IMPL;
virtual void bondContractDataProtoBuf(const protobuf::ContractData& contractDataProto) EWRAPPER_VIRTUAL_IMPL;
virtual void contractDataEndProtoBuf(const protobuf::ContractDataEnd& contractDataEndProto) EWRAPPER_VIRTUAL_IMPL;
virtual void tickPriceProtoBuf(const protobuf::TickPrice& tickPriceProto) EWRAPPER_VIRTUAL_IMPL;
virtual void tickSizeProtoBuf(const protobuf::TickSize& tickSizeProto) EWRAPPER_VIRTUAL_IMPL;
virtual void tickOptionComputationProtoBuf(const protobuf::TickOptionComputation& tickOptionComputationProto) EWRAPPER_VIRTUAL_IMPL;
virtual void tickGenericProtoBuf(const protobuf::TickGeneric& tickGenericProto) EWRAPPER_VIRTUAL_IMPL;
virtual void tickStringProtoBuf(const protobuf::TickString& tickStringProto) EWRAPPER_VIRTUAL_IMPL;
virtual void tickSnapshotEndProtoBuf(const protobuf::TickSnapshotEnd& tickSnapshotEndProto) EWRAPPER_VIRTUAL_IMPL;
virtual void updateMarketDepthProtoBuf(const protobuf::MarketDepth& marketDepthProto) EWRAPPER_VIRTUAL_IMPL;
virtual void updateMarketDepthL2ProtoBuf(const protobuf::MarketDepthL2& marketDepthL2Proto) EWRAPPER_VIRTUAL_IMPL;
virtual void marketDataTypeProtoBuf(const protobuf::MarketDataType& marketDataTypeProto) EWRAPPER_VIRTUAL_IMPL;
virtual void tickReqParamsProtoBuf(const protobuf::TickReqParams& tickReqParamsProto) EWRAPPER_VIRTUAL_IMPL;
virtual void updateAccountValueProtoBuf(const protobuf::AccountValue& accountValueProto) EWRAPPER_VIRTUAL_IMPL;
virtual void updatePortfolioProtoBuf(const protobuf::PortfolioValue& portfolioValueProto) EWRAPPER_VIRTUAL_IMPL;
virtual void updateAccountTimeProtoBuf(const protobuf::AccountUpdateTime& accountUpdateTimeProto) EWRAPPER_VIRTUAL_IMPL;
virtual void accountDataEndProtoBuf(const protobuf::AccountDataEnd& accountDataEndProto) EWRAPPER_VIRTUAL_IMPL;
virtual void managedAccountsProtoBuf(const protobuf::ManagedAccounts& managedAccountsProto) EWRAPPER_VIRTUAL_IMPL;
virtual void positionProtoBuf(const protobuf::Position& positionProto) EWRAPPER_VIRTUAL_IMPL;
virtual void positionEndProtoBuf(const protobuf::PositionEnd& positionEndProto) EWRAPPER_VIRTUAL_IMPL;
virtual void accountSummaryProtoBuf(const protobuf::AccountSummary& accountSummaryProto) EWRAPPER_VIRTUAL_IMPL;
virtual void accountSummaryEndProtoBuf(const protobuf::AccountSummaryEnd& accountSummaryEndProto) EWRAPPER_VIRTUAL_IMPL;
virtual void positionMultiProtoBuf(const protobuf::PositionMulti& positionMultiProto) EWRAPPER_VIRTUAL_IMPL;
virtual void positionMultiEndProtoBuf(const protobuf::PositionMultiEnd& positionMultiEndProto) EWRAPPER_VIRTUAL_IMPL;
virtual void accountUpdateMultiProtoBuf(const protobuf::AccountUpdateMulti& accountUpdateMultiProto) EWRAPPER_VIRTUAL_IMPL;
virtual void accountUpdateMultiEndProtoBuf(const protobuf::AccountUpdateMultiEnd& accountUpdateMultiEndProto) EWRAPPER_VIRTUAL_IMPL;
virtual void historicalDataProtoBuf(const protobuf::HistoricalData& historicalDataProto) EWRAPPER_VIRTUAL_IMPL;
virtual void historicalDataUpdateProtoBuf(const protobuf::HistoricalDataUpdate& historicalDataUpdateProto) EWRAPPER_VIRTUAL_IMPL;
virtual void historicalDataEndProtoBuf(const protobuf::HistoricalDataEnd& historicalDataEndProto) EWRAPPER_VIRTUAL_IMPL;
virtual void realTimeBarTickProtoBuf(const protobuf::RealTimeBarTick& realTimeBarTickProto) EWRAPPER_VIRTUAL_IMPL;
virtual void headTimestampProtoBuf(const protobuf::HeadTimestamp& headTimestampProto) EWRAPPER_VIRTUAL_IMPL;
virtual void histogramDataProtoBuf(const protobuf::HistogramData& histogramDataProto) EWRAPPER_VIRTUAL_IMPL;
virtual void historicalTicksProtoBuf(const protobuf::HistoricalTicks& historicalTicksProto) EWRAPPER_VIRTUAL_IMPL;
virtual void historicalTicksBidAskProtoBuf(const protobuf::HistoricalTicksBidAsk& historicalTicksBidAskProto) EWRAPPER_VIRTUAL_IMPL;
virtual void historicalTicksLastProtoBuf(const protobuf::HistoricalTicksLast& historicalTicksLastProto) EWRAPPER_VIRTUAL_IMPL;
virtual void tickByTickDataProtoBuf(const protobuf::TickByTickData& tickByTickDataProto) EWRAPPER_VIRTUAL_IMPL;
virtual void updateNewsBulletinProtoBuf(const protobuf::NewsBulletin& newsBulletinProto) EWRAPPER_VIRTUAL_IMPL;
virtual void newsArticleProtoBuf(const protobuf::NewsArticle& newsArticleProto) EWRAPPER_VIRTUAL_IMPL;
virtual void newsProvidersProtoBuf(const protobuf::NewsProviders& newsProvidersProto) EWRAPPER_VIRTUAL_IMPL;
virtual void historicalNewsProtoBuf(const protobuf::HistoricalNews& historicalNewsProto) EWRAPPER_VIRTUAL_IMPL;
virtual void historicalNewsEndProtoBuf(const protobuf::HistoricalNewsEnd& historicalNewsEndProto) EWRAPPER_VIRTUAL_IMPL;
virtual void wshMetaDataProtoBuf(const protobuf::WshMetaData& wshMetaDataProto) EWRAPPER_VIRTUAL_IMPL;
virtual void wshEventDataProtoBuf(const protobuf::WshEventData& wshEventDataProto) EWRAPPER_VIRTUAL_IMPL;
virtual void tickNewsProtoBuf(const protobuf::TickNews& tickNewsProto) EWRAPPER_VIRTUAL_IMPL;
virtual void scannerParametersProtoBuf(const protobuf::ScannerParameters& scannerParametersProto) EWRAPPER_VIRTUAL_IMPL;
virtual void scannerDataProtoBuf(const protobuf::ScannerData& scannerDataProto) EWRAPPER_VIRTUAL_IMPL;
virtual void fundamentalsDataProtoBuf(const protobuf::FundamentalsData& fundamentalsDataProto) EWRAPPER_VIRTUAL_IMPL;
virtual void pnlProtoBuf(const protobuf::PnL& pnlProto) EWRAPPER_VIRTUAL_IMPL;
virtual void pnlSingleProtoBuf(const protobuf::PnLSingle& pnlSingleProto) EWRAPPER_VIRTUAL_IMPL;
virtual void receiveFAProtoBuf(const protobuf::ReceiveFA& receiveFAProto) EWRAPPER_VIRTUAL_IMPL;
virtual void replaceFAEndProtoBuf(const protobuf::ReplaceFAEnd& replaceFAEndProto) EWRAPPER_VIRTUAL_IMPL;
virtual void commissionAndFeesReportProtoBuf(const protobuf::CommissionAndFeesReport& commissionAndFeesReportProto) EWRAPPER_VIRTUAL_IMPL;
virtual void historicalScheduleProtoBuf(const protobuf::HistoricalSchedule& historicalScheduleProto) EWRAPPER_VIRTUAL_IMPL;
virtual void rerouteMarketDataRequestProtoBuf(const protobuf::RerouteMarketDataRequest& rerouteMarketDataRequestProto) EWRAPPER_VIRTUAL_IMPL;
virtual void rerouteMarketDepthRequestProtoBuf(const protobuf::RerouteMarketDepthRequest& rerouteMarketDepthRequestProto) EWRAPPER_VIRTUAL_IMPL;
virtual void secDefOptParameterProtoBuf(const protobuf::SecDefOptParameter& secDefOptParameterProto) EWRAPPER_VIRTUAL_IMPL;
virtual void secDefOptParameterEndProtoBuf(const protobuf::SecDefOptParameterEnd& secDefOptParameterEndProto) EWRAPPER_VIRTUAL_IMPL;
virtual void softDollarTiersProtoBuf(const protobuf::SoftDollarTiers& softDollarTiersProto) EWRAPPER_VIRTUAL_IMPL;
virtual void familyCodesProtoBuf(const protobuf::FamilyCodes& familyCodesProto) EWRAPPER_VIRTUAL_IMPL;
virtual void symbolSamplesProtoBuf(const protobuf::SymbolSamples& symbolSamplesProto) EWRAPPER_VIRTUAL_IMPL;
virtual void smartComponentsProtoBuf(const protobuf::SmartComponents& smartComponentsProto) EWRAPPER_VIRTUAL_IMPL;
virtual void marketRuleProtoBuf(const protobuf::MarketRule& marketRuleProto) EWRAPPER_VIRTUAL_IMPL;
virtual void userInfoProtoBuf(const protobuf::UserInfo& userInfoProto) EWRAPPER_VIRTUAL_IMPL;
virtual void nextValidIdProtoBuf(const protobuf::NextValidId& nextValidIdProto) EWRAPPER_VIRTUAL_IMPL;
virtual void currentTimeProtoBuf(const protobuf::CurrentTime& currentTimeProto) EWRAPPER_VIRTUAL_IMPL;
virtual void currentTimeInMillisProtoBuf(const protobuf::CurrentTimeInMillis& currentTimeInMillisProto) EWRAPPER_VIRTUAL_IMPL;
virtual void verifyMessageApiProtoBuf(const protobuf::VerifyMessageApi& verifyMessageApiProto) EWRAPPER_VIRTUAL_IMPL;
virtual void verifyCompletedProtoBuf(const protobuf::VerifyCompleted& verifyCompletedProto) EWRAPPER_VIRTUAL_IMPL;
virtual void displayGroupListProtoBuf(const protobuf::DisplayGroupList& displayGroupListProto) EWRAPPER_VIRTUAL_IMPL;
virtual void displayGroupUpdatedProtoBuf(const protobuf::DisplayGroupUpdated& displayGroupUpdatedProto) EWRAPPER_VIRTUAL_IMPL;
virtual void marketDepthExchangesProtoBuf(const protobuf::MarketDepthExchanges& marketDepthExchangesProto) EWRAPPER_VIRTUAL_IMPL;
#endif

#undef EWRAPPER_VIRTUAL_IMPL
