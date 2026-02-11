/* Copyright (C) 2025 Interactive Brokers LLC. All rights reserved. This code is subject to the terms
 * and conditions of the IB API Non-Commercial License or the IB API Commercial License, as applicable. */

#include "StdAfx.h"
#include "EWrapper.h"
#include "Order.h"
#include "Contract.h"
#include "OrderState.h"
#include "Execution.h"
#include "FamilyCode.h"
#include "CommissionAndFeesReport.h"
#include "TwsSocketClientErrors.h"
#include "EDecoder.h"
#include "EDecoderUtils.h"
#include "EClientMsgSink.h"
#include "PriceIncrement.h"
#include "EOrderDecoder.h"
#include "Utils.h"
#include "IneligibilityReason.h"
#include "EClient.h"
#include "ExecutionDetails.pb.h"
#include "ExecutionDetailsEnd.pb.h"

#include <string.h>
#include <cstdlib>
#include <sstream>
#include <assert.h>
#include <string>
#include <bitset>
#include <cmath>

using namespace ibapi::client_constants;

EDecoder::EDecoder(int serverVersion, EWrapper *callback, EClientMsgSink *clientMsgSink) {
	m_pEWrapper = callback;
	m_serverVersion = serverVersion;
	m_pClientMsgSink = clientMsgSink;
}

const char* EDecoder::processTickPriceMsg(const char* ptr, const char* endPtr) {
	int version;
	int tickerId;
	int tickTypeInt;
	double price;

	Decimal size;
	int attrMask;

	DECODE_FIELD( version);
	DECODE_FIELD( tickerId);
	DECODE_FIELD( tickTypeInt);
	DECODE_FIELD( price);
	DECODE_FIELD( size); // ver 2 field
	DECODE_FIELD( attrMask); // ver 3 field

	TickAttrib attrib = {};

	attrib.canAutoExecute = attrMask == 1;

	if (m_serverVersion >= MIN_SERVER_VER_PAST_LIMIT)
	{
		std::bitset<32> mask(attrMask);

		attrib.canAutoExecute = mask[0];
		attrib.pastLimit = mask[1];

		if (m_serverVersion >= MIN_SERVER_VER_PRE_OPEN_BID_ASK)
		{
			attrib.preOpen = mask[2];
		}
	}

	m_pEWrapper->tickPrice( tickerId, (TickType)tickTypeInt, price, attrib);

	// process ver 2 fields
	{
		TickType sizeTickType = NOT_SET;
		switch( (TickType)tickTypeInt) {
		case BID:
			sizeTickType = BID_SIZE;
			break;
		case ASK:
			sizeTickType = ASK_SIZE;
			break;
		case LAST:
			sizeTickType = LAST_SIZE;
			break;
		case DELAYED_BID:
			sizeTickType = DELAYED_BID_SIZE;
			break;
		case DELAYED_ASK:
			sizeTickType = DELAYED_ASK_SIZE;
			break;
		case DELAYED_LAST:
			sizeTickType = DELAYED_LAST_SIZE;
			break;
		default:
			break;
		}
		if( sizeTickType != NOT_SET)
			m_pEWrapper->tickSize( tickerId, sizeTickType, size);
	}

	return ptr;
}

const char* EDecoder::processTickPriceMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::TickPrice tickPriceProto;
	tickPriceProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->tickPriceProtoBuf(tickPriceProto);
#endif

	int reqId = tickPriceProto.has_reqid() ? tickPriceProto.reqid() : NO_VALID_ID;
	int tickType = tickPriceProto.has_ticktype() ? tickPriceProto.ticktype() : 0;
	double price = tickPriceProto.has_price() ? tickPriceProto.price() : 0;
	Decimal size = tickPriceProto.has_size() ? DecimalFunctions::stringToDecimal(tickPriceProto.size()) : UNSET_DECIMAL;
	int attrMask = tickPriceProto.has_attrmask() ? tickPriceProto.attrmask() : 0;

	TickAttrib attrib = {};
	std::bitset<32> mask(attrMask);
	attrib.canAutoExecute = mask[0];
	attrib.pastLimit = mask[1];
	attrib.preOpen = mask[2];

	m_pEWrapper->tickPrice(reqId, (TickType)tickType, price, attrib);

	// process size tick
	TickType sizeTickType = NOT_SET;
	switch ((TickType)tickType) {
		case BID:
			sizeTickType = BID_SIZE;
			break;
		case ASK:
			sizeTickType = ASK_SIZE;
			break;
		case LAST:
			sizeTickType = LAST_SIZE;
			break;
		case DELAYED_BID:
			sizeTickType = DELAYED_BID_SIZE;
			break;
		case DELAYED_ASK:
			sizeTickType = DELAYED_ASK_SIZE;
			break;
		case DELAYED_LAST:
			sizeTickType = DELAYED_LAST_SIZE;
			break;
		default:
			break;
	}
	if (sizeTickType != NOT_SET)
		m_pEWrapper->tickSize(reqId, sizeTickType, size);

	return ptr;
}

const char* EDecoder::processTickSizeMsg(const char* ptr, const char* endPtr) {
	int version;
	int tickerId;
	int tickTypeInt;
	Decimal size;

	DECODE_FIELD( version);
	DECODE_FIELD( tickerId);
	DECODE_FIELD( tickTypeInt);
	DECODE_FIELD( size);

	m_pEWrapper->tickSize( tickerId, (TickType)tickTypeInt, size);

	return ptr;
}

const char* EDecoder::processTickSizeMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::TickSize tickSizeProto;
	tickSizeProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->tickSizeProtoBuf(tickSizeProto);
#endif

	int reqId = tickSizeProto.has_reqid() ? tickSizeProto.reqid() : NO_VALID_ID;
	int tickType = tickSizeProto.has_ticktype() ? tickSizeProto.ticktype() : 0;
	Decimal size = tickSizeProto.has_size() ? DecimalFunctions::stringToDecimal(tickSizeProto.size()) : UNSET_DECIMAL;

	m_pEWrapper->tickSize(reqId, (TickType)tickType, size);

	return ptr;
}

const char* EDecoder::processTickOptionComputationMsg(const char* ptr, const char* endPtr) {
	int version = m_serverVersion;
	int tickerId;
	int tickTypeInt;
	int tickAttrib = 0;
	double impliedVol;
	double delta;

	double optPrice = DBL_MAX;
	double pvDividend = DBL_MAX;

	double gamma = DBL_MAX;
	double vega = DBL_MAX;
	double theta = DBL_MAX;
	double undPrice = DBL_MAX;

	if (m_serverVersion < MIN_SERVER_VER_PRICE_BASED_VOLATILITY)
	{
		DECODE_FIELD(version);
	}

	DECODE_FIELD( tickerId);
	DECODE_FIELD( tickTypeInt);

	if (m_serverVersion >= MIN_SERVER_VER_PRICE_BASED_VOLATILITY)
	{
		DECODE_FIELD( tickAttrib);
	}

	DECODE_FIELD( impliedVol);
	DECODE_FIELD( delta);

	if( impliedVol == -1) { // -1 is the "not computed" indicator
		impliedVol = DBL_MAX;
	}
	if( delta == -2) { // -2 is the "not computed" indicator
		delta = DBL_MAX;
	}

	if( version >= 6 || tickTypeInt == MODEL_OPTION || tickTypeInt == DELAYED_MODEL_OPTION_COMPUTATION) { // introduced in version == 5

		DECODE_FIELD( optPrice);
		DECODE_FIELD( pvDividend);

		if( optPrice == -1) { // -1 is the "not computed" indicator
			optPrice = DBL_MAX;
		}
		if( pvDividend == -1) { // -1 is the "not computed" indicator
			pvDividend = DBL_MAX;
		}
	}
	if( version >= 6) {

		DECODE_FIELD( gamma);
		DECODE_FIELD( vega);
		DECODE_FIELD( theta);
		DECODE_FIELD( undPrice);

		if( gamma == -2) { // -2 is the "not yet computed" indicator
			gamma = DBL_MAX;
		}
		if( vega == -2) { // -2 is the "not yet computed" indicator
			vega = DBL_MAX;
		}
		if( theta == -2) { // -2 is the "not yet computed" indicator
			theta = DBL_MAX;
		}
		if( undPrice == -1) { // -1 is the "not computed" indicator
			undPrice = DBL_MAX;
		}
	}
	m_pEWrapper->tickOptionComputation( tickerId, (TickType)tickTypeInt, tickAttrib,
		impliedVol, delta, optPrice, pvDividend, gamma, vega, theta, undPrice);

	return ptr;
}

const char* EDecoder::processTickOptionComputationMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::TickOptionComputation tickOptionComputationProto;
	tickOptionComputationProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->tickOptionComputationProtoBuf(tickOptionComputationProto);
#endif

	int reqId = tickOptionComputationProto.has_reqid() ? tickOptionComputationProto.reqid() : NO_VALID_ID;
	int tickType = tickOptionComputationProto.has_ticktype() ? tickOptionComputationProto.ticktype() : 0;
	int tickAttrib = tickOptionComputationProto.has_tickattrib() ? tickOptionComputationProto.tickattrib() : 0;

	double impliedVol = tickOptionComputationProto.has_impliedvol() ? tickOptionComputationProto.impliedvol() : UNSET_DOUBLE;
	if (impliedVol == -1) { // -1 is the "not computed" indicator
		impliedVol = UNSET_DOUBLE;
	}
	double delta = tickOptionComputationProto.has_delta() ? tickOptionComputationProto.delta() : UNSET_DOUBLE;
	if (delta == -2) { // -2 is the "not computed" indicator
		delta = UNSET_DOUBLE;
	}
	double optPrice = tickOptionComputationProto.has_optprice() ? tickOptionComputationProto.optprice() : UNSET_DOUBLE;
	if (optPrice == -1) { // -1 is the "not computed" indicator
		optPrice = UNSET_DOUBLE;
	}
	double pvDividend = tickOptionComputationProto.has_pvdividend() ? tickOptionComputationProto.pvdividend() : UNSET_DOUBLE;
	if (pvDividend == -1) { // -1 is the "not computed" indicator
		pvDividend = UNSET_DOUBLE;
	}
	double gamma = tickOptionComputationProto.has_gamma() ? tickOptionComputationProto.gamma() : UNSET_DOUBLE;
	if (gamma == -2) { // -2 is the "not computed" indicator
		gamma = UNSET_DOUBLE;
	}
	double vega = tickOptionComputationProto.has_vega() ? tickOptionComputationProto.vega() : UNSET_DOUBLE;
	if (vega == -2) { // -2 is the "not computed" indicator
		vega = UNSET_DOUBLE;
	}
	double theta = tickOptionComputationProto.has_theta() ? tickOptionComputationProto.theta() : UNSET_DOUBLE;
	if (theta == -2) { // -2 is the "not computed" indicator
		theta = UNSET_DOUBLE;
	}
	double undPrice = tickOptionComputationProto.has_undprice() ? tickOptionComputationProto.undprice() : UNSET_DOUBLE;
	if (undPrice == -1) { // -1 is the "not computed" indicator
		undPrice = UNSET_DOUBLE;
	}

	m_pEWrapper->tickOptionComputation(reqId, (TickType)tickType, tickAttrib, impliedVol, delta, optPrice, pvDividend, gamma, vega, theta, undPrice);

	return ptr;
}

const char* EDecoder::processTickGenericMsg(const char* ptr, const char* endPtr) {
	int version;
	int tickerId;
	int tickTypeInt;
	double value;

	DECODE_FIELD( version);
	DECODE_FIELD( tickerId);
	DECODE_FIELD( tickTypeInt);
	DECODE_FIELD( value);

	m_pEWrapper->tickGeneric( tickerId, (TickType)tickTypeInt, value);

	return ptr;
}

const char* EDecoder::processTickGenericMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::TickGeneric tickGenericProto;
	tickGenericProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->tickGenericProtoBuf(tickGenericProto);
#endif

	int reqId = tickGenericProto.has_reqid() ? tickGenericProto.reqid() : NO_VALID_ID;
	int tickType = tickGenericProto.has_ticktype() ? tickGenericProto.ticktype() : 0;
	double value = tickGenericProto.has_value() ? tickGenericProto.value() : 0;

	m_pEWrapper->tickGeneric(reqId, (TickType)tickType, value);

	return ptr;
}

const char* EDecoder::processTickStringMsg(const char* ptr, const char* endPtr) {
	int version;
	int tickerId;
	int tickTypeInt;
	std::string value;

	DECODE_FIELD( version);
	DECODE_FIELD( tickerId);
	DECODE_FIELD( tickTypeInt);
	DECODE_FIELD( value);

	m_pEWrapper->tickString( tickerId, (TickType)tickTypeInt, value);

	return ptr;
}

const char* EDecoder::processTickStringMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::TickString tickStringProto;
	tickStringProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->tickStringProtoBuf(tickStringProto);
#endif

	int reqId = tickStringProto.has_reqid() ? tickStringProto.reqid() : NO_VALID_ID;
	int tickType = tickStringProto.has_ticktype() ? tickStringProto.ticktype() : 0;
	std::string value = tickStringProto.has_value() ? tickStringProto.value() : "";

	m_pEWrapper->tickString(reqId, (TickType)tickType, value);

	return ptr;
}

const char* EDecoder::processTickEfpMsg(const char* ptr, const char* endPtr) {
	int version;
	int tickerId;
	int tickTypeInt;
	double basisPoints;
	std::string formattedBasisPoints;
	double impliedFuturesPrice;
	int holdDays;
	std::string futureLastTradeDate;
	double dividendImpact;
	double dividendsToLastTradeDate;

	DECODE_FIELD( version);
	DECODE_FIELD( tickerId);
	DECODE_FIELD( tickTypeInt);
	DECODE_FIELD( basisPoints);
	DECODE_FIELD( formattedBasisPoints);
	DECODE_FIELD( impliedFuturesPrice);
	DECODE_FIELD( holdDays);
	DECODE_FIELD( futureLastTradeDate);
	DECODE_FIELD( dividendImpact);
	DECODE_FIELD( dividendsToLastTradeDate);

	m_pEWrapper->tickEFP( tickerId, (TickType)tickTypeInt, basisPoints, formattedBasisPoints,
		impliedFuturesPrice, holdDays, futureLastTradeDate, dividendImpact, dividendsToLastTradeDate);

	return ptr;
}

const char* EDecoder::processOrderStatusMsg(const char* ptr, const char* endPtr) {
    int version = INT_MAX;
	int orderId;
	std::string status;
	Decimal filled;
	Decimal remaining;
	double avgFillPrice;
	long long permId;
	int parentId;
	double lastFillPrice;
	int clientId;
	std::string whyHeld;

    if (m_serverVersion < MIN_SERVER_VER_MARKET_CAP_PRICE) 
    {
	    DECODE_FIELD( version);
    }
	
    DECODE_FIELD( orderId);
	DECODE_FIELD( status);
	DECODE_FIELD( filled);
	DECODE_FIELD( remaining);
	DECODE_FIELD( avgFillPrice);
	DECODE_FIELD( permId); // ver 2 field
	DECODE_FIELD( parentId); // ver 3 field
	DECODE_FIELD( lastFillPrice); // ver 4 field
	DECODE_FIELD( clientId); // ver 5 field
	DECODE_FIELD( whyHeld); // ver 6 field

	double mktCapPrice = UNSET_DOUBLE;

	if (m_serverVersion >= MIN_SERVER_VER_MARKET_CAP_PRICE)
	{
		DECODE_FIELD(mktCapPrice);
	}

	m_pEWrapper->orderStatus( orderId, status, filled, remaining,
		avgFillPrice, permId, parentId, lastFillPrice, clientId, whyHeld, mktCapPrice);


	return ptr;
}

const char* EDecoder::processOrderStatusMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::OrderStatus orderStatusProto;
	orderStatusProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->orderStatusProtoBuf(orderStatusProto);
#endif

	int orderId = orderStatusProto.has_orderid() ? orderStatusProto.orderid() : UNSET_INTEGER;
	std::string status = orderStatusProto.has_status() ? orderStatusProto.status() : "";
	Decimal filled = orderStatusProto.has_filled() ? DecimalFunctions::stringToDecimal(orderStatusProto.filled()) : UNSET_DECIMAL;
	Decimal remaining = orderStatusProto.has_remaining() ? DecimalFunctions::stringToDecimal(orderStatusProto.remaining()) : UNSET_DECIMAL;
	double avgFillPrice = orderStatusProto.has_avgfillprice() ? orderStatusProto.avgfillprice() : UNSET_DOUBLE;
	long long permId = orderStatusProto.has_permid() ? orderStatusProto.permid() : UNSET_LLONG;
	int parentId = orderStatusProto.has_parentid() ? orderStatusProto.parentid() : UNSET_INTEGER;
	double lastFillPrice = orderStatusProto.has_lastfillprice() ? orderStatusProto.lastfillprice() : UNSET_DOUBLE;
	int clientId = orderStatusProto.has_clientid() ? orderStatusProto.clientid() : UNSET_INTEGER;
	std::string whyHeld = orderStatusProto.has_whyheld() ? orderStatusProto.whyheld() : "";
	double mktCapPrice = orderStatusProto.has_mktcapprice() ? orderStatusProto.mktcapprice() : UNSET_DOUBLE;

	m_pEWrapper->orderStatus(orderId, status, filled, remaining, avgFillPrice, permId, parentId, lastFillPrice, clientId, whyHeld, mktCapPrice);

	return ptr;
}

const char* EDecoder::processErrMsgMsg(const char* ptr, const char* endPtr) {
	int version;
	int id;
	time_t errorTime = 0;
	int errorCode;
	std::string errorMsg;
	std::string advancedOrderRejectJson;

	if (m_serverVersion < MIN_SERVER_VER_ERROR_TIME) {
		DECODE_FIELD( version);
	}	

	DECODE_FIELD( id);
	DECODE_FIELD( errorCode);
	DECODE_FIELD( errorMsg);

	if (m_serverVersion >= MIN_SERVER_VER_ADVANCED_ORDER_REJECT)
	{
		DECODE_FIELD( advancedOrderRejectJson);
	}

	if (m_serverVersion >= MIN_SERVER_VER_ERROR_TIME) {
		DECODE_FIELD_TIME(errorTime);
	}

	m_pEWrapper->error( id, errorTime, errorCode, errorMsg, advancedOrderRejectJson);

	return ptr;
}

const char* EDecoder::processErrorMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::ErrorMessage errorMessageProto;
	errorMessageProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->errorProtoBuf(errorMessageProto);
#endif

	int id = errorMessageProto.has_id() ? errorMessageProto.id() : 0;
	int errorCode = errorMessageProto.has_errorcode() ? errorMessageProto.errorcode() : 0;
	std::string errorMsg = errorMessageProto.has_errormsg() ? errorMessageProto.errormsg() : "";
	std::string advancedOrderRejectJson = errorMessageProto.has_advancedorderrejectjson() ? errorMessageProto.advancedorderrejectjson() : "";
	time_t errorTime = errorMessageProto.has_errortime() ? errorMessageProto.errortime() : 0;

	m_pEWrapper->error(id, errorTime, errorCode, errorMsg, advancedOrderRejectJson);

	return ptr;
}

const char* EDecoder::processOpenOrderMsg(const char* ptr, const char* endPtr) {
	// read version
	int version = m_serverVersion;

    if (m_serverVersion < MIN_SERVER_VER_ORDER_CONTAINER) {
	    DECODE_FIELD(version);
    }

	Order order;
	Contract contract;
	OrderState orderState;
	EOrderDecoder eOrderDecoder(&contract, &order, &orderState, version, m_serverVersion);

	// read order id
        if (!eOrderDecoder.decodeOrderId(ptr, endPtr)) {
          return nullptr;
        }

	// read contract fields
        if (!eOrderDecoder.decodeContract(ptr, endPtr)) {
          return nullptr;
        }


	// read order fields
        bool success =
             eOrderDecoder.decodeAction(ptr, endPtr)
          && eOrderDecoder.decodeTotalQuantity(ptr, endPtr)
          && eOrderDecoder.decodeOrderType(ptr, endPtr)
          && eOrderDecoder.decodeLmtPrice(ptr, endPtr)
          && eOrderDecoder.decodeAuxPrice(ptr, endPtr)
          && eOrderDecoder.decodeTIF(ptr, endPtr)
          && eOrderDecoder.decodeOcaGroup(ptr, endPtr)
          && eOrderDecoder.decodeAccount(ptr, endPtr)
          && eOrderDecoder.decodeOpenClose(ptr, endPtr)
          && eOrderDecoder.decodeOrigin(ptr, endPtr)
          && eOrderDecoder.decodeOrderRef(ptr, endPtr)
          && eOrderDecoder.decodeClientId(ptr, endPtr)
          && eOrderDecoder.decodePermId(ptr, endPtr)
          && eOrderDecoder.decodeOutsideRth(ptr, endPtr)
          && eOrderDecoder.decodeHidden(ptr, endPtr)
          && eOrderDecoder.decodeDiscretionaryAmount(ptr, endPtr)
          && eOrderDecoder.decodeGoodAfterTime(ptr, endPtr)
          && eOrderDecoder.skipSharesAllocation(ptr, endPtr)
          && eOrderDecoder.decodeFAParams(ptr, endPtr)
          && eOrderDecoder.decodeModelCode(ptr, endPtr)
          && eOrderDecoder.decodeGoodTillDate(ptr, endPtr)
          && eOrderDecoder.decodeRule80A(ptr, endPtr)
          && eOrderDecoder.decodePercentOffset(ptr, endPtr)
          && eOrderDecoder.decodeSettlingFirm(ptr, endPtr)
          && eOrderDecoder.decodeShortSaleParams(ptr, endPtr)
          && eOrderDecoder.decodeAuctionStrategy(ptr, endPtr)
          && eOrderDecoder.decodeBoxOrderParams(ptr, endPtr)
          && eOrderDecoder.decodePegToStkOrVolOrderParams(ptr, endPtr)
          && eOrderDecoder.decodeDisplaySize(ptr, endPtr)
          && eOrderDecoder.decodeBlockOrder(ptr, endPtr)
          && eOrderDecoder.decodeSweepToFill(ptr, endPtr)
          && eOrderDecoder.decodeAllOrNone(ptr, endPtr)
          && eOrderDecoder.decodeMinQty(ptr, endPtr)
          && eOrderDecoder.decodeOcaType(ptr, endPtr)
          && eOrderDecoder.skipETradeOnly(ptr, endPtr)
          && eOrderDecoder.skipFirmQuoteOnly(ptr, endPtr)
          && eOrderDecoder.skipNbboPriceCap(ptr, endPtr)
          && eOrderDecoder.decodeParentId(ptr, endPtr)
          && eOrderDecoder.decodeTriggerMethod(ptr, endPtr)
          && eOrderDecoder.decodeVolOrderParams(ptr, endPtr, true)
          && eOrderDecoder.decodeTrailParams(ptr, endPtr)
          && eOrderDecoder.decodeBasisPoints(ptr, endPtr)
          && eOrderDecoder.decodeComboLegs(ptr, endPtr)
          && eOrderDecoder.decodeSmartComboRoutingParams(ptr, endPtr)
          && eOrderDecoder.decodeScaleOrderParams(ptr, endPtr)
          && eOrderDecoder.decodeHedgeParams(ptr, endPtr)
          && eOrderDecoder.decodeOptOutSmartRouting(ptr, endPtr)
          && eOrderDecoder.decodeClearingParams(ptr, endPtr)
          && eOrderDecoder.decodeNotHeld(ptr, endPtr)
          && eOrderDecoder.decodeDeltaNeutral(ptr, endPtr)
          && eOrderDecoder.decodeAlgoParams(ptr, endPtr)
          && eOrderDecoder.decodeSolicited(ptr, endPtr)
          && eOrderDecoder.decodeWhatIfInfoAndCommissionAndFees(ptr, endPtr)
          && eOrderDecoder.decodeVolRandomizeFlags(ptr, endPtr)
          && eOrderDecoder.decodePegBenchParams(ptr, endPtr)
          && eOrderDecoder.decodeConditions(ptr, endPtr)
          && eOrderDecoder.decodeAdjustedOrderParams(ptr, endPtr)
          && eOrderDecoder.decodeSoftDollarTier(ptr, endPtr)
          && eOrderDecoder.decodeCashQty(ptr, endPtr)
          && eOrderDecoder.decodeDontUseAutoPriceForHedge(ptr, endPtr)
          && eOrderDecoder.decodeIsOmsContainer(ptr, endPtr)
          && eOrderDecoder.decodeDiscretionaryUpToLimitPrice(ptr, endPtr)
          && eOrderDecoder.decodeUsePriceMgmtAlgo(ptr, endPtr)
          && eOrderDecoder.decodeDuration(ptr, endPtr)
          && eOrderDecoder.decodePostToAts(ptr, endPtr)
          && eOrderDecoder.decodeAutoCancelParent(ptr, endPtr, MIN_SERVER_VER_AUTO_CANCEL_PARENT)
          && eOrderDecoder.decodePegBestPegMidOrderAttributes(ptr, endPtr)
          && eOrderDecoder.decodeCustomerAccount(ptr, endPtr)
          && eOrderDecoder.decodeProfessionalCustomer(ptr, endPtr)
          && eOrderDecoder.decodeBondAccruedInterest(ptr, endPtr)
          && eOrderDecoder.decodeIncludeOvernight(ptr, endPtr)
          && eOrderDecoder.decodeCMETaggingFields(ptr, endPtr)
          && eOrderDecoder.decodeSubmitter(ptr, endPtr)
          && eOrderDecoder.decodeImbalanceOnly(ptr, endPtr, MIN_SERVER_VER_IMBALANCE_ONLY);

        if (!success) {
          return nullptr;
        }

	m_pEWrapper->openOrder((OrderId)order.orderId, contract, order, orderState);

	return ptr;
} 


const char* EDecoder::processOpenOrderMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::OpenOrder openOrderProto;
	openOrderProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->openOrderProtoBuf(openOrderProto);
#endif

	int orderId = openOrderProto.has_orderid() ? openOrderProto.orderid() : 0;
	Contract contract;
	if (openOrderProto.has_contract()) contract = EDecoderUtils::decodeContract(openOrderProto.contract());
	Order order;
	if (openOrderProto.has_order()) order = EDecoderUtils::decodeOrder(orderId, openOrderProto.contract(), openOrderProto.order());
	OrderState orderState;
	if (openOrderProto.has_orderstate()) orderState = EDecoderUtils::decodeOrderState(openOrderProto.orderstate());

	m_pEWrapper->openOrder((OrderId)orderId, contract, order, orderState);

	return ptr;
}

const char* EDecoder::processAcctValueMsg(const char* ptr, const char* endPtr) {
	int version;
	std::string key;
	std::string val;
	std::string cur;
	std::string accountName;

	DECODE_FIELD( version);
	DECODE_FIELD( key);
	DECODE_FIELD( val);
	DECODE_FIELD( cur);
	DECODE_FIELD( accountName); // ver 2 field

	m_pEWrapper->updateAccountValue( key, val, cur, accountName);
	return ptr;
}

const char* EDecoder::processAccountValueMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::AccountValue accountValueProto;
	accountValueProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->updateAccountValueProtoBuf(accountValueProto);
#endif

	std::string key = accountValueProto.has_key() ? accountValueProto.key() : "";
	std::string value = accountValueProto.has_value() ? accountValueProto.value() : "";
	std::string currency = accountValueProto.has_currency() ? accountValueProto.currency() : "";
	std::string accountName = accountValueProto.has_accountname() ? accountValueProto.accountname() : "";

	m_pEWrapper->updateAccountValue(key, value, currency, accountName);

	return ptr;
}

const char* EDecoder::processPortfolioValueMsg(const char* ptr, const char* endPtr) {
	// decode version
	int version;
	DECODE_FIELD( version);

	// read contract fields
	Contract contract;
	DECODE_FIELD( contract.conId); // ver 6 field
	DECODE_FIELD( contract.symbol);
	DECODE_FIELD( contract.secType);
	DECODE_FIELD( contract.lastTradeDateOrContractMonth);
	DECODE_FIELD( contract.strike);
	DECODE_FIELD( contract.right);

	if( version >= 7) {
		DECODE_FIELD( contract.multiplier);
		DECODE_FIELD( contract.primaryExchange);
	}

	DECODE_FIELD( contract.currency);
	DECODE_FIELD( contract.localSymbol); // ver 2 field
	if (version >= 8) {
		DECODE_FIELD( contract.tradingClass);
	}

	Decimal  position;
	double  marketPrice;
	double  marketValue;
	double  averageCost;
	double  unrealizedPNL;
	double  realizedPNL;

	DECODE_FIELD( position);
	DECODE_FIELD( marketPrice);
	DECODE_FIELD( marketValue);
	DECODE_FIELD( averageCost); // ver 3 field
	DECODE_FIELD( unrealizedPNL); // ver 3 field
	DECODE_FIELD( realizedPNL); // ver 3 field

	std::string accountName;
	DECODE_FIELD( accountName); // ver 4 field

	if( version == 6 && m_serverVersion == 39) {
		DECODE_FIELD( contract.primaryExchange);
	}

	m_pEWrapper->updatePortfolio( contract,
		position, marketPrice, marketValue, averageCost,
		unrealizedPNL, realizedPNL, accountName);

	return ptr;
}

const char* EDecoder::processPortfolioValueMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::PortfolioValue portfolioValueProto;
	portfolioValueProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->updatePortfolioProtoBuf(portfolioValueProto);
#endif

	if (!portfolioValueProto.has_contract()) {
		return ptr;
	}

	Contract contract = EDecoderUtils::decodeContract(portfolioValueProto.contract());
	Decimal position = portfolioValueProto.has_position() ? DecimalFunctions::stringToDecimal(portfolioValueProto.position()) : UNSET_DECIMAL;
	double marketPrice = portfolioValueProto.has_marketprice() ? portfolioValueProto.marketprice() : 0;
	double marketValue = portfolioValueProto.has_marketvalue() ? portfolioValueProto.marketvalue() : 0;
	double averageCost = portfolioValueProto.has_averagecost() ? portfolioValueProto.averagecost() : 0;
	double unrealizedPNL = portfolioValueProto.has_unrealizedpnl() ? portfolioValueProto.unrealizedpnl() : 0;
	double realizedPNL = portfolioValueProto.has_realizedpnl() ? portfolioValueProto.realizedpnl() : 0;
	std::string accountName = portfolioValueProto.has_accountname() ? portfolioValueProto.accountname() : "";

	m_pEWrapper->updatePortfolio(contract, position, marketPrice, marketValue, averageCost, unrealizedPNL, realizedPNL, accountName);

	return ptr;
}

const char* EDecoder::processAcctUpdateTimeMsg(const char* ptr, const char* endPtr) {
	int version;
	std::string accountTime;

	DECODE_FIELD( version);
	DECODE_FIELD( accountTime);

	m_pEWrapper->updateAccountTime( accountTime);

	return ptr;
}

const char* EDecoder::processAcctUpdateTimeMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::AccountUpdateTime accountUpdateTimeProto;
	accountUpdateTimeProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->updateAccountTimeProtoBuf(accountUpdateTimeProto);
#endif

	std::string timeStamp = accountUpdateTimeProto.has_timestamp() ? accountUpdateTimeProto.timestamp() : "";

	m_pEWrapper->updateAccountTime(timeStamp);

	return ptr;
}

const char* EDecoder::processNextValidIdMsg(const char* ptr, const char* endPtr) {
	int version;
	int orderId;

	DECODE_FIELD( version);
	DECODE_FIELD( orderId);

	m_pEWrapper->nextValidId(orderId);

	return ptr;
}

const char* EDecoder::processNextValidIdMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::NextValidId nextValidIdProto;
	nextValidIdProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->nextValidIdProtoBuf(nextValidIdProto);
#endif

	int orderId = nextValidIdProto.has_orderid() ? nextValidIdProto.orderid() : 0;

	m_pEWrapper->nextValidId(orderId);

	return ptr;
}

const char* EDecoder::processContractDataMsg(const char* ptr, const char* endPtr) {
	int version = 8;
	if (m_serverVersion < MIN_SERVER_VER_SIZE_RULES) {
		DECODE_FIELD(version);
	}

	int reqId = -1;
	if( version >= 3) {
		DECODE_FIELD( reqId);
	}

	ContractDetails contract;
	DECODE_FIELD( contract.contract.symbol);
	DECODE_FIELD( contract.contract.secType);
	ptr = decodeLastTradeDate(ptr, endPtr, contract, false);
	if (m_serverVersion >= MIN_SERVER_VER_LAST_TRADE_DATE) {
		DECODE_FIELD( contract.contract.lastTradeDate);
	}
	DECODE_FIELD( contract.contract.strike);
	DECODE_FIELD( contract.contract.right);
	DECODE_FIELD( contract.contract.exchange);
	DECODE_FIELD( contract.contract.currency);
	DECODE_FIELD( contract.contract.localSymbol);
	DECODE_FIELD( contract.marketName);
	DECODE_FIELD( contract.contract.tradingClass);
	DECODE_FIELD( contract.contract.conId);
	DECODE_FIELD( contract.minTick);
	if (m_serverVersion >= MIN_SERVER_VER_MD_SIZE_MULTIPLIER && m_serverVersion < MIN_SERVER_VER_SIZE_RULES) {
		int mdSizeMultiplier;
		DECODE_FIELD( mdSizeMultiplier); // not used anymore
	}
	DECODE_FIELD( contract.contract.multiplier);
	DECODE_FIELD( contract.orderTypes);
	DECODE_FIELD( contract.validExchanges);
	DECODE_FIELD( contract.priceMagnifier); // ver 2 field
	if( version >= 4) {
		DECODE_FIELD( contract.underConId);
	}
	if( version >= 5) {
		DECODE_FIELD( contract.longName);
		DECODE_FIELD( contract.contract.primaryExchange);
	}
	if( version >= 6) {
		DECODE_FIELD( contract.contractMonth);
		DECODE_FIELD( contract.industry);
		DECODE_FIELD( contract.category);
		DECODE_FIELD( contract.subcategory);
		DECODE_FIELD( contract.timeZoneId);
		DECODE_FIELD( contract.tradingHours);
		DECODE_FIELD( contract.liquidHours);
	}
	if( version >= 8) {
		DECODE_FIELD( contract.evRule);
		DECODE_FIELD( contract.evMultiplier);
	}
	if( version >= 7) {
		int secIdListCount = 0;
		DECODE_FIELD( secIdListCount);
		if( secIdListCount > 0) {
			TagValueListSPtr secIdList( new TagValueList);
			secIdList->reserve( secIdListCount);
			for( int i = 0; i < secIdListCount; ++i) {
				TagValueSPtr tagValue( new TagValue());
				DECODE_FIELD( tagValue->tag);
				DECODE_FIELD( tagValue->value);
				secIdList->push_back( tagValue);
			}
			contract.secIdList = secIdList;
		}
	}
	if (m_serverVersion >= MIN_SERVER_VER_AGG_GROUP) {
		DECODE_FIELD( contract.aggGroup);
	}
	if (m_serverVersion >= MIN_SERVER_VER_UNDERLYING_INFO) {
		DECODE_FIELD( contract.underSymbol);
		DECODE_FIELD( contract.underSecType);
	}
	if (m_serverVersion >= MIN_SERVER_VER_MARKET_RULES) {
		DECODE_FIELD( contract.marketRuleIds);
	}
	if (m_serverVersion >= MIN_SERVER_VER_REAL_EXPIRATION_DATE) {
		DECODE_FIELD( contract.realExpirationDate);
	}
	if (m_serverVersion >= MIN_SERVER_VER_STOCK_TYPE) {
		DECODE_FIELD( contract.stockType);
	}
	if (m_serverVersion >= MIN_SERVER_VER_FRACTIONAL_SIZE_SUPPORT && m_serverVersion < MIN_SERVER_VER_SIZE_RULES) {
		Decimal sizeMinTick;
		DECODE_FIELD(sizeMinTick); // not used anymore
	}
	if (m_serverVersion >= MIN_SERVER_VER_SIZE_RULES) {
		DECODE_FIELD(contract.minSize);
		DECODE_FIELD(contract.sizeIncrement);
		DECODE_FIELD(contract.suggestedSizeIncrement);
	}
	if (m_serverVersion >= MIN_SERVER_VER_FUND_DATA_FIELDS && contract.contract.secType == "FUND")	{
		DECODE_FIELD(contract.fundName);
		DECODE_FIELD(contract.fundFamily);
		DECODE_FIELD(contract.fundType);
		DECODE_FIELD(contract.fundFrontLoad);
		DECODE_FIELD(contract.fundBackLoad);
		DECODE_FIELD(contract.fundBackLoadTimeInterval);
		DECODE_FIELD(contract.fundManagementFee);
		DECODE_FIELD(contract.fundClosed);
		DECODE_FIELD(contract.fundClosedForNewInvestors);
		DECODE_FIELD(contract.fundClosedForNewMoney);
		DECODE_FIELD(contract.fundNotifyAmount);
		DECODE_FIELD(contract.fundMinimumInitialPurchase);
		DECODE_FIELD(contract.fundSubsequentMinimumPurchase);
		DECODE_FIELD(contract.fundBlueSkyStates);
		DECODE_FIELD(contract.fundBlueSkyTerritories);
		std::string fundDistributionPolicyIndicator;
		DECODE_FIELD(fundDistributionPolicyIndicator);
		contract.fundDistributionPolicyIndicator = Utils::getFundDistributionPolicyIndicator(fundDistributionPolicyIndicator);
		std::string fundAssetType;
		DECODE_FIELD(fundAssetType);
		contract.fundAssetType = Utils::getFundAssetType(fundAssetType);
	}

	if (m_serverVersion >= MIN_SERVER_VER_INELIGIBILITY_REASONS) {
		int ineligibilityReasonCount = 0;
		DECODE_FIELD(ineligibilityReasonCount);
		if (ineligibilityReasonCount > 0) {
			IneligibilityReasonListSPtr ineligibilityReasonList(new IneligibilityReasonList);
			ineligibilityReasonList->reserve(ineligibilityReasonCount);
			for (int i = 0; i < ineligibilityReasonCount; ++i) {
				IneligibilityReasonSPtr ineligibilityReason(new IneligibilityReason());
				DECODE_FIELD(ineligibilityReason->id);
				DECODE_FIELD(ineligibilityReason->description);
				ineligibilityReasonList->push_back(ineligibilityReason);
			}
			contract.ineligibilityReasonList = ineligibilityReasonList;
		}
	}

	m_pEWrapper->contractDetails( reqId, contract);

	return ptr;
}

const char* EDecoder::processContractDataMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::ContractData contractDataProto;
	contractDataProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->contractDataProtoBuf(contractDataProto);
#endif

	int reqId = contractDataProto.has_reqid() ? contractDataProto.reqid() : NO_VALID_ID;

	// set contract details fields
	ContractDetails contractDetails;
    if (contractDataProto.has_contract() && contractDataProto.has_contractdetails()) contractDetails = EDecoderUtils::decodeContractDetails(contractDataProto.contract(), contractDataProto.contractdetails(), false);

	m_pEWrapper->contractDetails(reqId, contractDetails);

	return ptr;
}

const char* EDecoder::processBondContractDataMsg(const char* ptr, const char* endPtr) {
	int version = 6;
	if (m_serverVersion < MIN_SERVER_VER_SIZE_RULES) {
		DECODE_FIELD(version);
	}

	int reqId = -1;
	if( version >= 3) {
		DECODE_FIELD( reqId);
	}

	ContractDetails contract;
	DECODE_FIELD( contract.contract.symbol);
	DECODE_FIELD( contract.contract.secType);
	DECODE_FIELD( contract.cusip);
	DECODE_FIELD( contract.coupon);
	ptr = decodeLastTradeDate(ptr, endPtr, contract, true);
	DECODE_FIELD( contract.issueDate);
	DECODE_FIELD( contract.ratings);
	DECODE_FIELD( contract.bondType);
	DECODE_FIELD( contract.couponType);
	DECODE_FIELD( contract.convertible);
	DECODE_FIELD( contract.callable);
	DECODE_FIELD( contract.putable);
	DECODE_FIELD( contract.descAppend);
	DECODE_FIELD( contract.contract.exchange);
	DECODE_FIELD( contract.contract.currency);
	DECODE_FIELD( contract.marketName);
	DECODE_FIELD( contract.contract.tradingClass);
	DECODE_FIELD( contract.contract.conId);
	DECODE_FIELD( contract.minTick);
	if (m_serverVersion >= MIN_SERVER_VER_MD_SIZE_MULTIPLIER && m_serverVersion < MIN_SERVER_VER_SIZE_RULES) {
		int mdSizeMultiplier;
		DECODE_FIELD( mdSizeMultiplier); // not used anymore
	}
	DECODE_FIELD( contract.orderTypes);
	DECODE_FIELD( contract.validExchanges);
	DECODE_FIELD( contract.nextOptionDate); // ver 2 field
	DECODE_FIELD( contract.nextOptionType); // ver 2 field
	DECODE_FIELD( contract.nextOptionPartial); // ver 2 field
	DECODE_FIELD( contract.notes); // ver 2 field
	if( version >= 4) {
		DECODE_FIELD( contract.longName);
	}
	if (m_serverVersion >= MIN_SERVER_VER_BOND_TRADING_HOURS) {
		DECODE_FIELD(contract.timeZoneId);
		DECODE_FIELD(contract.tradingHours);
		DECODE_FIELD(contract.liquidHours);
	}
	if( version >= 6) {
		DECODE_FIELD( contract.evRule);
		DECODE_FIELD( contract.evMultiplier);
	}
	if( version >= 5) {
		int secIdListCount = 0;
		DECODE_FIELD( secIdListCount);
		if( secIdListCount > 0) {
			TagValueListSPtr secIdList( new TagValueList);
			secIdList->reserve( secIdListCount);
			for( int i = 0; i < secIdListCount; ++i) {
				TagValueSPtr tagValue( new TagValue());
				DECODE_FIELD( tagValue->tag);
				DECODE_FIELD( tagValue->value);
				secIdList->push_back( tagValue);
			}
			contract.secIdList = secIdList;
		}
	}
	if (m_serverVersion >= MIN_SERVER_VER_AGG_GROUP) {
		DECODE_FIELD( contract.aggGroup);
	}
	if (m_serverVersion >= MIN_SERVER_VER_MARKET_RULES) {
		DECODE_FIELD( contract.marketRuleIds);
	}
	if (m_serverVersion >= MIN_SERVER_VER_SIZE_RULES) {
		DECODE_FIELD(contract.minSize);
		DECODE_FIELD(contract.sizeIncrement);
		DECODE_FIELD(contract.suggestedSizeIncrement);
	}

	m_pEWrapper->bondContractDetails( reqId, contract);

	return ptr;
}

const char* EDecoder::processBondContractDataMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::ContractData contractDataProto;
	contractDataProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->bondContractDataProtoBuf(contractDataProto);
#endif

	int reqId = contractDataProto.has_reqid() ? contractDataProto.reqid() : NO_VALID_ID;

	// set contract details fields
	ContractDetails contractDetails;
	if (contractDataProto.has_contract() && contractDataProto.has_contractdetails()) contractDetails = EDecoderUtils::decodeContractDetails(contractDataProto.contract(), contractDataProto.contractdetails(), true);

	m_pEWrapper->bondContractDetails(reqId, contractDetails);

	return ptr;
}

const char* EDecoder::processExecutionDetailsMsg(const char* ptr, const char* endPtr) {
    int version = m_serverVersion;

    if (m_serverVersion < MIN_SERVER_VER_LAST_LIQUIDITY) {
	    DECODE_FIELD(version);
    }

	int reqId = -1;
	if( version >= 7) {
		DECODE_FIELD(reqId);
	}

	int orderId;
	DECODE_FIELD( orderId);

	// decode contract fields
	Contract contract;
	DECODE_FIELD( contract.conId); // ver 5 field
	DECODE_FIELD( contract.symbol);
	DECODE_FIELD( contract.secType);
	DECODE_FIELD( contract.lastTradeDateOrContractMonth);
	DECODE_FIELD( contract.strike);
	DECODE_FIELD( contract.right);
	if( version >= 9) {
		DECODE_FIELD( contract.multiplier);
	}
	DECODE_FIELD( contract.exchange);
	DECODE_FIELD( contract.currency);
	DECODE_FIELD( contract.localSymbol);
	if (version >= 10) {
		DECODE_FIELD( contract.tradingClass);
	}

	// decode execution fields
	Execution exec;
	exec.orderId = orderId;
	DECODE_FIELD( exec.execId);
	DECODE_FIELD( exec.time);
	DECODE_FIELD( exec.acctNumber);
	DECODE_FIELD( exec.exchange);
	DECODE_FIELD( exec.side);
	DECODE_FIELD( exec.shares)
	DECODE_FIELD( exec.price);
	DECODE_FIELD( exec.permId); // ver 2 field
	DECODE_FIELD( exec.clientId); // ver 3 field
	DECODE_FIELD( exec.liquidation); // ver 4 field

	if( version >= 6) {
		DECODE_FIELD( exec.cumQty);
		DECODE_FIELD( exec.avgPrice);
	}

	if( version >= 8) {
		DECODE_FIELD( exec.orderRef);
	}

	if( version >= 9) {
		DECODE_FIELD( exec.evRule);
		DECODE_FIELD( exec.evMultiplier);
	}
	if( m_serverVersion >= MIN_SERVER_VER_MODELS_SUPPORT) {
		DECODE_FIELD( exec.modelCode);
	}

    if (m_serverVersion >= MIN_SERVER_VER_LAST_LIQUIDITY) {
        DECODE_FIELD(exec.lastLiquidity);
    }

    if (m_serverVersion >= MIN_SERVER_VER_PENDING_PRICE_REVISION) {
        DECODE_FIELD(exec.pendingPriceRevision);
    }

    if (m_serverVersion >= MIN_SERVER_VER_SUBMITTER) {
        DECODE_FIELD(exec.submitter);
    }

	m_pEWrapper->execDetails( reqId, contract, exec);

	return ptr;
}

const char* EDecoder::processExecutionDetailsMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::ExecutionDetails executionDetailsProto;
	executionDetailsProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->execDetailsProtoBuf(executionDetailsProto);
#endif

	int reqId = executionDetailsProto.has_reqid() ? executionDetailsProto.reqid() : NO_VALID_ID;
	Contract contract;
	if (executionDetailsProto.has_contract()) contract = EDecoderUtils::decodeContract(executionDetailsProto.contract());
	Execution execution;
	if (executionDetailsProto.has_execution()) execution = EDecoderUtils::decodeExecution(executionDetailsProto.execution());

	m_pEWrapper->execDetails(reqId, contract, execution);

	return ptr;
}

const char* EDecoder::processMarketDepthMsg(const char* ptr, const char* endPtr) {
	int version;
	int id;
	int position;
	int operation;
	int side;
	double price;
	Decimal size;

	DECODE_FIELD( version);
	DECODE_FIELD( id);
	DECODE_FIELD( position);
	DECODE_FIELD( operation);
	DECODE_FIELD( side);
	DECODE_FIELD( price);
	DECODE_FIELD( size);

	m_pEWrapper->updateMktDepth( id, position, operation, side, price, size);

	return ptr;
}

const char* EDecoder::processMarketDepthMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::MarketDepth marketDepthProto;
	marketDepthProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->updateMarketDepthProtoBuf(marketDepthProto);
#endif

	int reqId = marketDepthProto.has_reqid() ? marketDepthProto.reqid() : NO_VALID_ID;

	if (!marketDepthProto.has_marketdepthdata()) {
		return ptr;
	}

	const protobuf::MarketDepthData& marketDepthDataProto = marketDepthProto.marketdepthdata();
	int position = marketDepthDataProto.has_position() ? marketDepthDataProto.position() : 0;
	int operation = marketDepthDataProto.has_operation() ? marketDepthDataProto.operation() : 0;
	int side = marketDepthDataProto.has_side() ? marketDepthDataProto.side() : 0;
	double price = marketDepthDataProto.has_price() ? marketDepthDataProto.price() : 0;
	Decimal size = marketDepthDataProto.has_size() ? DecimalFunctions::stringToDecimal(marketDepthDataProto.size()) : UNSET_DECIMAL;

	m_pEWrapper->updateMktDepth(reqId, position, operation, side, price, size);

	return ptr;
}

const char* EDecoder::processMarketDepthL2Msg(const char* ptr, const char* endPtr) {
	int version;
	int id;
	int position;
	std::string marketMaker;
	int operation;
	int side;
	double price;
	Decimal size;
	bool isSmartDepth = false;

	DECODE_FIELD( version);
	DECODE_FIELD( id);
	DECODE_FIELD( position);
	DECODE_FIELD( marketMaker);
	DECODE_FIELD( operation);
	DECODE_FIELD( side);
	DECODE_FIELD( price);
	DECODE_FIELD( size);

	if( m_serverVersion >= MIN_SERVER_VER_SMART_DEPTH) {
		DECODE_FIELD( isSmartDepth);
	}

	m_pEWrapper->updateMktDepthL2( id, position, marketMaker, operation, side,
		price, size, isSmartDepth);

	return ptr;
}

const char* EDecoder::processMarketDepthL2MsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::MarketDepthL2 marketDepthL2Proto;
	marketDepthL2Proto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->updateMarketDepthL2ProtoBuf(marketDepthL2Proto);
#endif

	int reqId = marketDepthL2Proto.has_reqid() ? marketDepthL2Proto.reqid() : NO_VALID_ID;

	if (!marketDepthL2Proto.has_marketdepthdata()) {
		return ptr;
	}

	const protobuf::MarketDepthData& marketDepthDataProto = marketDepthL2Proto.marketdepthdata();
	int position = marketDepthDataProto.has_position() ? marketDepthDataProto.position() : 0;
	std::string marketMaker = marketDepthDataProto.has_marketmaker() ? marketDepthDataProto.marketmaker() : "";
	int operation = marketDepthDataProto.has_operation() ? marketDepthDataProto.operation() : 0;
	int side = marketDepthDataProto.has_side() ? marketDepthDataProto.side() : 0;
	double price = marketDepthDataProto.has_price() ? marketDepthDataProto.price() : 0;
	Decimal size = marketDepthDataProto.has_size() ? DecimalFunctions::stringToDecimal(marketDepthDataProto.size()) : UNSET_DECIMAL;
	bool isSmartDepth = marketDepthDataProto.has_issmartdepth() ? marketDepthDataProto.issmartdepth() : false;

	m_pEWrapper->updateMktDepthL2(reqId, position, marketMaker, operation, side, price, size, isSmartDepth);

	return ptr;
}

const char* EDecoder::processNewsBulletinsMsg(const char* ptr, const char* endPtr) {
	int version;
	int msgId;
	int msgType;
	std::string newsMessage;
	std::string originatingExch;

	DECODE_FIELD( version);
	DECODE_FIELD( msgId);
	DECODE_FIELD( msgType);
	DECODE_FIELD( newsMessage);
	DECODE_FIELD( originatingExch);

	m_pEWrapper->updateNewsBulletin( msgId, msgType, newsMessage, originatingExch);

	return ptr;
}

const char* EDecoder::processNewsBulletinMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::NewsBulletin newsBulletinProto;
	newsBulletinProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->updateNewsBulletinProtoBuf(newsBulletinProto);
#endif

	int msgId = newsBulletinProto.has_newsmsgid() ? newsBulletinProto.newsmsgid() : NO_VALID_ID;
	int msgType = newsBulletinProto.has_newsmsgtype() ? newsBulletinProto.newsmsgtype() : 0;
	std::string newsMessage = newsBulletinProto.has_newsmessage() ? newsBulletinProto.newsmessage() : "";
	std::string originExch = newsBulletinProto.has_originatingexch() ? newsBulletinProto.originatingexch() : "";

	m_pEWrapper->updateNewsBulletin(msgId, msgType, newsMessage, originExch);

	return ptr;
}

const char* EDecoder::processManagedAcctsMsg(const char* ptr, const char* endPtr) {
	int version;
	std::string accountsList;

	DECODE_FIELD( version);
	DECODE_FIELD( accountsList);

	m_pEWrapper->managedAccounts( accountsList);

	return ptr;
}

const char* EDecoder::processManagedAccountsMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::ManagedAccounts managedAccountsProto;
	managedAccountsProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->managedAccountsProtoBuf(managedAccountsProto);
#endif

	std::string accountsList = managedAccountsProto.has_accountslist() ? managedAccountsProto.accountslist() : "";

	m_pEWrapper->managedAccounts(accountsList);

	return ptr;
}

const char* EDecoder::processReceiveFaMsg(const char* ptr, const char* endPtr) {
	int version;
	int faDataTypeInt;
	std::string cxml;

	DECODE_FIELD( version);
	DECODE_FIELD( faDataTypeInt);
	DECODE_FIELD( cxml);

	m_pEWrapper->receiveFA( (faDataType)faDataTypeInt, cxml);

	return ptr;
}

const char* EDecoder::processReceiveFAMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::ReceiveFA receiveFAProto;
	receiveFAProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->receiveFAProtoBuf(receiveFAProto);
#endif

	int faDataTypeInt = receiveFAProto.has_fadatatype() ? receiveFAProto.fadatatype() : 0;
	std::string xml = receiveFAProto.has_xml() ? receiveFAProto.xml() : "";

	m_pEWrapper->receiveFA((faDataType)faDataTypeInt, xml);

	return ptr;
}

const char* EDecoder::processHistoricalDataMsg(const char* ptr, const char* endPtr) {
    int version = INT_MAX;
	int reqId;
	std::string startDateStr;
	std::string endDateStr;

    if (m_serverVersion < MIN_SERVER_VER_SYNT_REALTIME_BARS) {
	    DECODE_FIELD(version);
    }

	DECODE_FIELD( reqId);
	if (m_serverVersion < MIN_SERVER_VER_HISTORICAL_DATA_END) {
		DECODE_FIELD(startDateStr); // ver 2 field
		DECODE_FIELD(endDateStr); // ver 2 field
	}

	int itemCount;
	DECODE_FIELD( itemCount);

	typedef std::vector<Bar> BarDataList;
	BarDataList bars;

	bars.reserve( itemCount);

	for( int ctr = 0; ctr < itemCount; ++ctr) {
		Bar bar;

		DECODE_FIELD( bar.time);
		DECODE_FIELD( bar.open);
		DECODE_FIELD( bar.high);
		DECODE_FIELD( bar.low);
		DECODE_FIELD( bar.close);
        DECODE_FIELD( bar.volume);
		DECODE_FIELD( bar.wap);

        if (m_serverVersion < MIN_SERVER_VER_SYNT_REALTIME_BARS) {
	        std::string hasGaps;

		    DECODE_FIELD( hasGaps);
        }

		DECODE_FIELD( bar.count); // ver 3 field

		bars.push_back(bar);
	}

	assert( (int)bars.size() == itemCount);

	for( int ctr = 0; ctr < itemCount; ++ctr) {

		const Bar& bar = bars[ctr];

		m_pEWrapper->historicalData( reqId, bar);
	}

	if (m_serverVersion < MIN_SERVER_VER_HISTORICAL_DATA_END) {
		// send end of dataset marker
		m_pEWrapper->historicalDataEnd(reqId, startDateStr, endDateStr);
	}

	return ptr;
}

const char* EDecoder::processHistoricalDataMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::HistoricalData historicalDataProto;
	historicalDataProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->historicalDataProtoBuf(historicalDataProto);
#endif

	int reqId = historicalDataProto.has_reqid() ? historicalDataProto.reqid() : NO_VALID_ID;

	for (int i = 0; i < historicalDataProto.historicaldatabars_size(); ++i) {
		const protobuf::HistoricalDataBar& historicalDataBarProto = historicalDataProto.historicaldatabars(i);
		Bar bar = EDecoderUtils::decodeHistoricalDataBar(historicalDataBarProto);
		m_pEWrapper->historicalData(reqId, bar);
	}

	return ptr;
}

const char* EDecoder::processHistoricalDataEndMsg(const char* ptr, const char* endPtr) {
	int reqId;
	std::string startDateStr;
	std::string endDateStr;

	DECODE_FIELD(reqId);
	DECODE_FIELD(startDateStr);
	DECODE_FIELD(endDateStr);

	m_pEWrapper->historicalDataEnd(reqId, startDateStr, endDateStr);

	return ptr;
}

const char* EDecoder::processHistoricalDataEndMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::HistoricalDataEnd historicalDataEndProto;
	historicalDataEndProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->historicalDataEndProtoBuf(historicalDataEndProto);
#endif

	int reqId = historicalDataEndProto.has_reqid() ? historicalDataEndProto.reqid() : NO_VALID_ID;
	std::string startDate = historicalDataEndProto.has_startdatestr() ? historicalDataEndProto.startdatestr() : "";
	std::string endDate = historicalDataEndProto.has_enddatestr() ? historicalDataEndProto.enddatestr() : "";

	m_pEWrapper->historicalDataEnd(reqId, startDate, endDate);

	return ptr;
}

const char* EDecoder::processHistoricalDataUpdateMsg(const char* ptr, const char* endPtr) {
	int reqId;
	std::string startDateStr;
	std::string endDateStr;

	DECODE_FIELD( reqId);

	Bar bar;

	DECODE_FIELD(bar.count);
	DECODE_FIELD(bar.time);
	DECODE_FIELD(bar.open);
	DECODE_FIELD(bar.close);
	DECODE_FIELD(bar.high);
	DECODE_FIELD(bar.low);
	DECODE_FIELD(bar.wap);
	DECODE_FIELD(bar.volume);

	m_pEWrapper->historicalDataUpdate( reqId, bar);

	return ptr;
}

const char* EDecoder::processHistoricalDataUpdateMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::HistoricalDataUpdate historicalDataUpdateProto;
	historicalDataUpdateProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->historicalDataUpdateProtoBuf(historicalDataUpdateProto);
#endif

	int reqId = historicalDataUpdateProto.has_reqid() ? historicalDataUpdateProto.reqid() : NO_VALID_ID;

	if (historicalDataUpdateProto.has_historicaldatabar()) {
		Bar bar = EDecoderUtils::decodeHistoricalDataBar(historicalDataUpdateProto.historicaldatabar());
		m_pEWrapper->historicalDataUpdate(reqId, bar);
	}

	return ptr;
}

const char* EDecoder::processScannerDataMsg(const char* ptr, const char* endPtr) {
	int version;
	int tickerId;

	DECODE_FIELD( version);
	DECODE_FIELD( tickerId);

	int numberOfElements;
	DECODE_FIELD( numberOfElements);

	typedef std::vector<ScanData> ScanDataList;
	ScanDataList scannerDataList;

	scannerDataList.reserve( numberOfElements);

	for( int ctr=0; ctr < numberOfElements; ++ctr) {

		ScanData data;

		DECODE_FIELD( data.rank);
		DECODE_FIELD( data.contract.contract.conId); // ver 3 field
		DECODE_FIELD( data.contract.contract.symbol);
		DECODE_FIELD( data.contract.contract.secType);
		DECODE_FIELD( data.contract.contract.lastTradeDateOrContractMonth);
		DECODE_FIELD( data.contract.contract.strike);
		DECODE_FIELD( data.contract.contract.right);
		DECODE_FIELD( data.contract.contract.exchange);
		DECODE_FIELD( data.contract.contract.currency);
		DECODE_FIELD( data.contract.contract.localSymbol);
		DECODE_FIELD( data.contract.marketName);
		DECODE_FIELD( data.contract.contract.tradingClass);
		DECODE_FIELD( data.distance);
		DECODE_FIELD( data.benchmark);
		DECODE_FIELD( data.projection);
		DECODE_FIELD( data.legsStr);

		scannerDataList.push_back( data);
	}

	assert( (int)scannerDataList.size() == numberOfElements);

	for( int ctr=0; ctr < numberOfElements; ++ctr) {

		const ScanData& data = scannerDataList[ctr];
		m_pEWrapper->scannerData( tickerId, data.rank, data.contract,
			data.distance, data.benchmark, data.projection, data.legsStr);
	}

	m_pEWrapper->scannerDataEnd( tickerId);

	return ptr;
}

const char* EDecoder::processScannerDataMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::ScannerData scannerDataProto;
	scannerDataProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->scannerDataProtoBuf(scannerDataProto);
#endif

	int reqId = scannerDataProto.has_reqid() ? scannerDataProto.reqid() : NO_VALID_ID;

	if (scannerDataProto.scannerdataelement_size() > 0) {
		for (int i = 0; i < scannerDataProto.scannerdataelement_size(); i++) {
			const protobuf::ScannerDataElement& element = scannerDataProto.scannerdataelement(i);
			int rank = element.has_rank() ? element.rank() : 0;

			// Set contract details
			ContractDetails contractDetails;
			if (element.has_contract()) {
				Contract contract = EDecoderUtils::decodeContract(element.contract());
				contractDetails.contract = contract;
			}
			contractDetails.marketName = element.has_marketname() ? element.marketname() : "";

			std::string distance = element.has_distance() ? element.distance() : "";
			std::string benchmark = element.has_benchmark() ? element.benchmark() : "";
			std::string projection = element.has_projection() ? element.projection() : "";
			std::string comboKey = element.has_combokey() ? element.combokey() : "";

			m_pEWrapper->scannerData(reqId, rank, contractDetails, distance, benchmark, projection, comboKey);
		}
	}

	m_pEWrapper->scannerDataEnd(reqId);

	return ptr;
}

const char* EDecoder::processScannerParametersMsg(const char* ptr, const char* endPtr) {
	int version;
	std::string xml;

	DECODE_FIELD( version);
	DECODE_FIELD( xml);

	m_pEWrapper->scannerParameters( xml);

	return ptr;
}

const char* EDecoder::processScannerParametersMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::ScannerParameters scannerParametersProto;
	scannerParametersProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->scannerParametersProtoBuf(scannerParametersProto);
#endif

	std::string xml = scannerParametersProto.has_xml() ? scannerParametersProto.xml() : "";

	m_pEWrapper->scannerParameters(xml);

	return ptr;
}

const char* EDecoder::processCurrentTimeMsg(const char* ptr, const char* endPtr) {
	int version;
	int time;

	DECODE_FIELD(version);
	DECODE_FIELD(time);

	m_pEWrapper->currentTime( time);

	return ptr;
}

const char* EDecoder::processCurrentTimeMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::CurrentTime currentTimeProto;
	currentTimeProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->currentTimeProtoBuf(currentTimeProto);
#endif

	long time = currentTimeProto.has_currenttime() ? currentTimeProto.currenttime() : 0;

	m_pEWrapper->currentTime(time);

	return ptr;
}

const char* EDecoder::processRealTimeBarsMsg(const char* ptr, const char* endPtr) {
	int version;
	int reqId;
	int time;
	double open;
	double high;
	double low;
	double close;
	Decimal volume;
	Decimal wap;
	int count;

	DECODE_FIELD( version);
	DECODE_FIELD( reqId);
	DECODE_FIELD( time);
	DECODE_FIELD( open);
	DECODE_FIELD( high);
	DECODE_FIELD( low);
	DECODE_FIELD( close);
	DECODE_FIELD( volume);
	DECODE_FIELD( wap);
	DECODE_FIELD( count);

	m_pEWrapper->realtimeBar( reqId, time, open, high, low, close,
		volume, wap, count);

	return ptr;
}

const char* EDecoder::processRealTimeBarsMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::RealTimeBarTick realTimeBarTickProto;
	realTimeBarTickProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->realTimeBarTickProtoBuf(realTimeBarTickProto);
#endif

	int reqId = realTimeBarTickProto.has_reqid() ? realTimeBarTickProto.reqid() : NO_VALID_ID;
	long long time = realTimeBarTickProto.has_time() ? realTimeBarTickProto.time() : 0;
	double open = realTimeBarTickProto.has_open() ? realTimeBarTickProto.open() : 0.0;
	double high = realTimeBarTickProto.has_high() ? realTimeBarTickProto.high() : 0.0;
	double low = realTimeBarTickProto.has_low() ? realTimeBarTickProto.low() : 0.0;
	double close = realTimeBarTickProto.has_close() ? realTimeBarTickProto.close() : 0.0;
	Decimal volume = realTimeBarTickProto.has_volume() ? DecimalFunctions::stringToDecimal(realTimeBarTickProto.volume()) : UNSET_DECIMAL;
	Decimal wap = realTimeBarTickProto.has_wap() ? DecimalFunctions::stringToDecimal(realTimeBarTickProto.wap()) : UNSET_DECIMAL;
	int count = realTimeBarTickProto.has_count() ? realTimeBarTickProto.count() : 0;

	m_pEWrapper->realtimeBar(reqId, time, open, high, low, close, volume, wap, count);

	return ptr;
}

const char* EDecoder::processFundamentalDataMsg(const char* ptr, const char* endPtr) {
	int version;
	int reqId;
	std::string data;

	DECODE_FIELD( version);
	DECODE_FIELD( reqId);
	DECODE_FIELD( data);

	m_pEWrapper->fundamentalData( reqId, data);

	return ptr;
}

const char* EDecoder::processFundamentalsDataMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::FundamentalsData fundamentalsDataProto;
	fundamentalsDataProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->fundamentalsDataProtoBuf(fundamentalsDataProto);
#endif

	int reqId = fundamentalsDataProto.has_reqid() ? fundamentalsDataProto.reqid() : NO_VALID_ID;
	std::string data = fundamentalsDataProto.has_data() ? fundamentalsDataProto.data() : "";

	m_pEWrapper->fundamentalData(reqId, data);

	return ptr;
}

const char* EDecoder::processContractDataEndMsg(const char* ptr, const char* endPtr) {
	int version;
	int reqId;

	DECODE_FIELD( version);
	DECODE_FIELD( reqId);

	m_pEWrapper->contractDetailsEnd( reqId);

	return ptr;
}

const char* EDecoder::processContractDataEndMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::ContractDataEnd contractDataEndProto;
	contractDataEndProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->contractDataEndProtoBuf(contractDataEndProto);
#endif

	int reqId = contractDataEndProto.has_reqid() ? contractDataEndProto.reqid() : NO_VALID_ID;

	m_pEWrapper->contractDetailsEnd(reqId);

	return ptr;
}

const char* EDecoder::processOpenOrderEndMsg(const char* ptr, const char* endPtr) {
	int version;

	DECODE_FIELD( version);

	m_pEWrapper->openOrderEnd();

	return ptr;
}

const char* EDecoder::processOpenOrderEndMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::OpenOrdersEnd openOrdersEndProto;
	openOrdersEndProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->openOrdersEndProtoBuf(openOrdersEndProto);
#endif

	m_pEWrapper->openOrderEnd();

	return ptr;
}

const char* EDecoder::processAcctDownloadEndMsg(const char* ptr, const char* endPtr) {
	int version;
	std::string account;

	DECODE_FIELD( version);
	DECODE_FIELD( account);

	m_pEWrapper->accountDownloadEnd( account);

	return ptr;
}

const char* EDecoder::processAccountDataEndMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::AccountDataEnd accountDataEndProto;
	accountDataEndProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->accountDataEndProtoBuf(accountDataEndProto);
#endif

	std::string accountName = accountDataEndProto.has_accountname() ? accountDataEndProto.accountname() : "";

	m_pEWrapper->accountDownloadEnd(accountName);

	return ptr;
}

const char* EDecoder::processExecutionDetailsEndMsg(const char* ptr, const char* endPtr) {
	int version;
	int reqId;

	DECODE_FIELD( version);
	DECODE_FIELD( reqId);

	m_pEWrapper->execDetailsEnd( reqId);

	return ptr;
}

const char* EDecoder::processExecutionDetailsEndMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::ExecutionDetailsEnd executionDetailsEndProto;
	executionDetailsEndProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->execDetailsEndProtoBuf(executionDetailsEndProto);
#endif

	int reqId = executionDetailsEndProto.has_reqid() ? executionDetailsEndProto.reqid() : NO_VALID_ID;

	m_pEWrapper->execDetailsEnd(reqId);

	return ptr;
}

const char* EDecoder::processDeltaNeutralValidationMsg(const char* ptr, const char* endPtr) {
	int version;
	int reqId;

	DECODE_FIELD( version);
	DECODE_FIELD( reqId);

	DeltaNeutralContract deltaNeutralContract;

	DECODE_FIELD( deltaNeutralContract.conId);
	DECODE_FIELD( deltaNeutralContract.delta);
	DECODE_FIELD( deltaNeutralContract.price);

	m_pEWrapper->deltaNeutralValidation( reqId, deltaNeutralContract);

	return ptr;
}

const char* EDecoder::processTickSnapshotEndMsg(const char* ptr, const char* endPtr) {
	int version;
	int reqId;

	DECODE_FIELD( version);
	DECODE_FIELD( reqId);

	m_pEWrapper->tickSnapshotEnd( reqId);

	return ptr;
}

const char* EDecoder::processTickSnapshotEndMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::TickSnapshotEnd tickSnapshotEndProto;
	tickSnapshotEndProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->tickSnapshotEndProtoBuf(tickSnapshotEndProto);
#endif

	int reqId = tickSnapshotEndProto.has_reqid() ? tickSnapshotEndProto.reqid() : NO_VALID_ID;

	m_pEWrapper->tickSnapshotEnd(reqId);

	return ptr;
}

const char* EDecoder::processMarketDataTypeMsg(const char* ptr, const char* endPtr) {
	int version;
	int reqId;
	int marketDataType;

	DECODE_FIELD( version);
	DECODE_FIELD( reqId);
	DECODE_FIELD( marketDataType);

	m_pEWrapper->marketDataType( reqId, marketDataType);

	return ptr;
}

const char* EDecoder::processMarketDataTypeMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::MarketDataType marketDataTypeProto;
	marketDataTypeProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->marketDataTypeProtoBuf(marketDataTypeProto);
#endif

	int reqId = marketDataTypeProto.has_reqid() ? marketDataTypeProto.reqid() : NO_VALID_ID;
	int marketDataType = marketDataTypeProto.has_marketdatatype() ? marketDataTypeProto.marketdatatype() : 0;

	m_pEWrapper->marketDataType(reqId, marketDataType);

	return ptr;
}

const char* EDecoder::processCommissionAndFeesReportMsg(const char* ptr, const char* endPtr) {
	int version;
	DECODE_FIELD( version);

	CommissionAndFeesReport commissionAndFeesReport;
	DECODE_FIELD( commissionAndFeesReport.execId);
	DECODE_FIELD( commissionAndFeesReport.commissionAndFees);
	DECODE_FIELD( commissionAndFeesReport.currency);
	DECODE_FIELD( commissionAndFeesReport.realizedPNL);
	DECODE_FIELD( commissionAndFeesReport.yield);
	DECODE_FIELD( commissionAndFeesReport.yieldRedemptionDate);

	m_pEWrapper->commissionAndFeesReport( commissionAndFeesReport);

	return ptr;
}

const char* EDecoder::processCommissionAndFeesReportMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::CommissionAndFeesReport commissionAndFeesReportProto;
	commissionAndFeesReportProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->commissionAndFeesReportProtoBuf(commissionAndFeesReportProto);
#endif

	CommissionAndFeesReport commissionAndFeesReport;
	commissionAndFeesReport.execId = commissionAndFeesReportProto.has_execid() ? commissionAndFeesReportProto.execid() : "";
	commissionAndFeesReport.commissionAndFees = commissionAndFeesReportProto.has_commissionandfees() ? commissionAndFeesReportProto.commissionandfees() : 0;
	commissionAndFeesReport.currency = commissionAndFeesReportProto.has_currency() ? commissionAndFeesReportProto.currency() : "";
	commissionAndFeesReport.realizedPNL = commissionAndFeesReportProto.has_realizedpnl() ? commissionAndFeesReportProto.realizedpnl() : 0;
	commissionAndFeesReport.yield = commissionAndFeesReportProto.has_bondyield() ? commissionAndFeesReportProto.bondyield() : 0;
	commissionAndFeesReport.yieldRedemptionDate = commissionAndFeesReportProto.has_yieldredemptiondate() ? atoi(commissionAndFeesReportProto.yieldredemptiondate().c_str()) : 0;

	m_pEWrapper->commissionAndFeesReport(commissionAndFeesReport);

	return ptr;
}

const char* EDecoder::processPositionDataMsg(const char* ptr, const char* endPtr) {
	int version;
	std::string account;
	Decimal position;
	double avgCost = 0;

	DECODE_FIELD( version);
	DECODE_FIELD( account);

	// decode contract fields
	Contract contract;
	DECODE_FIELD( contract.conId);
	DECODE_FIELD( contract.symbol);
	DECODE_FIELD( contract.secType);
	DECODE_FIELD( contract.lastTradeDateOrContractMonth);
	DECODE_FIELD( contract.strike);
	DECODE_FIELD( contract.right);
	DECODE_FIELD( contract.multiplier);
	DECODE_FIELD( contract.exchange);
	DECODE_FIELD( contract.currency);
	DECODE_FIELD( contract.localSymbol);
	if (version >= 2) {
		DECODE_FIELD( contract.tradingClass);
	}
	DECODE_FIELD( position);
	if (version >= 3) {
		DECODE_FIELD( avgCost);
	}

	m_pEWrapper->position( account, contract, position, avgCost);

	return ptr;
}

const char* EDecoder::processPositionMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::Position positionProto;
	positionProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->positionProtoBuf(positionProto);
#endif

	if (!positionProto.has_contract()) {
		return ptr;
	}

	Contract contract = EDecoderUtils::decodeContract(positionProto.contract());
	Decimal position = positionProto.has_position() ? DecimalFunctions::stringToDecimal(positionProto.position()) : UNSET_DECIMAL;
	double avgCost = positionProto.has_avgcost() ? positionProto.avgcost() : 0;
	std::string account = positionProto.has_account() ? positionProto.account() : "";

	m_pEWrapper->position(account, contract, position, avgCost);

	return ptr;
}

const char* EDecoder::processPositionEndMsg(const char* ptr, const char* endPtr) {
	int version;

	DECODE_FIELD( version);

	m_pEWrapper->positionEnd();

	return ptr;
}

const char* EDecoder::processPositionEndMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::PositionEnd positionEndProto;
	positionEndProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->positionEndProtoBuf(positionEndProto);
#endif

	m_pEWrapper->positionEnd();

	return ptr;
}

const char* EDecoder::processAccountSummaryMsg(const char* ptr, const char* endPtr) {
	int version;
	int reqId;
	std::string account;
	std::string tag;
	std::string value;
	std::string currency;

	DECODE_FIELD( version);
	DECODE_FIELD( reqId);
	DECODE_FIELD( account);
	DECODE_FIELD( tag);
	DECODE_FIELD( value);
	DECODE_FIELD( currency);

	m_pEWrapper->accountSummary( reqId, account, tag, value, currency);

	return ptr;
}

const char* EDecoder::processAccountSummaryMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::AccountSummary accountSummaryProto;
	accountSummaryProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->accountSummaryProtoBuf(accountSummaryProto);
#endif

	int reqId = accountSummaryProto.has_reqid() ? accountSummaryProto.reqid() : NO_VALID_ID;
	std::string account = accountSummaryProto.has_account() ? accountSummaryProto.account() : "";
	std::string tag = accountSummaryProto.has_tag() ? accountSummaryProto.tag() : "";
	std::string value = accountSummaryProto.has_value() ? accountSummaryProto.value() : "";
	std::string currency = accountSummaryProto.has_currency() ? accountSummaryProto.currency() : "";

	m_pEWrapper->accountSummary(reqId, account, tag, value, currency);

	return ptr;
}

const char* EDecoder::processAccountSummaryEndMsg(const char* ptr, const char* endPtr) {
	int version;
	int reqId;

	DECODE_FIELD( version);
	DECODE_FIELD( reqId);

	m_pEWrapper->accountSummaryEnd( reqId);

	return ptr;
}

const char* EDecoder::processAccountSummaryEndMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::AccountSummaryEnd accountSummaryEndProto;
	accountSummaryEndProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->accountSummaryEndProtoBuf(accountSummaryEndProto);
#endif

	int reqId = accountSummaryEndProto.has_reqid() ? accountSummaryEndProto.reqid() : NO_VALID_ID;

	m_pEWrapper->accountSummaryEnd(reqId);

	return ptr;
}

const char* EDecoder::processVerifyMessageApiMsg(const char* ptr, const char* endPtr) {
	int version;
	std::string apiData;

	DECODE_FIELD( version);
	DECODE_FIELD( apiData);

	m_pEWrapper->verifyMessageAPI( apiData);

	return ptr;
}

const char* EDecoder::processVerifyMessageApiMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::VerifyMessageApi verifyMessageApiProto;
	verifyMessageApiProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->verifyMessageApiProtoBuf(verifyMessageApiProto);
#endif

	std::string apiData = verifyMessageApiProto.has_apidata() ? verifyMessageApiProto.apidata() : "";

	m_pEWrapper->verifyMessageAPI(apiData);

	return ptr;
}

const char* EDecoder::processVerifyCompletedMsg(const char* ptr, const char* endPtr) {
	int version;
	std::string isSuccessful;
	std::string errorText;

	DECODE_FIELD( version);
	DECODE_FIELD( isSuccessful);
	DECODE_FIELD( errorText);

	bool bRes = isSuccessful == "true";

	m_pEWrapper->verifyCompleted( bRes, errorText);

	return ptr;
}

const char* EDecoder::processVerifyCompletedMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::VerifyCompleted verifyCompletedProto;
	verifyCompletedProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->verifyCompletedProtoBuf(verifyCompletedProto);
#endif

	bool isSuccessful = verifyCompletedProto.has_issuccessful() ? verifyCompletedProto.issuccessful() : false;
	std::string errorText = verifyCompletedProto.has_errortext() ? verifyCompletedProto.errortext() : "";

	m_pEWrapper->verifyCompleted(isSuccessful, errorText);

	return ptr;
}

const char* EDecoder::processDisplayGroupListMsg(const char* ptr, const char* endPtr) {
	int version;
	int reqId;
	std::string groups;

	DECODE_FIELD( version);
	DECODE_FIELD( reqId);
	DECODE_FIELD( groups);

	m_pEWrapper->displayGroupList( reqId, groups);

	return ptr;
}

const char* EDecoder::processDisplayGroupListMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::DisplayGroupList displayGroupListProto;
	displayGroupListProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->displayGroupListProtoBuf(displayGroupListProto);
#endif

	int reqId = displayGroupListProto.has_reqid() ? displayGroupListProto.reqid() : NO_VALID_ID;
	std::string groups = displayGroupListProto.has_groups() ? displayGroupListProto.groups() : "";

	m_pEWrapper->displayGroupList(reqId, groups);

	return ptr;
}

const char* EDecoder::processDisplayGroupUpdatedMsg(const char* ptr, const char* endPtr) {
	int version;
	int reqId;
	std::string contractInfo;

	DECODE_FIELD( version);
	DECODE_FIELD( reqId);
	DECODE_FIELD( contractInfo);

	m_pEWrapper->displayGroupUpdated( reqId, contractInfo);

	return ptr;
}

const char* EDecoder::processDisplayGroupUpdatedMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::DisplayGroupUpdated displayGroupUpdatedProto;
	displayGroupUpdatedProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->displayGroupUpdatedProtoBuf(displayGroupUpdatedProto);
#endif

	int reqId = displayGroupUpdatedProto.has_reqid() ? displayGroupUpdatedProto.reqid() : NO_VALID_ID;
	std::string contractInfo = displayGroupUpdatedProto.has_contractinfo() ? displayGroupUpdatedProto.contractinfo() : "";

	m_pEWrapper->displayGroupUpdated(reqId, contractInfo);

	return ptr;
}

const char* EDecoder::processVerifyAndAuthMessageApiMsg(const char* ptr, const char* endPtr) {
	int version;
	std::string apiData;
	std::string xyzChallenge;

	DECODE_FIELD( version);
	DECODE_FIELD( apiData);
	DECODE_FIELD( xyzChallenge);

	m_pEWrapper->verifyAndAuthMessageAPI( apiData, xyzChallenge);

	return ptr;
}

const char* EDecoder::processVerifyAndAuthCompletedMsg(const char* ptr, const char* endPtr) {
	int version;
	std::string isSuccessful;
	std::string errorText;

	DECODE_FIELD( version);
	DECODE_FIELD( isSuccessful);
	DECODE_FIELD( errorText);

	bool bRes = isSuccessful == "true";

	m_pEWrapper->verifyAndAuthCompleted( bRes, errorText);

	return ptr;
}

const char* EDecoder::processPositionMultiMsg(const char* ptr, const char* endPtr) {
	int version;
	int reqId;
	std::string account;
	std::string modelCode;
	Decimal position;
	double avgCost = 0;

	DECODE_FIELD( version);
	DECODE_FIELD( reqId);
	DECODE_FIELD( account);

	// decode contract fields
	Contract contract;
	DECODE_FIELD( contract.conId);
	DECODE_FIELD( contract.symbol);
	DECODE_FIELD( contract.secType);
	DECODE_FIELD( contract.lastTradeDateOrContractMonth);
	DECODE_FIELD( contract.strike);
	DECODE_FIELD( contract.right);
	DECODE_FIELD( contract.multiplier);
	DECODE_FIELD( contract.exchange);
	DECODE_FIELD( contract.currency);
	DECODE_FIELD( contract.localSymbol);
	DECODE_FIELD( contract.tradingClass);
	DECODE_FIELD( position);
	DECODE_FIELD( avgCost);
	DECODE_FIELD( modelCode);

	m_pEWrapper->positionMulti( reqId, account, modelCode, contract, position, avgCost);

	return ptr;
}

const char* EDecoder::processPositionMultiMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::PositionMulti positionMultiProto;
	positionMultiProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->positionMultiProtoBuf(positionMultiProto);
#endif

	int reqId = positionMultiProto.has_reqid() ? positionMultiProto.reqid() : NO_VALID_ID;
	std::string account = positionMultiProto.has_account() ? positionMultiProto.account() : "";
	std::string modelCode = positionMultiProto.has_modelcode() ? positionMultiProto.modelcode() : "";

	if (!positionMultiProto.has_contract()) {
		return ptr;
	}

	Contract contract = EDecoderUtils::decodeContract(positionMultiProto.contract());
	Decimal position = positionMultiProto.has_position() ? DecimalFunctions::stringToDecimal(positionMultiProto.position()) : UNSET_DECIMAL;
	double avgCost = positionMultiProto.has_avgcost() ? positionMultiProto.avgcost() : 0;

	m_pEWrapper->positionMulti(reqId, account, modelCode, contract, position, avgCost);

	return ptr;
}

const char* EDecoder::processPositionMultiEndMsg(const char* ptr, const char* endPtr) {
	int version;
	int reqId;

	DECODE_FIELD( version);
	DECODE_FIELD( reqId);

	m_pEWrapper->positionMultiEnd( reqId);

	return ptr;
}

const char* EDecoder::processPositionMultiEndMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::PositionMultiEnd positionMultiEndProto;
	positionMultiEndProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->positionMultiEndProtoBuf(positionMultiEndProto);
#endif

	int reqId = positionMultiEndProto.has_reqid() ? positionMultiEndProto.reqid() : NO_VALID_ID;

	m_pEWrapper->positionMultiEnd(reqId);

	return ptr;
}

const char* EDecoder::processAccountUpdateMultiMsg(const char* ptr, const char* endPtr) {
	int version;
	int reqId;
	std::string account;
	std::string modelCode;
	std::string key;
	std::string value;
	std::string currency;

	DECODE_FIELD( version);
	DECODE_FIELD( reqId);
	DECODE_FIELD( account);
	DECODE_FIELD( modelCode);
	DECODE_FIELD( key);
	DECODE_FIELD( value);
	DECODE_FIELD( currency);

	m_pEWrapper->accountUpdateMulti( reqId, account, modelCode, key, value, currency);

	return ptr;
}

const char* EDecoder::processAccountUpdateMultiMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::AccountUpdateMulti accountUpdateMultiProto;
	accountUpdateMultiProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->accountUpdateMultiProtoBuf(accountUpdateMultiProto);
#endif

	int reqId = accountUpdateMultiProto.has_reqid() ? accountUpdateMultiProto.reqid() : NO_VALID_ID;
	std::string account = accountUpdateMultiProto.has_account() ? accountUpdateMultiProto.account() : "";
	std::string modelCode = accountUpdateMultiProto.has_modelcode() ? accountUpdateMultiProto.modelcode() : "";
	std::string key = accountUpdateMultiProto.has_key() ? accountUpdateMultiProto.key() : "";
	std::string value = accountUpdateMultiProto.has_value() ? accountUpdateMultiProto.value() : "";
	std::string currency = accountUpdateMultiProto.has_currency() ? accountUpdateMultiProto.currency() : "";

	m_pEWrapper->accountUpdateMulti(reqId, account, modelCode, key, value, currency);

	return ptr;
}

const char* EDecoder::processAccountUpdateMultiEndMsg(const char* ptr, const char* endPtr) {
	int version;
	int reqId;

	DECODE_FIELD( version);
	DECODE_FIELD( reqId);

	m_pEWrapper->accountUpdateMultiEnd( reqId);

	return ptr;
}

const char* EDecoder::processAccountUpdateMultiEndMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::AccountUpdateMultiEnd accountUpdateMultiEndProto;
	accountUpdateMultiEndProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->accountUpdateMultiEndProtoBuf(accountUpdateMultiEndProto);
#endif

	int reqId = accountUpdateMultiEndProto.has_reqid() ? accountUpdateMultiEndProto.reqid() : NO_VALID_ID;

	m_pEWrapper->accountUpdateMultiEnd(reqId);

	return ptr;
}

const char* EDecoder::processSecurityDefinitionOptionalParameterMsg(const char* ptr, const char* endPtr) {
	int reqId;
	std::string exchange;
	int underlyingConId;
	std::string tradingClass;
	std::string multiplier;
	int expirationsSize, strikesSize;
	std::set<std::string> expirations;
	std::set<double> strikes;

	DECODE_FIELD(reqId);
	DECODE_FIELD(exchange);
	DECODE_FIELD(underlyingConId);
	DECODE_FIELD(tradingClass);
	DECODE_FIELD(multiplier);
	DECODE_FIELD(expirationsSize);

	for (int i = 0; i < expirationsSize; i++) {
		std::string expiration;

		DECODE_FIELD(expiration);

		expirations.insert(expiration);
	}

	DECODE_FIELD(strikesSize);

	for (int i = 0; i < strikesSize; i++) {
		double strike;

		DECODE_FIELD(strike);

		strikes.insert(strike);
	}

	m_pEWrapper->securityDefinitionOptionalParameter(reqId, exchange, underlyingConId, tradingClass, multiplier, expirations, strikes);

	return ptr;
}

const char* EDecoder::processSecurityDefinitionOptionParameterMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::SecDefOptParameter secDefOptParameterProto;
	secDefOptParameterProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->secDefOptParameterProtoBuf(secDefOptParameterProto);
#endif

	int reqId = secDefOptParameterProto.has_reqid() ? secDefOptParameterProto.reqid() : NO_VALID_ID;
	std::string exchange = secDefOptParameterProto.has_exchange() ? secDefOptParameterProto.exchange() : "";
	int underlyingConId = secDefOptParameterProto.has_underlyingconid() ? secDefOptParameterProto.underlyingconid() : 0;
	std::string tradingClass = secDefOptParameterProto.has_tradingclass() ? secDefOptParameterProto.tradingclass() : "";
	std::string multiplier = secDefOptParameterProto.has_multiplier() ? secDefOptParameterProto.multiplier() : "";

	std::set<std::string> expirations;
	std::set<double> strikes;

	for (int i = 0; i < secDefOptParameterProto.expirations_size(); i++) {
		expirations.insert(secDefOptParameterProto.expirations(i));
	}

	for (int i = 0; i < secDefOptParameterProto.strikes_size(); i++) {
		strikes.insert(secDefOptParameterProto.strikes(i));
	}

	m_pEWrapper->securityDefinitionOptionalParameter(reqId, exchange, underlyingConId, tradingClass, multiplier, expirations, strikes);

	return ptr;
}

const char* EDecoder::processSecurityDefinitionOptionalParameterEndMsg(const char* ptr, const char* endPtr) {
	int reqId;

	DECODE_FIELD(reqId);

	m_pEWrapper->securityDefinitionOptionalParameterEnd(reqId);

	return ptr;
}

const char* EDecoder::processSecurityDefinitionOptionParameterEndMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::SecDefOptParameterEnd secDefOptParameterEndProto;
	secDefOptParameterEndProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->secDefOptParameterEndProtoBuf(secDefOptParameterEndProto);
#endif

	int reqId = secDefOptParameterEndProto.has_reqid() ? secDefOptParameterEndProto.reqid() : NO_VALID_ID;

	m_pEWrapper->securityDefinitionOptionalParameterEnd(reqId);

	return ptr;
}

const char* EDecoder::processSoftDollarTiersMsg(const char* ptr, const char* endPtr) 
{
	int reqId;
	int nTiers;

	DECODE_FIELD(reqId);
	DECODE_FIELD(nTiers);

	std::vector<SoftDollarTier> tiers(nTiers); 

	for (int i = 0; i < nTiers; i++) {
		std::string name, value, dislplayName;

		DECODE_FIELD(name);
		DECODE_FIELD(value);
		DECODE_FIELD(dislplayName);

		tiers[i] = SoftDollarTier(name, value, dislplayName); 
	}

	m_pEWrapper->softDollarTiers(reqId, tiers);

	return ptr;
}

const char* EDecoder::processSoftDollarTiersMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::SoftDollarTiers softDollarTiersProto;
	softDollarTiersProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->softDollarTiersProtoBuf(softDollarTiersProto);
#endif

	int reqId = softDollarTiersProto.has_reqid() ? softDollarTiersProto.reqid() : NO_VALID_ID;

	std::vector<SoftDollarTier> tiers;
	for (int i = 0; i < softDollarTiersProto.softdollartiers_size(); i++) {
		protobuf::SoftDollarTier softDollarTierProto = softDollarTiersProto.softdollartiers(i);
		tiers.push_back(EDecoderUtils::decodeSoftDollarTier(softDollarTierProto));
	}

	m_pEWrapper->softDollarTiers(reqId, tiers);

	return ptr;
}

const char* EDecoder::processFamilyCodesMsg(const char* ptr, const char* endPtr) 
{
	typedef std::vector<FamilyCode> FamilyCodeList;
	FamilyCodeList familyCodes;
	int nFamilyCodes = 0;
	DECODE_FIELD( nFamilyCodes);

	if (nFamilyCodes > 0) {
		familyCodes.resize(nFamilyCodes);
		for( int i = 0; i < nFamilyCodes; ++i) {
			DECODE_FIELD( familyCodes[i].accountID);
			DECODE_FIELD( familyCodes[i].familyCodeStr);
		}
	}

	m_pEWrapper->familyCodes(familyCodes);

	return ptr;
}

const char* EDecoder::processFamilyCodesMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::FamilyCodes familyCodesProto;
	familyCodesProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->familyCodesProtoBuf(familyCodesProto);
#endif

	typedef std::vector<FamilyCode> FamilyCodeList;
	FamilyCodeList familyCodes;

	for (int i = 0; i < familyCodesProto.familycodes_size(); i++) {
		protobuf::FamilyCode familyCodeProto = familyCodesProto.familycodes(i);
		familyCodes.push_back(EDecoderUtils::decodeFamilyCode(familyCodeProto));
	}

	m_pEWrapper->familyCodes(familyCodes);

	return ptr;
}

const char* EDecoder::processSymbolSamplesMsg(const char* ptr, const char* endPtr) 
{
	int reqId;
	typedef std::vector<ContractDescription> ContractDescriptionList;
	ContractDescriptionList contractDescriptions;
	int nContractDescriptions = 0;
	DECODE_FIELD( reqId);
	DECODE_FIELD( nContractDescriptions);

	if (nContractDescriptions > 0) {
		contractDescriptions.resize(nContractDescriptions);
		for( int i = 0; i < nContractDescriptions; ++i) {
			ContractDescription& contractDescription = contractDescriptions[i];
			Contract& contract = contractDescription.contract;
			DECODE_FIELD( contract.conId);
			DECODE_FIELD( contract.symbol);
			DECODE_FIELD( contract.secType);
			DECODE_FIELD( contract.primaryExchange);
			DECODE_FIELD( contract.currency);

			int nDerivativeSecTypes = 0;
			DECODE_FIELD(nDerivativeSecTypes);
			if (nDerivativeSecTypes > 0) {
				ContractDescription::DerivativeSecTypesList& derivativeSecTypes = contractDescription.derivativeSecTypes;
				derivativeSecTypes.resize(nDerivativeSecTypes);
				for (int j = 0; j < nDerivativeSecTypes; ++j) {
					DECODE_FIELD(derivativeSecTypes[j]);
				}
			}
			if (m_serverVersion >= MIN_SERVER_VER_BOND_ISSUERID) {
				DECODE_FIELD( contract.description);
				DECODE_FIELD( contract.issuerId);
			}
		}
	}

	m_pEWrapper->symbolSamples(reqId, contractDescriptions);

	return ptr;
}

const char* EDecoder::processSymbolSamplesMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::SymbolSamples symbolSamplesProto;
	symbolSamplesProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->symbolSamplesProtoBuf(symbolSamplesProto);
#endif

	int reqId = symbolSamplesProto.has_reqid() ? symbolSamplesProto.reqid() : NO_VALID_ID;

	typedef std::vector<ContractDescription> ContractDescriptionList;
	ContractDescriptionList contractDescriptions;

	for (int i = 0; i < symbolSamplesProto.contractdescriptions_size(); i++) {
		const auto& desc = symbolSamplesProto.contractdescriptions(i);
		Contract contract;

		if (desc.has_contract()) {
			contract = EDecoderUtils::decodeContract(desc.contract());
		}

		ContractDescription::DerivativeSecTypesList derivativeSecTypes;
		for (int j = 0; j < desc.derivativesectypes_size(); j++) {
			derivativeSecTypes.push_back(desc.derivativesectypes(j));
		}
		ContractDescription contractDescription;
		contractDescription.contract = contract;
		contractDescription.derivativeSecTypes = derivativeSecTypes;

		contractDescriptions.push_back(contractDescription);
	}

	m_pEWrapper->symbolSamples(reqId, contractDescriptions);

	return ptr;
}

const char* EDecoder::processMktDepthExchangesMsg(const char* ptr, const char* endPtr) 
{
	typedef std::vector<DepthMktDataDescription> DepthMktDataDescriptionList;
	DepthMktDataDescriptionList depthMktDataDescriptions;
	int nDepthMktDataDescriptions = 0;
	DECODE_FIELD( nDepthMktDataDescriptions);

	if (nDepthMktDataDescriptions > 0) {
		depthMktDataDescriptions.resize(nDepthMktDataDescriptions);
		for( int i = 0; i < nDepthMktDataDescriptions; ++i) {
			if (m_serverVersion >= MIN_SERVER_VER_SERVICE_DATA_TYPE) {
				DECODE_FIELD( depthMktDataDescriptions[i].exchange);
				DECODE_FIELD( depthMktDataDescriptions[i].secType);
				DECODE_FIELD( depthMktDataDescriptions[i].listingExch);
				DECODE_FIELD( depthMktDataDescriptions[i].serviceDataType);
				DECODE_FIELD( depthMktDataDescriptions[i].aggGroup);
			} else {
				DECODE_FIELD( depthMktDataDescriptions[i].exchange);
				DECODE_FIELD( depthMktDataDescriptions[i].secType);
				bool isL2;
				DECODE_FIELD( isL2);
				depthMktDataDescriptions[i].serviceDataType = isL2 ? "Deep2" : "Deep";
			}
		}
	}

	m_pEWrapper->mktDepthExchanges(depthMktDataDescriptions);

	return ptr;
}

const char* EDecoder::processMktDepthExchangesMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::MarketDepthExchanges marketDepthExchangesProto;
	marketDepthExchangesProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->marketDepthExchangesProtoBuf(marketDepthExchangesProto);
#endif

	std::vector<DepthMktDataDescription> depthMktDataDescriptions;

	if (marketDepthExchangesProto.depthmarketdatadescriptions_size() > 0) {
		for (int i = 0; i < marketDepthExchangesProto.depthmarketdatadescriptions_size(); i++) {
			const protobuf::DepthMarketDataDescription& proto = marketDepthExchangesProto.depthmarketdatadescriptions(i);
			depthMktDataDescriptions.push_back(EDecoderUtils::decodeDepthMarketDataDescription(proto));
		}
	}

	m_pEWrapper->mktDepthExchanges(depthMktDataDescriptions);

	return ptr;
}

const char* EDecoder::processTickNewsMsg(const char* ptr, const char* endPtr) 
{
	int tickerId;
	time_t timeStamp;
	std::string providerCode;
	std::string articleId;
	std::string headline;
	std::string extraData;

	DECODE_FIELD( tickerId);
	DECODE_FIELD_TIME( timeStamp);
	DECODE_FIELD( providerCode);
	DECODE_FIELD( articleId);
	DECODE_FIELD( headline);
	DECODE_FIELD( extraData);

	m_pEWrapper->tickNews(tickerId, timeStamp, providerCode, articleId, headline, extraData);

	return ptr;
}

const char* EDecoder::processTickNewsMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::TickNews tickNewsProto;
	tickNewsProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->tickNewsProtoBuf(tickNewsProto);
#endif

	int reqId = tickNewsProto.has_reqid() ? tickNewsProto.reqid() : NO_VALID_ID;
	time_t timestamp = tickNewsProto.has_timestamp() ? tickNewsProto.timestamp() : 0;
	std::string providerCode = tickNewsProto.has_providercode() ? tickNewsProto.providercode() : "";
	std::string articleId = tickNewsProto.has_articleid() ? tickNewsProto.articleid() : "";
	std::string headline = tickNewsProto.has_headline() ? tickNewsProto.headline() : "";
	std::string extraData = tickNewsProto.has_extradata() ? tickNewsProto.extradata() : "";

	m_pEWrapper->tickNews(reqId, timestamp, providerCode, articleId, headline, extraData);

	return ptr;
}

const char* EDecoder::processNewsProvidersMsg(const char* ptr, const char* endPtr) 
{
	typedef std::vector<NewsProvider> NewsProviderList;
	NewsProviderList newsProviders;
	int nNewsProviders = 0;
	DECODE_FIELD( nNewsProviders);

	if (nNewsProviders > 0) {
		newsProviders.resize(nNewsProviders);
		for( int i = 0; i < nNewsProviders; ++i) {
			DECODE_FIELD( newsProviders[i].providerCode);
			DECODE_FIELD( newsProviders[i].providerName);
		}
	}

	m_pEWrapper->newsProviders(newsProviders);

	return ptr;
}

const char* EDecoder::processNewsProvidersMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::NewsProviders newsProvidersProto;
	newsProvidersProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->newsProvidersProtoBuf(newsProvidersProto);
#endif

	std::vector<NewsProvider> newsProviders;

	for (int i = 0; i < newsProvidersProto.newsproviders_size(); ++i) {
		const protobuf::NewsProvider& newsProviderProto = newsProvidersProto.newsproviders(i);
		NewsProvider newsProvider;
		newsProvider.providerCode = newsProviderProto.has_providercode() ? newsProviderProto.providercode() : "";
		newsProvider.providerName = newsProviderProto.has_providername() ? newsProviderProto.providername() : "";
		newsProviders.push_back(newsProvider);
	}

	m_pEWrapper->newsProviders(newsProviders);

	return ptr;
}

const char* EDecoder::processNewsArticleMsg(const char* ptr, const char* endPtr) 
{
	int requestId;
	int articleType;
	std::string articleText;

	DECODE_FIELD( requestId);
	DECODE_FIELD( articleType);
	DECODE_FIELD( articleText);

	m_pEWrapper->newsArticle(requestId, articleType, articleText);

	return ptr;
}

const char* EDecoder::processNewsArticleMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::NewsArticle newsArticleProto;
	newsArticleProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->newsArticleProtoBuf(newsArticleProto);
#endif

	int reqId = newsArticleProto.has_reqid() ? newsArticleProto.reqid() : NO_VALID_ID;
	int articleType = newsArticleProto.has_articletype() ? newsArticleProto.articletype() : 0;
	std::string articleText = newsArticleProto.has_articletext() ? newsArticleProto.articletext() : "";

	m_pEWrapper->newsArticle(reqId, articleType, articleText);

	return ptr;
}

const char* EDecoder::processHistoricalNewsMsg(const char* ptr, const char* endPtr) 
{
	int requestId;
	std::string time;
	std::string providerCode;
	std::string articleId;
	std::string headline;

	DECODE_FIELD( requestId);
	DECODE_FIELD( time);
	DECODE_FIELD( providerCode);
	DECODE_FIELD( articleId);
	DECODE_FIELD( headline);

	m_pEWrapper->historicalNews(requestId, time, providerCode, articleId, headline);

	return ptr;
}

const char* EDecoder::processHistoricalNewsMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::HistoricalNews historicalNewsProto;
	historicalNewsProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->historicalNewsProtoBuf(historicalNewsProto);
#endif

	int reqId = historicalNewsProto.has_reqid() ? historicalNewsProto.reqid() : NO_VALID_ID;
	std::string time = historicalNewsProto.has_time() ? historicalNewsProto.time() : "";
	std::string providerCode = historicalNewsProto.has_providercode() ? historicalNewsProto.providercode() : "";
	std::string articleId = historicalNewsProto.has_articleid() ? historicalNewsProto.articleid() : "";
	std::string headline = historicalNewsProto.has_headline() ? historicalNewsProto.headline() : "";

	m_pEWrapper->historicalNews(reqId, time, providerCode, articleId, headline);

	return ptr;
}

const char* EDecoder::processHistoricalNewsEndMsg(const char* ptr, const char* endPtr) 
{
	int requestId;
	bool hasMore;

	DECODE_FIELD( requestId);
	DECODE_FIELD( hasMore);

	m_pEWrapper->historicalNewsEnd(requestId, hasMore);

	return ptr;
}

const char* EDecoder::processHistoricalNewsEndMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::HistoricalNewsEnd historicalNewsEndProto;
	historicalNewsEndProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->historicalNewsEndProtoBuf(historicalNewsEndProto);
#endif

	int reqId = historicalNewsEndProto.has_reqid() ? historicalNewsEndProto.reqid() : NO_VALID_ID;
	bool hasMore = historicalNewsEndProto.has_hasmore() ? historicalNewsEndProto.hasmore() : false;

	m_pEWrapper->historicalNewsEnd(reqId, hasMore);

	return ptr;
}

const char* EDecoder::processMarketRuleMsg(const char* ptr, const char* endPtr)
{
	int marketRuleId;
	typedef std::vector<PriceIncrement> PriceIncrementList;
	PriceIncrementList priceIncrements;
	int nPriceIncrements = 0;

	DECODE_FIELD( marketRuleId);
	DECODE_FIELD( nPriceIncrements);

	if (nPriceIncrements > 0) {
		priceIncrements.resize(nPriceIncrements);
		for( int i = 0; i < nPriceIncrements; ++i) {
			DECODE_FIELD( priceIncrements[i].lowEdge);
			DECODE_FIELD( priceIncrements[i].increment);
		}
	}

	m_pEWrapper->marketRule(marketRuleId, priceIncrements);

	return ptr;
}

const char* EDecoder::processMarketRuleMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::MarketRule marketRuleProto;
	marketRuleProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->marketRuleProtoBuf(marketRuleProto);
#endif

	int marketRuleId = marketRuleProto.has_marketruleid() ? marketRuleProto.marketruleid() : 0;

	std::vector<PriceIncrement> priceIncrements;

	for (int i = 0; i < marketRuleProto.priceincrements_size(); i++) {
		protobuf::PriceIncrement priceIncrement = marketRuleProto.priceincrements(i);
		priceIncrements.push_back(EDecoderUtils::decodePriceIncrement(priceIncrement));
	}

	m_pEWrapper->marketRule(marketRuleId, priceIncrements);

	return ptr;
}

int EDecoder::processConnectAck(const char*& beginPtr, const char* endPtr)
{
	// process a connect Ack message from the buffer;
	// return number of bytes consumed
	assert( beginPtr && beginPtr < endPtr);

	try {

		const char* ptr = beginPtr;

		// check server version
		DECODE_FIELD( m_serverVersion);

		std::string twsTime;

		if( m_serverVersion >= 20) {
			DECODE_FIELD(twsTime);
		}

		if (m_pClientMsgSink)
			m_pClientMsgSink->serverVersion(m_serverVersion, twsTime.c_str());

		m_pEWrapper->connectAck();

		int processed = ptr - beginPtr;
		beginPtr = ptr;
		return processed;
	}
	catch(const std::exception& e) {
		m_pEWrapper->error( NO_VALID_ID, Utils::currentTimeMillis(), SOCKET_EXCEPTION.code(), SOCKET_EXCEPTION.msg() + e.what(), "");
	}

	return 0;
}

const char* EDecoder::processSmartComponentsMsg(const char* ptr, const char* endPtr) {
	int reqId;
	int n;
	SmartComponentsMap theMap;

	DECODE_FIELD(reqId);
	DECODE_FIELD(n);

	for (int i = 0; i < n; i++) {
		int bitNumber;
		std::string exchange;
		char exchangeLetter;

		DECODE_FIELD(bitNumber);
		DECODE_FIELD(exchange);
		DECODE_FIELD(exchangeLetter);

		theMap[bitNumber] = std::make_tuple(exchange, exchangeLetter);
	}

	m_pEWrapper->smartComponents(reqId, theMap);
	
	return ptr;
}

const char* EDecoder::processSmartComponentsMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::SmartComponents smartComponentsProto;
	smartComponentsProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->smartComponentsProtoBuf(smartComponentsProto);
#endif

	int reqId = smartComponentsProto.has_reqid() ? smartComponentsProto.reqid() : NO_VALID_ID;
	SmartComponentsMap theMap = EDecoderUtils::decodeSmartComponents(smartComponentsProto);

	m_pEWrapper->smartComponents(reqId, theMap);

	return ptr;
}

const char* EDecoder::processTickReqParamsMsg(const char* ptr, const char* endPtr) {
	int tickerId;
	double minTick;
	std::string bboExchange;
	int snapshotPermissions;

	DECODE_FIELD(tickerId);
	DECODE_FIELD(minTick);
	DECODE_FIELD(bboExchange);
	DECODE_FIELD(snapshotPermissions);

	m_pEWrapper->tickReqParams(tickerId, minTick, bboExchange, snapshotPermissions);
	
	return ptr;
}

const char* EDecoder::processTickReqParamsMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::TickReqParams tickReqParamsProto;
	tickReqParamsProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->tickReqParamsProtoBuf(tickReqParamsProto);
#endif

	int reqId = tickReqParamsProto.has_reqid() ? tickReqParamsProto.reqid() : NO_VALID_ID;
	double minTick = tickReqParamsProto.has_mintick() ? atof(tickReqParamsProto.mintick().c_str()) : UNSET_DOUBLE;
	std::string bboExchange = tickReqParamsProto.has_bboexchange() ? tickReqParamsProto.bboexchange() : "";
	int snapshotPermissions = tickReqParamsProto.has_snapshotpermissions() ? tickReqParamsProto.snapshotpermissions() : UNSET_INTEGER;

	m_pEWrapper->tickReqParams(reqId, minTick, bboExchange, snapshotPermissions);

	return ptr;
}

const char* EDecoder::processHeadTimestampMsg(const char* ptr, const char* endPtr) {
	int reqId;
	std::string headTimestamp;

	DECODE_FIELD(reqId);
	DECODE_FIELD(headTimestamp);

	m_pEWrapper->headTimestamp(reqId, headTimestamp);
	
	return ptr;
}

const char* EDecoder::processHeadTimestampMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::HeadTimestamp headTimestampProto;
	headTimestampProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->headTimestampProtoBuf(headTimestampProto);
#endif

	int reqId = headTimestampProto.has_reqid() ? headTimestampProto.reqid() : NO_VALID_ID;
	std::string headTimestamp = headTimestampProto.has_headtimestamp() ? headTimestampProto.headtimestamp() : "";

	m_pEWrapper->headTimestamp(reqId, headTimestamp);

	return ptr;
}

const char* EDecoder::processHistogramDataMsg(const char* ptr, const char* endPtr) {
	int reqId;
	int n;
	HistogramDataVector data;
	HistogramEntry entry;

	DECODE_FIELD(reqId);
	DECODE_FIELD(n);

	for (int i = 0; i < n; i++) {
		DECODE_FIELD(entry.price);
		DECODE_FIELD(entry.size);

		data.push_back(entry);
	}

	m_pEWrapper->histogramData(reqId, data);
	
	return ptr;
}

const char* EDecoder::processHistogramDataMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::HistogramData histogramDataProto;
	histogramDataProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->histogramDataProtoBuf(histogramDataProto);
#endif

	int reqId = histogramDataProto.has_reqid() ? histogramDataProto.reqid() : NO_VALID_ID;
	HistogramDataVector histogramData;

	for (int i = 0; i < histogramDataProto.histogramdataentries_size(); ++i) {
		const protobuf::HistogramDataEntry& histogramDataEntryProto = histogramDataProto.histogramdataentries(i);
		HistogramEntry histogramEntry = EDecoderUtils::decodeHistogramDataEntry(histogramDataEntryProto);
		histogramData.push_back(histogramEntry);
	}

	m_pEWrapper->histogramData(reqId, histogramData);

	return ptr;
}

const char* EDecoder::processRerouteMktDataReqMsg(const char* ptr, const char* endPtr) {
	int reqId;
	int conId;
	std::string exchange;

	DECODE_FIELD(reqId);
	DECODE_FIELD(conId);
	DECODE_FIELD(exchange);

	m_pEWrapper->rerouteMktDataReq(reqId, conId, exchange);
	
	return ptr;
}

const char* EDecoder::processRerouteMktDataReqMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::RerouteMarketDataRequest rerouteMarketDataRequestProto;
	rerouteMarketDataRequestProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->rerouteMarketDataRequestProtoBuf(rerouteMarketDataRequestProto);
#endif

	int reqId = rerouteMarketDataRequestProto.has_reqid() ? rerouteMarketDataRequestProto.reqid() : NO_VALID_ID;
	int conId = rerouteMarketDataRequestProto.has_conid() ? rerouteMarketDataRequestProto.conid() : 0;
	std::string exchange = rerouteMarketDataRequestProto.has_exchange() ? rerouteMarketDataRequestProto.exchange() : "";

	m_pEWrapper->rerouteMktDataReq(reqId, conId, exchange);

	return ptr;
}

const char* EDecoder::processRerouteMktDepthReqMsg(const char* ptr, const char* endPtr) {
	int reqId;
	int conId;
	std::string exchange;

	DECODE_FIELD(reqId);
	DECODE_FIELD(conId);
	DECODE_FIELD(exchange);

	m_pEWrapper->rerouteMktDepthReq(reqId, conId, exchange);
	
	return ptr;
}

const char* EDecoder::processRerouteMktDepthReqMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::RerouteMarketDepthRequest rerouteMarketDepthRequestProto;
	rerouteMarketDepthRequestProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->rerouteMarketDepthRequestProtoBuf(rerouteMarketDepthRequestProto);
#endif

	int reqId = rerouteMarketDepthRequestProto.has_reqid() ? rerouteMarketDepthRequestProto.reqid() : NO_VALID_ID;
	int conId = rerouteMarketDepthRequestProto.has_conid() ? rerouteMarketDepthRequestProto.conid() : 0;
	std::string exchange = rerouteMarketDepthRequestProto.has_exchange() ? rerouteMarketDepthRequestProto.exchange() : "";

	m_pEWrapper->rerouteMktDepthReq(reqId, conId, exchange);

	return ptr;
}

const char* EDecoder::processPnLMsg(const char* ptr, const char* endPtr) {
    int reqId;
    double dailyPnL;
    double unrealizedPnL = DBL_MAX;
    double realizedPnL = DBL_MAX;

    DECODE_FIELD(reqId)
    DECODE_FIELD(dailyPnL)

    if (m_serverVersion >= MIN_SERVER_VER_UNREALIZED_PNL) {
        DECODE_FIELD(unrealizedPnL)
    }

    if (m_serverVersion >= MIN_SERVER_VER_REALIZED_PNL) {
        DECODE_FIELD(realizedPnL)
    }

    m_pEWrapper->pnl(reqId, dailyPnL, unrealizedPnL, realizedPnL);
    
    return ptr;
}

const char* EDecoder::processPnLMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::PnL pnlProto;
	pnlProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->pnlProtoBuf(pnlProto);
#endif

	int reqId = pnlProto.has_reqid() ? pnlProto.reqid() : NO_VALID_ID;
	double dailyPnL = pnlProto.has_dailypnl() ? pnlProto.dailypnl() : UNSET_DOUBLE;
	double unrealizedPnL = pnlProto.has_unrealizedpnl() ? pnlProto.unrealizedpnl() : UNSET_DOUBLE;
	double realizedPnL = pnlProto.has_realizedpnl() ? pnlProto.realizedpnl() : UNSET_DOUBLE;

	m_pEWrapper->pnl(reqId, dailyPnL, unrealizedPnL, realizedPnL);

	return ptr;
}

const char* EDecoder::processPnLSingleMsg(const char* ptr, const char* endPtr) {
    int reqId;
    Decimal pos;
    double dailyPnL;
    double unrealizedPnL = DBL_MAX;
    double realizedPnL = DBL_MAX;
    double value;

    DECODE_FIELD(reqId);
    DECODE_FIELD(pos);
    DECODE_FIELD(dailyPnL);

    if (m_serverVersion >= MIN_SERVER_VER_UNREALIZED_PNL) {
        DECODE_FIELD(unrealizedPnL)
    }

    if (m_serverVersion >= MIN_SERVER_VER_REALIZED_PNL) {
        DECODE_FIELD(realizedPnL)
    }

    DECODE_FIELD(value);


    m_pEWrapper->pnlSingle(reqId, pos, dailyPnL, unrealizedPnL, realizedPnL, value);

    return ptr;
}

const char* EDecoder::processPnLSingleMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::PnLSingle pnlSingleProto;
	pnlSingleProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->pnlSingleProtoBuf(pnlSingleProto);
#endif

	int reqId = pnlSingleProto.has_reqid() ? pnlSingleProto.reqid() : NO_VALID_ID;
	Decimal pos = pnlSingleProto.has_position() ? DecimalFunctions::stringToDecimal(pnlSingleProto.position()) : UNSET_DECIMAL;
	double dailyPnL = pnlSingleProto.has_dailypnl() ? pnlSingleProto.dailypnl() : UNSET_DOUBLE;
	double unrealizedPnL = pnlSingleProto.has_unrealizedpnl() ? pnlSingleProto.unrealizedpnl() : UNSET_DOUBLE;
	double realizedPnL = pnlSingleProto.has_realizedpnl() ? pnlSingleProto.realizedpnl() : UNSET_DOUBLE;
	double value = pnlSingleProto.has_value() ? pnlSingleProto.value() : UNSET_DOUBLE;

	m_pEWrapper->pnlSingle(reqId, pos, dailyPnL, unrealizedPnL, realizedPnL, value);

	return ptr;
}

template<typename T>
const char* EDecoder::processHistoricalTicks(const char* ptr, const char* endPtr) {
    int reqId, nTicks;
    bool done;

    DECODE_FIELD(reqId);
    DECODE_FIELD(nTicks);

    std::vector<T> ticks(nTicks);

    for (int i = 0; i < nTicks; i++) {
        T &tick = ticks[i];

        ptr = decodeTick(tick, ptr, endPtr);
    }

    DECODE_FIELD(done);

    callEWrapperCallBack(reqId, ticks, done);

    return ptr;
}

const char* EDecoder::processHistoricalTicksMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::HistoricalTicks historicalTicksProto;
	historicalTicksProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->historicalTicksProtoBuf(historicalTicksProto);
#endif

	int reqId = historicalTicksProto.has_reqid() ? historicalTicksProto.reqid() : NO_VALID_ID;
	std::vector<HistoricalTick> historicalTicks;
	bool isDone = historicalTicksProto.has_isdone() ? historicalTicksProto.isdone() : false;

	for (int i = 0; i < historicalTicksProto.historicalticks_size(); ++i) {
		const protobuf::HistoricalTick& historicalTickProto = historicalTicksProto.historicalticks(i);
		HistoricalTick historicalTick = EDecoderUtils::decodeHistoricalTick(historicalTickProto);
		historicalTicks.push_back(historicalTick);
	}

	m_pEWrapper->historicalTicks(reqId, historicalTicks, isDone);

	return ptr;
}

const char* EDecoder::processHistoricalTicksBidAskMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::HistoricalTicksBidAsk historicalTicksBidAskProto;
	historicalTicksBidAskProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->historicalTicksBidAskProtoBuf(historicalTicksBidAskProto);
#endif

	int reqId = historicalTicksBidAskProto.has_reqid() ? historicalTicksBidAskProto.reqid() : NO_VALID_ID;
	std::vector<HistoricalTickBidAsk> historicalTicksBidAsk;
	bool done = historicalTicksBidAskProto.has_isdone() ? historicalTicksBidAskProto.isdone() : false;

	for (int i = 0; i < historicalTicksBidAskProto.historicalticksbidask_size(); ++i) {
		const protobuf::HistoricalTickBidAsk& historicalTickBidAskProto = historicalTicksBidAskProto.historicalticksbidask(i);
		HistoricalTickBidAsk historicalTickBidAsk = EDecoderUtils::decodeHistoricalTickBidAsk(historicalTickBidAskProto);
		historicalTicksBidAsk.push_back(historicalTickBidAsk);
	}

	m_pEWrapper->historicalTicksBidAsk(reqId, historicalTicksBidAsk, done);

	return ptr;
}

const char* EDecoder::processHistoricalTicksLastMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::HistoricalTicksLast historicalTicksLastProto;
	historicalTicksLastProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->historicalTicksLastProtoBuf(historicalTicksLastProto);
#endif

	int reqId = historicalTicksLastProto.has_reqid() ? historicalTicksLastProto.reqid() : NO_VALID_ID;
	std::vector<HistoricalTickLast> historicalTicksLast;
	bool done = historicalTicksLastProto.has_isdone() ? historicalTicksLastProto.isdone() : false;

	for (int i = 0; i < historicalTicksLastProto.historicaltickslast_size(); ++i) {
		const protobuf::HistoricalTickLast& historicalTickLastProto = historicalTicksLastProto.historicaltickslast(i);
		HistoricalTickLast historicalTickLast = EDecoderUtils::decodeHistoricalTickLast(historicalTickLastProto);
		historicalTicksLast.push_back(historicalTickLast);
	}

	m_pEWrapper->historicalTicksLast(reqId, historicalTicksLast, done);

	return ptr;
}

const char* EDecoder::decodeTick(HistoricalTick& tick, const char* ptr, const char* endPtr) {
    int nope;

    DECODE_FIELD(tick.time);
    DECODE_FIELD(nope);
    DECODE_FIELD(tick.price);
    DECODE_FIELD(tick.size);

    return ptr;
}

void EDecoder::callEWrapperCallBack(int reqId, const std::vector<HistoricalTick> &ticks, bool done) {
    m_pEWrapper->historicalTicks(reqId, ticks, done);
}

const char* EDecoder::decodeTick(HistoricalTickBidAsk& tick, const char* ptr, const char* endPtr) {
    DECODE_FIELD(tick.time);
    int attrMask;
    DECODE_FIELD(attrMask);
    std::bitset<32> mask(attrMask);
    tick.tickAttribBidAsk.askPastHigh = mask[0];
    tick.tickAttribBidAsk.bidPastLow = mask[1];
    DECODE_FIELD(tick.priceBid);
    DECODE_FIELD(tick.priceAsk);
    DECODE_FIELD(tick.sizeBid);
    DECODE_FIELD(tick.sizeAsk);

    return ptr;
}

void EDecoder::callEWrapperCallBack(int reqId, const std::vector<HistoricalTickBidAsk> &ticks, bool done) {
    m_pEWrapper->historicalTicksBidAsk(reqId, ticks, done);
}

const char* EDecoder::decodeTick(HistoricalTickLast& tick, const char* ptr, const char* endPtr) {
    DECODE_FIELD(tick.time);
    int attrMask;
    DECODE_FIELD(attrMask);
    std::bitset<32> mask(attrMask);
    tick.tickAttribLast.pastLimit = mask[0];
    tick.tickAttribLast.unreported = mask[1];
    DECODE_FIELD(tick.price);
    DECODE_FIELD(tick.size);
    DECODE_FIELD(tick.exchange);
    DECODE_FIELD(tick.specialConditions);

    return ptr;
}

void EDecoder::callEWrapperCallBack(int reqId, const std::vector<HistoricalTickLast> &ticks, bool done) {
    m_pEWrapper->historicalTicksLast(reqId, ticks, done);
}

const char* EDecoder::processHistoricalTicks(const char* ptr, const char* endPtr) {
    return processHistoricalTicks<HistoricalTick>(ptr, endPtr);
}

const char* EDecoder::processHistoricalTicksBidAsk(const char* ptr, const char* endPtr) {
    return processHistoricalTicks<HistoricalTickBidAsk>(ptr, endPtr);
}

const char* EDecoder::processHistoricalTicksLast(const char* ptr, const char* endPtr) {
    return processHistoricalTicks<HistoricalTickLast>(ptr, endPtr);
}

const char* EDecoder::processTickByTickDataMsg(const char* ptr, const char* endPtr) {
    int reqId;
    int tickType = 0;
	time_t time;

    DECODE_FIELD(reqId);
    DECODE_FIELD(tickType);
    DECODE_FIELD(time);

    if (tickType == 1 || tickType == 2) { // Last/AllLast
            double price;
            Decimal size;
            int attrMask;
            TickAttribLast tickAttribLast = {};
            std::string exchange;
            std::string specialConditions;

            DECODE_FIELD(price);
            DECODE_FIELD(size);
            DECODE_FIELD(attrMask);

            std::bitset<32> mask(attrMask);
            tickAttribLast.pastLimit = mask[0];
            tickAttribLast.unreported = mask[1];

            DECODE_FIELD(exchange);
            DECODE_FIELD(specialConditions);

            m_pEWrapper->tickByTickAllLast(reqId, tickType, time, price, size, tickAttribLast, exchange, specialConditions);

    } else if (tickType == 3) { // BidAsk
            double bidPrice;
            double askPrice;
            Decimal bidSize;
            Decimal askSize;
            int attrMask;
            DECODE_FIELD(bidPrice);
            DECODE_FIELD(askPrice);
            DECODE_FIELD(bidSize);
            DECODE_FIELD(askSize);
            DECODE_FIELD(attrMask);

            TickAttribBidAsk tickAttribBidAsk = {};
            std::bitset<32> mask(attrMask);
            tickAttribBidAsk.bidPastLow = mask[0];
            tickAttribBidAsk.askPastHigh = mask[1];

            m_pEWrapper->tickByTickBidAsk(reqId, time, bidPrice, askPrice, bidSize, askSize, tickAttribBidAsk);
    } else if (tickType == 4) { // MidPoint
            double midPoint;
            DECODE_FIELD(midPoint);

            m_pEWrapper->tickByTickMidPoint(reqId, time, midPoint);
    }

    return ptr;
}

const char* EDecoder::processTickByTickMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::TickByTickData tickByTickDataProto;
	tickByTickDataProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->tickByTickDataProtoBuf(tickByTickDataProto);
#endif

	int reqId = tickByTickDataProto.has_reqid() ? tickByTickDataProto.reqid() : NO_VALID_ID;
	int tickType = tickByTickDataProto.has_ticktype() ? tickByTickDataProto.ticktype() : 0;

	switch (tickType) {
	case 0: // None
		break;
	case 1: // Last
	case 2: // AllLast
		if (tickByTickDataProto.has_historicalticklast()) {
			HistoricalTickLast historicalTickLast = EDecoderUtils::decodeHistoricalTickLast(tickByTickDataProto.historicalticklast());
			m_pEWrapper->tickByTickAllLast(reqId, tickType, historicalTickLast.time, historicalTickLast.price,
				historicalTickLast.size, historicalTickLast.tickAttribLast,
				historicalTickLast.exchange, historicalTickLast.specialConditions);
		}
		break;

	case 3: // BidAsk
		if (tickByTickDataProto.has_historicaltickbidask()) {
			HistoricalTickBidAsk historicalTickBidAsk = EDecoderUtils::decodeHistoricalTickBidAsk(tickByTickDataProto.historicaltickbidask());
			m_pEWrapper->tickByTickBidAsk(reqId, historicalTickBidAsk.time, historicalTickBidAsk.priceBid,
				historicalTickBidAsk.priceAsk, historicalTickBidAsk.sizeBid,
				historicalTickBidAsk.sizeAsk, historicalTickBidAsk.tickAttribBidAsk);
		}
		break;

	case 4: // MidPoint
		if (tickByTickDataProto.has_historicaltickmidpoint()) {
			HistoricalTick historicalTick = EDecoderUtils::decodeHistoricalTick(tickByTickDataProto.historicaltickmidpoint());
			m_pEWrapper->tickByTickMidPoint(reqId, historicalTick.time, historicalTick.price);
		}
		break;
	}

	return ptr;
}

const char* EDecoder::processOrderBoundMsg(const char* ptr, const char* endPtr) {
	long long permId;
	int clientId;
	int orderId;

	DECODE_FIELD( permId);
	DECODE_FIELD( clientId);
	DECODE_FIELD( orderId);

	m_pEWrapper->orderBound( permId, clientId, orderId);

	return ptr;
}

const char* EDecoder::processOrderBoundMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::OrderBound orderBoundProto;
	orderBoundProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->orderBoundProtoBuf(orderBoundProto);
#endif

	long permId = orderBoundProto.has_permid() ? orderBoundProto.permid() : UNSET_LONG;
	int clientId = orderBoundProto.has_clientid() ? orderBoundProto.clientid() : UNSET_INTEGER;
	int orderId = orderBoundProto.has_orderid() ? orderBoundProto.orderid() : UNSET_INTEGER;

	m_pEWrapper->orderBound(permId, clientId, orderId);

	return ptr;
}

const char* EDecoder::processCompletedOrderMsg(const char* ptr, const char* endPtr) 
{
	Order order;
	Contract contract;
	OrderState orderState;
	EOrderDecoder eOrderDecoder(&contract, &order, &orderState, UNSET_INTEGER, m_serverVersion);

	// read contract fields
        if (!eOrderDecoder.decodeContract(ptr, endPtr)) {
          return nullptr;
        }

	// read order fields
        bool success =
             eOrderDecoder.decodeAction(ptr, endPtr)
          && eOrderDecoder.decodeTotalQuantity(ptr, endPtr)
          && eOrderDecoder.decodeOrderType(ptr, endPtr)
          && eOrderDecoder.decodeLmtPrice(ptr, endPtr)
          && eOrderDecoder.decodeAuxPrice(ptr, endPtr)
          && eOrderDecoder.decodeTIF(ptr, endPtr)
          && eOrderDecoder.decodeOcaGroup(ptr, endPtr)
          && eOrderDecoder.decodeAccount(ptr, endPtr)
          && eOrderDecoder.decodeOpenClose(ptr, endPtr)
          && eOrderDecoder.decodeOrigin(ptr, endPtr)
          && eOrderDecoder.decodeOrderRef(ptr, endPtr)
          && eOrderDecoder.decodePermId(ptr, endPtr)
          && eOrderDecoder.decodeOutsideRth(ptr, endPtr)
          && eOrderDecoder.decodeHidden(ptr, endPtr)
          && eOrderDecoder.decodeDiscretionaryAmount(ptr, endPtr)
          && eOrderDecoder.decodeGoodAfterTime(ptr, endPtr)
          && eOrderDecoder.decodeFAParams(ptr, endPtr)
          && eOrderDecoder.decodeModelCode(ptr, endPtr)
          && eOrderDecoder.decodeGoodTillDate(ptr, endPtr)
          && eOrderDecoder.decodeRule80A(ptr, endPtr)
          && eOrderDecoder.decodePercentOffset(ptr, endPtr)
          && eOrderDecoder.decodeSettlingFirm(ptr, endPtr)
          && eOrderDecoder.decodeShortSaleParams(ptr, endPtr)
          && eOrderDecoder.decodeBoxOrderParams(ptr, endPtr)
          && eOrderDecoder.decodePegToStkOrVolOrderParams(ptr, endPtr)
          && eOrderDecoder.decodeDisplaySize(ptr, endPtr)
          && eOrderDecoder.decodeSweepToFill(ptr, endPtr)
          && eOrderDecoder.decodeAllOrNone(ptr, endPtr)
          && eOrderDecoder.decodeMinQty(ptr, endPtr)
          && eOrderDecoder.decodeOcaType(ptr, endPtr)
          && eOrderDecoder.decodeTriggerMethod(ptr, endPtr)
          && eOrderDecoder.decodeVolOrderParams(ptr, endPtr, false)
          && eOrderDecoder.decodeTrailParams(ptr, endPtr)
          && eOrderDecoder.decodeComboLegs(ptr, endPtr)
          && eOrderDecoder.decodeSmartComboRoutingParams(ptr, endPtr)
          && eOrderDecoder.decodeScaleOrderParams(ptr, endPtr)
          && eOrderDecoder.decodeHedgeParams(ptr, endPtr)
          && eOrderDecoder.decodeClearingParams(ptr, endPtr)
          && eOrderDecoder.decodeNotHeld(ptr, endPtr)
          && eOrderDecoder.decodeDeltaNeutral(ptr, endPtr)
          && eOrderDecoder.decodeAlgoParams(ptr, endPtr)
          && eOrderDecoder.decodeSolicited(ptr, endPtr)
          && eOrderDecoder.decodeOrderStatus(ptr, endPtr)
          && eOrderDecoder.decodeVolRandomizeFlags(ptr, endPtr)
          && eOrderDecoder.decodePegBenchParams(ptr, endPtr)
          && eOrderDecoder.decodeConditions(ptr, endPtr)
          && eOrderDecoder.decodeStopPriceAndLmtPriceOffset(ptr, endPtr)
          && eOrderDecoder.decodeCashQty(ptr, endPtr)
          && eOrderDecoder.decodeDontUseAutoPriceForHedge(ptr, endPtr)
          && eOrderDecoder.decodeIsOmsContainer(ptr, endPtr)
          && eOrderDecoder.decodeAutoCancelDate(ptr, endPtr)
          && eOrderDecoder.decodeFilledQuantity(ptr, endPtr)
          && eOrderDecoder.decodeRefFuturesConId(ptr, endPtr)
          && eOrderDecoder.decodeAutoCancelParent(ptr, endPtr)
          && eOrderDecoder.decodeShareholder(ptr, endPtr)
          && eOrderDecoder.decodeImbalanceOnly(ptr, endPtr)
          && eOrderDecoder.decodeRouteMarketableToBbo(ptr, endPtr)
          && eOrderDecoder.decodeParentPermId(ptr, endPtr)
          && eOrderDecoder.decodeCompletedTime(ptr, endPtr)
          && eOrderDecoder.decodeCompletedStatus(ptr, endPtr)
          && eOrderDecoder.decodePegBestPegMidOrderAttributes(ptr, endPtr)
          && eOrderDecoder.decodeCustomerAccount(ptr, endPtr)
          && eOrderDecoder.decodeProfessionalCustomer(ptr, endPtr)
          && eOrderDecoder.decodeSubmitter(ptr, endPtr);

        if (!success) {
          return nullptr;
        }

	m_pEWrapper->completedOrder(contract, order, orderState);

	return ptr;
}

const char* EDecoder::processCompletedOrderMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::CompletedOrder completedOrderProto;
	completedOrderProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->completedOrderProtoBuf(completedOrderProto);
#endif

	Contract contract;
	if (completedOrderProto.has_contract()) contract = EDecoderUtils::decodeContract(completedOrderProto.contract());
	Order order;
	if (completedOrderProto.has_order()) order = EDecoderUtils::decodeOrder(UNSET_INTEGER, completedOrderProto.contract(), completedOrderProto.order());
	OrderState orderState;
	if (completedOrderProto.has_orderstate()) orderState = EDecoderUtils::decodeOrderState(completedOrderProto.orderstate());

	m_pEWrapper->completedOrder(contract, order, orderState);

	return ptr;
}

const char* EDecoder::processCompletedOrdersEndMsg(const char* ptr, const char* endPtr) 
{
	m_pEWrapper->completedOrdersEnd();
	return ptr;
}

const char* EDecoder::processCompletedOrdersEndMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::CompletedOrdersEnd completedOrdersEndProto;
	completedOrdersEndProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->completedOrdersEndProtoBuf(completedOrdersEndProto);
#endif

	m_pEWrapper->completedOrdersEnd();

	return ptr;
}

const char* EDecoder::processReplaceFAEndMsg(const char* ptr, const char* endPtr) {
	int reqId;
	std::string text;

	DECODE_FIELD(reqId);
	DECODE_FIELD(text);

	m_pEWrapper->replaceFAEnd(reqId, text);

	return ptr;
}

const char* EDecoder::processReplaceFAEndMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::ReplaceFAEnd replaceFAEndProto;
	replaceFAEndProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->replaceFAEndProtoBuf(replaceFAEndProto);
#endif

	int reqId = replaceFAEndProto.has_reqid() ? replaceFAEndProto.reqid() : NO_VALID_ID;
	std::string text = replaceFAEndProto.has_text() ? replaceFAEndProto.text() : "";

	m_pEWrapper->replaceFAEnd(reqId, text);

	return ptr;
}

const char* EDecoder::processWshEventData(const char* ptr, const char* endPtr) {
	int reqId;
	std::string dataJson;

	DECODE_FIELD(reqId);
	DECODE_FIELD(dataJson);

	m_pEWrapper->wshEventData(reqId, dataJson);

	return ptr;
}

const char* EDecoder::processWshEventDataMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::WshEventData wshEventDataProto;
	wshEventDataProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->wshEventDataProtoBuf(wshEventDataProto);
#endif

	int reqId = wshEventDataProto.has_reqid() ? wshEventDataProto.reqid() : NO_VALID_ID;
	std::string dataJson = wshEventDataProto.has_datajson() ? wshEventDataProto.datajson() : "";

	m_pEWrapper->wshEventData(reqId, dataJson);

	return ptr;
}

const char* EDecoder::processWshMetaData(const char* ptr, const char* endPtr) {
	int reqId;
	std::string dataJson;

	DECODE_FIELD(reqId);
	DECODE_FIELD(dataJson);

	m_pEWrapper->wshMetaData(reqId, dataJson);

	return ptr;
}

const char* EDecoder::processWshMetaDataMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::WshMetaData wshMetaDataProto;
	wshMetaDataProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->wshMetaDataProtoBuf(wshMetaDataProto);
#endif

	int reqId = wshMetaDataProto.has_reqid() ? wshMetaDataProto.reqid() : NO_VALID_ID;
	std::string dataJson = wshMetaDataProto.has_datajson() ? wshMetaDataProto.datajson() : "";

	m_pEWrapper->wshMetaData(reqId, dataJson);

	return ptr;
}

const char* EDecoder::processHistoricalSchedule(const char* ptr, const char* endPtr) {
	int reqId, sessionsCount;
	std::string startDateTime;
	std::string endDateTime;
	std::string timeZone;

	DECODE_FIELD(reqId);
	DECODE_FIELD(startDateTime);
	DECODE_FIELD(endDateTime);
	DECODE_FIELD(timeZone);

	DECODE_FIELD(sessionsCount);

	std::vector<HistoricalSession> sessions(sessionsCount);
	for (int i = 0; i < sessionsCount; i++) {
		HistoricalSession& session = sessions[i];
		DECODE_FIELD(session.startDateTime);
		DECODE_FIELD(session.endDateTime);
		DECODE_FIELD(session.refDate);
	}

	m_pEWrapper->historicalSchedule(reqId, startDateTime, endDateTime, timeZone, sessions);

	return ptr;
}

const char* EDecoder::processHistoricalScheduleMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::HistoricalSchedule historicalScheduleProto;
	historicalScheduleProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->historicalScheduleProtoBuf(historicalScheduleProto);
#endif

	int reqId = historicalScheduleProto.has_reqid() ? historicalScheduleProto.reqid() : NO_VALID_ID;
	std::string startDateTime = historicalScheduleProto.has_startdatetime() ? historicalScheduleProto.startdatetime() : "";
	std::string endDateTime = historicalScheduleProto.has_enddatetime() ? historicalScheduleProto.enddatetime() : "";
	std::string timeZone = historicalScheduleProto.has_timezone() ? historicalScheduleProto.timezone() : "";

	std::vector<HistoricalSession> historicalSessions;
	for (int i = 0; i < historicalScheduleProto.historicalsessions_size(); i++) {
		HistoricalSession historicalSession;
		const protobuf::HistoricalSession& sessionProto = historicalScheduleProto.historicalsessions(i);
		historicalSession.startDateTime = sessionProto.has_startdatetime() ? sessionProto.startdatetime() : "";
		historicalSession.endDateTime = sessionProto.has_enddatetime() ? sessionProto.enddatetime() : "";
		historicalSession.refDate = sessionProto.has_refdate() ? sessionProto.refdate() : "";
		historicalSessions.push_back(historicalSession);
	}

	m_pEWrapper->historicalSchedule(reqId, startDateTime, endDateTime, timeZone, historicalSessions);

	return ptr;
}


const char* EDecoder::processUserInfo(const char* ptr, const char* endPtr) {
    int reqId;
    std::string whiteBrandingId;

    DECODE_FIELD(reqId);
    DECODE_FIELD(whiteBrandingId);

    m_pEWrapper->userInfo(reqId, whiteBrandingId);

    return ptr;
}

const char* EDecoder::processUserInfoMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::UserInfo userInfoProto;
	userInfoProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->userInfoProtoBuf(userInfoProto);
#endif

	int reqId = userInfoProto.has_reqid() ? userInfoProto.reqid() : NO_VALID_ID;
	std::string whiteBrandingId = userInfoProto.has_whitebrandingid() ? userInfoProto.whitebrandingid() : "";

	m_pEWrapper->userInfo(reqId, whiteBrandingId);

	return ptr;
}

const char* EDecoder::processCurrentTimeInMillisMsg(const char* ptr, const char* endPtr) {
	time_t timeInMillis;

	DECODE_FIELD(timeInMillis);

	m_pEWrapper->currentTimeInMillis(timeInMillis);

	return ptr;
}

const char* EDecoder::processCurrentTimeInMillisMsgProtoBuf(const char* ptr, const char* endPtr) {
	protobuf::CurrentTimeInMillis currentTimeInMillisProto;
	currentTimeInMillisProto.ParseFromArray(ptr, endPtr - ptr);
	ptr = endPtr;

#if !defined(USE_WIN_DLL)
	m_pEWrapper->currentTimeInMillisProtoBuf(currentTimeInMillisProto);
#endif

	time_t timeInMillis = currentTimeInMillisProto.has_currenttimeinmillis() ? currentTimeInMillisProto.currenttimeinmillis() : 0;

	m_pEWrapper->currentTimeInMillis(timeInMillis);

	return ptr;
}

int EDecoder::parseAndProcessMsg(const char*& beginPtr, const char* endPtr) {
	// process a single message from the buffer;
	// return number of bytes consumed

	assert( beginPtr && beginPtr < endPtr);

	if (m_serverVersion == 0)
		return processConnectAck(beginPtr, endPtr);

	try {

		const char* ptr = beginPtr;

		int msgId;

		if (m_serverVersion >= MIN_SERVER_VER_PROTOBUF) {
			DECODE_RAW_INT(msgId);
		} else {
			DECODE_FIELD(msgId);
		}

		bool useProtoBuf = false;
		if (msgId > PROTOBUF_MSG_ID) {
			useProtoBuf = true;
			msgId -= PROTOBUF_MSG_ID;
		}

		if (useProtoBuf) {
			switch (msgId) {
			case ORDER_STATUS:
				ptr = processOrderStatusMsgProtoBuf(ptr, endPtr);
				break;

			case ERR_MSG:
				ptr = processErrorMsgProtoBuf(ptr, endPtr);
				break;

			case OPEN_ORDER:
				ptr = processOpenOrderMsgProtoBuf(ptr, endPtr);
				break;

			case EXECUTION_DATA:
				ptr = processExecutionDetailsMsgProtoBuf(ptr, endPtr);
				break;

			case OPEN_ORDER_END:
				ptr = processOpenOrderEndMsgProtoBuf(ptr, endPtr);
				break;

			case EXECUTION_DATA_END:
				ptr = processExecutionDetailsEndMsgProtoBuf(ptr, endPtr);
				break;

			case COMPLETED_ORDER:
				ptr = processCompletedOrderMsgProtoBuf(ptr, endPtr);
				break;

			case COMPLETED_ORDERS_END:
				ptr = processCompletedOrdersEndMsgProtoBuf(ptr, endPtr);
				break;

			case ORDER_BOUND:
				ptr = processOrderBoundMsgProtoBuf(ptr, endPtr);
				break;

			case CONTRACT_DATA:
				ptr = processContractDataMsgProtoBuf(ptr, endPtr);
				break;

			case BOND_CONTRACT_DATA:
				ptr = processBondContractDataMsgProtoBuf(ptr, endPtr);
				break;

			case CONTRACT_DATA_END:
				ptr = processContractDataEndMsgProtoBuf(ptr, endPtr);
				break;

			case TICK_PRICE:
				ptr = processTickPriceMsgProtoBuf(ptr, endPtr);
				break;

			case TICK_SIZE:
				ptr = processTickSizeMsgProtoBuf(ptr, endPtr);
				break;

			case MARKET_DEPTH:
				ptr = processMarketDepthMsgProtoBuf(ptr, endPtr);
				break;

			case MARKET_DEPTH_L2:
				ptr = processMarketDepthL2MsgProtoBuf(ptr, endPtr);
				break;

			case TICK_OPTION_COMPUTATION:
				ptr = processTickOptionComputationMsgProtoBuf(ptr, endPtr);
				break;

			case TICK_GENERIC:
				ptr = processTickGenericMsgProtoBuf(ptr, endPtr);
				break;

			case TICK_STRING:
				ptr = processTickStringMsgProtoBuf(ptr, endPtr);
				break;

			case TICK_SNAPSHOT_END:
				ptr = processTickSnapshotEndMsgProtoBuf(ptr, endPtr);
				break;

			case MARKET_DATA_TYPE:
				ptr = processMarketDataTypeMsgProtoBuf(ptr, endPtr);
				break;

			case TICK_REQ_PARAMS:
				ptr = processTickReqParamsMsgProtoBuf(ptr, endPtr);
				break;

			case ACCT_VALUE:
				ptr = processAccountValueMsgProtoBuf(ptr, endPtr);
				break;

			case PORTFOLIO_VALUE:
				ptr = processPortfolioValueMsgProtoBuf(ptr, endPtr);
				break;

			case ACCT_UPDATE_TIME:
				ptr = processAcctUpdateTimeMsgProtoBuf(ptr, endPtr);
				break;

			case ACCT_DOWNLOAD_END:
				ptr = processAccountDataEndMsgProtoBuf(ptr, endPtr);
				break;

			case MANAGED_ACCTS:
				ptr = processManagedAccountsMsgProtoBuf(ptr, endPtr);
				break;

			case POSITION_DATA:
				ptr = processPositionMsgProtoBuf(ptr, endPtr);
				break;

			case POSITION_END:
				ptr = processPositionEndMsgProtoBuf(ptr, endPtr);
				break;

			case ACCOUNT_SUMMARY:
				ptr = processAccountSummaryMsgProtoBuf(ptr, endPtr);
				break;

			case ACCOUNT_SUMMARY_END:
				ptr = processAccountSummaryEndMsgProtoBuf(ptr, endPtr);
				break;

			case POSITION_MULTI:
				ptr = processPositionMultiMsgProtoBuf(ptr, endPtr);
				break;

			case POSITION_MULTI_END:
				ptr = processPositionMultiEndMsgProtoBuf(ptr, endPtr);
				break;

			case ACCOUNT_UPDATE_MULTI:
				ptr = processAccountUpdateMultiMsgProtoBuf(ptr, endPtr);
				break;

			case ACCOUNT_UPDATE_MULTI_END:
				ptr = processAccountUpdateMultiEndMsgProtoBuf(ptr, endPtr);
				break;

			case HISTORICAL_DATA:
				ptr = processHistoricalDataMsgProtoBuf(ptr, endPtr);
				break;

			case HISTORICAL_DATA_UPDATE:
				ptr = processHistoricalDataUpdateMsgProtoBuf(ptr, endPtr);
				break;

			case HISTORICAL_DATA_END:
				ptr = processHistoricalDataEndMsgProtoBuf(ptr, endPtr);
				break;

			case REAL_TIME_BARS:
				ptr = processRealTimeBarsMsgProtoBuf(ptr, endPtr);
				break;

			case HEAD_TIMESTAMP:
				ptr = processHeadTimestampMsgProtoBuf(ptr, endPtr);
				break;

			case HISTOGRAM_DATA:
				ptr = processHistogramDataMsgProtoBuf(ptr, endPtr);
				break;

			case HISTORICAL_TICKS:
				ptr = processHistoricalTicksMsgProtoBuf(ptr, endPtr);
				break;

			case HISTORICAL_TICKS_BID_ASK:
				ptr = processHistoricalTicksBidAskMsgProtoBuf(ptr, endPtr);
				break;

			case HISTORICAL_TICKS_LAST:
				ptr = processHistoricalTicksLastMsgProtoBuf(ptr, endPtr);
				break;

			case TICK_BY_TICK:
				ptr = processTickByTickMsgProtoBuf(ptr, endPtr);
				break;

			case NEWS_BULLETINS:
				ptr = processNewsBulletinMsgProtoBuf(ptr, endPtr);
				break;

			case NEWS_ARTICLE:
				ptr = processNewsArticleMsgProtoBuf(ptr, endPtr);
				break;

			case NEWS_PROVIDERS:
				ptr = processNewsProvidersMsgProtoBuf(ptr, endPtr);
				break;

			case HISTORICAL_NEWS:
				ptr = processHistoricalNewsMsgProtoBuf(ptr, endPtr);
				break;

			case HISTORICAL_NEWS_END:
				ptr = processHistoricalNewsEndMsgProtoBuf(ptr, endPtr);
				break;

			case WSH_META_DATA:
				ptr = processWshMetaDataMsgProtoBuf(ptr, endPtr);
				break;

			case WSH_EVENT_DATA:
				ptr = processWshEventDataMsgProtoBuf(ptr, endPtr);
				break;

			case TICK_NEWS:
				ptr = processTickNewsMsgProtoBuf(ptr, endPtr);
				break;

			case SCANNER_PARAMETERS:
				ptr = processScannerParametersMsgProtoBuf(ptr, endPtr);
				break;

			case SCANNER_DATA:
				ptr = processScannerDataMsgProtoBuf(ptr, endPtr);
				break;

			case FUNDAMENTAL_DATA:
				ptr = processFundamentalsDataMsgProtoBuf(ptr, endPtr);
				break;

			case PNL:
				ptr = processPnLMsgProtoBuf(ptr, endPtr);
				break;

			case PNL_SINGLE:
				ptr = processPnLSingleMsgProtoBuf(ptr, endPtr);
				break;

			case RECEIVE_FA:
				ptr = processReceiveFAMsgProtoBuf(ptr, endPtr);
				break;

			case REPLACE_FA_END:
				ptr = processReplaceFAEndMsgProtoBuf(ptr, endPtr);
				break;

			case COMMISSION_AND_FEES_REPORT:
				ptr = processCommissionAndFeesReportMsgProtoBuf(ptr, endPtr);
				break;

			case HISTORICAL_SCHEDULE:
				ptr = processHistoricalScheduleMsgProtoBuf(ptr, endPtr);
				break;

			case REROUTE_MKT_DATA_REQ:
				ptr = processRerouteMktDataReqMsgProtoBuf(ptr, endPtr);
				break;

			case REROUTE_MKT_DEPTH_REQ:
				ptr = processRerouteMktDepthReqMsgProtoBuf(ptr, endPtr);
				break;

			case SECURITY_DEFINITION_OPTION_PARAMETER:
				ptr = processSecurityDefinitionOptionParameterMsgProtoBuf(ptr, endPtr);
				break;

			case SECURITY_DEFINITION_OPTION_PARAMETER_END:
				ptr = processSecurityDefinitionOptionParameterEndMsgProtoBuf(ptr, endPtr);
				break;

			case SOFT_DOLLAR_TIERS:
				ptr = processSoftDollarTiersMsgProtoBuf(ptr, endPtr);
				break;

			case FAMILY_CODES:
				ptr = processFamilyCodesMsgProtoBuf(ptr, endPtr);
				break;

			case SYMBOL_SAMPLES:
				ptr = processSymbolSamplesMsgProtoBuf(ptr, endPtr);
				break;

			case SMART_COMPONENTS:
				ptr = processSmartComponentsMsgProtoBuf(ptr, endPtr);
				break;

			case MARKET_RULE:
				ptr = processMarketRuleMsgProtoBuf(ptr, endPtr);
				break;

			case USER_INFO:
				ptr = processUserInfoMsgProtoBuf(ptr, endPtr);
				break;

			case NEXT_VALID_ID:
				ptr = processNextValidIdMsgProtoBuf(ptr, endPtr);
				break;

			case CURRENT_TIME:
				ptr = processCurrentTimeMsgProtoBuf(ptr, endPtr);
				break;

			case CURRENT_TIME_IN_MILLIS:
				ptr = processCurrentTimeInMillisMsgProtoBuf(ptr, endPtr);
				break;

			case VERIFY_MESSAGE_API:
				ptr = processVerifyMessageApiMsgProtoBuf(ptr, endPtr);
				break;

			case VERIFY_COMPLETED:
				ptr = processVerifyCompletedMsgProtoBuf(ptr, endPtr);
				break;

			case DISPLAY_GROUP_LIST:
				ptr = processDisplayGroupListMsgProtoBuf(ptr, endPtr);
				break;

			case DISPLAY_GROUP_UPDATED:
				ptr = processDisplayGroupUpdatedMsgProtoBuf(ptr, endPtr);
				break;

			case MKT_DEPTH_EXCHANGES:
				ptr = processMktDepthExchangesMsgProtoBuf(ptr, endPtr);
				break;

			default:
			{
				m_pEWrapper->error(msgId, Utils::currentTimeMillis(), UNKNOWN_ID.code(), UNKNOWN_ID.msg(), "");
				m_pEWrapper->connectionClosed();
				break;
			}
			}
		}
		else {
			switch (msgId) {
			case TICK_PRICE:
				ptr = processTickPriceMsg(ptr, endPtr);
				break;

			case TICK_SIZE:
				ptr = processTickSizeMsg(ptr, endPtr);
				break;

			case TICK_OPTION_COMPUTATION:
				ptr = processTickOptionComputationMsg(ptr, endPtr);
				break;

			case TICK_GENERIC:
				ptr = processTickGenericMsg(ptr, endPtr);
				break;

			case TICK_STRING:
				ptr = processTickStringMsg(ptr, endPtr);
				break;

			case TICK_EFP:
				ptr = processTickEfpMsg(ptr, endPtr);
				break;

			case ORDER_STATUS:
				ptr = processOrderStatusMsg(ptr, endPtr);
				break;

			case ERR_MSG:
				ptr = processErrMsgMsg(ptr, endPtr);
				break;

			case OPEN_ORDER:
				ptr = processOpenOrderMsg(ptr, endPtr);
				break;

			case ACCT_VALUE:
				ptr = processAcctValueMsg(ptr, endPtr);
				break;

			case PORTFOLIO_VALUE:
				ptr = processPortfolioValueMsg(ptr, endPtr);
				break;

			case ACCT_UPDATE_TIME:
				ptr = processAcctUpdateTimeMsg(ptr, endPtr);
				break;

			case NEXT_VALID_ID:
				ptr = processNextValidIdMsg(ptr, endPtr);
				break;

			case CONTRACT_DATA:
				ptr = processContractDataMsg(ptr, endPtr);
				break;

			case BOND_CONTRACT_DATA:
				ptr = processBondContractDataMsg(ptr, endPtr);
				break;

			case EXECUTION_DATA:
				ptr = processExecutionDetailsMsg(ptr, endPtr);
				break;

			case MARKET_DEPTH:
				ptr = processMarketDepthMsg(ptr, endPtr);
				break;

			case MARKET_DEPTH_L2:
				ptr = processMarketDepthL2Msg(ptr, endPtr);
				break;

			case NEWS_BULLETINS:
				ptr = processNewsBulletinsMsg(ptr, endPtr);
				break;

			case MANAGED_ACCTS:
				ptr = processManagedAcctsMsg(ptr, endPtr);
				break;

			case RECEIVE_FA:
				ptr = processReceiveFaMsg(ptr, endPtr);
				break;

			case HISTORICAL_DATA:
				ptr = processHistoricalDataMsg(ptr, endPtr);
				break;

			case SCANNER_DATA:
				ptr = processScannerDataMsg(ptr, endPtr);
				break;

			case SCANNER_PARAMETERS:
				ptr = processScannerParametersMsg(ptr, endPtr);
				break;

			case CURRENT_TIME:
				ptr = processCurrentTimeMsg(ptr, endPtr);
				break;

			case REAL_TIME_BARS:
				ptr = processRealTimeBarsMsg(ptr, endPtr);
				break;

			case FUNDAMENTAL_DATA:
				ptr = processFundamentalDataMsg(ptr, endPtr);
				break;

			case CONTRACT_DATA_END:
				ptr = processContractDataEndMsg(ptr, endPtr);
				break;

			case OPEN_ORDER_END:
				ptr = processOpenOrderEndMsg(ptr, endPtr);
				break;

			case ACCT_DOWNLOAD_END:
				ptr = processAcctDownloadEndMsg(ptr, endPtr);
				break;

			case EXECUTION_DATA_END:
				ptr = processExecutionDetailsEndMsg(ptr, endPtr);
				break;

			case DELTA_NEUTRAL_VALIDATION:
				ptr = processDeltaNeutralValidationMsg(ptr, endPtr);
				break;

			case TICK_SNAPSHOT_END:
				ptr = processTickSnapshotEndMsg(ptr, endPtr);
				break;

			case MARKET_DATA_TYPE:
				ptr = processMarketDataTypeMsg(ptr, endPtr);
				break;

			case COMMISSION_AND_FEES_REPORT:
				ptr = processCommissionAndFeesReportMsg(ptr, endPtr);
				break;

			case POSITION_DATA:
				ptr = processPositionDataMsg(ptr, endPtr);
				break;

			case POSITION_END:
				ptr = processPositionEndMsg(ptr, endPtr);
				break;

			case ACCOUNT_SUMMARY:
				ptr = processAccountSummaryMsg(ptr, endPtr);
				break;

			case ACCOUNT_SUMMARY_END:
				ptr = processAccountSummaryEndMsg(ptr, endPtr);
				break;

			case VERIFY_MESSAGE_API:
				ptr = processVerifyMessageApiMsg(ptr, endPtr);
				break;

			case VERIFY_COMPLETED:
				ptr = processVerifyCompletedMsg(ptr, endPtr);
				break;

			case DISPLAY_GROUP_LIST:
				ptr = processDisplayGroupListMsg(ptr, endPtr);
				break;

			case DISPLAY_GROUP_UPDATED:
				ptr = processDisplayGroupUpdatedMsg(ptr, endPtr);
				break;

			case VERIFY_AND_AUTH_MESSAGE_API:
				ptr = processVerifyAndAuthMessageApiMsg(ptr, endPtr);
				break;

			case VERIFY_AND_AUTH_COMPLETED:
				ptr = processVerifyAndAuthCompletedMsg(ptr, endPtr);
				break;

			case POSITION_MULTI:
				ptr = processPositionMultiMsg(ptr, endPtr);
				break;

			case POSITION_MULTI_END:
				ptr = processPositionMultiEndMsg(ptr, endPtr);
				break;

			case ACCOUNT_UPDATE_MULTI:
				ptr = processAccountUpdateMultiMsg(ptr, endPtr);
				break;

			case ACCOUNT_UPDATE_MULTI_END:
				ptr = processAccountUpdateMultiEndMsg(ptr, endPtr);
				break;

			case SECURITY_DEFINITION_OPTION_PARAMETER:
				ptr = processSecurityDefinitionOptionalParameterMsg(ptr, endPtr);
				break;

			case SECURITY_DEFINITION_OPTION_PARAMETER_END:
				ptr = processSecurityDefinitionOptionalParameterEndMsg(ptr, endPtr);
				break;

			case SOFT_DOLLAR_TIERS:
				ptr = processSoftDollarTiersMsg(ptr, endPtr);
				break;

			case FAMILY_CODES:
				ptr = processFamilyCodesMsg(ptr, endPtr);
				break;

			case SMART_COMPONENTS:
				ptr = processSmartComponentsMsg(ptr, endPtr);
				break;

			case TICK_REQ_PARAMS:
				ptr = processTickReqParamsMsg(ptr, endPtr);
				break;

			case SYMBOL_SAMPLES:
				ptr = processSymbolSamplesMsg(ptr, endPtr);
				break;

			case MKT_DEPTH_EXCHANGES:
				ptr = processMktDepthExchangesMsg(ptr, endPtr);
				break;

			case TICK_NEWS:
				ptr = processTickNewsMsg(ptr, endPtr);
				break;

			case NEWS_PROVIDERS:
				ptr = processNewsProvidersMsg(ptr, endPtr);
				break;

			case NEWS_ARTICLE:
				ptr = processNewsArticleMsg(ptr, endPtr);
				break;

			case HISTORICAL_NEWS:
				ptr = processHistoricalNewsMsg(ptr, endPtr);
				break;

			case HISTORICAL_NEWS_END:
				ptr = processHistoricalNewsEndMsg(ptr, endPtr);
				break;

			case HEAD_TIMESTAMP:
				ptr = processHeadTimestampMsg(ptr, endPtr);
				break;

			case HISTOGRAM_DATA:
				ptr = processHistogramDataMsg(ptr, endPtr);
				break;

			case HISTORICAL_DATA_UPDATE:
				ptr = processHistoricalDataUpdateMsg(ptr, endPtr);
				break;

			case REROUTE_MKT_DATA_REQ:
				ptr = processRerouteMktDataReqMsg(ptr, endPtr);
				break;

			case REROUTE_MKT_DEPTH_REQ:
				ptr = processRerouteMktDepthReqMsg(ptr, endPtr);
				break;

			case MARKET_RULE:
				ptr = processMarketRuleMsg(ptr, endPtr);
				break;

			case PNL:
				ptr = processPnLMsg(ptr, endPtr);
				break;

			case PNL_SINGLE:
				ptr = processPnLSingleMsg(ptr, endPtr);
				break;

			case HISTORICAL_TICKS:
				ptr = processHistoricalTicks(ptr, endPtr);
				break;

			case HISTORICAL_TICKS_BID_ASK:
				ptr = processHistoricalTicksBidAsk(ptr, endPtr);
				break;

			case HISTORICAL_TICKS_LAST:
				ptr = processHistoricalTicksLast(ptr, endPtr);
				break;

			case TICK_BY_TICK:
				ptr = processTickByTickDataMsg(ptr, endPtr);
				break;

			case ORDER_BOUND:
				ptr = processOrderBoundMsg(ptr, endPtr);
				break;

			case COMPLETED_ORDER:
				ptr = processCompletedOrderMsg(ptr, endPtr);
				break;

			case COMPLETED_ORDERS_END:
				ptr = processCompletedOrdersEndMsg(ptr, endPtr);
				break;

			case REPLACE_FA_END:
				ptr = processReplaceFAEndMsg(ptr, endPtr);
				break;

			case WSH_META_DATA:
				ptr = processWshMetaData(ptr, endPtr);
				break;

			case WSH_EVENT_DATA:
				ptr = processWshEventData(ptr, endPtr);
				break;

			case HISTORICAL_SCHEDULE:
				ptr = processHistoricalSchedule(ptr, endPtr);
				break;

			case USER_INFO:
				ptr = processUserInfo(ptr, endPtr);
				break;

			case HISTORICAL_DATA_END:
				ptr = processHistoricalDataEndMsg(ptr, endPtr);
				break;

			case CURRENT_TIME_IN_MILLIS:
				ptr = processCurrentTimeInMillisMsg(ptr, endPtr);
				break;

			default:
			{
				m_pEWrapper->error(msgId, Utils::currentTimeMillis(), UNKNOWN_ID.code(), UNKNOWN_ID.msg(), "");
				m_pEWrapper->connectionClosed();
				break;
			}
			}
		}

		if (!ptr)
			return 0;

		int processed = ptr - beginPtr;
		beginPtr = ptr;
		return processed;
	}
	catch(const std::exception& e) {
		m_pEWrapper->error( NO_VALID_ID, Utils::currentTimeMillis(), BAD_MESSAGE.code(), BAD_MESSAGE.msg() + e.what(), "");
	}
	return 0;
}


bool EDecoder::CheckOffset(const char* ptr, const char* endPtr)
{
	assert (ptr && ptr <= endPtr);
	return (ptr && ptr < endPtr);
}

const char* EDecoder::FindFieldEnd(const char* ptr, const char* endPtr)
{
	return (const char*)memchr(ptr, 0, endPtr - ptr);
}

bool EDecoder::DecodeField(bool& boolValue, const char*& ptr, const char* endPtr)
{
	int intValue;
	if( !DecodeField(intValue, ptr, endPtr))
		return false;
	boolValue = (intValue > 0);
	return true;
}

bool EDecoder::DecodeRawInt(int& intValue, const char*& ptr, const char* endPtr)
{
	if (!CheckOffset(ptr, endPtr))
		return false;

	char arrayOfBytes[RAW_INT_LEN] = { 0 };
	memcpy(arrayOfBytes, ptr, RAW_INT_LEN);
	std::reverse(arrayOfBytes, arrayOfBytes + RAW_INT_LEN);
	memcpy(&intValue, arrayOfBytes, RAW_INT_LEN);
	ptr += RAW_INT_LEN;
	return true;
}

bool EDecoder::DecodeField(int& intValue, const char*& ptr, const char* endPtr)
{
	if( !CheckOffset(ptr, endPtr))
		return false;
	const char* fieldBeg = ptr;
	const char* fieldEnd = FindFieldEnd(fieldBeg, endPtr);
	if( !fieldEnd)
		return false;
	intValue = atoi(fieldBeg);
	ptr = ++fieldEnd;
	return true;
}

bool EDecoder::DecodeFieldTime(time_t& time_tValue, const char*& ptr, const char* endPtr)
{
	if( !CheckOffset(ptr, endPtr))
		return false;
	const char* fieldBeg = ptr;
	const char* fieldEnd = FindFieldEnd(fieldBeg, endPtr);
	if( !fieldEnd)
		return false;
	time_tValue = atoll(fieldBeg);
	ptr = ++fieldEnd;
	return true;
}

bool EDecoder::DecodeField(long long& longLongValue, const char*& ptr, const char* endPtr)
{
	if( !CheckOffset(ptr, endPtr))
		return false;
	const char* fieldBeg = ptr;
	const char* fieldEnd = FindFieldEnd(fieldBeg, endPtr);
	if( !fieldEnd)
		return false;
	longLongValue = atoll(fieldBeg);
	ptr = ++fieldEnd;
	return true;
}

bool EDecoder::DecodeField(long& longValue, const char*& ptr, const char* endPtr)
{
	if( !CheckOffset(ptr, endPtr))
		return false;
	const char* fieldBeg = ptr;
	const char* fieldEnd = FindFieldEnd(fieldBeg, endPtr);
	if( !fieldEnd)
		return false;
	longValue = atol(fieldBeg);
	ptr = ++fieldEnd;
	return true;
}

bool EDecoder::DecodeField(double& doubleValue, const char*& ptr, const char* endPtr)
{
	if( !CheckOffset(ptr, endPtr))
		return false;
	const char* fieldBeg = ptr;
	const char* fieldEnd = FindFieldEnd(fieldBeg, endPtr);
	if( !fieldEnd)
		return false;
	doubleValue = fieldBeg == INFINITY_STR ? INFINITY : atof(fieldBeg);
	ptr = ++fieldEnd;
	return true;
}

bool EDecoder::DecodeField(std::string& stringValue,
						   const char*& ptr, const char* endPtr)
{
	if( !CheckOffset(ptr, endPtr))
		return false;
	const char* fieldBeg = ptr;
	const char* fieldEnd = FindFieldEnd(ptr, endPtr);
	if( !fieldEnd)
		return false;
	stringValue = fieldBeg; // better way?
	ptr = ++fieldEnd;
	return true;
}

bool EDecoder::DecodeField(char& charValue,
						   const char*& ptr, const char* endPtr)
{
	if( !CheckOffset(ptr, endPtr))
		return false;
	const char* fieldBeg = ptr;
	const char* fieldEnd = FindFieldEnd(ptr, endPtr);
	if( !fieldEnd)
		return false;
	charValue = fieldBeg[0]; // better way?
	ptr = ++fieldEnd;
	return true;
}

bool EDecoder::DecodeField(Decimal& decimalValue, const char*& ptr, const char* endPtr)
{
	if (!CheckOffset(ptr, endPtr))
		return false;
	const char* fieldBeg = ptr;
	const char* fieldEnd = FindFieldEnd(fieldBeg, endPtr);
	if (!fieldEnd)
		return false;
	decimalValue = DecimalFunctions::stringToDecimal(fieldBeg);
	ptr = ++fieldEnd;
	return true;
}

bool EDecoder::DecodeFieldMax(int& intValue, const char*& ptr, const char* endPtr)
{
	std::string stringValue;
	if( !DecodeField(stringValue, ptr, endPtr))
		return false;
	intValue = stringValue.empty() ? UNSET_INTEGER : atoi(stringValue.c_str());
	return true;
}

bool EDecoder::DecodeFieldMax(long& longValue, const char*& ptr, const char* endPtr)
{
	int intValue;
	if( !DecodeFieldMax(intValue, ptr, endPtr))
		return false;
	longValue = intValue;
	return true;
}

bool EDecoder::DecodeFieldMax(double& doubleValue, const char*& ptr, const char* endPtr)
{
	std::string stringValue;
	if( !DecodeField(stringValue, ptr, endPtr))
		return false;
	doubleValue = stringValue.empty() ? UNSET_DOUBLE : atof(stringValue.c_str());
	return true;
}

const char* EDecoder::decodeLastTradeDate(const char* ptr, const char* endPtr, ContractDetails& contract, bool isBond) {
	std::string lastTradeDateOrContractMonth;
	DECODE_FIELD(lastTradeDateOrContractMonth);
	EDecoderUtils::setLastTradeDate(lastTradeDateOrContractMonth, contract, isBond);
	return ptr;
}

