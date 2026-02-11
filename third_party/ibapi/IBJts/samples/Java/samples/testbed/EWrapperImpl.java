/* Copyright (C) 2025 Interactive Brokers LLC. All rights reserved. This code is subject to the terms
 * and conditions of the IB API Non-Commercial License or the IB API Commercial License, as applicable. */

package samples.testbed;

import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Set;

import com.ib.client.*;
import com.ib.client.protobuf.AccountDataEndProto;
import com.ib.client.protobuf.AccountSummaryEndProto;
import com.ib.client.protobuf.AccountSummaryProto;
import com.ib.client.protobuf.AccountUpdateMultiEndProto;
import com.ib.client.protobuf.AccountUpdateMultiProto;
import com.ib.client.protobuf.AccountUpdateTimeProto;
import com.ib.client.protobuf.AccountValueProto;
import com.ib.client.protobuf.CommissionAndFeesReportProto;
import com.ib.client.protobuf.CompletedOrderProto;
import com.ib.client.protobuf.CompletedOrdersEndProto;
import com.ib.client.protobuf.ContractDataEndProto;
import com.ib.client.protobuf.ContractDataProto;
import com.ib.client.protobuf.CurrentTimeInMillisProto;
import com.ib.client.protobuf.CurrentTimeProto;
import com.ib.client.protobuf.DisplayGroupListProto;
import com.ib.client.protobuf.DisplayGroupUpdatedProto;
import com.ib.client.protobuf.ErrorMessageProto;
import com.ib.client.protobuf.ExecutionDetailsEndProto;
import com.ib.client.protobuf.ExecutionDetailsProto;
import com.ib.client.protobuf.FamilyCodesProto;
import com.ib.client.protobuf.FundamentalsDataProto;
import com.ib.client.protobuf.HeadTimestampProto;
import com.ib.client.protobuf.HistogramDataProto;
import com.ib.client.protobuf.HistoricalDataEndProto;
import com.ib.client.protobuf.HistoricalDataProto;
import com.ib.client.protobuf.HistoricalDataUpdateProto;
import com.ib.client.protobuf.HistoricalNewsEndProto;
import com.ib.client.protobuf.HistoricalNewsProto;
import com.ib.client.protobuf.HistoricalScheduleProto;
import com.ib.client.protobuf.HistoricalTicksBidAskProto;
import com.ib.client.protobuf.HistoricalTicksLastProto;
import com.ib.client.protobuf.HistoricalTicksProto;
import com.ib.client.protobuf.ManagedAccountsProto;
import com.ib.client.protobuf.MarketDataTypeProto;
import com.ib.client.protobuf.MarketDepthExchangesProto;
import com.ib.client.protobuf.MarketDepthL2Proto;
import com.ib.client.protobuf.MarketDepthProto;
import com.ib.client.protobuf.MarketRuleProto;
import com.ib.client.protobuf.NewsArticleProto;
import com.ib.client.protobuf.NewsBulletinProto;
import com.ib.client.protobuf.NewsProvidersProto;
import com.ib.client.protobuf.NextValidIdProto;
import com.ib.client.protobuf.OpenOrderProto;
import com.ib.client.protobuf.OpenOrdersEndProto;
import com.ib.client.protobuf.OrderBoundProto;
import com.ib.client.protobuf.OrderStatusProto;
import com.ib.client.protobuf.PnLProto;
import com.ib.client.protobuf.PnLSingleProto;
import com.ib.client.protobuf.PortfolioValueProto;
import com.ib.client.protobuf.PositionEndProto;
import com.ib.client.protobuf.PositionMultiEndProto;
import com.ib.client.protobuf.PositionMultiProto;
import com.ib.client.protobuf.PositionProto;
import com.ib.client.protobuf.RealTimeBarTickProto;
import com.ib.client.protobuf.ReceiveFAProto;
import com.ib.client.protobuf.ReplaceFAEndProto;
import com.ib.client.protobuf.RerouteMarketDataRequestProto;
import com.ib.client.protobuf.RerouteMarketDepthRequestProto;
import com.ib.client.protobuf.ScannerDataProto;
import com.ib.client.protobuf.ScannerParametersProto;
import com.ib.client.protobuf.SecDefOptParameterEndProto;
import com.ib.client.protobuf.SecDefOptParameterProto;
import com.ib.client.protobuf.SmartComponentsProto;
import com.ib.client.protobuf.SoftDollarTiersProto;
import com.ib.client.protobuf.SymbolSamplesProto;
import com.ib.client.protobuf.TickByTickDataProto;
import com.ib.client.protobuf.TickGenericProto;
import com.ib.client.protobuf.TickNewsProto;
import com.ib.client.protobuf.TickOptionComputationProto;
import com.ib.client.protobuf.TickPriceProto;
import com.ib.client.protobuf.TickReqParamsProto;
import com.ib.client.protobuf.TickSizeProto;
import com.ib.client.protobuf.TickSnapshotEndProto;
import com.ib.client.protobuf.TickStringProto;
import com.ib.client.protobuf.UserInfoProto;
import com.ib.client.protobuf.VerifyCompletedProto;
import com.ib.client.protobuf.VerifyMessageApiProto;
import com.ib.client.protobuf.WshEventDataProto;
import com.ib.client.protobuf.WshMetaDataProto;

//! [ewrapperimpl]
public class EWrapperImpl implements EWrapper {
	//! [ewrapperimpl]
	
	//! [socket_declare]
	private EReaderSignal readerSignal;
	private EClientSocket clientSocket;
	protected int currentOrderId = -1;
	//! [socket_declare]
	
	//! [socket_init]
	public EWrapperImpl() {
		readerSignal = new EJavaSignal();
		clientSocket = new EClientSocket(this, readerSignal);
	}
	//! [socket_init]
	public EClientSocket getClient() {
		return clientSocket;
	}
	
	public EReaderSignal getSignal() {
		return readerSignal;
	}
	
	public int getCurrentOrderId() {
		return currentOrderId;
	}
	
	 //! [tickprice]
	@Override
	public void tickPrice(int tickerId, int field, double price, TickAttrib attribs) {
		System.out.println("Tick Price: " + EWrapperMsgGenerator.tickPrice( tickerId, field, price, attribs));
	}
	//! [tickprice]
	
	//! [ticksize]
	@Override
	public void tickSize(int tickerId, int field, Decimal size) {
		System.out.println("Tick Size: " + EWrapperMsgGenerator.tickSize( tickerId, field, size));
	}
	//! [ticksize]
	
	//! [tickoptioncomputation]
	@Override
	public void tickOptionComputation(int tickerId, int field, int tickAttrib, double impliedVol, double delta, double optPrice,
			double pvDividend, double gamma, double vega, double theta, double undPrice) {
		System.out.println("TickOptionComputation: " + EWrapperMsgGenerator.tickOptionComputation( tickerId, field, tickAttrib, impliedVol, delta, optPrice, pvDividend, gamma, vega, theta, undPrice));
	}
	//! [tickoptioncomputation]
	
	//! [tickgeneric]
	@Override
	public void tickGeneric(int tickerId, int tickType, double value) {
		System.out.println("Tick Generic: " + EWrapperMsgGenerator.tickGeneric(tickerId, tickType, value));
	}
	//! [tickgeneric]
	
	//! [tickstring]
	@Override
	public void tickString(int tickerId, int tickType, String value) {
		System.out.println("Tick String: " + EWrapperMsgGenerator.tickString(tickerId, tickType, value));
	}
	//! [tickstring]
	@Override
	public void tickEFP(int tickerId, int tickType, double basisPoints, String formattedBasisPoints, double impliedFuture, int holdDays,
			String futureLastTradeDate, double dividendImpact, double dividendsToLastTradeDate) {
		System.out.println("TickEFP: " + EWrapperMsgGenerator.tickEFP(tickerId, tickType, basisPoints, formattedBasisPoints,
				impliedFuture, holdDays, futureLastTradeDate, dividendImpact, dividendsToLastTradeDate));
	}
	//! [orderstatus]
	@Override
	public void orderStatus(int orderId, String status, Decimal filled, Decimal remaining, double avgFillPrice, long permId, int parentId,
			double lastFillPrice, int clientId, String whyHeld, double mktCapPrice) {
		System.out.println(EWrapperMsgGenerator.orderStatus( orderId, status, filled, remaining, avgFillPrice, permId, parentId, lastFillPrice, clientId, whyHeld, mktCapPrice));
	}
	//! [orderstatus]
	
	//! [openorder]
	@Override
	public void openOrder(int orderId, Contract contract, Order order, OrderState orderState) {
		System.out.println(EWrapperMsgGenerator.openOrder(orderId, contract, order, orderState));
	}
	//! [openorder]
	
	//! [openorderend]
	@Override
	public void openOrderEnd() {
		System.out.println("Open Order End: " + EWrapperMsgGenerator.openOrderEnd());
	}
	//! [openorderend]
	
	//! [updateaccountvalue]
	@Override
	public void updateAccountValue(String key, String value, String currency, String accountName) {
		System.out.println(EWrapperMsgGenerator.updateAccountValue( key, value, currency, accountName));
	}
	//! [updateaccountvalue]
	
	//! [updateportfolio]
	@Override
	public void updatePortfolio(Contract contract, Decimal position, double marketPrice, double marketValue, double averageCost,
			double unrealizedPNL, double realizedPNL, String accountName) {
		System.out.println(EWrapperMsgGenerator.updatePortfolio( contract, position, marketPrice, marketValue, averageCost, unrealizedPNL, realizedPNL, accountName));
	}
	//! [updateportfolio]
	
	//! [updateaccounttime]
	@Override
	public void updateAccountTime(String timeStamp) {
		System.out.println(EWrapperMsgGenerator.updateAccountTime( timeStamp));
	}
	//! [updateaccounttime]
	
	//! [accountdownloadend]
	@Override
	public void accountDownloadEnd(String accountName) {
		System.out.println(EWrapperMsgGenerator.accountDownloadEnd(accountName));
	}
	//! [accountdownloadend]
	
	//! [nextvalidid]
	@Override
	public void nextValidId(int orderId) {
		System.out.println(EWrapperMsgGenerator.nextValidId(orderId));
		currentOrderId = orderId;
	}
	//! [nextvalidid]
	
	//! [contractdetails]
	@Override
	public void contractDetails(int reqId, ContractDetails contractDetails) {
		System.out.println(EWrapperMsgGenerator.contractDetails(reqId, contractDetails)); 
	}
	//! [contractdetails]
	@Override
	public void bondContractDetails(int reqId, ContractDetails contractDetails) {
		System.out.println(EWrapperMsgGenerator.bondContractDetails(reqId, contractDetails)); 
	}
	//! [contractdetailsend]
	@Override
	public void contractDetailsEnd(int reqId) {
		System.out.println("Contract Details End: " + EWrapperMsgGenerator.contractDetailsEnd(reqId));
	}
	//! [contractdetailsend]
	
	//! [execdetails]
	@Override
	public void execDetails(int reqId, Contract contract, Execution execution) {
		System.out.println(EWrapperMsgGenerator.execDetails( reqId, contract, execution));
	}
	//! [execdetails]
	
	//! [execdetailsend]
	@Override
	public void execDetailsEnd(int reqId) {
		System.out.println("Exec Details End: " + EWrapperMsgGenerator.execDetailsEnd( reqId));
	}
	//! [execdetailsend]
	
	//! [updatemktdepth]
	@Override
	public void updateMktDepth(int tickerId, int position, int operation, int side, double price, Decimal size) {
		System.out.println(EWrapperMsgGenerator.updateMktDepth(tickerId, position, operation, side, price, size));
	}
	//! [updatemktdepth]
	
	//! [updatemktdepthl2]
	@Override
	public void updateMktDepthL2(int tickerId, int position, String marketMaker, int operation, int side, double price, Decimal size, boolean isSmartDepth) {
		System.out.println(EWrapperMsgGenerator.updateMktDepthL2( tickerId, position, marketMaker, operation, side, price, size, isSmartDepth));
	}
	//! [updatemktdepthl2]
	
	//! [updatenewsbulletin]
	@Override
	public void updateNewsBulletin(int msgId, int msgType, String message, String origExchange) {
		System.out.println("News Bulletin: " + EWrapperMsgGenerator.updateNewsBulletin( msgId, msgType, message, origExchange));
	}
	//! [updatenewsbulletin]
	
	//! [managedaccounts]
	@Override
	public void managedAccounts(String accountsList) {
		System.out.println("Account list: " + accountsList);
	}
	//! [managedaccounts]

	//! [receivefa]
	@Override
	public void receiveFA(int faDataType, String xml) {
		System.out.println("Receiving FA: " + faDataType + " - " + xml);
	}
	//! [receivefa]
	
	//! [historicaldata]
	@Override
	public void historicalData(int reqId, Bar bar) {
		System.out.println("HistoricalData:  " + EWrapperMsgGenerator.historicalData(reqId, bar.time(), bar.open(), bar.high(), bar.low(), bar.close(), bar.volume(), bar.count(), bar.wap()));
	}
	//! [historicaldata]
	
	//! [historicaldataend]
	@Override
	public void historicalDataEnd(int reqId, String startDateStr, String endDateStr) {
		System.out.println("HistoricalDataEnd. " + EWrapperMsgGenerator.historicalDataEnd(reqId, startDateStr, endDateStr));
	}
	//! [historicaldataend]
	
	
	//! [scannerparameters]
	@Override
	public void scannerParameters(String xml) {
		System.out.println("ScannerParameters. " + xml + "\n");
	}
	//! [scannerparameters]
	
	//! [scannerdata]
	@Override
	public void scannerData(int reqId, int rank, ContractDetails contractDetails, String distance, String benchmark, String projection, String legsStr) {
		System.out.println("ScannerData: " + EWrapperMsgGenerator.scannerData(reqId, rank, contractDetails, distance, benchmark, projection, legsStr));
	}
	//! [scannerdata]
	
	//! [scannerdataend]
	@Override
	public void scannerDataEnd(int reqId) {
		System.out.println("ScannerDataEnd: " + EWrapperMsgGenerator.scannerDataEnd(reqId));
	}
	//! [scannerdataend]
	
	//! [realtimebar]
	@Override
	public void realtimeBar(int reqId, long time, double open, double high, double low, double close, Decimal volume, Decimal wap, int count) {
		System.out.println("RealTimeBar: " + EWrapperMsgGenerator.realtimeBar(reqId, time, open, high, low, close, volume, wap, count));
	}
	//! [realtimebar]
	@Override
	public void currentTime(long time) {
		System.out.println(EWrapperMsgGenerator.currentTime(time));
	}
	//! [fundamentaldata]
	@Override
	public void fundamentalData(int reqId, String data) {
		System.out.println("FundamentalData: " + EWrapperMsgGenerator.fundamentalData(reqId, data));
	}
	//! [fundamentaldata]
	@Override
	public void deltaNeutralValidation(int reqId, DeltaNeutralContract deltaNeutralContract) {
		System.out.println("Delta Neutral Validation: " + EWrapperMsgGenerator.deltaNeutralValidation(reqId, deltaNeutralContract));
	}
	//! [ticksnapshotend]
	@Override
	public void tickSnapshotEnd(int reqId) {
		System.out.println("TickSnapshotEnd: " + EWrapperMsgGenerator.tickSnapshotEnd(reqId));
	}
	//! [ticksnapshotend]
	
	//! [marketdatatype]
	@Override
	public void marketDataType(int reqId, int marketDataType) {
		System.out.println("MarketDataType: " + EWrapperMsgGenerator.marketDataType(reqId, marketDataType));
	}
	//! [marketdatatype]
	
	//! [commissionandfeesreport]
	@Override
	public void commissionAndFeesReport(CommissionAndFeesReport commissionAndFeesReport) {
		System.out.println(EWrapperMsgGenerator.commissionAndFeesReport(commissionAndFeesReport));
	}
	//! [commissionandfeesreport]
	
	//! [position]
	@Override
	public void position(String account, Contract contract, Decimal pos, double avgCost) {
		System.out.println(EWrapperMsgGenerator.position(account, contract, pos, avgCost));
	}
	//! [position]
	
	//! [positionend]
	@Override
	public void positionEnd() {
		System.out.println("Position End: " + EWrapperMsgGenerator.positionEnd());
	}
	//! [positionend]
	
	//! [accountsummary]
	@Override
	public void accountSummary(int reqId, String account, String tag, String value, String currency) {
		System.out.println(EWrapperMsgGenerator.accountSummary(reqId, account, tag, value, currency));
	}
	//! [accountsummary]
	
	//! [accountsummaryend]
	@Override
	public void accountSummaryEnd(int reqId) {
		System.out.println("Account Summary End. Req Id: " + EWrapperMsgGenerator.accountSummaryEnd(reqId));
	}
	//! [accountsummaryend]
	@Override
	public void verifyMessageAPI(String apiData) {
		System.out.println("VerifyMessageAPI. ApiData: " + apiData);
	}

	@Override
	public void verifyCompleted(boolean isSuccessful, String errorText) {
		System.out.println("VerifyCompleted. IsSuccessful: " + isSuccessful + " errorText: " + errorText);
	}

	@Override
	public void verifyAndAuthMessageAPI(String apiData, String xyzChallenge) {
		System.out.println("verifyAndAuthMessageAPI");
	}

	@Override
	public void verifyAndAuthCompleted(boolean isSuccessful, String errorText) {
		System.out.println("verifyAndAuthCompleted");
	}
	//! [displaygrouplist]
	@Override
	public void displayGroupList(int reqId, String groups) {
		System.out.println("Display Group List. ReqId: " + reqId + ", Groups: " + groups + "\n");
	}
	//! [displaygrouplist]
	
	//! [displaygroupupdated]
	@Override
	public void displayGroupUpdated(int reqId, String contractInfo) {
		System.out.println("Display Group Updated. ReqId: " + reqId + ", Contract info: " + contractInfo + "\n");
	}
	//! [displaygroupupdated]
	@Override
	public void error(Exception e) {
		System.out.println("Exception: " + e.getMessage());
	}

	@Override
	public void error(String str) {
		System.out.println("Error: " + str);
	}
	//! [error]
	@Override
	public void error(int id, long errorTime, int errorCode, String errorMsg, String advancedOrderRejectJson) {
		String errorTimeStr = errorTime != 0 ? ", Time: " + Util.UnixMillisecondsToString(errorTime, "yyyyMMdd-HH:mm:ss") : ""; 
		String str = "Error. Id: " + id + errorTimeStr + ", Code: " + errorCode + ", Msg: " + errorMsg;
		if (advancedOrderRejectJson != null) {
			str += (", AdvancedOrderRejectJson: " + advancedOrderRejectJson);
		}
		System.out.println(str + "\n");
	}
	//! [error]
	@Override
	public void connectionClosed() {
		System.out.println("Connection closed");
	}

	//! [connectack]
	@Override
	public void connectAck() {
		if (clientSocket.isAsyncEConnect()) {
			System.out.println("Acknowledging connection");
			clientSocket.startAPI();
		}
	}
	//! [connectack]
	
	//! [positionmulti]
	@Override
	public void positionMulti(int reqId, String account, String modelCode, Contract contract, Decimal pos, double avgCost) {
		System.out.println(EWrapperMsgGenerator.positionMulti(reqId, account, modelCode, contract, pos, avgCost));
	}
	//! [positionmulti]
	
	//! [positionmultiend]
	@Override
	public void positionMultiEnd(int reqId) {
		System.out.println("Position Multi End: " + EWrapperMsgGenerator.positionMultiEnd(reqId));
	}
	//! [positionmultiend]
	
	//! [accountupdatemulti]
	@Override
	public void accountUpdateMulti(int reqId, String account, String modelCode, String key, String value, String currency) {
		System.out.println("Account Update Multi: " + EWrapperMsgGenerator.accountUpdateMulti(reqId, account, modelCode, key, value, currency));
	}
	//! [accountupdatemulti]
	
	//! [accountupdatemultiend]
	@Override
	public void accountUpdateMultiEnd(int reqId) {
		System.out.println("Account Update Multi End: " + EWrapperMsgGenerator.accountUpdateMultiEnd(reqId));
	}
	//! [accountupdatemultiend]
	
	//! [securityDefinitionOptionParameter]
	@Override
	public void securityDefinitionOptionalParameter(int reqId, String exchange, int underlyingConId, String tradingClass, String multiplier,
			Set<String> expirations, Set<Double> strikes) {
		System.out.println("Security Definition Optional Parameter: " + EWrapperMsgGenerator.securityDefinitionOptionalParameter(reqId, exchange, underlyingConId, tradingClass, multiplier, expirations, strikes));
	}
	//! [securityDefinitionOptionParameter]

	//! [securityDefinitionOptionParameterEnd]
	@Override
	public void securityDefinitionOptionalParameterEnd(int reqId) {
		System.out.println("Security Definition Optional Parameter End. Request Id: " + reqId);
	}
	//! [securityDefinitionOptionParameterEnd]

    //! [softDollarTiers]
	@Override
	public void softDollarTiers(int reqId, SoftDollarTier[] tiers) {
		System.out.print(EWrapperMsgGenerator.softDollarTiers(tiers));
	}
    //! [softDollarTiers]

    //! [familyCodes]
    @Override
    public void familyCodes(FamilyCode[] familyCodes) {
        System.out.print(EWrapperMsgGenerator.familyCodes(familyCodes));
    }
    //! [familyCodes]
    
    //! [symbolSamples]
    @Override
    public void symbolSamples(int reqId, ContractDescription[] contractDescriptions) {
        System.out.println(EWrapperMsgGenerator.symbolSamples(reqId, contractDescriptions));
    }
    //! [symbolSamples]
    
	//! [mktDepthExchanges]
	@Override
	public void mktDepthExchanges(DepthMktDataDescription[] depthMktDataDescriptions) {
		System.out.println(EWrapperMsgGenerator.mktDepthExchanges(depthMktDataDescriptions));
	}
	//! [mktDepthExchanges]
	
	//! [tickNews]
	@Override
	public void tickNews(int tickerId, long timeStamp, String providerCode, String articleId, String headline, String extraData) {
		System.out.println(EWrapperMsgGenerator.tickNews(tickerId, timeStamp, providerCode, articleId, headline, extraData));
	}
	//! [tickNews]

	//! [smartcomponents]
	@Override
	public void smartComponents(int reqId, Map<Integer, Entry<String, Character>> theMap) {
		System.out.println(EWrapperMsgGenerator.smartComponents(reqId, theMap));
	}
	//! [smartcomponents]

	//! [tickReqParams]
	@Override
	public void tickReqParams(int tickerId, double minTick, String bboExchange, int snapshotPermissions) {
		System.out.println("Tick req params: " + EWrapperMsgGenerator.tickReqParams(tickerId, minTick, bboExchange, snapshotPermissions));
	}
	//! [tickReqParams]

	//! [newsProviders]
	@Override
	public void newsProviders(NewsProvider[] newsProviders) {
		System.out.print(EWrapperMsgGenerator.newsProviders(newsProviders));
	}
	//! [newsProviders]

	//! [newsArticle]
	@Override
	public void newsArticle(int requestId, int articleType, String articleText) {
		System.out.println(EWrapperMsgGenerator.newsArticle(requestId, articleType, articleText));
	}
	//! [newsArticle]

	//! [historicalNews]
	@Override
	public void historicalNews(int requestId, String time, String providerCode, String articleId, String headline) {
		System.out.println(EWrapperMsgGenerator.historicalNews(requestId, time, providerCode, articleId, headline));
	}
	//! [historicalNews]

	//! [historicalNewsEnd]
	@Override
	public void historicalNewsEnd(int requestId, boolean hasMore) {
		System.out.println(EWrapperMsgGenerator.historicalNewsEnd(requestId, hasMore));
	}
	//! [historicalNewsEnd]

	//! [headTimestamp]
	@Override
	public void headTimestamp(int reqId, String headTimestamp) {
		System.out.println(EWrapperMsgGenerator.headTimestamp(reqId, headTimestamp));
	}
	//! [headTimestamp]
	
	//! [histogramData]
	@Override
	public void histogramData(int reqId, List<HistogramEntry> items) {
		System.out.println(EWrapperMsgGenerator.histogramData(reqId, items));
	}
	//! [histogramData]

	//! [historicalDataUpdate]
	@Override
    public void historicalDataUpdate(int reqId, Bar bar) {
        System.out.println("HistoricalDataUpdate. " + EWrapperMsgGenerator.historicalData(reqId, bar.time(), bar.open(), bar.high(), bar.low(), bar.close(), bar.volume(), bar.count(), bar.wap()));
    }
	//! [historicalDataUpdate]
	
	//! [rerouteMktDataReq]
	@Override
	public void rerouteMktDataReq(int reqId, int conId, String exchange) {
		System.out.println(EWrapperMsgGenerator.rerouteMktDataReq(reqId, conId, exchange));
	}
	//! [rerouteMktDataReq]
	
	//! [rerouteMktDepthReq]
	@Override
	public void rerouteMktDepthReq(int reqId, int conId, String exchange) {
		System.out.println(EWrapperMsgGenerator.rerouteMktDepthReq(reqId, conId, exchange));
	}
	//! [rerouteMktDepthReq]
	
	//! [marketRule]
	@Override
	public void marketRule(int marketRuleId, PriceIncrement[] priceIncrements) {
		System.out.println(EWrapperMsgGenerator.marketRule(marketRuleId, priceIncrements));
	}
	//! [marketRule]
	
	//! [pnl]
    @Override
    public void pnl(int reqId, double dailyPnL, double unrealizedPnL, double realizedPnL) {
        System.out.println(EWrapperMsgGenerator.pnl(reqId, dailyPnL, unrealizedPnL, realizedPnL));
    }
    //! [pnl]
	
	//! [pnlsingle]
    @Override
    public void pnlSingle(int reqId, Decimal pos, double dailyPnL, double unrealizedPnL, double realizedPnL, double value) {
        System.out.println(EWrapperMsgGenerator.pnlSingle(reqId, pos, dailyPnL, unrealizedPnL, realizedPnL, value));                
    }
    //! [pnlsingle]
	
	//! [historicalticks]
    @Override
    public void historicalTicks(int reqId, List<HistoricalTick> ticks, boolean done) {
        for (HistoricalTick tick : ticks) {
            System.out.println(EWrapperMsgGenerator.historicalTick(reqId, tick.time(), tick.price(), tick.size()));
        }
    }
    //! [historicalticks]
	
	//! [historicalticksbidask]
    @Override
    public void historicalTicksBidAsk(int reqId, List<HistoricalTickBidAsk> ticks, boolean done) {
        for (HistoricalTickBidAsk tick : ticks) {
            System.out.println(EWrapperMsgGenerator.historicalTickBidAsk(reqId, tick.time(), tick.tickAttribBidAsk(), tick.priceBid(), tick.priceAsk(), tick.sizeBid(),
                    tick.sizeAsk()));
        }
    }   
    //! [historicalticksbidask]
	
    @Override
	//! [historicaltickslast]
    public void historicalTicksLast(int reqId, List<HistoricalTickLast> ticks, boolean done) {
        for (HistoricalTickLast tick : ticks) {
            System.out.println(EWrapperMsgGenerator.historicalTickLast(reqId, tick.time(), tick.tickAttribLast(), tick.price(), tick.size(), tick.exchange(), 
                tick.specialConditions()));
        }
    }
    //! [historicaltickslast]

    //! [tickbytickalllast]
   @Override
    public void tickByTickAllLast(int reqId, int tickType, long time, double price, Decimal size, TickAttribLast tickAttribLast,
            String exchange, String specialConditions) {
        System.out.println(EWrapperMsgGenerator.tickByTickAllLast(reqId, tickType, time, price, size, tickAttribLast, exchange, specialConditions));
    }
    //! [tickbytickalllast]

    //! [tickbytickbidask]
    @Override
    public void tickByTickBidAsk(int reqId, long time, double bidPrice, double askPrice, Decimal bidSize, Decimal askSize,
            TickAttribBidAsk tickAttribBidAsk) {
        System.out.println(EWrapperMsgGenerator.tickByTickBidAsk(reqId, time, bidPrice, askPrice, bidSize, askSize, tickAttribBidAsk));
    }
    //! [tickbytickbidask]
    
    //! [tickbytickmidpoint]
    @Override
    public void tickByTickMidPoint(int reqId, long time, double midPoint) {
        System.out.println(EWrapperMsgGenerator.tickByTickMidPoint(reqId, time, midPoint));
    }
    //! [tickbytickmidpoint]

    //! [orderbound]
    @Override
    public void orderBound(long permId, int clientId, int orderId) {
        System.out.println(EWrapperMsgGenerator.orderBound(permId, clientId, orderId));
    }
    //! [orderbound]

    //! [completedorder]
    @Override
    public void completedOrder(Contract contract, Order order, OrderState orderState) {
        System.out.println(EWrapperMsgGenerator.completedOrder(contract, order, orderState));
    }
    //! [completedorder]

    //! [completedordersend]
    @Override
    public void completedOrdersEnd() {
        System.out.println(EWrapperMsgGenerator.completedOrdersEnd());
    }
    //! [completedordersend]

    //! [replacefaend]
    @Override
    public void replaceFAEnd(int reqId, String text) {
        System.out.println(EWrapperMsgGenerator.replaceFAEnd(reqId, text));
    }
    //! [replacefaend]
    
    //! [wshMetaData]
	@Override
	public void wshMetaData(int reqId, String dataJson) {
		System.out.println(EWrapperMsgGenerator.wshMetaData(reqId, dataJson));
	}
    //! [wshMetaData]

    //! [wshEventData]
	@Override
	public void wshEventData(int reqId, String dataJson) {
        System.out.println(EWrapperMsgGenerator.wshEventData(reqId, dataJson));
	}
    //! [wshEventData]

    //! [historicalSchedule]
    @Override
    public void historicalSchedule(int reqId, String startDateTime, String endDateTime, String timeZone, List<HistoricalSession> sessions) {
        System.out.println(EWrapperMsgGenerator.historicalSchedule(reqId, startDateTime, endDateTime, timeZone, sessions));
    }
    //! [historicalSchedule]
    
    //! [userInfo]
    @Override
    public void userInfo(int reqId, String whiteBrandingId) {
        System.out.println(EWrapperMsgGenerator.userInfo(reqId, whiteBrandingId));
    }
    //! [userInfo]

    //! [currentTimeInMillis]
    @Override
    public void currentTimeInMillis(long timeInMillis) {
        System.out.println(EWrapperMsgGenerator.currentTimeInMillis(timeInMillis));
    }
    //! [currentTimeInMillis]
    
    // ---------------------------------------------- Protobuf ---------------------------------------------
    //! [orderStatus]
    @Override public void orderStatusProtoBuf(OrderStatusProto.OrderStatus orderStatusProto) { }
    //! [orderStatus]

    //! [openOrder]
    @Override public void openOrderProtoBuf(OpenOrderProto.OpenOrder openOrderProto) { }
    //! [openOrder]

    //! [openOrdersEnd]
    @Override public void openOrdersEndProtoBuf(OpenOrdersEndProto.OpenOrdersEnd openOrdersEnd) { }
    //! [openOrdersEnd]

    //! [error]
    @Override public void errorProtoBuf(ErrorMessageProto.ErrorMessage errorMessageProto) { }
    //! [error]

    //! [execDetails]
    @Override public void execDetailsProtoBuf(ExecutionDetailsProto.ExecutionDetails executionDetailsProto) { }
    //! [execDetails]

    //! [execDetailsEnd]
    @Override public void execDetailsEndProtoBuf(ExecutionDetailsEndProto.ExecutionDetailsEnd executionDetailsEndProto) { }
    //! [execDetailsEnd]

    //! [completedOrders]
    @Override public void completedOrderProtoBuf(CompletedOrderProto.CompletedOrder completedOrderProto) { }
    //! [completedOrders]

    //! [completedOrdersEnd]
    @Override public void completedOrdersEndProtoBuf(CompletedOrdersEndProto.CompletedOrdersEnd completedOrdersEndProto) { }
    //! [completedOrdersEnd]

    //! [orderBound]
    @Override public void orderBoundProtoBuf(OrderBoundProto.OrderBound orderBoundProto) { }
    //! [orderBound]

    //! [contractData]
    @Override public void contractDataProtoBuf(ContractDataProto.ContractData contractDataProto) { }
    //! [contractData]

    //! [bondContractData]
    @Override public void bondContractDataProtoBuf(ContractDataProto.ContractData contractDataProto) { }
    //! [bondContractData]

    //! [contractDataEnd]
    @Override public void contractDataEndProtoBuf(ContractDataEndProto.ContractDataEnd contractDataEndProto) { }
    //! [contractDataEnd]

    //! [tickPrice]
    @Override public void tickPriceProtoBuf(TickPriceProto.TickPrice tickPriceProto) { }
    //! [tickPrice]

    //! [tickPrice]
    @Override public void tickSizeProtoBuf(TickSizeProto.TickSize tickSizeProto) { }
    //! [tickPrice]

    //! [tickOptionComputation]
    @Override public void tickOptionComputationProtoBuf(TickOptionComputationProto.TickOptionComputation tickOptionComputationProto) { }
    //! [tickOptionComputation]

    //! [tickGeneric]
    @Override public void tickGenericProtoBuf(TickGenericProto.TickGeneric tickGenericProto) { }
    //! [tickGeneric]

    //! [tickString]
    @Override public void tickStringProtoBuf(TickStringProto.TickString tickStringProto) { }
    //! [tickString]

    //! [tickSnapshotEnd]
    @Override public void tickSnapshotEndProtoBuf(TickSnapshotEndProto.TickSnapshotEnd tickSnapshotEndProto) { }
    //! [tickSnapshotEnd]

    //! [updateMarketDepth]
    @Override public void updateMarketDepthProtoBuf(MarketDepthProto.MarketDepth marketDepthProto) { }
    //! [updateMarketDepth]

    //! [updateMarketDepthL2]
    @Override public void updateMarketDepthL2ProtoBuf(MarketDepthL2Proto.MarketDepthL2 marketDepthL2Proto) { }
    //! [updateMarketDepthL2]

    //! [marketDataType]
    @Override public void marketDataTypeProtoBuf(MarketDataTypeProto.MarketDataType marketDataTypeProto) { }
    //! [marketDataType]

    //! [tickReqParams]
    @Override public void tickReqParamsProtoBuf(TickReqParamsProto.TickReqParams tickReqParamsProto) { }
    //! [tickReqParams]

    //! [updateAccountValue]
    @Override public void updateAccountValueProtoBuf(AccountValueProto.AccountValue accounValueProto) { }
    //! [updateAccountValue]

    //! [updatePortfolioValue]
    @Override public void updatePortfolioProtoBuf(PortfolioValueProto.PortfolioValue portfolioValueProto) { }
    //! [updatePortfolioValue]

    //! [updateAccountTime]
    @Override public void updateAccountTimeProtoBuf(AccountUpdateTimeProto.AccountUpdateTime accountUpdateTimeProto) { }
    //! [updateAccountTime]

    //! [accountDataEnd]
    @Override public void accountDataEndProtoBuf(AccountDataEndProto.AccountDataEnd accountDataEndProto) { }
    //! [accountDataEnd]

    //! [managedAccounts]
    @Override public void managedAccountsProtoBuf(ManagedAccountsProto.ManagedAccounts managedAccountsProto) { }
    //! [managedAccounts]

    //! [position]
    @Override public void positionProtoBuf(PositionProto.Position positionProto) { }
    //! [position]

    //! [positionEnd]
    @Override public void positionEndProtoBuf(PositionEndProto.PositionEnd positionEndProto) { }
    //! [positionEnd]

    //! [accountSummary]
    @Override public void accountSummaryProtoBuf(AccountSummaryProto.AccountSummary accountSummaryProto) { }
    //! [accountSummary]

    //! [accountSummaryEnd]
    @Override public void accountSummaryEndProtoBuf(AccountSummaryEndProto.AccountSummaryEnd accountSummaryEndProto) { }
    //! [accountSummaryEnd]

    //! [positionMulti]
    @Override public void positionMultiProtoBuf(PositionMultiProto.PositionMulti positionMultiProto) { }
    //! [positionMulti]

    //! [positionMultiEnd]
    @Override public void positionMultiEndProtoBuf(PositionMultiEndProto.PositionMultiEnd positionMultiEndProto) { }
    //! [positionMultiEnd]

    //! [accountUpdateMulti]
    @Override public void accountUpdateMultiProtoBuf(AccountUpdateMultiProto.AccountUpdateMulti accountUpdateMultiProto) { }
    //! [accountUpdateMulti]

    //! [accountUpdateMultiEnd]
    @Override public void accountUpdateMultiEndProtoBuf(AccountUpdateMultiEndProto.AccountUpdateMultiEnd accountUpdateMultiEndProto) { }
    //! [accountUpdateMultiEnd]
    
    //! [historicalData]
    @Override public void historicalDataProtoBuf(HistoricalDataProto.HistoricalData historicalDataProto) { }
    //! [historicalData]

    //! [historicalDataUpdate]
    @Override public void historicalDataUpdateProtoBuf(HistoricalDataUpdateProto.HistoricalDataUpdate historicalDataUpdateProto) { }
    //! [historicalDataUpdate]

    //! [historicalDataEnd]
    @Override public void historicalDataEndProtoBuf(HistoricalDataEndProto.HistoricalDataEnd historicalDataEndProto) { }
    //! [historicalDataEnd]

    //! [realTimeBarTick]
    @Override public void realTimeBarTickProtoBuf(RealTimeBarTickProto.RealTimeBarTick realTimeBarTickProto) { }
    //! [realTimeBarTick]

    //! [headTimestamp]
    @Override public void headTimestampProtoBuf(HeadTimestampProto.HeadTimestamp headTimestampProto) { }
    //! [headTimestamp]

    //! [histogramData]
    @Override public void histogramDataProtoBuf(HistogramDataProto.HistogramData histogramDataProto) { }
    //! [histogramData]

    //! [historicalTicks]
    @Override public void historicalTicksProtoBuf(HistoricalTicksProto.HistoricalTicks historicalTicksProto) { }
    //! [historicalTicks]

    //! [historicalTicksBidAsk]
    @Override public void historicalTicksBidAskProtoBuf(HistoricalTicksBidAskProto.HistoricalTicksBidAsk historicalTicksBidAskProto) { }
    //! [historicalTicksBidAsk]

    //! [historicalTicksLast]
    @Override public void historicalTicksLastProtoBuf(HistoricalTicksLastProto.HistoricalTicksLast historicalTicksLastProto) { }
    //! [historicalTicksLast]

    //! [tickByTickData]
    @Override public void tickByTickDataProtoBuf(TickByTickDataProto.TickByTickData tickByTickDataProto) { }
    //! [tickByTickData]

  //! [newsBulletin]
    @Override public void updateNewsBulletinProtoBuf(NewsBulletinProto.NewsBulletin newsBulletinProto) { }
    //! [newsBulletin]

    //! [newsArticle]
    @Override public void newsArticleProtoBuf(NewsArticleProto.NewsArticle newsArticleProto) { }
    //! [newsArticle]

    //! [newsProviders]
    @Override public void newsProvidersProtoBuf(NewsProvidersProto.NewsProviders newsProvidersProto) { }
    //! [newsProviders]

    //! [historicalNews]
    @Override public void historicalNewsProtoBuf(HistoricalNewsProto.HistoricalNews historicalNewsProto) { }
    //! [historicalNews]

    //! [historicalNewsEnd]
    @Override public void historicalNewsEndProtoBuf(HistoricalNewsEndProto.HistoricalNewsEnd historicalNewsEndProto) { }
    //! [historicalNewsEnd]

    //! [wshMetaData]
    @Override public void wshMetaDataProtoBuf(WshMetaDataProto.WshMetaData wshMetaDataProto) { }
    //! [wshMetaData]

    //! [wshEventData]
    @Override public void wshEventDataProtoBuf(WshEventDataProto.WshEventData wshEventDataProto) { }
    //! [wshEventData]

    //! [tickNews]
    @Override public void tickNewsProtoBuf(TickNewsProto.TickNews tickNewsProto) { }
    //! [tickNews]

  //! [scannerParameters]
    @Override public void scannerParametersProtoBuf(ScannerParametersProto.ScannerParameters scannerParametersProto) { }
    //! [scannerParameters]

    //! [scannerData]
    @Override public void scannerDataProtoBuf(ScannerDataProto.ScannerData scannerDataProto) { }
    //! [scannerData]

    //! [fundamentalsData]
    @Override public void fundamentalsDataProtoBuf(FundamentalsDataProto.FundamentalsData fundamentalsDataProto) { }
    //! [fundamentalsData]

    //! [pnl]
    @Override public void pnlProtoBuf(PnLProto.PnL pnlProto) { }
    //! [pnl]

    //! [pnlSingle]
    @Override public void pnlSingleProtoBuf(PnLSingleProto.PnLSingle pnlSingleProto) { }
    //! [pnlSingle]

  //! [receiveFA]
    @Override public void receiveFAProtoBuf(ReceiveFAProto.ReceiveFA receiveFAProto) { }
    //! [receiveFA]

    //! [replaceFAEnd]
    @Override public void replaceFAEndProtoBuf(ReplaceFAEndProto.ReplaceFAEnd replaceFAEndProto) { }
    //! [replaceFAEnd]

    //! [commissionAndFeesReport]
    @Override public void commissionAndFeesReportProtoBuf(CommissionAndFeesReportProto.CommissionAndFeesReport commissionAndFeesReportProto) { }
    //! [commissionAndFeesReport]

    //! [historicalSchedule]
    @Override public void historicalScheduleProtoBuf(HistoricalScheduleProto.HistoricalSchedule historicalScheduleProto) { }
    //! [historicalSchedule]

    //! [rerouteMarketDataRequest]
    @Override public void rerouteMarketDataRequestProtoBuf(RerouteMarketDataRequestProto.RerouteMarketDataRequest rerouteMarketDataRequestProto) { }
    //! [rerouteMarketDataRequest]

    //! [rerouteMarketDepthRequest]
    @Override public void rerouteMarketDepthRequestProtoBuf(RerouteMarketDepthRequestProto.RerouteMarketDepthRequest rerouteMarketDepthRequestProto) { }
    //! [rerouteMarketDepthRequest]

  //! [secDefOptParameter]
    @Override public void secDefOptParameterProtoBuf(SecDefOptParameterProto.SecDefOptParameter secDefOptParameterProto) { }
    //! [secDefOptParameter]

    //! [secDefOptParameterEnd]
    @Override public void secDefOptParameterEndProtoBuf(SecDefOptParameterEndProto.SecDefOptParameterEnd secDefOptParameterEndProto) { }
    //! [secDefOptParameterEnd]

    //! [softDollarTiers]
    @Override public void softDollarTiersProtoBuf(SoftDollarTiersProto.SoftDollarTiers softDollarTiersProto) { }
    //! [softDollarTiers]

    //! [familyCodes]
    @Override public void familyCodesProtoBuf(FamilyCodesProto.FamilyCodes familyCodesProto) { }
    //! [familyCodes]

    //! [symbolSamples]
    @Override public void symbolSamplesProtoBuf(SymbolSamplesProto.SymbolSamples symbolSamplesProto) { }
    //! [symbolSamples]

    //! [smartComponents]
    @Override public void smartComponentsProtoBuf(SmartComponentsProto.SmartComponents smartComponentsProto) { }
    //! [smartComponents]

    //! [marketRule]
    @Override public void marketRuleProtoBuf(MarketRuleProto.MarketRule marketRuleProto) { }
    //! [marketRule]

    //! [userInfo]
    @Override public void userInfoProtoBuf(UserInfoProto.UserInfo userInfoProto) { }
    //! [userInfo]

  //! [nextValidId]
    @Override public void nextValidIdProtoBuf(NextValidIdProto.NextValidId nextValidIdProto) { }
    //! [nextValidId]

    //! [currentTime]
    @Override public void currentTimeProtoBuf(CurrentTimeProto.CurrentTime currentTimeProto) { }
    //! [currentTime]

    //! [currentTimeInMillis]
    @Override public void currentTimeInMillisProtoBuf(CurrentTimeInMillisProto.CurrentTimeInMillis currentTimeInMillisProto) { }
    //! [currentTimeInMillis]

    //! [verifyMessageApi]
    @Override public void verifyMessageApiProtoBuf(VerifyMessageApiProto.VerifyMessageApi verifyMessageApiProto) { }
    //! [verifyMessageApi]

    //! [verifyCompleted]
    @Override public void verifyCompletedProtoBuf(VerifyCompletedProto.VerifyCompleted verifyCompletedProto) { }
    //! [verifyCompleted]

    //! [displayGroupList]
    @Override public void displayGroupListProtoBuf(DisplayGroupListProto.DisplayGroupList displayGroupListProto) { }
    //! [displayGroupList]

    //! [displayGroupUpdated]
    @Override public void displayGroupUpdatedProtoBuf(DisplayGroupUpdatedProto.DisplayGroupUpdated displayGroupUpdatedProto) { }
    //! [displayGroupUpdated]

    //! [marketDepthExchanges]
    @Override public void marketDepthExchangesProtoBuf(MarketDepthExchangesProto.MarketDepthExchanges marketDepthExchangesProto) { }
    //! [marketDepthExchanges]

}
