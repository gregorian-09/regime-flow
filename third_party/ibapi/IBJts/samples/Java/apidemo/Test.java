/* Copyright (C) 2025 Interactive Brokers LLC. All rights reserved. This code is subject to the terms
 * and conditions of the IB API Non-Commercial License or the IB API Commercial License, as applicable. */

package apidemo;

import static apidemo.util.Util.sleep;

import java.io.IOException;
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

import javax.swing.*;

public class Test implements EWrapper {
	private EJavaSignal m_signal = new EJavaSignal();
	private EClientSocket m_s = new EClientSocket(this, m_signal);
	private int NextOrderId = -1;

	public static void main(String[] args) {
		new Test().run();
	}

	private void run() {
		m_s.eConnect("localhost", 7497, 0);
		
        final EReader reader = new EReader(m_s, m_signal);
        
        reader.start();
       
		new Thread(() -> {
            while (m_s.isConnected()) {
                m_signal.waitForSignal();
                try {
                    SwingUtilities.invokeAndWait(() -> {
                    	try {
                    		reader.processMsgs();
                    	} catch (IOException e) {
                    		error(e);
                    	}
                    });
                } catch (Exception e) {
                    error(e);
                }
            }
        }).start();

		if (NextOrderId < 0) {
			sleep(1000);
		}

		m_s.reqSecDefOptParams(0, "IBM", "",/* "",*/ "STK", 8314);

		try {
			System.in.read();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		m_s.eDisconnect();
	}

	@Override public void nextValidId(int orderId) {
		NextOrderId = orderId;
		System.out.println(EWrapperMsgGenerator.nextValidId(orderId));
	}

	@Override public void error(Exception e) {
		System.out.println(EWrapperMsgGenerator.error(e));
	}

	@Override public void error(int id, long errorTime, int errorCode, String errorMsg, String advancedOrderRejectJson) {
		System.out.println(EWrapperMsgGenerator.error(id, errorTime, errorCode, errorMsg, advancedOrderRejectJson));
	}

	@Override public void connectionClosed() {
		System.out.println(EWrapperMsgGenerator.connectionClosed());
	}

	@Override public void error(String str) {
		System.out.println(EWrapperMsgGenerator.error(str));
	}

	@Override public void tickPrice(int tickerId, int field, double price, TickAttrib attribs) {
		System.out.println(EWrapperMsgGenerator.tickPrice(tickerId, field, price, attribs));
	}

	@Override public void tickSize(int tickerId, int field, Decimal size) {
		System.out.println(EWrapperMsgGenerator.tickSize(tickerId, field, size));
	}

	@Override public void tickOptionComputation(int tickerId, int field, int tickAttrib, double impliedVol, double delta, double optPrice, double pvDividend, double gamma, double vega, double theta, double undPrice) {
		System.out.println(EWrapperMsgGenerator.tickOptionComputation(tickerId, field, tickAttrib, impliedVol, delta, optPrice, pvDividend, gamma, vega, theta, undPrice));
	}

	@Override public void tickGeneric(int tickerId, int tickType, double value) {
		System.out.println(EWrapperMsgGenerator.tickGeneric(tickerId, tickType, value));
	}

	@Override public void tickString(int tickerId, int tickType, String value) {
		System.out.println(EWrapperMsgGenerator.tickString(tickerId, tickType, value));
	}

	@Override public void tickEFP(int tickerId, int tickType, double basisPoints, String formattedBasisPoints, double impliedFuture, int holdDays, String futureLastTradeDate, double dividendImpact,
			double dividendsToLastTradeDate) {
		System.out.println(EWrapperMsgGenerator.tickEFP( tickerId, tickType, basisPoints, formattedBasisPoints, impliedFuture, holdDays, futureLastTradeDate, dividendImpact, dividendsToLastTradeDate));
	}

	@Override public void orderStatus(int orderId, String status, Decimal filled, Decimal remaining, double avgFillPrice, long permId, int parentId, double lastFillPrice, int clientId, String whyHeld, double mktCapPrice) {
		System.out.println(EWrapperMsgGenerator.orderStatus( orderId,  status, filled, remaining, avgFillPrice, permId, parentId, lastFillPrice, clientId, whyHeld, mktCapPrice));
	}

	@Override public void openOrder(int orderId, Contract contract, Order order, OrderState orderState) {
		System.out.println(EWrapperMsgGenerator.openOrder( orderId, contract, order, orderState));
	}

	@Override public void openOrderEnd() {
		System.out.println(EWrapperMsgGenerator.openOrderEnd());
	}

	@Override public void updateAccountValue(String key, String value, String currency, String accountName) {
		System.out.println(EWrapperMsgGenerator.updateAccountValue( key, value, currency, accountName));
	}

	@Override public void updatePortfolio(Contract contract, Decimal position, double marketPrice, double marketValue, double averageCost, double unrealizedPNL, double realizedPNL, String accountName) {
		System.out.println(EWrapperMsgGenerator.updatePortfolio( contract, position, marketPrice, marketValue, averageCost, unrealizedPNL, realizedPNL, accountName));
	}

	@Override public void updateAccountTime(String timeStamp) {
		System.out.println(EWrapperMsgGenerator.updateAccountTime( timeStamp));
	}

	@Override public void accountDownloadEnd(String accountName) {
		System.out.println(EWrapperMsgGenerator.accountDownloadEnd(accountName));
	}

	@Override public void contractDetails(int reqId, ContractDetails contractDetails) {
		System.out.println(EWrapperMsgGenerator.contractDetails( reqId, contractDetails));
	}

	@Override public void bondContractDetails(int reqId, ContractDetails contractDetails) {
		System.out.println(EWrapperMsgGenerator.bondContractDetails( reqId, contractDetails));
	}

	@Override public void contractDetailsEnd(int reqId) {
		System.out.println(EWrapperMsgGenerator.contractDetailsEnd(reqId));
	}

	@Override public void execDetails(int reqId, Contract contract, Execution execution) {
		System.out.println(EWrapperMsgGenerator.execDetails( reqId, contract, execution));
	}

	@Override public void execDetailsEnd(int reqId) {
		System.out.println(EWrapperMsgGenerator.execDetailsEnd( reqId));
	}

	@Override public void updateMktDepth(int tickerId, int position, int operation, int side, double price, Decimal size) {
		System.out.println(EWrapperMsgGenerator.updateMktDepth(tickerId, position, operation, side, price, size));
	}

	@Override public void updateMktDepthL2(int tickerId, int position, String marketMaker, int operation, int side, double price, Decimal size, boolean isSmartDepth) {
		System.out.println(EWrapperMsgGenerator.updateMktDepthL2( tickerId, position, marketMaker, operation, side, price, size, isSmartDepth));
	}

	@Override public void updateNewsBulletin(int msgId, int msgType, String message, String origExchange) {
		System.out.println(EWrapperMsgGenerator.updateNewsBulletin( msgId, msgType, message, origExchange));
	}

	@Override public void managedAccounts(String accountsList) {
		System.out.println(EWrapperMsgGenerator.managedAccounts( accountsList));
	}

	@Override public void receiveFA(int faDataType, String xml) {
		System.out.println(EWrapperMsgGenerator.receiveFA( faDataType, xml));
	}

	@Override public void historicalData(int reqId, Bar bar) {
		System.out.println(EWrapperMsgGenerator.historicalData( reqId, bar.time(), bar.open(), bar.high(), bar.low(), bar.close(), bar.volume(), bar.count(), bar.wap()));
	}

	@Override public void scannerParameters(String xml) {
		System.out.println(EWrapperMsgGenerator.scannerParameters(xml));
	}

	@Override public void scannerData(int reqId, int rank, ContractDetails contractDetails, String distance, String benchmark, String projection, String legsStr) {
		System.out.println(EWrapperMsgGenerator.scannerData( reqId, rank, contractDetails, distance, benchmark, projection, legsStr));
	}

	@Override public void scannerDataEnd(int reqId) {
		System.out.println(EWrapperMsgGenerator.scannerDataEnd(reqId));
	}

	@Override public void realtimeBar(int reqId, long time, double open, double high, double low, double close, Decimal volume, Decimal wap, int count) {
		System.out.println(EWrapperMsgGenerator.realtimeBar( reqId, time, open, high, low, close, volume, wap, count));
	}

	@Override public void currentTime(long time) {
		System.out.println(EWrapperMsgGenerator.currentTime( time));
	}

	@Override public void fundamentalData(int reqId, String data) {
		System.out.println(EWrapperMsgGenerator.fundamentalData( reqId,  data));
	}

	@Override public void deltaNeutralValidation(int reqId, DeltaNeutralContract deltaNeutralContract) {
		System.out.println(EWrapperMsgGenerator.deltaNeutralValidation( reqId, deltaNeutralContract));
	}

	@Override public void tickSnapshotEnd(int reqId) {
		System.out.println(EWrapperMsgGenerator.tickSnapshotEnd( reqId));
	}

	@Override public void marketDataType(int reqId, int marketDataType) {
		System.out.println(EWrapperMsgGenerator.marketDataType( reqId, marketDataType));
	}

	@Override public void commissionAndFeesReport(CommissionAndFeesReport commissionAndFeesReport) {
		System.out.println(EWrapperMsgGenerator.commissionAndFeesReport( commissionAndFeesReport));
	}

	@Override public void position(String account, Contract contract, Decimal pos, double avgCost) {
		System.out.println(EWrapperMsgGenerator.position( account,  contract,  pos,  avgCost));
	}

	@Override public void positionEnd() {
		System.out.println(EWrapperMsgGenerator.positionEnd());
	}

	@Override public void accountSummary(int reqId, String account, String tag, String value, String currency) {
		System.out.println(EWrapperMsgGenerator.accountSummary( reqId, account, tag, value, currency));
	}

	@Override public void accountSummaryEnd(int reqId) {
		System.out.println(EWrapperMsgGenerator.accountSummaryEnd( reqId));
	}
	
	@Override public void verifyMessageAPI( String apiData) {
	}

	@Override public void verifyCompleted( boolean isSuccessful, String errorText){
	}

	@Override public void verifyAndAuthMessageAPI( String apiData, String xyzChallenge) {
	}

	@Override public void verifyAndAuthCompleted( boolean isSuccessful, String errorText){
	}

	@Override public void displayGroupList( int reqId, String groups){
	}

	@Override public void displayGroupUpdated( int reqId, String contractInfo){
	}
	
	@Override public void positionMulti( int reqId, String account, String modelCode, Contract contract, Decimal pos, double avgCost) {
		System.out.println(EWrapperMsgGenerator.positionMulti( reqId, account, modelCode, contract, pos, avgCost));
	}

	@Override public void positionMultiEnd( int reqId) {
		System.out.println(EWrapperMsgGenerator.positionMultiEnd( reqId));
	}

	@Override public void accountUpdateMulti( int reqId, String account, String modelCode, String key, String value, String currency) {
		System.out.println(EWrapperMsgGenerator.accountUpdateMulti( reqId, account, modelCode, key, value, currency));
	}

	@Override public void accountUpdateMultiEnd( int reqId) {
		System.out.println(EWrapperMsgGenerator.accountUpdateMultiEnd( reqId));
	}
	
	public void connectAck() {
	}

	@Override
	public void securityDefinitionOptionalParameter(int reqId, String exchange, int underlyingConId, String tradingClass,
			String multiplier, Set<String> expirations, Set<Double> strikes) {
		System.out.println(EWrapperMsgGenerator.securityDefinitionOptionalParameter( reqId, exchange, underlyingConId, tradingClass, multiplier, expirations, strikes));
	}

	@Override
	public void securityDefinitionOptionalParameterEnd(int reqId) {
		System.out.println(EWrapperMsgGenerator.securityDefinitionOptionalParameterEnd( reqId));
	}

	@Override
	public void softDollarTiers(int reqId, SoftDollarTier[] tiers) {
		System.out.println(EWrapperMsgGenerator.softDollarTiers( reqId,tiers));
	}

    @Override
    public void familyCodes(FamilyCode[] familyCodes) {
		System.out.println(EWrapperMsgGenerator.familyCodes(familyCodes));
    }

    @Override
    public void symbolSamples(int reqId, ContractDescription[] contractDescriptions) {
		System.out.println(EWrapperMsgGenerator.symbolSamples( reqId, contractDescriptions));
    }

	@Override
	public void historicalDataEnd(int reqId, String startDateStr, String endDateStr) {
		System.out.println(EWrapperMsgGenerator.historicalDataEnd( reqId, startDateStr, endDateStr));
	}

	@Override
	public void mktDepthExchanges(DepthMktDataDescription[] depthMktDataDescriptions) {
		System.out.println(EWrapperMsgGenerator.mktDepthExchanges(depthMktDataDescriptions));
	}

	@Override
	public void tickNews(int tickerId, long timeStamp, String providerCode, String articleId, String headline,
			String extraData) {
		System.out.println(EWrapperMsgGenerator.tickNews(tickerId, timeStamp, providerCode, articleId, headline, extraData));
	}

	@Override
	public void smartComponents(int reqId, Map<Integer, Entry<String, Character>> theMap) {
		System.out.println(EWrapperMsgGenerator.smartComponents(reqId, theMap));
	}

	@Override
	public void tickReqParams(int tickerId, double minTick, String bboExchange, int snapshotPermissions) {
		System.out.println(EWrapperMsgGenerator.tickReqParams(tickerId, minTick, bboExchange, snapshotPermissions));
	}

	@Override
	public void newsProviders(NewsProvider[] newsProviders) {
		System.out.println(EWrapperMsgGenerator.newsProviders(newsProviders));
	}

	@Override
	public void newsArticle(int requestId, int articleType, String articleText) {
		System.out.println(EWrapperMsgGenerator.newsArticle(requestId, articleType, articleText));
	}

	@Override
	public void historicalNews(int requestId, String time, String providerCode, String articleId, String headline) {
		System.out.println(EWrapperMsgGenerator.historicalNews(requestId, time, providerCode, articleId, headline));
	}

	@Override
	public void historicalNewsEnd(int requestId, boolean hasMore) {
		System.out.println(EWrapperMsgGenerator.historicalNewsEnd(requestId, hasMore));
	}
	
	@Override
	public void headTimestamp(int reqId, String headTimestamp) {
		System.out.println(EWrapperMsgGenerator.headTimestamp(reqId, headTimestamp));
	}

	@Override
	public void histogramData(int reqId, List<HistogramEntry> items) {
		System.out.println(EWrapperMsgGenerator.histogramData(reqId, items));
	}

    @Override
    public void historicalDataUpdate(int reqId, Bar bar) {
        historicalData(reqId, bar);
    }

	@Override
	public void rerouteMktDataReq(int reqId, int conId, String exchange) {
		System.out.println(EWrapperMsgGenerator.rerouteMktDataReq(reqId, conId, exchange));
	}

	@Override
	public void rerouteMktDepthReq(int reqId, int conId, String exchange) {
		System.out.println(EWrapperMsgGenerator.rerouteMktDepthReq(reqId, conId, exchange));
	}

	@Override
	public void marketRule(int marketRuleId, PriceIncrement[] priceIncrements) {
		System.out.println(EWrapperMsgGenerator.marketRule(marketRuleId, priceIncrements));
	}
	
	@Override
    public void pnl(int reqId, double dailyPnL, double unrealizedPnL, double realizedPnL) {
        System.out.println(EWrapperMsgGenerator.pnl(reqId, dailyPnL, unrealizedPnL, realizedPnL));
    }

    @Override
    public void pnlSingle(int reqId, Decimal pos, double dailyPnL, double unrealizedPnL, double realizedPnL, double value) {
        System.out.println(EWrapperMsgGenerator.pnlSingle(reqId, pos, dailyPnL, unrealizedPnL, realizedPnL, value));
    }
    
    @Override
    public void historicalTicks(int reqId, List<HistoricalTick> ticks, boolean done) {
        for (HistoricalTick tick : ticks) {
            System.out.println(EWrapperMsgGenerator.historicalTick(reqId, tick.time(), tick.price(), tick.size()));
        }
    }
    
    @Override
    public void historicalTicksBidAsk(int reqId, List<HistoricalTickBidAsk> ticks, boolean done) {
        for (HistoricalTickBidAsk tick : ticks) {
            System.out.println(EWrapperMsgGenerator.historicalTickBidAsk(reqId, tick.time(), tick.tickAttribBidAsk(), tick.priceBid(), tick.priceAsk(), tick.sizeBid(),
                    tick.sizeAsk()));
        }
    }   
    
    @Override
    public void historicalTicksLast(int reqId, List<HistoricalTickLast> ticks, boolean done) {
        for (HistoricalTickLast tick : ticks) {
            System.out.println(EWrapperMsgGenerator.historicalTickLast(reqId, tick.time(), tick.tickAttribLast(), tick.price(), tick.size(), tick.exchange(), 
                tick.specialConditions()));
        }
    }

    @Override
    public void tickByTickAllLast(int reqId, int tickType, long time, double price, Decimal size, TickAttribLast tickAttribLast,
            String exchange, String specialConditions) {
        System.out.println(EWrapperMsgGenerator.tickByTickAllLast(reqId, tickType, time, price, size, tickAttribLast, exchange, specialConditions));
    }

    @Override
    public void tickByTickBidAsk(int reqId, long time, double bidPrice, double askPrice, Decimal bidSize, Decimal askSize,
            TickAttribBidAsk tickAttribBidAsk) {
        System.out.println(EWrapperMsgGenerator.tickByTickBidAsk(reqId, time, bidPrice, askPrice, bidSize, askSize, tickAttribBidAsk));
    }

    @Override
    public void tickByTickMidPoint(int reqId, long time, double midPoint) {
        System.out.println(EWrapperMsgGenerator.tickByTickMidPoint(reqId, time, midPoint));
    }

    @Override
    public void orderBound(long permId, int clientId, int orderId) {
        System.out.println(EWrapperMsgGenerator.orderBound(permId, clientId, orderId));
    }

    @Override
    public void completedOrder(Contract contract, Order order, OrderState orderState) {
        System.out.println(EWrapperMsgGenerator.completedOrder(contract, order, orderState));
    }

    @Override
    public void completedOrdersEnd() {
        System.out.println(EWrapperMsgGenerator.completedOrdersEnd());
    }

    @Override
    public void replaceFAEnd(int reqId, String text) {
        System.out.println(EWrapperMsgGenerator.replaceFAEnd(reqId, text));
    }

    @Override
    public void wshMetaData(int reqId, String dataJson) {
        System.out.println(EWrapperMsgGenerator.wshMetaData(reqId, dataJson));
    }

    @Override
    public void wshEventData(int reqId, String dataJson) {
        System.out.println(EWrapperMsgGenerator.wshEventData(reqId, dataJson));
    }

    @Override
    public void historicalSchedule(int reqId, String startDateTime, String endDateTime, String timeZone, List<HistoricalSession> sessions) {
        System.out.println(EWrapperMsgGenerator.historicalSchedule(reqId, startDateTime, endDateTime, timeZone, sessions));
    }
    
    @Override
    public void userInfo(int reqId, String whiteBrandingId) {
        System.out.println(EWrapperMsgGenerator.userInfo(reqId, whiteBrandingId));
    }

    @Override public void currentTimeInMillis(long timeInMillis) {
    	System.out.println(EWrapperMsgGenerator.currentTimeInMillis(timeInMillis));
    }
    
    // ---------------------------------------------- Protobuf ---------------------------------------------
    @Override public void orderStatusProtoBuf(OrderStatusProto.OrderStatus orderStatusProto) { }
    @Override public void openOrderProtoBuf(OpenOrderProto.OpenOrder openOrderProto) { }
    @Override public void openOrdersEndProtoBuf(OpenOrdersEndProto.OpenOrdersEnd openOrdersEnd) { }
    @Override public void errorProtoBuf(ErrorMessageProto.ErrorMessage errorMessageProto) { }
    @Override public void execDetailsProtoBuf(ExecutionDetailsProto.ExecutionDetails executionDetailsProto) { }
    @Override public void execDetailsEndProtoBuf(ExecutionDetailsEndProto.ExecutionDetailsEnd executionDetailsEndProto) { }
    @Override public void completedOrderProtoBuf(CompletedOrderProto.CompletedOrder completedOrderProto) { }
    @Override public void completedOrdersEndProtoBuf(CompletedOrdersEndProto.CompletedOrdersEnd completedOrdersEndProto) { }
    @Override public void orderBoundProtoBuf(OrderBoundProto.OrderBound orderBoundProto) { }
    @Override public void contractDataProtoBuf(ContractDataProto.ContractData contractDataProto) { }
    @Override public void bondContractDataProtoBuf(ContractDataProto.ContractData contractDataProto) { }
    @Override public void contractDataEndProtoBuf(ContractDataEndProto.ContractDataEnd contractDataEndProto) { }
    @Override public void tickPriceProtoBuf(TickPriceProto.TickPrice tickPriceProto) { }
    @Override public void tickSizeProtoBuf(TickSizeProto.TickSize tickSizeProto) { }
    @Override public void tickOptionComputationProtoBuf(TickOptionComputationProto.TickOptionComputation tickOptionComputationProto) { }
    @Override public void tickGenericProtoBuf(TickGenericProto.TickGeneric tickGenericProto) { }
    @Override public void tickStringProtoBuf(TickStringProto.TickString tickStringProto) { }
    @Override public void tickSnapshotEndProtoBuf(TickSnapshotEndProto.TickSnapshotEnd tickSnapshotEndProto) { }
    @Override public void updateMarketDepthProtoBuf(MarketDepthProto.MarketDepth marketDepthProto) { }
    @Override public void updateMarketDepthL2ProtoBuf(MarketDepthL2Proto.MarketDepthL2 marketDepthL2Proto) { }
    @Override public void marketDataTypeProtoBuf(MarketDataTypeProto.MarketDataType marketDataTypeProto) { }
    @Override public void tickReqParamsProtoBuf(TickReqParamsProto.TickReqParams tickReqParamsProto) { }
    @Override public void updateAccountValueProtoBuf(AccountValueProto.AccountValue accounValueProto) { }
    @Override public void updatePortfolioProtoBuf(PortfolioValueProto.PortfolioValue portfolioValueProto) { }
    @Override public void updateAccountTimeProtoBuf(AccountUpdateTimeProto.AccountUpdateTime accountUpdateTimeProto) { }
    @Override public void accountDataEndProtoBuf(AccountDataEndProto.AccountDataEnd accountDataEndProto) { }
    @Override public void managedAccountsProtoBuf(ManagedAccountsProto.ManagedAccounts managedAccountsProto) { }
    @Override public void positionProtoBuf(PositionProto.Position positionProto) { }
    @Override public void positionEndProtoBuf(PositionEndProto.PositionEnd positionEndProto) { }
    @Override public void accountSummaryProtoBuf(AccountSummaryProto.AccountSummary accountSummaryProto) { }
    @Override public void accountSummaryEndProtoBuf(AccountSummaryEndProto.AccountSummaryEnd accountSummaryEndProto) { }
    @Override public void positionMultiProtoBuf(PositionMultiProto.PositionMulti positionMultiProto) { }
    @Override public void positionMultiEndProtoBuf(PositionMultiEndProto.PositionMultiEnd positionMultiEndProto) { }
    @Override public void accountUpdateMultiProtoBuf(AccountUpdateMultiProto.AccountUpdateMulti accountUpdateMultiProto) { }
    @Override public void accountUpdateMultiEndProtoBuf(AccountUpdateMultiEndProto.AccountUpdateMultiEnd accountUpdateMultiEndProto) { }
    @Override public void historicalDataProtoBuf(HistoricalDataProto.HistoricalData historicalDataProto) { }
    @Override public void historicalDataUpdateProtoBuf(HistoricalDataUpdateProto.HistoricalDataUpdate historicalDataUpdateProto) { }
    @Override public void historicalDataEndProtoBuf(HistoricalDataEndProto.HistoricalDataEnd historicalDataEndProto) { }
    @Override public void realTimeBarTickProtoBuf(RealTimeBarTickProto.RealTimeBarTick realTimeBarTickProto) { }
    @Override public void headTimestampProtoBuf(HeadTimestampProto.HeadTimestamp headTimestampProto) { }
    @Override public void histogramDataProtoBuf(HistogramDataProto.HistogramData histogramDataProto) { }
    @Override public void historicalTicksProtoBuf(HistoricalTicksProto.HistoricalTicks historicalTicksProto) { }
    @Override public void historicalTicksBidAskProtoBuf(HistoricalTicksBidAskProto.HistoricalTicksBidAsk historicalTicksBidAskProto) { }
    @Override public void historicalTicksLastProtoBuf(HistoricalTicksLastProto.HistoricalTicksLast historicalTicksLastProto) { }
    @Override public void tickByTickDataProtoBuf(TickByTickDataProto.TickByTickData tickByTickDataProto) { }
    @Override public void updateNewsBulletinProtoBuf(NewsBulletinProto.NewsBulletin newsBulletinProto) { }
    @Override public void newsArticleProtoBuf(NewsArticleProto.NewsArticle newsArticleProto) { }
    @Override public void newsProvidersProtoBuf(NewsProvidersProto.NewsProviders newsProvidersProto) { }
    @Override public void historicalNewsProtoBuf(HistoricalNewsProto.HistoricalNews historicalNewsProto) { }
    @Override public void historicalNewsEndProtoBuf(HistoricalNewsEndProto.HistoricalNewsEnd historicalNewsEndProto) { }
    @Override public void wshMetaDataProtoBuf(WshMetaDataProto.WshMetaData wshMetaDataProto) { }
    @Override public void wshEventDataProtoBuf(WshEventDataProto.WshEventData wshEventDataProto) { }
    @Override public void tickNewsProtoBuf(TickNewsProto.TickNews tickNewsProto) { }
    @Override public void scannerParametersProtoBuf(ScannerParametersProto.ScannerParameters scannerParametersProto) { }
    @Override public void scannerDataProtoBuf(ScannerDataProto.ScannerData scannerDataProto) { }
    @Override public void fundamentalsDataProtoBuf(FundamentalsDataProto.FundamentalsData fundamentalsDataProto) { }
    @Override public void pnlProtoBuf(PnLProto.PnL pnlProto) { }
    @Override public void pnlSingleProtoBuf(PnLSingleProto.PnLSingle pnlSingleProto) { }
    @Override public void receiveFAProtoBuf(ReceiveFAProto.ReceiveFA receiveFAProto) { }
    @Override public void replaceFAEndProtoBuf(ReplaceFAEndProto.ReplaceFAEnd replaceFAEndProto) { }
    @Override public void commissionAndFeesReportProtoBuf(CommissionAndFeesReportProto.CommissionAndFeesReport commissionAndFeesReportProto) { }
    @Override public void historicalScheduleProtoBuf(HistoricalScheduleProto.HistoricalSchedule historicalScheduleProto) { }
    @Override public void rerouteMarketDataRequestProtoBuf(RerouteMarketDataRequestProto.RerouteMarketDataRequest rerouteMarketDataRequestProto) { }
    @Override public void rerouteMarketDepthRequestProtoBuf(RerouteMarketDepthRequestProto.RerouteMarketDepthRequest rerouteMarketDepthRequestProto) { }
    @Override public void secDefOptParameterProtoBuf(SecDefOptParameterProto.SecDefOptParameter secDefOptParameterProto) { }
    @Override public void secDefOptParameterEndProtoBuf(SecDefOptParameterEndProto.SecDefOptParameterEnd secDefOptParameterEndProto) { }
    @Override public void softDollarTiersProtoBuf(SoftDollarTiersProto.SoftDollarTiers softDollarTiersProto) { }
    @Override public void familyCodesProtoBuf(FamilyCodesProto.FamilyCodes familyCodesProto) { }
    @Override public void symbolSamplesProtoBuf(SymbolSamplesProto.SymbolSamples symbolSamplesProto) { }
    @Override public void smartComponentsProtoBuf(SmartComponentsProto.SmartComponents smartComponentsProto) { }
    @Override public void marketRuleProtoBuf(MarketRuleProto.MarketRule marketRuleProto) { }
    @Override public void userInfoProtoBuf(UserInfoProto.UserInfo userInfoProto) { }
    @Override public void nextValidIdProtoBuf(NextValidIdProto.NextValidId nextValidIdProto) { }
    @Override public void currentTimeProtoBuf(CurrentTimeProto.CurrentTime currentTimeProto) { }
    @Override public void currentTimeInMillisProtoBuf(CurrentTimeInMillisProto.CurrentTimeInMillis currentTimeInMillisProto) { }
    @Override public void verifyMessageApiProtoBuf(VerifyMessageApiProto.VerifyMessageApi verifyMessageApiProto) { }
    @Override public void verifyCompletedProtoBuf(VerifyCompletedProto.VerifyCompleted verifyCompletedProto) { }
    @Override public void displayGroupListProtoBuf(DisplayGroupListProto.DisplayGroupList displayGroupListProto) { }
    @Override public void displayGroupUpdatedProtoBuf(DisplayGroupUpdatedProto.DisplayGroupUpdated displayGroupUpdatedProto) { }
    @Override public void marketDepthExchangesProtoBuf(MarketDepthExchangesProto.MarketDepthExchanges marketDepthExchangesProto) { }
}
