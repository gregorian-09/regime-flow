/* Copyright (C) 2025 Interactive Brokers LLC. All rights reserved. This code is subject to the terms
 * and conditions of the IB API Non-Commercial License or the IB API Commercial License, as applicable. */

#include <string>
#include <sstream>
#include "StdAfx.h"
#include "Utils.h"
#include "EDecoderUtils.h"
#include "EDecoder.h"

Contract EDecoderUtils::decodeContract(const protobuf::Contract& contractProto) {
    Contract contract;
    if (contractProto.has_conid()) contract.conId = contractProto.conid();
    if (contractProto.has_symbol()) contract.symbol = contractProto.symbol();
    if (contractProto.has_sectype()) contract.secType = contractProto.sectype();
    if (contractProto.has_lasttradedateorcontractmonth()) contract.lastTradeDateOrContractMonth = contractProto.lasttradedateorcontractmonth();
    if (contractProto.has_strike()) contract.strike = contractProto.strike();
    if (contractProto.has_right()) contract.right = contractProto.right();
    if (contractProto.has_multiplier()) contract.multiplier = std::to_string(contractProto.multiplier());
    if (contractProto.has_exchange()) contract.exchange = contractProto.exchange();
    if (contractProto.has_currency()) contract.currency = contractProto.currency();
    if (contractProto.has_localsymbol()) contract.localSymbol = contractProto.localsymbol();
    if (contractProto.has_tradingclass()) contract.tradingClass = contractProto.tradingclass();
    if (contractProto.has_combolegsdescrip()) contract.comboLegsDescrip = contractProto.combolegsdescrip();

    Contract::ComboLegListSPtr comboLegs = decodeComboLegs(contractProto);
    if (!comboLegs->empty()) contract.comboLegs = comboLegs;

    DeltaNeutralContract* deltaNeutralContract = decodeDeltaNeutralContract(contractProto);
    if (deltaNeutralContract != NULL) contract.deltaNeutralContract = deltaNeutralContract;

    if (contractProto.has_lasttradedate()) contract.lastTradeDate = contractProto.lasttradedate();
    if (contractProto.has_primaryexch()) contract.primaryExchange = contractProto.primaryexch();
    if (contractProto.has_issuerid()) contract.issuerId = contractProto.issuerid();
    if (contractProto.has_description()) contract.description = contractProto.description();

    return contract;
}

Contract::ComboLegListSPtr EDecoderUtils::decodeComboLegs(const protobuf::Contract& contractProto) {
    Contract::ComboLegListSPtr comboLegs(new Contract::ComboLegList);
    int comboLegsCount = contractProto.combolegs_size();
    if (comboLegsCount > 0) {
        comboLegs->reserve(comboLegsCount);

        for (int i = 0; i < comboLegsCount; ++i) {
            protobuf::ComboLeg comboLegProto = contractProto.combolegs().at(i);
            ComboLegSPtr comboLeg(new ComboLeg());
            if (comboLegProto.has_conid()) comboLeg->conId = comboLegProto.conid();
            if (comboLegProto.has_ratio()) comboLeg->ratio = comboLegProto.ratio();
            if (comboLegProto.has_action()) comboLeg->action = comboLegProto.action();
            if (comboLegProto.has_exchange()) comboLeg->exchange = comboLegProto.exchange();
            if (comboLegProto.has_openclose()) comboLeg->openClose = comboLegProto.openclose();
            if (comboLegProto.has_shortsalesslot()) comboLeg->shortSaleSlot = comboLegProto.shortsalesslot();
            if (comboLegProto.has_designatedlocation()) comboLeg->designatedLocation = comboLegProto.designatedlocation();
            if (comboLegProto.has_exemptcode()) comboLeg->exemptCode = comboLegProto.exemptcode();
            comboLegs->push_back(comboLeg);
        }
    }
    return comboLegs;
}

Order::OrderComboLegListSPtr EDecoderUtils::decodeOrderComboLegs(const protobuf::Contract& contractProto) {
    Order::OrderComboLegListSPtr orderComboLegs(new Order::OrderComboLegList);
    int comboLegsCount = contractProto.combolegs_size();
    if (comboLegsCount > 0) {
        orderComboLegs->reserve(comboLegsCount);

        for (int i = 0; i < comboLegsCount; ++i) {
            protobuf::ComboLeg comboLegProto = contractProto.combolegs().at(i);
            OrderComboLegSPtr orderComboLeg(new OrderComboLeg());
            if (comboLegProto.has_perlegprice()) orderComboLeg->price = comboLegProto.perlegprice();
            orderComboLegs->push_back(orderComboLeg);
        }
    }
    return orderComboLegs;
}

DeltaNeutralContract* EDecoderUtils::decodeDeltaNeutralContract(const protobuf::Contract& contractProto) {
    DeltaNeutralContract* deltaNeutralContract = NULL;
    if (contractProto.has_deltaneutralcontract()) {
        deltaNeutralContract = new DeltaNeutralContract();
        protobuf::DeltaNeutralContract deltaNeutralContractProto = contractProto.deltaneutralcontract();
        if (deltaNeutralContractProto.has_conid()) deltaNeutralContract->conId = deltaNeutralContractProto.conid();
        if (deltaNeutralContractProto.has_delta()) deltaNeutralContract->delta = deltaNeutralContractProto.delta();
        if (deltaNeutralContractProto.has_price()) deltaNeutralContract->price = deltaNeutralContractProto.price();
    }
    return deltaNeutralContract;
}

Execution EDecoderUtils::decodeExecution(const protobuf::Execution& executionProto) {
    Execution execution;
    if (executionProto.has_orderid()) execution.orderId = executionProto.orderid();
    if (executionProto.has_clientid()) execution.clientId = executionProto.clientid();
    if (executionProto.has_execid()) execution.execId = executionProto.execid();
    if (executionProto.has_time()) execution.time = executionProto.time();
    if (executionProto.has_acctnumber()) execution.acctNumber = executionProto.acctnumber();
    if (executionProto.has_exchange()) execution.exchange = executionProto.exchange();
    if (executionProto.has_side()) execution.side = executionProto.side();
    if (executionProto.has_shares()) execution.shares = DecimalFunctions::stringToDecimal(executionProto.shares());
    if (executionProto.has_price()) execution.price = executionProto.price();
    if (executionProto.has_permid()) execution.permId = executionProto.permid();
    if (executionProto.has_isliquidation()) execution.liquidation = executionProto.isliquidation() ? 1 : 0;
    if (executionProto.has_cumqty()) execution.cumQty = DecimalFunctions::stringToDecimal(executionProto.cumqty());
    if (executionProto.has_avgprice()) execution.avgPrice = executionProto.avgprice();
    if (executionProto.has_orderref()) execution.orderRef = executionProto.orderref();
    if (executionProto.has_evrule()) execution.evRule = executionProto.evrule();
    if (executionProto.has_evmultiplier()) execution.evMultiplier = executionProto.evmultiplier();
    if (executionProto.has_modelcode()) execution.modelCode = executionProto.modelcode();
    if (executionProto.has_lastliquidity()) execution.lastLiquidity = executionProto.lastliquidity();
    if (executionProto.has_ispricerevisionpending()) execution.pendingPriceRevision = executionProto.ispricerevisionpending();
    if (executionProto.has_submitter()) execution.submitter = executionProto.submitter();
    if (executionProto.has_optexerciseorlapsetype()) execution.optExerciseOrLapseType = Utils::getOptionExerciseType(executionProto.optexerciseorlapsetype());
    return execution;
}

Order EDecoderUtils::decodeOrder(int orderId, const protobuf::Contract& contractProto, const protobuf::Order& orderProto) {
    Order order;
    if (Utils::isValidValue(orderId)) order.orderId = orderId;
    if (orderProto.has_orderid()) order.orderId = orderProto.orderid();
    if (orderProto.has_action()) order.action = orderProto.action();
    if (orderProto.has_totalquantity()) order.totalQuantity = DecimalFunctions::stringToDecimal(orderProto.totalquantity());
    if (orderProto.has_ordertype()) order.orderType = orderProto.ordertype();
    if (orderProto.has_lmtprice()) order.lmtPrice = orderProto.lmtprice();
    if (orderProto.has_auxprice()) order.auxPrice = orderProto.auxprice();
    if (orderProto.has_tif()) order.tif = orderProto.tif();
    if (orderProto.has_ocagroup()) order.ocaGroup = orderProto.ocagroup();
    if (orderProto.has_account()) order.account = orderProto.account();
    if (orderProto.has_openclose()) order.openClose = orderProto.openclose();
    if (orderProto.has_origin()) order.origin = (Origin)orderProto.origin();
    if (orderProto.has_orderref()) order.orderRef = orderProto.orderref();
    if (orderProto.has_clientid()) order.clientId = orderProto.clientid();
    if (orderProto.has_permid()) order.permId = orderProto.permid();
    if (orderProto.has_outsiderth()) order.outsideRth = orderProto.outsiderth();
    if (orderProto.has_hidden()) order.hidden = orderProto.hidden();
    if (orderProto.has_discretionaryamt()) order.discretionaryAmt = orderProto.discretionaryamt();
    if (orderProto.has_goodaftertime()) order.goodAfterTime = orderProto.goodaftertime();
    if (orderProto.has_fagroup()) order.faGroup = orderProto.fagroup();
    if (orderProto.has_famethod()) order.faMethod = orderProto.famethod();
    if (orderProto.has_fapercentage()) order.faPercentage = orderProto.fapercentage();
    if (orderProto.has_modelcode()) order.modelCode = orderProto.modelcode();
    if (orderProto.has_goodtilldate()) order.goodTillDate = orderProto.goodtilldate();
    if (orderProto.has_rule80a()) order.rule80A = orderProto.rule80a();
    if (orderProto.has_percentoffset()) order.percentOffset = orderProto.percentoffset();
    if (orderProto.has_settlingfirm()) order.settlingFirm = orderProto.settlingfirm();
    if (orderProto.has_shortsaleslot()) order.shortSaleSlot = orderProto.shortsaleslot();
    if (orderProto.has_designatedlocation()) order.designatedLocation = orderProto.designatedlocation();
    if (orderProto.has_exemptcode()) order.exemptCode = orderProto.exemptcode();
    if (orderProto.has_startingprice()) order.startingPrice = orderProto.startingprice();
    if (orderProto.has_stockrefprice()) order.stockRefPrice = orderProto.stockrefprice();
    if (orderProto.has_delta()) order.delta = orderProto.delta();
    if (orderProto.has_stockrangelower()) order.stockRangeLower = orderProto.stockrangelower();
    if (orderProto.has_stockrangeupper()) order.stockRangeUpper = orderProto.stockrangeupper();
    if (orderProto.has_displaysize()) order.displaySize = orderProto.displaysize();
    if (orderProto.has_blockorder()) order.blockOrder = orderProto.blockorder();
    if (orderProto.has_sweeptofill()) order.sweepToFill = orderProto.sweeptofill();
    if (orderProto.has_allornone()) order.allOrNone = orderProto.allornone();
    if (orderProto.has_minqty()) order.minQty = orderProto.minqty();
    if (orderProto.has_ocatype()) order.ocaType = orderProto.ocatype();
    if (orderProto.has_parentid()) order.parentId = orderProto.parentid();
    if (orderProto.has_triggermethod()) order.triggerMethod = orderProto.triggermethod();
    if (orderProto.has_volatility()) order.volatility = orderProto.volatility();
    if (orderProto.has_volatilitytype()) order.volatilityType = orderProto.volatilitytype();
    if (orderProto.has_deltaneutralordertype()) order.deltaNeutralOrderType = orderProto.deltaneutralordertype();
    if (orderProto.has_deltaneutralauxprice()) order.deltaNeutralAuxPrice = orderProto.deltaneutralauxprice();
    if (orderProto.has_deltaneutralconid()) order.deltaNeutralConId = orderProto.deltaneutralconid();
    if (orderProto.has_deltaneutralsettlingfirm()) order.deltaNeutralSettlingFirm = orderProto.deltaneutralsettlingfirm();
    if (orderProto.has_deltaneutralclearingaccount()) order.deltaNeutralClearingAccount = orderProto.deltaneutralclearingaccount();
    if (orderProto.has_deltaneutralclearingintent()) order.deltaNeutralClearingIntent = orderProto.deltaneutralclearingintent();
    if (orderProto.has_deltaneutralopenclose()) order.deltaNeutralOpenClose = orderProto.deltaneutralopenclose();
    if (orderProto.has_deltaneutralshortsale()) order.deltaNeutralShortSale = orderProto.deltaneutralshortsale();
    if (orderProto.has_deltaneutralshortsaleslot()) order.deltaNeutralShortSaleSlot = orderProto.deltaneutralshortsaleslot();
    if (orderProto.has_deltaneutraldesignatedlocation()) order.deltaNeutralDesignatedLocation = orderProto.deltaneutraldesignatedlocation();
    if (orderProto.has_continuousupdate()) order.continuousUpdate = orderProto.continuousupdate();
    if (orderProto.has_referencepricetype()) order.referencePriceType = orderProto.referencepricetype();
    if (orderProto.has_trailstopprice()) order.trailStopPrice = orderProto.trailstopprice();
    if (orderProto.has_trailingpercent()) order.trailingPercent = orderProto.trailingpercent();

    Order::OrderComboLegListSPtr orderComboLegs = decodeOrderComboLegs(contractProto);
    if (!orderComboLegs->empty()) order.orderComboLegs = orderComboLegs;

    TagValueListSPtr tagValueList = decodeTagValueList(orderProto.smartcomboroutingparams());
    if (tagValueList->size() > 0) order.smartComboRoutingParams = tagValueList;

    if (orderProto.has_scaleinitlevelsize()) order.scaleInitLevelSize = orderProto.scaleinitlevelsize();
    if (orderProto.has_scalesubslevelsize()) order.scaleSubsLevelSize = orderProto.scalesubslevelsize();
    if (orderProto.has_scalepriceincrement()) order.scalePriceIncrement = orderProto.scalepriceincrement();
    if (orderProto.has_scalepriceadjustvalue()) order.scalePriceAdjustValue = orderProto.scalepriceadjustvalue();
    if (orderProto.has_scalepriceadjustinterval()) order.scalePriceAdjustInterval = orderProto.scalepriceadjustinterval();
    if (orderProto.has_scaleprofitoffset()) order.scaleProfitOffset = orderProto.scaleprofitoffset();
    if (orderProto.has_scaleautoreset()) order.scaleAutoReset = orderProto.scaleautoreset();
    if (orderProto.has_scaleinitposition()) order.scaleInitPosition = orderProto.scaleinitposition();
    if (orderProto.has_scaleinitfillqty()) order.scaleInitFillQty = orderProto.scaleinitfillqty();
    if (orderProto.has_scalerandompercent()) order.scaleRandomPercent = orderProto.scalerandompercent();
    if (orderProto.has_hedgetype()) order.hedgeType = orderProto.hedgetype();
    if (orderProto.has_hedgetype() && orderProto.has_hedgeparam() && !Utils::stringIsEmpty(orderProto.hedgetype())) order.hedgeParam = orderProto.hedgeparam();
    if (orderProto.has_optoutsmartrouting()) order.optOutSmartRouting = orderProto.optoutsmartrouting();
    if (orderProto.has_clearingaccount()) order.clearingAccount = orderProto.clearingaccount();
    if (orderProto.has_clearingintent()) order.clearingIntent = orderProto.clearingintent();
    if (orderProto.has_notheld()) order.notHeld = orderProto.notheld();

    if (orderProto.has_algostrategy()) {
        order.algoStrategy = orderProto.algostrategy();
        if (orderProto.algoparams_size() > 0) {
            TagValueListSPtr tagValueList = decodeTagValueList(orderProto.algoparams());
            if (tagValueList->size() > 0) order.algoParams = tagValueList;
        }
    }

    if (orderProto.has_solicited()) order.solicited = orderProto.solicited();
    if (orderProto.has_whatif()) order.whatIf = orderProto.whatif();
    if (orderProto.has_randomizesize()) order.randomizeSize = orderProto.randomizesize();
    if (orderProto.has_randomizeprice()) order.randomizePrice = orderProto.randomizeprice();
    if (orderProto.has_referencecontractid()) order.referenceContractId = orderProto.referencecontractid();
    if (orderProto.has_ispeggedchangeamountdecrease()) order.isPeggedChangeAmountDecrease = orderProto.ispeggedchangeamountdecrease();
    if (orderProto.has_peggedchangeamount()) order.peggedChangeAmount = orderProto.peggedchangeamount();
    if (orderProto.has_referencechangeamount()) order.referenceChangeAmount = orderProto.referencechangeamount();
    if (orderProto.has_referenceexchangeid()) order.referenceExchangeId = orderProto.referenceexchangeid();

    std::vector<std::shared_ptr<OrderCondition>> conditions = decodeConditions(orderProto);
    if (conditions.size() > 0) order.conditions = conditions;
    if (orderProto.has_conditionsignorerth()) order.conditionsIgnoreRth = orderProto.conditionsignorerth();
    if (orderProto.has_conditionscancelorder()) order.conditionsCancelOrder = orderProto.conditionscancelorder();

    if (orderProto.has_adjustedordertype()) order.adjustedOrderType = orderProto.adjustedordertype();
    if (orderProto.has_triggerprice()) order.triggerPrice = orderProto.triggerprice();
    if (orderProto.has_lmtpriceoffset()) order.lmtPriceOffset = orderProto.lmtpriceoffset();
    if (orderProto.has_adjustedstopprice()) order.adjustedStopPrice = orderProto.adjustedstopprice();
    if (orderProto.has_adjustedstoplimitprice()) order.adjustedStopLimitPrice = orderProto.adjustedstoplimitprice();
    if (orderProto.has_adjustedtrailingamount()) order.adjustedTrailingAmount = orderProto.adjustedtrailingamount();
    if (orderProto.has_adjustabletrailingunit()) order.adjustableTrailingUnit = orderProto.adjustabletrailingunit();

    SoftDollarTier softDollarTier = decodeSoftDollarTier(orderProto);
    order.softDollarTier = softDollarTier;

    if (orderProto.has_cashqty()) order.cashQty = orderProto.cashqty();
    if (orderProto.has_dontuseautopriceforhedge()) order.dontUseAutoPriceForHedge = orderProto.dontuseautopriceforhedge();
    if (orderProto.has_isomscontainer()) order.isOmsContainer = orderProto.isomscontainer();
    if (orderProto.has_discretionaryuptolimitprice()) order.discretionaryUpToLimitPrice = orderProto.discretionaryuptolimitprice();
    if (orderProto.has_usepricemgmtalgo()) order.usePriceMgmtAlgo = orderProto.usepricemgmtalgo() != 0 ? UsePriceMmgtAlgo::USE : UsePriceMmgtAlgo::DONT_USE;
    if (orderProto.has_duration()) order.duration = orderProto.duration();
    if (orderProto.has_posttoats()) order.postToAts = orderProto.posttoats();
    if (orderProto.has_autocancelparent()) order.autoCancelParent = orderProto.autocancelparent();
    if (orderProto.has_mintradeqty()) order.minTradeQty = orderProto.mintradeqty();
    if (orderProto.has_mincompetesize()) order.minCompeteSize = orderProto.mincompetesize();
    if (orderProto.has_competeagainstbestoffset()) order.competeAgainstBestOffset = orderProto.competeagainstbestoffset();
    if (orderProto.has_midoffsetatwhole()) order.midOffsetAtWhole = orderProto.midoffsetatwhole();
    if (orderProto.has_midoffsetathalf()) order.midOffsetAtHalf = orderProto.midoffsetathalf();
    if (orderProto.has_customeraccount()) order.customerAccount = orderProto.customeraccount();
    if (orderProto.has_professionalcustomer()) order.professionalCustomer = orderProto.professionalcustomer();
    if (orderProto.has_bondaccruedinterest()) order.bondAccruedInterest = orderProto.bondaccruedinterest();
    if (orderProto.has_includeovernight()) order.includeOvernight = orderProto.includeovernight();
    if (orderProto.has_extoperator()) order.extOperator = orderProto.extoperator();
    if (orderProto.has_manualorderindicator()) order.manualOrderIndicator = orderProto.manualorderindicator();
    if (orderProto.has_submitter()) order.submitter = orderProto.submitter();
    if (orderProto.has_imbalanceonly()) order.imbalanceOnly = orderProto.imbalanceonly();
    if (orderProto.has_autocanceldate()) order.autoCancelDate = orderProto.autocanceldate();
    if (orderProto.has_filledquantity()) order.filledQuantity = DecimalFunctions::stringToDecimal(orderProto.filledquantity());
    if (orderProto.has_reffuturesconid()) order.refFuturesConId = orderProto.reffuturesconid();
    if (orderProto.has_shareholder()) order.shareholder = orderProto.shareholder();
    if (orderProto.has_routemarketabletobbo()) order.routeMarketableToBbo = orderProto.routemarketabletobbo() != 0 ? ThreeStateBoolean::STATE_YES : ThreeStateBoolean::STATE_NO;
    if (orderProto.has_parentpermid()) order.parentPermId = orderProto.parentpermid();
    if (orderProto.has_postonly()) order.postOnly = orderProto.postonly();
    if (orderProto.has_allowpreopen()) order.allowPreOpen = orderProto.allowpreopen();
    if (orderProto.has_ignoreopenauction()) order.ignoreOpenAuction = orderProto.ignoreopenauction();
    if (orderProto.has_deactivate()) order.deactivate = orderProto.deactivate();
    if (orderProto.has_activestarttime()) order.activeStartTime = orderProto.activestarttime();
    if (orderProto.has_activestoptime()) order.activeStopTime = orderProto.activestoptime();
    if (orderProto.has_seekpriceimprovement()) order.seekPriceImprovement = orderProto.seekpriceimprovement() != 0 ? ThreeStateBoolean::STATE_YES : ThreeStateBoolean::STATE_NO;
    if (orderProto.has_whatiftype()) order.whatIfType = orderProto.whatiftype();

    return order;
}

std::vector<std::shared_ptr<OrderCondition>> EDecoderUtils::decodeConditions(const protobuf::Order& order) {
    std::vector<std::shared_ptr<OrderCondition>> orderConditions;
    if (order.conditions_size() > 0) {
        for (protobuf::OrderCondition orderConditionProto : order.conditions()) {
            OrderCondition::OrderConditionType conditionType = (OrderCondition::OrderConditionType)(orderConditionProto.has_type() ? orderConditionProto.type() : 0);

            OrderCondition* orderCondition = NULL;
            switch (conditionType) {
            case OrderCondition::OrderConditionType::Price:
                orderCondition = createPriceCondition(orderConditionProto);
                break;
            case OrderCondition::OrderConditionType::Time:
                orderCondition = createTimeCondition(orderConditionProto);
                break;
            case OrderCondition::OrderConditionType::Margin:
                orderCondition = createMarginCondition(orderConditionProto);
                break;
            case OrderCondition::OrderConditionType::Execution:
                orderCondition = createExecutionCondition(orderConditionProto);
                break;
            case OrderCondition::OrderConditionType::Volume:
                orderCondition = createVolumeCondition(orderConditionProto);
                break;
            case OrderCondition::OrderConditionType::PercentChange:
                orderCondition = createPercentChangeCondition(orderConditionProto);
                break;
            }
            if (orderCondition != NULL) orderConditions.push_back(std::shared_ptr<OrderCondition>(orderCondition));
        }
    }
    return orderConditions;
}

void EDecoderUtils::setConditionFields(const protobuf::OrderCondition& orderConditionProto, OrderCondition& orderCondition) {
    if (orderConditionProto.has_isconjunctionconnection()) orderCondition.conjunctionConnection(orderConditionProto.isconjunctionconnection());
}

void EDecoderUtils::setOperatorConditionFields(const protobuf::OrderCondition& orderConditionProto, OperatorCondition& operatorCondition) {
    setConditionFields(orderConditionProto, operatorCondition);
    if (orderConditionProto.has_ismore()) operatorCondition.isMore(orderConditionProto.ismore());
}

void EDecoderUtils::setContractConditionFields(const protobuf::OrderCondition& orderConditionProto, ContractCondition& contractCondition) {
    setOperatorConditionFields(orderConditionProto, contractCondition);
    if (orderConditionProto.has_conid()) contractCondition.conId(orderConditionProto.conid());
    if (orderConditionProto.has_exchange()) contractCondition.exchange(orderConditionProto.exchange());
}

PriceCondition* EDecoderUtils::createPriceCondition(const protobuf::OrderCondition& orderConditionProto) {
    PriceCondition* priceCondition = dynamic_cast<PriceCondition*>(OrderCondition::create(OrderCondition::OrderConditionType::Price));
    setContractConditionFields(orderConditionProto, *priceCondition);
    if (orderConditionProto.has_price()) priceCondition->price(orderConditionProto.price());
    if (orderConditionProto.has_triggermethod()) priceCondition->triggerMethod(orderConditionProto.triggermethod());
    return priceCondition;
}

TimeCondition* EDecoderUtils::createTimeCondition(const protobuf::OrderCondition& orderConditionProto) {
    TimeCondition* timeCondition = dynamic_cast<TimeCondition*>(OrderCondition::create(OrderCondition::OrderConditionType::Time));
    setOperatorConditionFields(orderConditionProto, *timeCondition);
    if (orderConditionProto.has_time()) timeCondition->time(orderConditionProto.time());
    return timeCondition;
}

MarginCondition* EDecoderUtils::createMarginCondition(const protobuf::OrderCondition& orderConditionProto) {
    MarginCondition* marginCondition = dynamic_cast<MarginCondition*>(OrderCondition::create(OrderCondition::OrderConditionType::Margin));
    setOperatorConditionFields(orderConditionProto, *marginCondition);
    if (orderConditionProto.has_percent()) marginCondition->percent(orderConditionProto.percent());
    return marginCondition;
}

ExecutionCondition* EDecoderUtils::createExecutionCondition(const protobuf::OrderCondition& orderConditionProto) {
    ExecutionCondition* executionCondition = dynamic_cast<ExecutionCondition*>(OrderCondition::create(OrderCondition::OrderConditionType::Execution));
    setConditionFields(orderConditionProto, *executionCondition);
    if (orderConditionProto.has_sectype()) executionCondition->secType(orderConditionProto.sectype());
    if (orderConditionProto.has_exchange()) executionCondition->exchange(orderConditionProto.exchange());
    if (orderConditionProto.has_symbol()) executionCondition->symbol(orderConditionProto.symbol());
    return executionCondition;
}

VolumeCondition* EDecoderUtils::createVolumeCondition(const protobuf::OrderCondition& orderConditionProto) {
    VolumeCondition* volumeCondition = dynamic_cast<VolumeCondition*>(OrderCondition::create(OrderCondition::OrderConditionType::Volume));
    setContractConditionFields(orderConditionProto, *volumeCondition);
    if (orderConditionProto.has_volume()) volumeCondition->volume(orderConditionProto.volume());
    return volumeCondition;
}

PercentChangeCondition* EDecoderUtils::createPercentChangeCondition(const protobuf::OrderCondition& orderConditionProto) {
    PercentChangeCondition* percentChangeCondition = dynamic_cast<PercentChangeCondition*>(OrderCondition::create(OrderCondition::OrderConditionType::PercentChange));
    setContractConditionFields(orderConditionProto, *percentChangeCondition);
    if (orderConditionProto.has_changepercent()) percentChangeCondition->changePercent(orderConditionProto.changepercent());
    return percentChangeCondition;
}

SoftDollarTier EDecoderUtils::decodeSoftDollarTier(const protobuf::Order& order) {
    if (order.has_softdollartier()) {
        return decodeSoftDollarTier(order.softdollartier());
    }
    return SoftDollarTier();
}

SoftDollarTier EDecoderUtils::decodeSoftDollarTier(const protobuf::SoftDollarTier& softDollarTierProto) {
    SoftDollarTier softDollarTier;
    std::string name = softDollarTierProto.has_name() ? softDollarTierProto.name() : "";
    std::string value = softDollarTierProto.has_value() ? softDollarTierProto.value() : "";
    std::string displayName = softDollarTierProto.has_displayname() ? softDollarTierProto.displayname() : "";
    softDollarTier = SoftDollarTier(name, value, displayName);
    return softDollarTier;
}

TagValueListSPtr EDecoderUtils::decodeTagValueList(google::protobuf::Map<std::string, std::string> stringStringMap) {
    TagValueListSPtr params(new TagValueList);
    int paramsCount = stringStringMap.size();
    if (paramsCount > 0) {
        params->reserve(paramsCount);

        for (auto const& p : stringStringMap)
        {
            TagValueSPtr tagValue(new TagValue(p.first, p.second));
            params->push_back(tagValue);
        }
    }
    return params;
}

OrderState EDecoderUtils::decodeOrderState(const protobuf::OrderState& orderStateProto) {
    OrderState orderState;
    if (orderStateProto.has_status()) orderState.status = orderStateProto.status();
    if (orderStateProto.has_initmarginbefore()) orderState.initMarginBefore = std::to_string(orderStateProto.initmarginbefore());
    if (orderStateProto.has_maintmarginbefore()) orderState.maintMarginBefore = std::to_string(orderStateProto.maintmarginbefore());
    if (orderStateProto.has_equitywithloanbefore()) orderState.equityWithLoanBefore = std::to_string(orderStateProto.equitywithloanbefore());
    if (orderStateProto.has_initmarginchange()) orderState.initMarginChange = std::to_string(orderStateProto.initmarginchange());
    if (orderStateProto.has_maintmarginchange()) orderState.maintMarginChange = std::to_string(orderStateProto.maintmarginchange());
    if (orderStateProto.has_equitywithloanchange()) orderState.equityWithLoanChange = std::to_string(orderStateProto.equitywithloanchange());
    if (orderStateProto.has_initmarginafter()) orderState.initMarginAfter = std::to_string(orderStateProto.initmarginafter());
    if (orderStateProto.has_maintmarginafter()) orderState.maintMarginAfter = std::to_string(orderStateProto.maintmarginafter());
    if (orderStateProto.has_equitywithloanafter()) orderState.equityWithLoanAfter = std::to_string(orderStateProto.equitywithloanafter());
    if (orderStateProto.has_commissionandfees()) orderState.commissionAndFees = orderStateProto.commissionandfees();
    if (orderStateProto.has_mincommissionandfees()) orderState.minCommissionAndFees = orderStateProto.mincommissionandfees();
    if (orderStateProto.has_maxcommissionandfees()) orderState.maxCommissionAndFees = orderStateProto.maxcommissionandfees();
    if (orderStateProto.has_commissionandfeescurrency()) orderState.commissionAndFeesCurrency = orderStateProto.commissionandfeescurrency();
    if (orderStateProto.has_warningtext()) orderState.warningText = orderStateProto.warningtext();
    if (orderStateProto.has_margincurrency()) orderState.marginCurrency = orderStateProto.margincurrency();
    if (orderStateProto.has_initmarginbeforeoutsiderth()) orderState.initMarginBeforeOutsideRTH = orderStateProto.initmarginbeforeoutsiderth();
    if (orderStateProto.has_maintmarginbeforeoutsiderth()) orderState.maintMarginBeforeOutsideRTH = orderStateProto.maintmarginbeforeoutsiderth();
    if (orderStateProto.has_equitywithloanbeforeoutsiderth()) orderState.equityWithLoanBeforeOutsideRTH = orderStateProto.equitywithloanbeforeoutsiderth();
    if (orderStateProto.has_initmarginchangeoutsiderth()) orderState.initMarginChangeOutsideRTH = orderStateProto.initmarginchangeoutsiderth();
    if (orderStateProto.has_maintmarginchangeoutsiderth()) orderState.maintMarginChangeOutsideRTH = orderStateProto.maintmarginchangeoutsiderth();
    if (orderStateProto.has_equitywithloanchangeoutsiderth()) orderState.equityWithLoanChangeOutsideRTH = orderStateProto.equitywithloanchangeoutsiderth();
    if (orderStateProto.has_initmarginafteroutsiderth()) orderState.initMarginAfterOutsideRTH = orderStateProto.initmarginafteroutsiderth();
    if (orderStateProto.has_maintmarginafteroutsiderth()) orderState.maintMarginAfterOutsideRTH = orderStateProto.maintmarginafteroutsiderth();
    if (orderStateProto.has_equitywithloanafteroutsiderth()) orderState.equityWithLoanAfterOutsideRTH = orderStateProto.equitywithloanafteroutsiderth();
    if (orderStateProto.has_suggestedsize()) orderState.suggestedSize = DecimalFunctions::stringToDecimal(orderStateProto.suggestedsize());
    if (orderStateProto.has_rejectreason()) orderState.rejectReason = orderStateProto.rejectreason();

    OrderAllocationListSPtr orderAllocations = decodeOrderAllocations(orderStateProto);
    if (!orderAllocations->empty()) orderState.orderAllocations = orderAllocations;

    if (orderStateProto.has_completedtime()) orderState.completedTime = orderStateProto.completedtime();
    if (orderStateProto.has_completedstatus()) orderState.completedStatus = orderStateProto.completedstatus();

    return orderState;
}

OrderAllocationListSPtr EDecoderUtils::decodeOrderAllocations(const protobuf::OrderState& orderStateProto) {
    OrderAllocationListSPtr orderAllocations(new OrderAllocationList);
    int orderAllocationsCount = orderStateProto.orderallocations_size();
    if (orderAllocationsCount > 0) {
        orderAllocations->reserve(orderAllocationsCount);

        for (int i = 0; i < orderAllocationsCount; ++i) {
            protobuf::OrderAllocation orderAllocationProto = orderStateProto.orderallocations().at(i);
            OrderAllocationSPtr orderAllocation(new OrderAllocation());

            if (orderAllocationProto.has_account()) orderAllocation->account = orderAllocationProto.account();
            if (orderAllocationProto.has_position()) orderAllocation->position = DecimalFunctions::stringToDecimal(orderAllocationProto.position());
            if (orderAllocationProto.has_positiondesired()) orderAllocation->positionDesired = DecimalFunctions::stringToDecimal(orderAllocationProto.positiondesired());
            if (orderAllocationProto.has_positionafter()) orderAllocation->positionAfter = DecimalFunctions::stringToDecimal(orderAllocationProto.positionafter());
            if (orderAllocationProto.has_desiredallocqty()) orderAllocation->desiredAllocQty = DecimalFunctions::stringToDecimal(orderAllocationProto.desiredallocqty());
            if (orderAllocationProto.has_allowedallocqty()) orderAllocation->allowedAllocQty = DecimalFunctions::stringToDecimal(orderAllocationProto.allowedallocqty());
            if (orderAllocationProto.has_ismonetary()) orderAllocation->isMonetary = orderAllocationProto.ismonetary();

            orderAllocations->push_back(orderAllocation);
        }
    }
    return orderAllocations;
}

ContractDetails EDecoderUtils::decodeContractDetails(const protobuf::Contract& contractProto, const protobuf::ContractDetails& contractDetailsProto, bool isBond) {
    ContractDetails contractDetails;
    Contract contract = decodeContract(contractProto);
    contractDetails.contract = contract;

    if (contractDetailsProto.has_marketname()) contractDetails.marketName = contractDetailsProto.marketname();
    if (contractDetailsProto.has_mintick()) contractDetails.minTick = atof(contractDetailsProto.mintick().c_str());
    if (contractDetailsProto.has_pricemagnifier()) contractDetails.priceMagnifier = contractDetailsProto.pricemagnifier();
    if (contractDetailsProto.has_ordertypes()) contractDetails.orderTypes = contractDetailsProto.ordertypes();
    if (contractDetailsProto.has_validexchanges()) contractDetails.validExchanges = contractDetailsProto.validexchanges();
    if (contractDetailsProto.has_underconid()) contractDetails.underConId = contractDetailsProto.underconid();
    if (contractDetailsProto.has_longname()) contractDetails.longName = contractDetailsProto.longname();
    if (contractDetailsProto.has_contractmonth()) contractDetails.contractMonth = contractDetailsProto.contractmonth();
    if (contractDetailsProto.has_industry()) contractDetails.industry = contractDetailsProto.industry();
    if (contractDetailsProto.has_category()) contractDetails.category = contractDetailsProto.category();
    if (contractDetailsProto.has_subcategory()) contractDetails.subcategory = contractDetailsProto.subcategory();
    if (contractDetailsProto.has_timezoneid()) contractDetails.timeZoneId = contractDetailsProto.timezoneid();
    if (contractDetailsProto.has_tradinghours()) contractDetails.tradingHours = contractDetailsProto.tradinghours();
    if (contractDetailsProto.has_liquidhours()) contractDetails.liquidHours = contractDetailsProto.liquidhours();
    if (contractDetailsProto.has_evrule()) contractDetails.evRule = contractDetailsProto.evrule();
    if (contractDetailsProto.has_evmultiplier()) contractDetails.evMultiplier = contractDetailsProto.evmultiplier();

    TagValueListSPtr tagValueList = decodeTagValueList(contractDetailsProto.secidlist());
    if (tagValueList->size() > 0) contractDetails.secIdList = tagValueList;

    if (contractDetailsProto.has_agggroup()) contractDetails.aggGroup = contractDetailsProto.agggroup();
    if (contractDetailsProto.has_undersymbol()) contractDetails.underSymbol = contractDetailsProto.undersymbol();
    if (contractDetailsProto.has_undersectype()) contractDetails.underSecType = contractDetailsProto.undersectype();
    if (contractDetailsProto.has_marketruleids()) contractDetails.marketRuleIds = contractDetailsProto.marketruleids();
    if (contractDetailsProto.has_realexpirationdate()) contractDetails.realExpirationDate = contractDetailsProto.realexpirationdate();
    if (contractDetailsProto.has_stocktype()) contractDetails.stockType = contractDetailsProto.stocktype();
    if (contractDetailsProto.has_minsize()) contractDetails.minSize = DecimalFunctions::stringToDecimal(contractDetailsProto.minsize());
    if (contractDetailsProto.has_sizeincrement()) contractDetails.sizeIncrement = DecimalFunctions::stringToDecimal(contractDetailsProto.sizeincrement());
    if (contractDetailsProto.has_suggestedsizeincrement()) contractDetails.suggestedSizeIncrement = DecimalFunctions::stringToDecimal(contractDetailsProto.suggestedsizeincrement());
    if (contractDetailsProto.has_minalgosize()) contractDetails.minAlgoSize = DecimalFunctions::stringToDecimal(contractDetailsProto.minalgosize());

    setLastTradeDate(contract.lastTradeDateOrContractMonth, contractDetails, isBond);

    if (contractDetailsProto.has_cusip()) contractDetails.cusip = contractDetailsProto.cusip();
    if (contractDetailsProto.has_ratings()) contractDetails.ratings = contractDetailsProto.ratings();
    if (contractDetailsProto.has_descappend()) contractDetails.descAppend = contractDetailsProto.descappend();
    if (contractDetailsProto.has_bondtype()) contractDetails.bondType = contractDetailsProto.bondtype();
    if (contractDetailsProto.has_coupon()) contractDetails.coupon = contractDetailsProto.coupon();
    if (contractDetailsProto.has_coupontype()) contractDetails.couponType = contractDetailsProto.coupontype();
    if (contractDetailsProto.has_callable()) contractDetails.callable = contractDetailsProto.callable();
    if (contractDetailsProto.has_puttable()) contractDetails.putable = contractDetailsProto.puttable();
    if (contractDetailsProto.has_convertible()) contractDetails.convertible = contractDetailsProto.convertible();
    if (contractDetailsProto.has_issuedate()) contractDetails.issueDate = contractDetailsProto.issuedate();
    if (contractDetailsProto.has_nextoptiondate()) contractDetails.nextOptionDate = contractDetailsProto.nextoptiondate();
    if (contractDetailsProto.has_nextoptiontype()) contractDetails.nextOptionType = contractDetailsProto.nextoptiontype();
    if (contractDetailsProto.has_nextoptionpartial()) contractDetails.nextOptionPartial = contractDetailsProto.nextoptionpartial();
    if (contractDetailsProto.has_bondnotes()) contractDetails.notes = contractDetailsProto.bondnotes();

    if (contractDetailsProto.has_fundname()) contractDetails.fundName = contractDetailsProto.fundname();
    if (contractDetailsProto.has_fundfamily()) contractDetails.fundFamily = contractDetailsProto.fundfamily();
    if (contractDetailsProto.has_fundtype()) contractDetails.fundType = contractDetailsProto.fundtype();
    if (contractDetailsProto.has_fundfrontload()) contractDetails.fundFrontLoad = contractDetailsProto.fundfrontload();
    if (contractDetailsProto.has_fundbackload()) contractDetails.fundBackLoad = contractDetailsProto.fundbackload();
    if (contractDetailsProto.has_fundbackloadtimeinterval()) contractDetails.fundBackLoadTimeInterval = contractDetailsProto.fundbackloadtimeinterval();
    if (contractDetailsProto.has_fundmanagementfee()) contractDetails.fundManagementFee = contractDetailsProto.fundmanagementfee();
    if (contractDetailsProto.has_fundclosed()) contractDetails.fundClosed = contractDetailsProto.fundclosed();
    if (contractDetailsProto.has_fundclosedfornewinvestors()) contractDetails.fundClosedForNewInvestors = contractDetailsProto.fundclosedfornewinvestors();
    if (contractDetailsProto.has_fundclosedfornewmoney()) contractDetails.fundClosedForNewMoney = contractDetailsProto.fundclosedfornewmoney();
    if (contractDetailsProto.has_fundnotifyamount()) contractDetails.fundNotifyAmount = contractDetailsProto.fundnotifyamount();
    if (contractDetailsProto.has_fundminimuminitialpurchase()) contractDetails.fundMinimumInitialPurchase = contractDetailsProto.fundminimuminitialpurchase();
    if (contractDetailsProto.has_fundminimumsubsequentpurchase()) contractDetails.fundSubsequentMinimumPurchase = contractDetailsProto.fundminimumsubsequentpurchase();
    if (contractDetailsProto.has_fundblueskystates()) contractDetails.fundBlueSkyStates = contractDetailsProto.fundblueskystates();
    if (contractDetailsProto.has_fundblueskyterritories()) contractDetails.fundBlueSkyTerritories = contractDetailsProto.fundblueskyterritories();

    if (contractDetailsProto.has_funddistributionpolicyindicator()) contractDetails.fundDistributionPolicyIndicator = Utils::getFundDistributionPolicyIndicator(contractDetailsProto.funddistributionpolicyindicator());
    if (contractDetailsProto.has_fundassettype()) contractDetails.fundAssetType = Utils::getFundAssetType(contractDetailsProto.fundassettype());

    IneligibilityReasonListSPtr ineligibilityReasonList = decodeIneligibilityReasonList(contractDetailsProto);
    if (!ineligibilityReasonList->empty()) contractDetails.ineligibilityReasonList = ineligibilityReasonList;

    if (contractDetailsProto.has_eventcontract1()) contractDetails.eventContract1 = contractDetailsProto.eventcontract1();
    if (contractDetailsProto.has_eventcontractdescription1()) contractDetails.eventContractDescription1 = contractDetailsProto.eventcontractdescription1();
    if (contractDetailsProto.has_eventcontractdescription2()) contractDetails.eventContractDescription2 = contractDetailsProto.eventcontractdescription2();

    return contractDetails;
}

IneligibilityReasonListSPtr EDecoderUtils::decodeIneligibilityReasonList(protobuf::ContractDetails contractDetailsProto) {

    IneligibilityReasonListSPtr ineligibilityReasonList(new IneligibilityReasonList);
    int ineligibilityReasonListCount = contractDetailsProto.ineligibilityreasonlist_size();
    if (ineligibilityReasonListCount > 0) {
        ineligibilityReasonList->reserve(ineligibilityReasonListCount);

        for (int i = 0; i < ineligibilityReasonListCount; ++i) {
            protobuf::IneligibilityReason ineligibilityReasonProto = contractDetailsProto.ineligibilityreasonlist().at(i);
            IneligibilityReasonSPtr ineligibilityReason(new IneligibilityReason());
            if (ineligibilityReasonProto.has_id()) ineligibilityReason->id = ineligibilityReasonProto.id();
            if (ineligibilityReasonProto.has_description()) ineligibilityReason->description = ineligibilityReasonProto.description();
            ineligibilityReasonList->push_back(ineligibilityReason);
        }
    }

    return ineligibilityReasonList;
}

void EDecoderUtils::setLastTradeDate(std::string lastTradeDateOrContractMonth, ContractDetails& contract, bool isBond) {
    if (!lastTradeDateOrContractMonth.empty()) {
        char split_with = ' ';
        if (lastTradeDateOrContractMonth.find("-") != std::string::npos) {
            split_with = '-';
        }
        std::vector<std::string> split;
        std::istringstream buf(lastTradeDateOrContractMonth);
        std::string s;
        while (getline(buf, s, split_with)) {
            split.push_back(s);
        }
        if (split.size() > 0) {
            if (isBond) {
                contract.maturity = split[0];
            }
            else {
                contract.contract.lastTradeDateOrContractMonth = split[0];
            }
        }
        if (split.size() > 1) {
            contract.lastTradeTime = split[1];
        }
        if (isBond && split.size() > 2) {
            contract.timeZoneId = split[2];
        }
    }
}

HistoricalTick EDecoderUtils::decodeHistoricalTick(const protobuf::HistoricalTick& historicalTickProto) {
    HistoricalTick historicalTick{};
    if (historicalTickProto.has_time()) historicalTick.time = historicalTickProto.time();
    if (historicalTickProto.has_price()) historicalTick.price = historicalTickProto.price();
    if (historicalTickProto.has_size()) historicalTick.size = DecimalFunctions::stringToDecimal(historicalTickProto.size());
    return historicalTick;
}

HistoricalTickBidAsk EDecoderUtils::decodeHistoricalTickBidAsk(const protobuf::HistoricalTickBidAsk& historicalTickBidAskProto) {
    HistoricalTickBidAsk historicalTickBidAsk{};
    if (historicalTickBidAskProto.has_time()) historicalTickBidAsk.time = historicalTickBidAskProto.time();
    if (historicalTickBidAskProto.has_tickattribbidask()) {
        const protobuf::TickAttribBidAsk& tickAttribProto = historicalTickBidAskProto.tickattribbidask();
        if (tickAttribProto.has_askpasthigh()) historicalTickBidAsk.tickAttribBidAsk.askPastHigh = tickAttribProto.askpasthigh();
        if (tickAttribProto.has_bidpastlow()) historicalTickBidAsk.tickAttribBidAsk.bidPastLow = tickAttribProto.bidpastlow();
    }
    if (historicalTickBidAskProto.has_pricebid()) historicalTickBidAsk.priceBid = historicalTickBidAskProto.pricebid();
    if (historicalTickBidAskProto.has_priceask()) historicalTickBidAsk.priceAsk = historicalTickBidAskProto.priceask();
    if (historicalTickBidAskProto.has_sizebid()) historicalTickBidAsk.sizeBid = DecimalFunctions::stringToDecimal(historicalTickBidAskProto.sizebid());
    if (historicalTickBidAskProto.has_sizeask()) historicalTickBidAsk.sizeAsk = DecimalFunctions::stringToDecimal(historicalTickBidAskProto.sizeask());
    return historicalTickBidAsk;
}

HistoricalTickLast EDecoderUtils::decodeHistoricalTickLast(const protobuf::HistoricalTickLast& historicalTickLastProto) {
    HistoricalTickLast historicalTickLast{};
    if (historicalTickLastProto.has_time()) historicalTickLast.time = historicalTickLastProto.time();
    if (historicalTickLastProto.has_tickattriblast()) {
        const protobuf::TickAttribLast& tickAttribProto = historicalTickLastProto.tickattriblast();
        if (tickAttribProto.has_pastlimit()) historicalTickLast.tickAttribLast.pastLimit = tickAttribProto.pastlimit();
        if (tickAttribProto.has_unreported()) historicalTickLast.tickAttribLast.unreported = tickAttribProto.unreported();
    }
    if (historicalTickLastProto.has_price()) historicalTickLast.price = historicalTickLastProto.price();
    if (historicalTickLastProto.has_size()) historicalTickLast.size = DecimalFunctions::stringToDecimal(historicalTickLastProto.size());
    if (historicalTickLastProto.has_exchange()) historicalTickLast.exchange = historicalTickLastProto.exchange();
    if (historicalTickLastProto.has_specialconditions()) historicalTickLast.specialConditions = historicalTickLastProto.specialconditions();
    return historicalTickLast;
}

HistogramEntry EDecoderUtils::decodeHistogramDataEntry(const protobuf::HistogramDataEntry& histogramDataEntryProto) {
    HistogramEntry histogramEntry{};
    if (histogramDataEntryProto.has_price()) histogramEntry.price = histogramDataEntryProto.price();
    if (histogramDataEntryProto.has_size()) histogramEntry.size = DecimalFunctions::stringToDecimal(histogramDataEntryProto.size());
    return histogramEntry;
}

Bar EDecoderUtils::decodeHistoricalDataBar(const protobuf::HistoricalDataBar& historicalDataBarProto) {
    Bar bar;
    if (historicalDataBarProto.has_date()) bar.time = historicalDataBarProto.date() ;
    if (historicalDataBarProto.has_open()) bar.open = historicalDataBarProto.open();
    if (historicalDataBarProto.has_high()) bar.high = historicalDataBarProto.high();
    if (historicalDataBarProto.has_low()) bar.low = historicalDataBarProto.low();
    if (historicalDataBarProto.has_close()) bar.close = historicalDataBarProto.close();
    if (historicalDataBarProto.has_volume()) bar.volume = DecimalFunctions::stringToDecimal(historicalDataBarProto.volume());
    if (historicalDataBarProto.has_barcount()) bar.count = historicalDataBarProto.barcount();
    if (historicalDataBarProto.has_wap()) bar .wap = DecimalFunctions::stringToDecimal(historicalDataBarProto.wap());
    return bar;
}

FamilyCode EDecoderUtils::decodeFamilyCode(const protobuf::FamilyCode& familyCodeProto) {
    FamilyCode familyCode;
    familyCode.accountID = familyCodeProto.has_accountid() ? familyCodeProto.accountid() : "";
    familyCode.familyCodeStr = familyCodeProto.has_familycode() ? familyCodeProto.familycode() : "";
    return familyCode;
}

SmartComponentsMap EDecoderUtils::decodeSmartComponents(const protobuf::SmartComponents& smartComponentsProto) {
    SmartComponentsMap theMap;
    int smartComponentsCount = smartComponentsProto.smartcomponents_size();
    if (smartComponentsCount > 0) {
        for (int i = 0; i < smartComponentsCount; ++i) {
            const protobuf::SmartComponent& smartComponentProto = smartComponentsProto.smartcomponents(i);
            int bitNumber = smartComponentProto.has_bitnumber() ? smartComponentProto.bitnumber() : 0;
            std::string exchange = smartComponentProto.has_exchange() ? smartComponentProto.exchange() : "";
            char exchangeLetter = smartComponentProto.has_exchangeletter() && !smartComponentProto.exchangeletter().empty() ? smartComponentProto.exchangeletter()[0] : ' ';
            theMap[bitNumber] = std::make_tuple(exchange, exchangeLetter);
        }
    }
    return theMap;
}

PriceIncrement EDecoderUtils::decodePriceIncrement(const protobuf::PriceIncrement& priceIncrementProto) {
    PriceIncrement priceIncrement;
    priceIncrement.lowEdge = priceIncrementProto.has_lowedge() ? priceIncrementProto.lowedge() : 0.0;
    priceIncrement.increment = priceIncrementProto.has_increment() ? priceIncrementProto.increment() : 0.0;
    return priceIncrement;
}

DepthMktDataDescription EDecoderUtils::decodeDepthMarketDataDescription(const protobuf::DepthMarketDataDescription& depthMarketDataDescriptionProto) {
    DepthMktDataDescription depthMktDataDescription;
    if (depthMarketDataDescriptionProto.has_exchange()) depthMktDataDescription.exchange = depthMarketDataDescriptionProto.exchange();
    if (depthMarketDataDescriptionProto.has_sectype()) depthMktDataDescription.secType = depthMarketDataDescriptionProto.sectype();
    if (depthMarketDataDescriptionProto.has_listingexch()) depthMktDataDescription.listingExch = depthMarketDataDescriptionProto.listingexch();
    if (depthMarketDataDescriptionProto.has_servicedatatype()) depthMktDataDescription.serviceDataType = depthMarketDataDescriptionProto.servicedatatype();
    if (depthMarketDataDescriptionProto.has_agggroup()) depthMktDataDescription.aggGroup = depthMarketDataDescriptionProto.agggroup();
    return depthMktDataDescription;
}
