/* Copyright (C) 2025 Interactive Brokers LLC. All rights reserved. This code is subject to the terms
 * and conditions of the IB API Non-Commercial License or the IB API Commercial License, as applicable. */

#include "StdAfx.h"
#include "EClientUtils.h"
#include "Utils.h"
#include "OperatorCondition.h"
#include "ContractCondition.h"
#include "PriceCondition.h"
#include "TimeCondition.h"
#include "MarginCondition.h"
#include "ExecutionCondition.h"
#include "VolumeCondition.h"
#include "PercentChangeCondition.h"
#include "TwsSocketClientErrors.h"
#include "EClientException.h"

protobuf::ExecutionRequest EClientUtils::createExecutionRequestProto(int reqId, const ExecutionFilter& filter) {
    protobuf::ExecutionRequest executionRequestProto;
    executionRequestProto.set_reqid(reqId);

    protobuf::ExecutionFilter* executionFilterProto = executionRequestProto.mutable_executionfilter();
    if (Utils::isValidValue(filter.m_clientId)) executionFilterProto->set_clientid(filter.m_clientId);
    if (!Utils::stringIsEmpty(filter.m_acctCode)) executionFilterProto->set_acctcode(filter.m_acctCode);
    if (!Utils::stringIsEmpty(filter.m_time)) executionFilterProto->set_time(filter.m_time);
    if (!Utils::stringIsEmpty(filter.m_symbol)) executionFilterProto->set_symbol(filter.m_symbol);
    if (!Utils::stringIsEmpty(filter.m_secType)) executionFilterProto->set_sectype(filter.m_secType);
    if (!Utils::stringIsEmpty(filter.m_exchange)) executionFilterProto->set_exchange(filter.m_exchange);
    if (!Utils::stringIsEmpty(filter.m_side)) executionFilterProto->set_side(filter.m_side);
    if (Utils::isValidValue(filter.m_lastNDays)) executionFilterProto->set_lastndays(filter.m_lastNDays);
    if (!filter.m_specificDates.empty()) {
        for (long specificDate : filter.m_specificDates) {
            executionFilterProto->add_specificdates(specificDate);
        }
    }
    return executionRequestProto;
}

protobuf::PlaceOrderRequest EClientUtils::createPlaceOrderRequestProto(OrderId orderId, const Contract& contract, const Order& order) {
    protobuf::PlaceOrderRequest placeOrderRequestProto;
    if (Utils::isValidValue(orderId)) placeOrderRequestProto.set_orderid(orderId);
    placeOrderRequestProto.mutable_contract()->CopyFrom(createContractProto(contract, order));
    placeOrderRequestProto.mutable_order()->CopyFrom(createOrderProto(order));
    placeOrderRequestProto.mutable_attachedorders()->CopyFrom(createAttachedOrdersProto(order));
    return placeOrderRequestProto;
}

protobuf::AttachedOrders EClientUtils::createAttachedOrdersProto(const Order& order) {
    protobuf::AttachedOrders attachedOrdersProto;
    if (Utils::isValidValue(order.slOrderId)) attachedOrdersProto.set_slorderid(order.slOrderId);
    if (!Utils::stringIsEmpty(order.slOrderType)) attachedOrdersProto.set_slordertype(order.slOrderType.c_str());
    if (Utils::isValidValue(order.ptOrderId)) attachedOrdersProto.set_ptorderid(order.ptOrderId);
    if (!Utils::stringIsEmpty(order.ptOrderType)) attachedOrdersProto.set_ptordertype(order.ptOrderType.c_str());
    return attachedOrdersProto;
}

protobuf::Order EClientUtils::createOrderProto(const Order& order) {
    protobuf::Order orderProto;
    if (Utils::isValidValue(order.clientId)) orderProto.set_clientid(order.clientId);
    if (Utils::isValidValue(order.permId)) orderProto.set_permid(order.permId);
    if (Utils::isValidValue(order.parentId)) orderProto.set_parentid(order.parentId);
    if (!Utils::stringIsEmpty(order.action)) orderProto.set_action(order.action.c_str());
    if (Utils::isValidValue(order.totalQuantity)) orderProto.set_totalquantity(DecimalFunctions::decimalStringToDisplay(order.totalQuantity));
    if (Utils::isValidValue(order.displaySize)) orderProto.set_displaysize(order.displaySize);
    if (!Utils::stringIsEmpty(order.orderType)) orderProto.set_ordertype(order.orderType);
    if (Utils::isValidValue(order.lmtPrice)) orderProto.set_lmtprice(order.lmtPrice);
    if (Utils::isValidValue(order.auxPrice)) orderProto.set_auxprice(order.auxPrice);
    if (!Utils::stringIsEmpty(order.tif)) orderProto.set_tif(order.tif);
    if (!Utils::stringIsEmpty(order.account)) orderProto.set_account(order.account);
    if (!Utils::stringIsEmpty(order.settlingFirm)) orderProto.set_settlingfirm(order.settlingFirm);
    if (!Utils::stringIsEmpty(order.clearingAccount)) orderProto.set_clearingaccount(order.clearingAccount);
    if (!Utils::stringIsEmpty(order.clearingIntent)) orderProto.set_clearingintent(order.clearingIntent);
    if (order.allOrNone) orderProto.set_allornone(order.allOrNone);
    if (order.blockOrder) orderProto.set_blockorder(order.blockOrder);
    if (order.hidden) orderProto.set_hidden(order.hidden);
    if (order.outsideRth) orderProto.set_outsiderth(order.outsideRth);
    if (order.sweepToFill) orderProto.set_sweeptofill(order.sweepToFill);
    if (Utils::isValidValue(order.percentOffset)) orderProto.set_percentoffset(order.percentOffset);
    if (Utils::isValidValue(order.trailingPercent)) orderProto.set_trailingpercent(order.trailingPercent);
    if (Utils::isValidValue(order.trailStopPrice)) orderProto.set_trailstopprice(order.trailStopPrice);
    if (Utils::isValidValue(order.minQty)) orderProto.set_minqty(order.minQty);
    if (!Utils::stringIsEmpty(order.goodAfterTime)) orderProto.set_goodaftertime(order.goodAfterTime);
    if (!Utils::stringIsEmpty(order.goodTillDate)) orderProto.set_goodtilldate(order.goodTillDate);
    if (!Utils::stringIsEmpty(order.ocaGroup)) orderProto.set_ocagroup(order.ocaGroup);
    if (!Utils::stringIsEmpty(order.orderRef)) orderProto.set_orderref(order.orderRef);
    if (!Utils::stringIsEmpty(order.rule80A)) orderProto.set_rule80a(order.rule80A);
    if (Utils::isValidValue(order.ocaType)) orderProto.set_ocatype(order.ocaType);
    if (Utils::isValidValue(order.triggerMethod)) orderProto.set_triggermethod(order.triggerMethod);
    if (!Utils::stringIsEmpty(order.activeStartTime)) orderProto.set_activestarttime(order.activeStartTime);
    if (!Utils::stringIsEmpty(order.activeStopTime)) orderProto.set_activestoptime(order.activeStopTime);
    if (!Utils::stringIsEmpty(order.faGroup)) orderProto.set_fagroup(order.faGroup);
    if (!Utils::stringIsEmpty(order.faMethod)) orderProto.set_famethod(order.faMethod);
    if (!Utils::stringIsEmpty(order.faPercentage)) orderProto.set_fapercentage(order.faPercentage);
    if (Utils::isValidValue(order.volatility))  orderProto.set_volatility(order.volatility);
    if (Utils::isValidValue(order.volatilityType)) orderProto.set_volatilitytype(order.volatilityType);
    if (Utils::isValidValue(order.continuousUpdate)) orderProto.set_continuousupdate(order.continuousUpdate);
    if (Utils::isValidValue(order.referencePriceType)) orderProto.set_referencepricetype(order.referencePriceType);
    if (!Utils::stringIsEmpty(order.deltaNeutralOrderType)) orderProto.set_deltaneutralordertype(order.deltaNeutralOrderType);
    if (Utils::isValidValue(order.deltaNeutralAuxPrice)) orderProto.set_deltaneutralauxprice(order.deltaNeutralAuxPrice);
    if (Utils::isValidValue(order.deltaNeutralConId)) orderProto.set_deltaneutralconid(order.deltaNeutralConId);
    if (!Utils::stringIsEmpty(order.deltaNeutralOpenClose)) orderProto.set_deltaneutralopenclose(order.deltaNeutralOpenClose);
    if (order.deltaNeutralShortSale) orderProto.set_deltaneutralshortsale(order.deltaNeutralShortSale);
    if (Utils::isValidValue(order.deltaNeutralShortSaleSlot)) orderProto.set_deltaneutralshortsaleslot(order.deltaNeutralShortSaleSlot);
    if (!Utils::stringIsEmpty(order.deltaNeutralDesignatedLocation)) orderProto.set_deltaneutraldesignatedlocation(order.deltaNeutralDesignatedLocation);
    if (Utils::isValidValue(order.scaleInitLevelSize)) orderProto.set_scaleinitlevelsize(order.scaleInitLevelSize);
    if (Utils::isValidValue(order.scaleSubsLevelSize)) orderProto.set_scalesubslevelsize(order.scaleSubsLevelSize);
    if (Utils::isValidValue(order.scalePriceIncrement)) orderProto.set_scalepriceincrement(order.scalePriceIncrement);
    if (Utils::isValidValue(order.scalePriceAdjustValue)) orderProto.set_scalepriceadjustvalue(order.scalePriceAdjustValue);
    if (Utils::isValidValue(order.scalePriceAdjustInterval)) orderProto.set_scalepriceadjustinterval(order.scalePriceAdjustInterval);
    if (Utils::isValidValue(order.scaleProfitOffset)) orderProto.set_scaleprofitoffset(order.scaleProfitOffset);
    if (order.scaleAutoReset) orderProto.set_scaleautoreset(order.scaleAutoReset);
    if (Utils::isValidValue(order.scaleInitPosition)) orderProto.set_scaleinitposition(order.scaleInitPosition);
    if (Utils::isValidValue(order.scaleInitFillQty)) orderProto.set_scaleinitfillqty(order.scaleInitFillQty);
    if (order.scaleRandomPercent) orderProto.set_scalerandompercent(order.scaleRandomPercent);
    if (!Utils::stringIsEmpty(order.scaleTable)) orderProto.set_scaletable(order.scaleTable);
    if (!Utils::stringIsEmpty(order.hedgeType)) orderProto.set_hedgetype(order.hedgeType);
    if (!Utils::stringIsEmpty(order.hedgeParam)) orderProto.set_hedgeparam(order.hedgeParam);

    if (!Utils::stringIsEmpty(order.algoStrategy)) {
        orderProto.set_algostrategy(order.algoStrategy);

        std::map<std::string, std::string> algoParamsMap = createStringStringMap(order.algoParams);
        for (std::pair<std::string, std::string> algoParam : algoParamsMap) {
            (*orderProto.mutable_algoparams())[algoParam.first] = algoParam.second;
        }
    }
    if (!Utils::stringIsEmpty(order.algoId)) orderProto.set_algoid(order.algoId);

    std::map<std::string, std::string> smartComboRoutingParamsMap = createStringStringMap(order.smartComboRoutingParams);
    for (std::pair<std::string, std::string> smartComboRoutingParam : smartComboRoutingParamsMap) {
        (*orderProto.mutable_smartcomboroutingparams())[smartComboRoutingParam.first] = smartComboRoutingParam.second;
    }

    if (order.whatIf) orderProto.set_whatif(order.whatIf);
    if (order.transmit) orderProto.set_transmit(order.transmit);
    if (order.overridePercentageConstraints) orderProto.set_overridepercentageconstraints(order.overridePercentageConstraints);
    if (!Utils::stringIsEmpty(order.openClose)) orderProto.set_openclose(order.openClose);
    if (Utils::isValidValue(order.origin)) orderProto.set_origin(order.origin);
    if (Utils::isValidValue(order.shortSaleSlot)) orderProto.set_shortsaleslot(order.shortSaleSlot);
    if (!Utils::stringIsEmpty(order.designatedLocation)) orderProto.set_designatedlocation(order.designatedLocation);
    if (Utils::isValidValue(order.exemptCode)) orderProto.set_exemptcode(order.exemptCode);
    if (!Utils::stringIsEmpty(order.deltaNeutralSettlingFirm)) orderProto.set_deltaneutralsettlingfirm(order.deltaNeutralSettlingFirm);
    if (!Utils::stringIsEmpty(order.deltaNeutralClearingAccount)) orderProto.set_deltaneutralclearingaccount(order.deltaNeutralClearingAccount);
    if (!Utils::stringIsEmpty(order.deltaNeutralClearingIntent)) orderProto.set_deltaneutralclearingintent(order.deltaNeutralClearingIntent);
    if (Utils::isValidValue(order.discretionaryAmt)) orderProto.set_discretionaryamt(order.discretionaryAmt);
    if (order.optOutSmartRouting) orderProto.set_optoutsmartrouting(order.optOutSmartRouting);
    if (Utils::isValidValue(order.exemptCode)) orderProto.set_exemptcode(order.exemptCode);
    if (Utils::isValidValue(order.startingPrice)) orderProto.set_startingprice(order.startingPrice);
    if (Utils::isValidValue(order.stockRefPrice)) orderProto.set_stockrefprice(order.stockRefPrice);
    if (Utils::isValidValue(order.delta)) orderProto.set_delta(order.delta);
    if (Utils::isValidValue(order.stockRangeLower)) orderProto.set_stockrangelower(order.stockRangeLower);
    if (Utils::isValidValue(order.stockRangeUpper)) orderProto.set_stockrangeupper(order.stockRangeUpper);
    if (order.notHeld) orderProto.set_notheld(order.notHeld);

    std::map<std::string, std::string> orderMiscOptionsMap = createStringStringMap(order.orderMiscOptions);
    for (std::pair<std::string, std::string> orderMiscOption : orderMiscOptionsMap) {
        (*orderProto.mutable_ordermiscoptions())[orderMiscOption.first] = orderMiscOption.second;
    }

    if (order.solicited) orderProto.set_solicited(order.solicited);
    if (order.randomizeSize) orderProto.set_randomizesize(order.randomizeSize);
    if (order.randomizePrice) orderProto.set_randomizeprice(order.randomizePrice);
    if (Utils::isValidValue(order.referenceContractId)) orderProto.set_referencecontractid(order.referenceContractId);
    if (Utils::isValidValue(order.peggedChangeAmount)) orderProto.set_peggedchangeamount(order.peggedChangeAmount);
    if (order.isPeggedChangeAmountDecrease) orderProto.set_ispeggedchangeamountdecrease(order.isPeggedChangeAmountDecrease);
    if (Utils::isValidValue(order.referenceChangeAmount)) orderProto.set_referencechangeamount(order.referenceChangeAmount);
    if (!Utils::stringIsEmpty(order.referenceExchangeId)) orderProto.set_referenceexchangeid(order.referenceExchangeId);
    if (!Utils::stringIsEmpty(order.adjustedOrderType)) orderProto.set_adjustedordertype(order.adjustedOrderType);
    if (Utils::isValidValue(order.triggerPrice)) orderProto.set_triggerprice(order.triggerPrice);
    if (Utils::isValidValue(order.adjustedStopPrice)) orderProto.set_adjustedstopprice(order.adjustedStopPrice);
    if (Utils::isValidValue(order.adjustedStopLimitPrice)) orderProto.set_adjustedstoplimitprice(order.adjustedStopLimitPrice);
    if (Utils::isValidValue(order.adjustedTrailingAmount)) orderProto.set_adjustedtrailingamount(order.adjustedTrailingAmount);
    if (Utils::isValidValue(order.adjustableTrailingUnit)) orderProto.set_adjustabletrailingunit(order.adjustableTrailingUnit);
    if (Utils::isValidValue(order.lmtPriceOffset)) orderProto.set_lmtpriceoffset(order.lmtPriceOffset);

    std::list<protobuf::OrderCondition> orderConditionList = createConditionsProto(order);
    if (!orderConditionList.empty()) {
        for (protobuf::OrderCondition orderConditionProto : orderConditionList) {
            orderProto.add_conditions()->CopyFrom(orderConditionProto);
        }
    }
    if (order.conditionsCancelOrder) orderProto.set_conditionscancelorder(order.conditionsCancelOrder);
    if (order.conditionsIgnoreRth) orderProto.set_conditionsignorerth(order.conditionsIgnoreRth);

    if (!Utils::stringIsEmpty(order.modelCode)) orderProto.set_modelcode(order.modelCode);
    if (!Utils::stringIsEmpty(order.extOperator)) orderProto.set_extoperator(order.extOperator);

    orderProto.mutable_softdollartier()->CopyFrom(createSoftDollarTierProto(order));

    if (Utils::isValidValue(order.cashQty)) orderProto.set_cashqty(order.cashQty);
    if (!Utils::stringIsEmpty(order.mifid2DecisionMaker)) orderProto.set_mifid2decisionmaker(order.mifid2DecisionMaker);
    if (!Utils::stringIsEmpty(order.mifid2DecisionAlgo)) orderProto.set_mifid2decisionalgo(order.mifid2DecisionAlgo);
    if (!Utils::stringIsEmpty(order.mifid2ExecutionTrader)) orderProto.set_mifid2executiontrader(order.mifid2ExecutionTrader);
    if (!Utils::stringIsEmpty(order.mifid2ExecutionAlgo)) orderProto.set_mifid2executionalgo(order.mifid2ExecutionAlgo);
    if (order.dontUseAutoPriceForHedge) orderProto.set_dontuseautopriceforhedge(order.dontUseAutoPriceForHedge);
    if (order.isOmsContainer) orderProto.set_isomscontainer(order.isOmsContainer);
    if (order.discretionaryUpToLimitPrice) orderProto.set_discretionaryuptolimitprice(order.discretionaryUpToLimitPrice);
    if (Utils::isValidValue(order.usePriceMgmtAlgo)) orderProto.set_usepricemgmtalgo(order.usePriceMgmtAlgo);
    if (Utils::isValidValue(order.duration)) orderProto.set_duration(order.duration);
    if (Utils::isValidValue(order.postToAts)) orderProto.set_posttoats(order.postToAts);
    if (!Utils::stringIsEmpty(order.advancedErrorOverride)) orderProto.set_advancederroroverride(order.advancedErrorOverride);
    if (!Utils::stringIsEmpty(order.manualOrderTime)) orderProto.set_manualordertime(order.manualOrderTime);
    if (Utils::isValidValue(order.minTradeQty)) orderProto.set_mintradeqty(order.minTradeQty);
    if (Utils::isValidValue(order.minCompeteSize)) orderProto.set_mincompetesize(order.minCompeteSize);
    if (Utils::isValidValue(order.competeAgainstBestOffset)) orderProto.set_competeagainstbestoffset(order.competeAgainstBestOffset);
    if (Utils::isValidValue(order.midOffsetAtWhole)) orderProto.set_midoffsetatwhole(order.midOffsetAtWhole);
    if (Utils::isValidValue(order.midOffsetAtHalf)) orderProto.set_midoffsetathalf(order.midOffsetAtHalf);
    if (!Utils::stringIsEmpty(order.customerAccount)) orderProto.set_customeraccount(order.customerAccount);
    if (order.professionalCustomer) orderProto.set_professionalcustomer(order.professionalCustomer);
    if (!Utils::stringIsEmpty(order.bondAccruedInterest)) orderProto.set_bondaccruedinterest(order.bondAccruedInterest);
    if (order.includeOvernight) orderProto.set_includeovernight(order.includeOvernight);
    if (Utils::isValidValue(order.manualOrderIndicator)) orderProto.set_manualorderindicator(order.manualOrderIndicator);
    if (!Utils::stringIsEmpty(order.submitter)) orderProto.set_submitter(order.submitter);
    if (order.autoCancelParent) orderProto.set_autocancelparent(order.autoCancelParent);
    if (order.imbalanceOnly) orderProto.set_imbalanceonly(order.imbalanceOnly);
    if (order.postOnly) orderProto.set_postonly(order.postOnly);
    if (order.allowPreOpen) orderProto.set_allowpreopen(order.allowPreOpen);
    if (order.ignoreOpenAuction) orderProto.set_ignoreopenauction(order.ignoreOpenAuction);
    if (order.deactivate) orderProto.set_deactivate(order.deactivate);
    if (Utils::isValidValue(order.seekPriceImprovement)) orderProto.set_seekpriceimprovement(order.seekPriceImprovement);
    if (Utils::isValidValue(order.whatIfType)) orderProto.set_whatiftype(order.whatIfType);
    if (Utils::isValidValue(order.routeMarketableToBbo)) orderProto.set_routemarketabletobbo(order.routeMarketableToBbo);

    return orderProto;
}

std::list<protobuf::OrderCondition> EClientUtils::createConditionsProto(Order order) {
    std::list<protobuf::OrderCondition> orderConditionList;

    try {
        if (order.conditions.size() > 0) {
            for (std::shared_ptr<OrderCondition> orderCondition : order.conditions) {
                OrderCondition::OrderConditionType type = orderCondition->type();

                protobuf::OrderCondition orderConditionProto;
                switch (type) {
                case OrderCondition::OrderConditionType::Price:
                    orderConditionProto = createPriceConditionProto(*orderCondition);
                    break;
                case OrderCondition::OrderConditionType::Time:
                    orderConditionProto = createTimeConditionProto(*orderCondition);
                    break;
                case OrderCondition::OrderConditionType::Margin:
                    orderConditionProto = createMarginConditionProto(*orderCondition);
                    break;
                case OrderCondition::OrderConditionType::Execution:
                    orderConditionProto = createExecutionConditionProto(*orderCondition);
                    break;
                case OrderCondition::OrderConditionType::Volume:
                    orderConditionProto = createVolumeConditionProto(*orderCondition);
                    break;
                case OrderCondition::OrderConditionType::PercentChange:
                    orderConditionProto = createPercentChangeConditionProto(*orderCondition);
                    break;
                }
                orderConditionList.push_back(orderConditionProto);
            }
        }
    }
    catch (const std::exception& e) {
        throw new EClientException(ERROR_ENCODING_PROTOBUF, "Error encoding conditions");
    }

    return orderConditionList;
}

protobuf::OrderCondition EClientUtils::createOrderConditionProto(OrderCondition& condition) {
    int type = condition.type();
    bool isConjunctionConnection = condition.conjunctionConnection();
    protobuf::OrderCondition orderConditionProto;
    if (Utils::isValidValue(type)) orderConditionProto.set_type(type);
    orderConditionProto.set_isconjunctionconnection(isConjunctionConnection);
    return orderConditionProto;
}

protobuf::OrderCondition EClientUtils::createOperatorConditionProto(OrderCondition& condition) {
    protobuf::OrderCondition orderConditionProto = createOrderConditionProto(condition);
    OperatorCondition* operatorCondition = dynamic_cast<OperatorCondition*>(&condition);
    bool isMore = operatorCondition->isMore();
    protobuf::OrderCondition operatorConditionProto;
    operatorConditionProto.MergeFrom(orderConditionProto);
    operatorConditionProto.set_ismore(isMore);
    return operatorConditionProto;
}

protobuf::OrderCondition EClientUtils::createContractConditionProto(OrderCondition& condition) {
    protobuf::OrderCondition orderConditionProto = createOperatorConditionProto(condition);
    ContractCondition* contractCondition = dynamic_cast<ContractCondition*>(&condition);
    int conId = contractCondition->conId();
    std::string exchange = contractCondition->exchange();
    protobuf::OrderCondition contractConditionProto;
    contractConditionProto.MergeFrom(orderConditionProto);
    if (Utils::isValidValue(conId)) contractConditionProto.set_conid(conId);
    if (!Utils::stringIsEmpty(exchange)) contractConditionProto.set_exchange(exchange);
    return contractConditionProto;
}

protobuf::OrderCondition EClientUtils::createPriceConditionProto(OrderCondition& condition) {
    protobuf::OrderCondition orderConditionProto = createContractConditionProto(condition);
    PriceCondition* priceCondition = dynamic_cast< PriceCondition*>(&condition);
    double price = priceCondition->price();
    int triggerMethod = priceCondition->triggerMethod();
    protobuf::OrderCondition priceConditionProto;
    priceConditionProto.MergeFrom(orderConditionProto);
    if (Utils::isValidValue(price)) priceConditionProto.set_price(price);
    if (Utils::isValidValue(triggerMethod)) priceConditionProto.set_triggermethod(triggerMethod);
    return priceConditionProto;
}

protobuf::OrderCondition EClientUtils::createTimeConditionProto(OrderCondition& condition) {
    protobuf::OrderCondition operatorConditionProto = createOperatorConditionProto(condition);
    TimeCondition* timeCondition = dynamic_cast<TimeCondition*>(&condition);
    std::string time = timeCondition->time();
    protobuf::OrderCondition timeConditionProto;
    timeConditionProto.MergeFrom(operatorConditionProto);
    if (!Utils::stringIsEmpty(time)) timeConditionProto.set_time(time);
    return timeConditionProto;
}

protobuf::OrderCondition EClientUtils::createMarginConditionProto(OrderCondition& condition) {
    protobuf::OrderCondition operatorConditionProto = createOperatorConditionProto(condition);
    MarginCondition* marginCondition = dynamic_cast<MarginCondition*>(&condition);
    int percent = marginCondition->percent();
    protobuf::OrderCondition marginConditionProto;
    marginConditionProto.MergeFrom(operatorConditionProto);
    if (Utils::isValidValue(percent)) marginConditionProto.set_percent(percent);
    return marginConditionProto;
}

protobuf::OrderCondition EClientUtils::createExecutionConditionProto(OrderCondition& condition) {
    protobuf::OrderCondition orderConditionProto = createOrderConditionProto(condition);
    ExecutionCondition* executionCondition = dynamic_cast<ExecutionCondition*>(&condition);
    std::string secType = executionCondition->secType();
    std::string exchange = executionCondition->exchange();
    std::string symbol = executionCondition->symbol();
    protobuf::OrderCondition executionConditionProto;
    executionConditionProto.MergeFrom(orderConditionProto);
    if (!Utils::stringIsEmpty(secType)) executionConditionProto.set_sectype(secType);
    if (!Utils::stringIsEmpty(exchange)) executionConditionProto.set_exchange(exchange);
    if (!Utils::stringIsEmpty(symbol)) executionConditionProto.set_symbol(symbol);
    return executionConditionProto;
}

protobuf::OrderCondition EClientUtils::createVolumeConditionProto(OrderCondition& condition) {
    protobuf::OrderCondition orderConditionProto = createContractConditionProto(condition);
    VolumeCondition* volumeCondition = dynamic_cast<VolumeCondition*>(&condition);
    int volume = volumeCondition->volume();
    protobuf::OrderCondition volumeConditionProto;
    volumeConditionProto.MergeFrom(orderConditionProto);
    if (Utils::isValidValue(volume)) volumeConditionProto.set_volume(volume);
    return volumeConditionProto;
}

protobuf::OrderCondition EClientUtils::createPercentChangeConditionProto(OrderCondition& condition) {
    protobuf::OrderCondition orderConditionProto = createContractConditionProto(condition);
    PercentChangeCondition* percentChangeCondition = dynamic_cast<PercentChangeCondition*>(&condition);
    double changePercent = percentChangeCondition->changePercent();
    protobuf::OrderCondition percentChangeConditionProto;
    percentChangeConditionProto.MergeFrom(orderConditionProto);
    if (Utils::isValidValue(changePercent)) percentChangeConditionProto.set_changepercent(changePercent);
    return percentChangeConditionProto;
}

protobuf::SoftDollarTier EClientUtils::createSoftDollarTierProto(Order order) {
    SoftDollarTier tier = order.softDollarTier;
    protobuf::SoftDollarTier softDollarTierProto;
    if (!Utils::stringIsEmpty(tier.name())) softDollarTierProto.set_name(tier.name());
    if (!Utils::stringIsEmpty(tier.val())) softDollarTierProto.set_value(tier.val());
    if (!Utils::stringIsEmpty(tier.displayName())) softDollarTierProto.set_displayname(tier.displayName());
    return softDollarTierProto;
}

std::map<std::string, std::string> EClientUtils::createStringStringMap(TagValueListSPtr tagValueListSPtr) {
    std::map<std::string, std::string> stringStringMap;
    const TagValueList* tagValueList = tagValueListSPtr.get();
    const int tagValueListCount = tagValueList ? tagValueList->size() : 0;
    if (tagValueListCount > 0) {
        for (int i = 0; i < tagValueListCount; ++i) {
            const TagValue* tagValue = ((*tagValueList)[i]).get();
            stringStringMap.insert(std::pair<std::string, std::string>(tagValue->tag, tagValue->value));
        }
    }
    return stringStringMap;
}

protobuf::Contract EClientUtils::createContractProto(const Contract& contract, const Order& order) {
    protobuf::Contract contractProto;
    if (Utils::isValidValue(contract.conId)) contractProto.set_conid(contract.conId);
    if (!Utils::stringIsEmpty(contract.symbol)) contractProto.set_symbol(contract.symbol);
    if (!Utils::stringIsEmpty(contract.secType)) contractProto.set_sectype(contract.secType);
    if (!Utils::stringIsEmpty(contract.lastTradeDateOrContractMonth)) contractProto.set_lasttradedateorcontractmonth(contract.lastTradeDateOrContractMonth);
    if (Utils::isValidValue(contract.strike)) contractProto.set_strike(contract.strike);
    if (!Utils::stringIsEmpty(contract.right)) contractProto.set_right(contract.right);
    if (!Utils::stringIsEmpty(contract.multiplier)) contractProto.set_multiplier(atof(contract.multiplier.c_str()));
    if (!Utils::stringIsEmpty(contract.exchange)) contractProto.set_exchange(contract.exchange);
    if (!Utils::stringIsEmpty(contract.primaryExchange)) contractProto.set_primaryexch(contract.primaryExchange);
    if (!Utils::stringIsEmpty(contract.currency)) contractProto.set_currency(contract.currency);
    if (!Utils::stringIsEmpty(contract.localSymbol)) contractProto.set_localsymbol(contract.localSymbol);
    if (!Utils::stringIsEmpty(contract.tradingClass)) contractProto.set_tradingclass(contract.tradingClass);
    if (!Utils::stringIsEmpty(contract.secIdType)) contractProto.set_secidtype(contract.secIdType);
    if (!Utils::stringIsEmpty(contract.secId)) contractProto.set_secid(contract.secId);
    if (contract.includeExpired) contractProto.set_includeexpired(contract.includeExpired);
    if (!Utils::stringIsEmpty(contract.comboLegsDescrip)) contractProto.set_combolegsdescrip(contract.comboLegsDescrip);
    if (!Utils::stringIsEmpty(contract.description)) contractProto.set_description(contract.description);
    if (!Utils::stringIsEmpty(contract.issuerId)) contractProto.set_issuerid(contract.issuerId);

    std::list<protobuf::ComboLeg> comboLegProtoList = createComboLegProtoList(contract, order);
    if (!comboLegProtoList.empty()) {
        for (protobuf::ComboLeg comboLegProto : comboLegProtoList) {
            contractProto.add_combolegs()->CopyFrom(comboLegProto);
        }
    }

    protobuf::DeltaNeutralContract* deltaNeutralContractProto = createDeltaNeutralContractProto(contract);
    if (deltaNeutralContractProto != NULL) {
        contractProto.mutable_deltaneutralcontract()->CopyFrom(*deltaNeutralContractProto);
    }

    return contractProto;
}

protobuf::DeltaNeutralContract* EClientUtils::createDeltaNeutralContractProto(const Contract& contract) {
    if (contract.deltaNeutralContract == NULL) {
        return NULL;
    }
    DeltaNeutralContract* deltaNeutralContract = contract.deltaNeutralContract;
    protobuf::DeltaNeutralContract* deltaNeutralContractProto = new protobuf::DeltaNeutralContract();
    if (Utils::isValidValue(deltaNeutralContract->conId)) deltaNeutralContractProto->set_conid(deltaNeutralContract->conId);
    if (Utils::isValidValue(deltaNeutralContract->delta)) deltaNeutralContractProto->set_delta(deltaNeutralContract->delta);
    if (Utils::isValidValue(deltaNeutralContract->price)) deltaNeutralContractProto->set_price(deltaNeutralContract->price);
    return deltaNeutralContractProto;
}

std::list<protobuf::ComboLeg> EClientUtils::createComboLegProtoList(const Contract& contract, const Order& order) {
    std::list<protobuf::ComboLeg> comboLegProtoList;

    const Contract::ComboLegList* const comboLegs = contract.comboLegs.get();
    const int comboLegsCount = comboLegs ? comboLegs->size() : 0;
    const Order::OrderComboLegList* const orderComboLegs = order.orderComboLegs.get();
    const int orderComboLegsCount = orderComboLegs ? orderComboLegs->size() : 0;

    for (int i = 0; i < comboLegsCount; ++i) {
        const ComboLeg* comboLeg = ((*comboLegs)[i]).get();
        assert(comboLeg);
        double perLegPrice = UNSET_DOUBLE;
        if (i < orderComboLegsCount) {
            const OrderComboLeg* orderComboLeg = ((*orderComboLegs)[i]).get();
            assert(orderComboLeg);
            perLegPrice = orderComboLeg->price;
        }

        protobuf::ComboLeg comboLegProto = createComboLegProto(*comboLeg, perLegPrice);
        comboLegProtoList.push_back(comboLegProto);
    }

    return comboLegProtoList;
}

protobuf::ComboLeg EClientUtils::createComboLegProto(const ComboLeg& comboLeg, double perLegPrice) {
    protobuf::ComboLeg comboLegProto;
    if (Utils::isValidValue(comboLeg.conId)) comboLegProto.set_conid(comboLeg.conId);
    if (Utils::isValidValue(comboLeg.ratio)) comboLegProto.set_ratio(comboLeg.ratio);
    if (!Utils::stringIsEmpty(comboLeg.action)) comboLegProto.set_action(comboLeg.action);
    if (!Utils::stringIsEmpty(comboLeg.exchange)) comboLegProto.set_exchange(comboLeg.exchange);
    if (Utils::isValidValue(comboLeg.openClose)) comboLegProto.set_openclose(comboLeg.openClose);
    if (Utils::isValidValue(comboLeg.shortSaleSlot)) comboLegProto.set_shortsalesslot(comboLeg.shortSaleSlot);
    if (!Utils::stringIsEmpty(comboLeg.designatedLocation)) comboLegProto.set_designatedlocation(comboLeg.designatedLocation);
    if (Utils::isValidValue(comboLeg.exemptCode)) comboLegProto.set_exemptcode(comboLeg.exemptCode);
    if (Utils::isValidValue(perLegPrice)) comboLegProto.set_perlegprice(perLegPrice);
    return comboLegProto;
}

protobuf::CancelOrderRequest EClientUtils::createCancelOrderRequestProto(OrderId orderId, const OrderCancel& orderCancel) {
    protobuf::CancelOrderRequest cancelOrderRequestProto;
    if (Utils::isValidValue(orderId)) cancelOrderRequestProto.set_orderid(orderId);
    cancelOrderRequestProto.mutable_ordercancel()->CopyFrom(createOrderCancelProto(orderCancel));
    return cancelOrderRequestProto;
}

protobuf::GlobalCancelRequest EClientUtils::createGlobalCancelRequestProto(const OrderCancel& orderCancel) {
    protobuf::GlobalCancelRequest globalCancelRequestProto;
    globalCancelRequestProto.mutable_ordercancel()->CopyFrom(createOrderCancelProto(orderCancel));
    return globalCancelRequestProto;
}

protobuf::OrderCancel EClientUtils::createOrderCancelProto(const OrderCancel& orderCancel) {
    protobuf::OrderCancel orderCancelProto;
    if (!Utils::stringIsEmpty(orderCancel.manualOrderCancelTime)) orderCancelProto.set_manualordercanceltime(orderCancel.manualOrderCancelTime.c_str());
    if (!Utils::stringIsEmpty(orderCancel.extOperator)) orderCancelProto.set_extoperator(orderCancel.extOperator.c_str());
    if (Utils::isValidValue(orderCancel.manualOrderIndicator)) orderCancelProto.set_manualorderindicator(orderCancel.manualOrderIndicator);
    return orderCancelProto;;
}

protobuf::AllOpenOrdersRequest EClientUtils::createAllOpenOrdersRequestProto() {
    protobuf::AllOpenOrdersRequest allOpenOrdersRequestProto;
    return allOpenOrdersRequestProto;
}

protobuf::AutoOpenOrdersRequest EClientUtils::createAutoOpenOrdersRequestProto(bool autoBind) {
    protobuf::AutoOpenOrdersRequest autoOpenOrdersRequestProto;
    if (autoBind) autoOpenOrdersRequestProto.set_autobind(autoBind);
    return autoOpenOrdersRequestProto;
}

protobuf::OpenOrdersRequest EClientUtils::createOpenOrdersRequestProto() {
    protobuf::OpenOrdersRequest openOrdersRequestProto;
    return openOrdersRequestProto;
}

protobuf::CompletedOrdersRequest EClientUtils::createCompletedOrdersRequestProto(bool apiOnly) {
    protobuf::CompletedOrdersRequest completedOrdersRequestProto;
    if (apiOnly) completedOrdersRequestProto.set_apionly(apiOnly);
    return completedOrdersRequestProto;
}

protobuf::ContractDataRequest EClientUtils::createContractDataRequestProto(int reqId, const Contract& contract) {
    protobuf::ContractDataRequest contractDataRequestProto;
    if (Utils::isValidValue(reqId)) contractDataRequestProto.set_reqid(reqId);
    Order order;
    contractDataRequestProto.mutable_contract()->CopyFrom(createContractProto(contract, order));
    return contractDataRequestProto;
}

protobuf::MarketDataRequest EClientUtils::createMarketDataRequestProto(int reqId, const Contract& contract, const std::string& genericTickList, bool snapshot, bool regulatorySnapshot, const TagValueListSPtr marketDataOptionsList) {
    protobuf::MarketDataRequest marketDataRequestProto;
    if (Utils::isValidValue(reqId)) marketDataRequestProto.set_reqid(reqId);
    Order order;
    marketDataRequestProto.mutable_contract()->CopyFrom(createContractProto(contract, order));
    if (!Utils::stringIsEmpty(genericTickList)) marketDataRequestProto.set_genericticklist(genericTickList);
    if (snapshot) marketDataRequestProto.set_snapshot(snapshot);
    if (regulatorySnapshot) marketDataRequestProto.set_regulatorysnapshot(regulatorySnapshot);

    std::map<std::string, std::string> marketDataOptionsMap = createStringStringMap(marketDataOptionsList);
    for (std::pair<std::string, std::string> marketDataOptionsParam : marketDataOptionsMap) {
        (*marketDataRequestProto.mutable_marketdataoptions())[marketDataOptionsParam.first] = marketDataOptionsParam.second;
    }
    return marketDataRequestProto;
}

protobuf::MarketDepthRequest EClientUtils::createMarketDepthRequestProto(int reqId, const Contract& contract, int numRows, bool isSmartDepth, const TagValueListSPtr marketDepthOptionsList) {
    protobuf::MarketDepthRequest marketDepthRequestProto;
    if (Utils::isValidValue(reqId)) marketDepthRequestProto.set_reqid(reqId);
    Order order;
    marketDepthRequestProto.mutable_contract()->CopyFrom(createContractProto(contract, order));
    if (Utils::isValidValue(numRows)) marketDepthRequestProto.set_numrows(numRows);
    if (isSmartDepth) marketDepthRequestProto.set_issmartdepth(isSmartDepth);

    std::map<std::string, std::string> marketDepthOptionsMap = createStringStringMap(marketDepthOptionsList);
    for (std::pair<std::string, std::string> marketDepthOption : marketDepthOptionsMap) {
        (*marketDepthRequestProto.mutable_marketdepthoptions())[marketDepthOption.first] = marketDepthOption.second;
    }
    return marketDepthRequestProto;
}

protobuf::MarketDataTypeRequest EClientUtils::createMarketDataTypeRequestProto(int marketDataType) {
    protobuf::MarketDataTypeRequest marketDataTypeRequestProto;
    if (Utils::isValidValue(marketDataType)) marketDataTypeRequestProto.set_marketdatatype(marketDataType);
    return marketDataTypeRequestProto;
}

protobuf::CancelMarketData EClientUtils::createCancelMarketDataProto(int reqId) {
    protobuf::CancelMarketData cancelMarketDataProto;
    if (Utils::isValidValue(reqId)) cancelMarketDataProto.set_reqid(reqId);
    return cancelMarketDataProto;
}

protobuf::CancelMarketDepth EClientUtils::createCancelMarketDepthProto(int reqId, bool isSmartDepth) {
    protobuf::CancelMarketDepth cancelMarketDepthProto;
    if (Utils::isValidValue(reqId)) cancelMarketDepthProto.set_reqid(reqId);
    if (isSmartDepth) cancelMarketDepthProto.set_issmartdepth(isSmartDepth);
    return cancelMarketDepthProto;
}

protobuf::AccountDataRequest EClientUtils::createAccountDataRequestProto(bool subscribe, const std::string& acctCode) {
    protobuf::AccountDataRequest accountDataRequestProto;
    if (subscribe) accountDataRequestProto.set_subscribe(subscribe);
    if (!Utils::stringIsEmpty(acctCode)) accountDataRequestProto.set_acctcode(acctCode);
    return accountDataRequestProto;
}

protobuf::ManagedAccountsRequest EClientUtils::createManagedAccountsRequestProto() {
    protobuf::ManagedAccountsRequest managedAccountsRequestProto;
    return managedAccountsRequestProto;
}

protobuf::PositionsRequest EClientUtils::createPositionsRequestProto() {
    protobuf::PositionsRequest positionsRequestProto;
    return positionsRequestProto;
}

protobuf::CancelPositions EClientUtils::createCancelPositionsRequestProto() {
    protobuf::CancelPositions cancelPositionsProto;
    return cancelPositionsProto;
}

protobuf::AccountSummaryRequest EClientUtils::createAccountSummaryRequestProto(int reqId, const std::string& group, const std::string& tags) {
    protobuf::AccountSummaryRequest accountSummaryRequestProto;
    if (Utils::isValidValue(reqId)) accountSummaryRequestProto.set_reqid(reqId);
    if (!Utils::stringIsEmpty(group)) accountSummaryRequestProto.set_group(group);
    if (!Utils::stringIsEmpty(tags)) accountSummaryRequestProto.set_tags(tags);
    return accountSummaryRequestProto;
}

protobuf::CancelAccountSummary EClientUtils::createCancelAccountSummaryRequestProto(int reqId) {
    protobuf::CancelAccountSummary cancelAccountSummaryProto;
    if (Utils::isValidValue(reqId)) cancelAccountSummaryProto.set_reqid(reqId);
    return cancelAccountSummaryProto;
}

protobuf::PositionsMultiRequest EClientUtils::createPositionsMultiRequestProto(int reqId, const std::string& account, const std::string& modelCode) {
    protobuf::PositionsMultiRequest positionsMultiRequestProto;
    if (Utils::isValidValue(reqId)) positionsMultiRequestProto.set_reqid(reqId);
    if (!Utils::stringIsEmpty(account)) positionsMultiRequestProto.set_account(account);
    if (!Utils::stringIsEmpty(modelCode)) positionsMultiRequestProto.set_modelcode(modelCode);
    return positionsMultiRequestProto;
}

protobuf::CancelPositionsMulti EClientUtils::createCancelPositionsMultiRequestProto(int reqId) {
    protobuf::CancelPositionsMulti cancelPositionsMultiProto;
    if (Utils::isValidValue(reqId)) cancelPositionsMultiProto.set_reqid(reqId);
    return cancelPositionsMultiProto;
}

protobuf::AccountUpdatesMultiRequest EClientUtils::createAccountUpdatesMultiRequestProto(int reqId, const std::string& account, const std::string& modelCode, bool ledgerAndNLV) {
    protobuf::AccountUpdatesMultiRequest accountUpdatesMultiRequestProto;
    if (Utils::isValidValue(reqId)) accountUpdatesMultiRequestProto.set_reqid(reqId);
    if (!Utils::stringIsEmpty(account)) accountUpdatesMultiRequestProto.set_account(account);
    if (!Utils::stringIsEmpty(modelCode)) accountUpdatesMultiRequestProto.set_modelcode(modelCode);
    if (ledgerAndNLV) accountUpdatesMultiRequestProto.set_ledgerandnlv(ledgerAndNLV);
    return accountUpdatesMultiRequestProto;
}

protobuf::CancelAccountUpdatesMulti EClientUtils::createCancelAccountUpdatesMultiRequestProto(int reqId) {
    protobuf::CancelAccountUpdatesMulti cancelAccountUpdatesMultiProto;
    if (Utils::isValidValue(reqId)) cancelAccountUpdatesMultiProto.set_reqid(reqId);
    return cancelAccountUpdatesMultiProto;
}

protobuf::HistoricalDataRequest EClientUtils::createHistoricalDataRequestProto(int reqId, const Contract& contract, const std::string& endDateTime, const std::string& duration, 
    const std::string& barSizeSetting, const std::string& whatToShow, bool useRTH, int formatDate, bool keepUpToDate, const TagValueListSPtr& chartOptionsList) {

    protobuf::HistoricalDataRequest historicalDataRequestProto;
    if (Utils::isValidValue(reqId)) historicalDataRequestProto.set_reqid(reqId);
    Order order;
    historicalDataRequestProto.mutable_contract()->CopyFrom(createContractProto(contract, order));
    if (!Utils::stringIsEmpty(endDateTime)) historicalDataRequestProto.set_enddatetime(endDateTime);
    if (!Utils::stringIsEmpty(duration)) historicalDataRequestProto.set_duration(duration);
    if (!Utils::stringIsEmpty(barSizeSetting)) historicalDataRequestProto.set_barsizesetting(barSizeSetting);
    if (!Utils::stringIsEmpty(whatToShow)) historicalDataRequestProto.set_whattoshow(whatToShow);
    if (useRTH) historicalDataRequestProto.set_userth(useRTH);
    if (Utils::isValidValue(formatDate)) historicalDataRequestProto.set_formatdate(formatDate);
    if (keepUpToDate) historicalDataRequestProto.set_keepuptodate(keepUpToDate);

    std::map<std::string, std::string> chartOptionsMap = createStringStringMap(chartOptionsList);
    for (std::pair<std::string, std::string> chartOption : chartOptionsMap) {
        (*historicalDataRequestProto.mutable_chartoptions())[chartOption.first] = chartOption.second;
    }

    return historicalDataRequestProto;
}

protobuf::RealTimeBarsRequest EClientUtils::createRealTimeBarsRequestProto(int reqId, const Contract& contract, int barSize, const std::string& whatToShow, bool useRTH, 
    const TagValueListSPtr& realTimeBarsOptionsList) {

    protobuf::RealTimeBarsRequest realTimeBarsRequestProto;
    if (Utils::isValidValue(reqId)) realTimeBarsRequestProto.set_reqid(reqId);
    Order order;
    realTimeBarsRequestProto.mutable_contract()->CopyFrom(createContractProto(contract, order));
    if (Utils::isValidValue(barSize)) realTimeBarsRequestProto.set_barsize(barSize);
    if (!Utils::stringIsEmpty(whatToShow)) realTimeBarsRequestProto.set_whattoshow(whatToShow);
    if (useRTH) realTimeBarsRequestProto.set_userth(useRTH);

    std::map<std::string, std::string> realTimeBarsOptionsMap = createStringStringMap(realTimeBarsOptionsList);
    for (std::pair<std::string, std::string> realTimeBarsOption : realTimeBarsOptionsMap) {
        (*realTimeBarsRequestProto.mutable_realtimebarsoptions())[realTimeBarsOption.first] = realTimeBarsOption.second;
    }

    return realTimeBarsRequestProto;
}

protobuf::HeadTimestampRequest EClientUtils::createHeadTimestampRequestProto(int reqId, const Contract& contract, const std::string& whatToShow, bool useRTH, int formatDate) {
    protobuf::HeadTimestampRequest headTimestampRequestProto;
    if (Utils::isValidValue(reqId)) headTimestampRequestProto.set_reqid(reqId);
    Order order;
    headTimestampRequestProto.mutable_contract()->CopyFrom(createContractProto(contract, order));
    if (!Utils::stringIsEmpty(whatToShow)) headTimestampRequestProto.set_whattoshow(whatToShow);
    if (useRTH) headTimestampRequestProto.set_userth(useRTH);
    if (Utils::isValidValue(formatDate)) headTimestampRequestProto.set_formatdate(formatDate);
    return headTimestampRequestProto;
}

protobuf::HistogramDataRequest EClientUtils::createHistogramDataRequestProto(int reqId, const Contract& contract, bool useRTH, const std::string& timePeriod) {
    protobuf::HistogramDataRequest histogramDataRequestProto;
    if (Utils::isValidValue(reqId)) histogramDataRequestProto.set_reqid(reqId);
    Order order;
    histogramDataRequestProto.mutable_contract()->CopyFrom(createContractProto(contract, order));

    if (useRTH) histogramDataRequestProto.set_userth(useRTH);
    if (!Utils::stringIsEmpty(timePeriod)) histogramDataRequestProto.set_timeperiod(timePeriod);

    return histogramDataRequestProto;
}

protobuf::HistoricalTicksRequest EClientUtils::createHistoricalTicksRequestProto(int reqId, const Contract& contract, const std::string& startDateTime, 
    const std::string& endDateTime, int numberOfTicks, const std::string& whatToShow, bool useRTH, bool ignoreSize, const TagValueListSPtr& miscOptionsList) {

    protobuf::HistoricalTicksRequest historicalTicksRequestProto;
    if (Utils::isValidValue(reqId)) historicalTicksRequestProto.set_reqid(reqId);
    Order order;
    historicalTicksRequestProto.mutable_contract()->CopyFrom(createContractProto(contract, order));
    if (!Utils::stringIsEmpty(startDateTime)) historicalTicksRequestProto.set_startdatetime(startDateTime);
    if (!Utils::stringIsEmpty(endDateTime)) historicalTicksRequestProto.set_enddatetime(endDateTime);
    if (Utils::isValidValue(numberOfTicks)) historicalTicksRequestProto.set_numberofticks(numberOfTicks);
    if (!Utils::stringIsEmpty(whatToShow)) historicalTicksRequestProto.set_whattoshow(whatToShow);
    if (useRTH) historicalTicksRequestProto.set_userth(useRTH);
    if (ignoreSize) historicalTicksRequestProto.set_ignoresize(ignoreSize);

    std::map<std::string, std::string> miscOptionsMap = createStringStringMap(miscOptionsList);
    for (std::pair<std::string, std::string> miscOption : miscOptionsMap) {
        (*historicalTicksRequestProto.mutable_miscoptions())[miscOption.first] = miscOption.second;
    }

    return historicalTicksRequestProto;
}

protobuf::TickByTickRequest EClientUtils::createTickByTickRequestProto(int reqId, const Contract& contract, const std::string& tickType, int numberOfTicks, bool ignoreSize) {
    protobuf::TickByTickRequest tickByTickRequestProto;
    if (Utils::isValidValue(reqId)) tickByTickRequestProto.set_reqid(reqId);
    Order order;
    tickByTickRequestProto.mutable_contract()->CopyFrom(createContractProto(contract, order));
    if (!Utils::stringIsEmpty(tickType)) tickByTickRequestProto.set_ticktype(tickType);
    if (Utils::isValidValue(numberOfTicks)) tickByTickRequestProto.set_numberofticks(numberOfTicks);
    if (ignoreSize) tickByTickRequestProto.set_ignoresize(ignoreSize);
    return tickByTickRequestProto;
}

protobuf::CancelHistoricalData EClientUtils::createCancelHistoricalDataProto(int reqId) {
    protobuf::CancelHistoricalData cancelHistoricalDataProto;
    if (Utils::isValidValue(reqId)) cancelHistoricalDataProto.set_reqid(reqId);
    return cancelHistoricalDataProto;
}

protobuf::CancelRealTimeBars EClientUtils::createCancelRealTimeBarsProto(int reqId) {
    protobuf::CancelRealTimeBars cancelRealTimeBarsProto;
    if (Utils::isValidValue(reqId)) cancelRealTimeBarsProto.set_reqid(reqId);
    return cancelRealTimeBarsProto;
}

protobuf::CancelHeadTimestamp EClientUtils::createCancelHeadTimestampProto(int reqId) {
    protobuf::CancelHeadTimestamp cancelHeadTimestampProto;
    if (Utils::isValidValue(reqId)) cancelHeadTimestampProto.set_reqid(reqId);
    return cancelHeadTimestampProto;
}

protobuf::CancelHistogramData EClientUtils::createCancelHistogramDataProto(int reqId) {
    protobuf::CancelHistogramData cancelHistogramDataProto;
    if (Utils::isValidValue(reqId)) cancelHistogramDataProto.set_reqid(reqId);
    return cancelHistogramDataProto;
}

protobuf::CancelTickByTick EClientUtils::createCancelTickByTickProto(int reqId) {
    protobuf::CancelTickByTick cancelTickByTickProto;
    if (Utils::isValidValue(reqId)) cancelTickByTickProto.set_reqid(reqId);
    return cancelTickByTickProto;
}

protobuf::NewsBulletinsRequest EClientUtils::createNewsBulletinsRequestProto(bool allMessages) {
    protobuf::NewsBulletinsRequest newsBulletinsRequestProto;
    if (allMessages) newsBulletinsRequestProto.set_allmessages(allMessages);
    return newsBulletinsRequestProto;
}

protobuf::CancelNewsBulletins EClientUtils::createCancelNewsBulletinsProto() {
    protobuf::CancelNewsBulletins cancelNewsBulletinsProto;
    return cancelNewsBulletinsProto;
}

protobuf::NewsArticleRequest EClientUtils::createNewsArticleRequestProto(int reqId, const std::string& providerCode, const std::string& articleId, const TagValueListSPtr newsArticleOptionsList) {
    protobuf::NewsArticleRequest newsArticleRequestProto;
    if (Utils::isValidValue(reqId)) newsArticleRequestProto.set_reqid(reqId);
    if (!Utils::stringIsEmpty(providerCode)) newsArticleRequestProto.set_providercode(providerCode);
    if (!Utils::stringIsEmpty(articleId)) newsArticleRequestProto.set_articleid(articleId);

    std::map<std::string, std::string> newsArticleOptionsMap = createStringStringMap(newsArticleOptionsList);
    for (std::pair<std::string, std::string> newsArticleOption : newsArticleOptionsMap) {
        (*newsArticleRequestProto.mutable_newsarticleoptions())[newsArticleOption.first] = newsArticleOption.second;
    }
    return newsArticleRequestProto;
}

protobuf::NewsProvidersRequest EClientUtils::createNewsProvidersRequestProto() {
    protobuf::NewsProvidersRequest newsProvidersRequestProto;
    return newsProvidersRequestProto;
}

protobuf::HistoricalNewsRequest EClientUtils::createHistoricalNewsRequestProto(int reqId, int conId, const std::string& providerCodes, const std::string& startDateTime, const std::string& endDateTime, int totalResults, const TagValueListSPtr historicalNewsOptionsList) {
    protobuf::HistoricalNewsRequest historicalNewsRequestProto;
    if (Utils::isValidValue(reqId)) historicalNewsRequestProto.set_reqid(reqId);
    if (Utils::isValidValue(conId)) historicalNewsRequestProto.set_conid(conId);
    if (!Utils::stringIsEmpty(providerCodes)) historicalNewsRequestProto.set_providercodes(providerCodes);
    if (!Utils::stringIsEmpty(startDateTime)) historicalNewsRequestProto.set_startdatetime(startDateTime);
    if (!Utils::stringIsEmpty(endDateTime)) historicalNewsRequestProto.set_enddatetime(endDateTime);
    if (Utils::isValidValue(totalResults)) historicalNewsRequestProto.set_totalresults(totalResults);

    std::map<std::string, std::string> historicalNewsOptionsMap = createStringStringMap(historicalNewsOptionsList);
    for (std::pair<std::string, std::string> historicalNewsOption : historicalNewsOptionsMap) {
        (*historicalNewsRequestProto.mutable_historicalnewsoptions())[historicalNewsOption.first] = historicalNewsOption.second;
    }
    return historicalNewsRequestProto;
}

protobuf::WshMetaDataRequest EClientUtils::createWshMetaDataRequestProto(int reqId) {
    protobuf::WshMetaDataRequest wshMetaDataRequestProto;
    if (Utils::isValidValue(reqId)) wshMetaDataRequestProto.set_reqid(reqId);
    return wshMetaDataRequestProto;
}

protobuf::CancelWshMetaData EClientUtils::createCancelWshMetaDataProto(int reqId) {
    protobuf::CancelWshMetaData cancelWshMetaDataProto;
    if (Utils::isValidValue(reqId)) cancelWshMetaDataProto.set_reqid(reqId);
    return cancelWshMetaDataProto;
}

protobuf::WshEventDataRequest EClientUtils::createWshEventDataRequestProto(int reqId, const WshEventData& wshEventData) {
    protobuf::WshEventDataRequest wshEventDataRequestProto;
    if (Utils::isValidValue(reqId)) wshEventDataRequestProto.set_reqid(reqId);

    if (Utils::isValidValue(wshEventData.conId)) wshEventDataRequestProto.set_conid(wshEventData.conId);
    if (!Utils::stringIsEmpty(wshEventData.filter)) wshEventDataRequestProto.set_filter(wshEventData.filter);
    if (wshEventData.fillWatchlist) wshEventDataRequestProto.set_fillwatchlist(wshEventData.fillWatchlist);
    if (wshEventData.fillPortfolio) wshEventDataRequestProto.set_fillportfolio(wshEventData.fillPortfolio);
    if (wshEventData.fillCompetitors) wshEventDataRequestProto.set_fillcompetitors(wshEventData.fillCompetitors);
    if (!Utils::stringIsEmpty(wshEventData.startDate)) wshEventDataRequestProto.set_startdate(wshEventData.startDate);
    if (!Utils::stringIsEmpty(wshEventData.endDate)) wshEventDataRequestProto.set_enddate(wshEventData.endDate);
    if (Utils::isValidValue(wshEventData.totalLimit)) wshEventDataRequestProto.set_totallimit(wshEventData.totalLimit);
    
    return wshEventDataRequestProto;
}

protobuf::CancelWshEventData EClientUtils::createCancelWshEventDataProto(int reqId) {
    protobuf::CancelWshEventData cancelWshEventDataProto;
    if (Utils::isValidValue(reqId)) cancelWshEventDataProto.set_reqid(reqId);
    return cancelWshEventDataProto;
}

protobuf::ScannerParametersRequest EClientUtils::createScannerParametersRequestProto() {
    protobuf::ScannerParametersRequest scannerParametersRequestProto;
    return scannerParametersRequestProto;
}

protobuf::ScannerSubscriptionRequest EClientUtils::createScannerSubscriptionRequestProto(int reqId, const ScannerSubscription& subscription,
        const TagValueListSPtr& scannerSubscriptionOptionsList, const TagValueListSPtr& scannerSubscriptionFilterOptionsList) {
    protobuf::ScannerSubscriptionRequest scannerSubscriptionRequestProto;
    if (Utils::isValidValue(reqId)) scannerSubscriptionRequestProto.set_reqid(reqId);
    scannerSubscriptionRequestProto.mutable_scannersubscription()->CopyFrom(createScannerSubscriptionProto(subscription, scannerSubscriptionOptionsList, scannerSubscriptionFilterOptionsList));
    return scannerSubscriptionRequestProto;
}

protobuf::ScannerSubscription EClientUtils::createScannerSubscriptionProto(const ScannerSubscription& subscription,
        const TagValueListSPtr& scannerSubscriptionOptionsList, const TagValueListSPtr& scannerSubscriptionFilterOptionsList) {
    protobuf::ScannerSubscription scannerSubscriptionProto;
    if (Utils::isValidValue(subscription.numberOfRows)) scannerSubscriptionProto.set_numberofrows(subscription.numberOfRows);
    if (!Utils::stringIsEmpty(subscription.instrument)) scannerSubscriptionProto.set_instrument(subscription.instrument);
    if (!Utils::stringIsEmpty(subscription.locationCode)) scannerSubscriptionProto.set_locationcode(subscription.locationCode);
    if (!Utils::stringIsEmpty(subscription.scanCode)) scannerSubscriptionProto.set_scancode(subscription.scanCode);
    if (Utils::isValidValue(subscription.abovePrice)) scannerSubscriptionProto.set_aboveprice(subscription.abovePrice);
    if (Utils::isValidValue(subscription.belowPrice)) scannerSubscriptionProto.set_belowprice(subscription.belowPrice);
    if (Utils::isValidValue(subscription.aboveVolume)) scannerSubscriptionProto.set_abovevolume(subscription.aboveVolume);
    if (Utils::isValidValue(subscription.averageOptionVolumeAbove)) scannerSubscriptionProto.set_averageoptionvolumeabove(subscription.averageOptionVolumeAbove);
    if (Utils::isValidValue(subscription.marketCapAbove)) scannerSubscriptionProto.set_marketcapabove(subscription.marketCapAbove);
    if (Utils::isValidValue(subscription.marketCapBelow)) scannerSubscriptionProto.set_marketcapbelow(subscription.marketCapBelow);
    if (!Utils::stringIsEmpty(subscription.moodyRatingAbove)) scannerSubscriptionProto.set_moodyratingabove(subscription.moodyRatingAbove);
    if (!Utils::stringIsEmpty(subscription.moodyRatingBelow)) scannerSubscriptionProto.set_moodyratingbelow(subscription.moodyRatingBelow);
    if (!Utils::stringIsEmpty(subscription.spRatingAbove)) scannerSubscriptionProto.set_spratingabove(subscription.spRatingAbove);
    if (!Utils::stringIsEmpty(subscription.spRatingBelow)) scannerSubscriptionProto.set_spratingbelow(subscription.spRatingBelow);
    if (!Utils::stringIsEmpty(subscription.maturityDateAbove)) scannerSubscriptionProto.set_maturitydateabove(subscription.maturityDateAbove);
    if (!Utils::stringIsEmpty(subscription.maturityDateBelow)) scannerSubscriptionProto.set_maturitydatebelow(subscription.maturityDateBelow);
    if (Utils::isValidValue(subscription.couponRateAbove)) scannerSubscriptionProto.set_couponrateabove(subscription.couponRateAbove);
    if (Utils::isValidValue(subscription.couponRateBelow)) scannerSubscriptionProto.set_couponratebelow(subscription.couponRateBelow);
    if (Utils::isValidValue(subscription.excludeConvertible) && subscription.excludeConvertible != 0) scannerSubscriptionProto.set_excludeconvertible(subscription.excludeConvertible);
    if (!Utils::stringIsEmpty(subscription.scannerSettingPairs)) scannerSubscriptionProto.set_scannersettingpairs(subscription.scannerSettingPairs);
    if (!Utils::stringIsEmpty(subscription.stockTypeFilter)) scannerSubscriptionProto.set_stocktypefilter(subscription.stockTypeFilter);
    std::map<std::string, std::string> scannerSubscriptionOptions = createStringStringMap(scannerSubscriptionOptionsList);
    for (std::pair<std::string, std::string> scannerSubscriptionOption : scannerSubscriptionOptions) {
        (*scannerSubscriptionProto.mutable_scannersubscriptionoptions())[scannerSubscriptionOption.first] = scannerSubscriptionOption.second;
    }
    std::map<std::string, std::string> scannerSubscriptionFilterOptions = createStringStringMap(scannerSubscriptionFilterOptionsList);
    for (std::pair<std::string, std::string> scannerSubscriptionFilterOption : scannerSubscriptionFilterOptions) {
        (*scannerSubscriptionProto.mutable_scannersubscriptionfilteroptions())[scannerSubscriptionFilterOption.first] = scannerSubscriptionFilterOption.second;
    }
    return scannerSubscriptionProto;
}

protobuf::FundamentalsDataRequest EClientUtils::createFundamentalsDataRequestProto(int reqId, const Contract& contract, const std::string& reportType, const TagValueListSPtr fundamentalsDataOptionsList) {
    protobuf::FundamentalsDataRequest fundamentalsDataRequestProto;
    if (Utils::isValidValue(reqId)) fundamentalsDataRequestProto.set_reqid(reqId);
    Order order;
    fundamentalsDataRequestProto.mutable_contract()->CopyFrom(createContractProto(contract, order));
    if (!Utils::stringIsEmpty(reportType)) fundamentalsDataRequestProto.set_reporttype(reportType);

    std::map<std::string, std::string> fundamentalsDataOptionsMap = createStringStringMap(fundamentalsDataOptionsList);
    for (std::pair<std::string, std::string> fundamentalsDataOption : fundamentalsDataOptionsMap) {
        (*fundamentalsDataRequestProto.mutable_fundamentalsdataoptions())[fundamentalsDataOption.first] = fundamentalsDataOption.second;
    }
    return fundamentalsDataRequestProto;
}

protobuf::PnLRequest EClientUtils::createPnLRequestProto(int reqId, const std::string& account, const std::string& modelCode) {
    protobuf::PnLRequest pnlRequestProto;
    if (Utils::isValidValue(reqId)) pnlRequestProto.set_reqid(reqId);
    if (!Utils::stringIsEmpty(account)) pnlRequestProto.set_account(account);
    if (!Utils::stringIsEmpty(modelCode)) pnlRequestProto.set_modelcode(modelCode);
    return pnlRequestProto;
}

protobuf::PnLSingleRequest EClientUtils::createPnLSingleRequestProto(int reqId, const std::string& account, const std::string& modelCode, int conId) {
    protobuf::PnLSingleRequest pnlSingleRequestProto;
    if (Utils::isValidValue(reqId)) pnlSingleRequestProto.set_reqid(reqId);
    if (!Utils::stringIsEmpty(account)) pnlSingleRequestProto.set_account(account);
    if (!Utils::stringIsEmpty(modelCode)) pnlSingleRequestProto.set_modelcode(modelCode);
    if (Utils::isValidValue(conId)) pnlSingleRequestProto.set_conid(conId);
    return pnlSingleRequestProto;
}

protobuf::CancelScannerSubscription EClientUtils::createCancelScannerSubscriptionProto(int reqId) {
    protobuf::CancelScannerSubscription cancelScannerSubscriptionProto;
    if (Utils::isValidValue(reqId)) cancelScannerSubscriptionProto.set_reqid(reqId);
    return cancelScannerSubscriptionProto;
}

protobuf::CancelFundamentalsData EClientUtils::createCancelFundamentalsDataProto(int reqId) {
    protobuf::CancelFundamentalsData cancelFundamentalsDataProto;
    if (Utils::isValidValue(reqId)) cancelFundamentalsDataProto.set_reqid(reqId);
    return cancelFundamentalsDataProto;
}

protobuf::CancelPnL EClientUtils::createCancelPnLProto(int reqId) {
    protobuf::CancelPnL cancelPnLProto;
    if (Utils::isValidValue(reqId)) cancelPnLProto.set_reqid(reqId);
    return cancelPnLProto;
}

protobuf::CancelPnLSingle EClientUtils::createCancelPnLSingleProto(int reqId) {
    protobuf::CancelPnLSingle cancelPnLSingleProto;
    if (Utils::isValidValue(reqId)) cancelPnLSingleProto.set_reqid(reqId);
    return cancelPnLSingleProto;
}

protobuf::FARequest EClientUtils::createFARequestProto(int faDataType) {
    protobuf::FARequest faRequestProto;
    if (Utils::isValidValue(faDataType)) faRequestProto.set_fadatatype(faDataType);
    return faRequestProto;
}

protobuf::FAReplace EClientUtils::createFAReplaceProto(int reqId, int faDataType, const std::string& xml) {
    protobuf::FAReplace faReplaceProto;
    if (Utils::isValidValue(reqId)) faReplaceProto.set_reqid(reqId);
    if (Utils::isValidValue(faDataType)) faReplaceProto.set_fadatatype(faDataType);
    if (!Utils::stringIsEmpty(xml)) faReplaceProto.set_xml(xml);
    return faReplaceProto;
}

protobuf::ExerciseOptionsRequest EClientUtils::createExerciseOptionsRequestProto(int orderId, const Contract& contract, int exerciseAction, int exerciseQuantity, const std::string& account, bool override, const std::string& manualOrderTime, const std::string& customerAccount, bool professionalCustomer) {
    protobuf::ExerciseOptionsRequest exerciseOptionsRequestProto;
    if (Utils::isValidValue(orderId)) exerciseOptionsRequestProto.set_orderid(orderId);
    Order order;
    exerciseOptionsRequestProto.mutable_contract()->CopyFrom(createContractProto(contract, order));
    if (Utils::isValidValue(exerciseAction)) exerciseOptionsRequestProto.set_exerciseaction(exerciseAction);
    if (Utils::isValidValue(exerciseQuantity)) exerciseOptionsRequestProto.set_exercisequantity(exerciseQuantity);
    if (!Utils::stringIsEmpty(account)) exerciseOptionsRequestProto.set_account(account);
    if (override) exerciseOptionsRequestProto.set_override(override);
    if (!Utils::stringIsEmpty(manualOrderTime)) exerciseOptionsRequestProto.set_manualordertime(manualOrderTime);
    if (!Utils::stringIsEmpty(customerAccount)) exerciseOptionsRequestProto.set_customeraccount(customerAccount);
    if (professionalCustomer) exerciseOptionsRequestProto.set_professionalcustomer(professionalCustomer);
    return exerciseOptionsRequestProto;
}

protobuf::CalculateImpliedVolatilityRequest EClientUtils::createCalculateImpliedVolatilityRequestProto(int reqId, const Contract& contract, double optionPrice, double underPrice, const TagValueListSPtr impliedVolatilityOptionsList) {
    protobuf::CalculateImpliedVolatilityRequest calculateImpliedVolatilityRequestProto;
    if (Utils::isValidValue(reqId)) calculateImpliedVolatilityRequestProto.set_reqid(reqId);
    Order order;
    calculateImpliedVolatilityRequestProto.mutable_contract()->CopyFrom(createContractProto(contract, order));
    if (Utils::isValidValue(optionPrice)) calculateImpliedVolatilityRequestProto.set_optionprice(optionPrice);
    if (Utils::isValidValue(underPrice)) calculateImpliedVolatilityRequestProto.set_underprice(underPrice);
    std::map<std::string, std::string> implVolOptionsMap = createStringStringMap(impliedVolatilityOptionsList);
    for (std::pair<std::string, std::string> implVolOption : implVolOptionsMap) {
        (*calculateImpliedVolatilityRequestProto.mutable_impliedvolatilityoptions())[implVolOption.first] = implVolOption.second;
    }
    return calculateImpliedVolatilityRequestProto;
}

protobuf::CancelCalculateImpliedVolatility EClientUtils::createCancelCalculateImpliedVolatilityProto(int reqId) {
    protobuf::CancelCalculateImpliedVolatility cancelCalculateImpliedVolatilityProto;
    if (Utils::isValidValue(reqId)) cancelCalculateImpliedVolatilityProto.set_reqid(reqId);
    return cancelCalculateImpliedVolatilityProto;
}

protobuf::CalculateOptionPriceRequest EClientUtils::createCalculateOptionPriceRequestProto(int reqId, const Contract& contract, double volatility, double underPrice, const TagValueListSPtr optionPriceOptionsList) {
    protobuf::CalculateOptionPriceRequest calculateOptionPriceRequestProto;
    if (Utils::isValidValue(reqId)) calculateOptionPriceRequestProto.set_reqid(reqId);
    Order order;
    calculateOptionPriceRequestProto.mutable_contract()->CopyFrom(createContractProto(contract, order));
    if (Utils::isValidValue(volatility)) calculateOptionPriceRequestProto.set_volatility(volatility);
    if (Utils::isValidValue(underPrice)) calculateOptionPriceRequestProto.set_underprice(underPrice);
    std::map<std::string, std::string> optPrcOptionsMap = createStringStringMap(optionPriceOptionsList);
    for (std::pair<std::string, std::string> optPrcOption : optPrcOptionsMap) {
        (*calculateOptionPriceRequestProto.mutable_optionpriceoptions())[optPrcOption.first] = optPrcOption.second;
    }
    return calculateOptionPriceRequestProto;
}

protobuf::CancelCalculateOptionPrice EClientUtils::createCancelCalculateOptionPriceProto(int reqId) {
    protobuf::CancelCalculateOptionPrice cancelCalculateOptionPriceProto;
    if (Utils::isValidValue(reqId)) cancelCalculateOptionPriceProto.set_reqid(reqId);
    return cancelCalculateOptionPriceProto;
}

protobuf::SecDefOptParamsRequest EClientUtils::createSecDefOptParamsRequestProto(int reqId, const std::string& underlyingSymbol, const std::string& futFopExchange, const std::string& underlyingSecType, int underlyingConId) {
    protobuf::SecDefOptParamsRequest secDefOptParamsRequestProto;
    if (Utils::isValidValue(reqId)) secDefOptParamsRequestProto.set_reqid(reqId);
    if (!Utils::stringIsEmpty(underlyingSymbol)) secDefOptParamsRequestProto.set_underlyingsymbol(underlyingSymbol);
    if (!Utils::stringIsEmpty(futFopExchange)) secDefOptParamsRequestProto.set_futfopexchange(futFopExchange);
    if (!Utils::stringIsEmpty(underlyingSecType)) secDefOptParamsRequestProto.set_underlyingsectype(underlyingSecType);
    if (Utils::isValidValue(underlyingConId)) secDefOptParamsRequestProto.set_underlyingconid(underlyingConId);
    return secDefOptParamsRequestProto;
}

protobuf::SoftDollarTiersRequest EClientUtils::createSoftDollarTiersRequestProto(int reqId) {
    protobuf::SoftDollarTiersRequest softDollarTiersRequestProto;
    if (Utils::isValidValue(reqId)) softDollarTiersRequestProto.set_reqid(reqId);
    return softDollarTiersRequestProto;
}

protobuf::FamilyCodesRequest EClientUtils::createFamilyCodesRequestProto() {
    protobuf::FamilyCodesRequest familyCodesRequestProto;
    return familyCodesRequestProto;
}

protobuf::MatchingSymbolsRequest EClientUtils::createMatchingSymbolsRequestProto(int reqId, const std::string& pattern) {
    protobuf::MatchingSymbolsRequest matchingSymbolsRequestProto;
    if (Utils::isValidValue(reqId)) matchingSymbolsRequestProto.set_reqid(reqId);
    if (!Utils::stringIsEmpty(pattern)) matchingSymbolsRequestProto.set_pattern(pattern);
    return matchingSymbolsRequestProto;
}

protobuf::SmartComponentsRequest EClientUtils::createSmartComponentsRequestProto(int reqId, const std::string& bboExchange) {
    protobuf::SmartComponentsRequest smartComponentsRequestProto;
    if (Utils::isValidValue(reqId)) smartComponentsRequestProto.set_reqid(reqId);
    if (!Utils::stringIsEmpty(bboExchange)) smartComponentsRequestProto.set_bboexchange(bboExchange);
    return smartComponentsRequestProto;
}

protobuf::MarketRuleRequest EClientUtils::createMarketRuleRequestProto(int marketRuleId) {
    protobuf::MarketRuleRequest marketRuleRequestProto;
    if (Utils::isValidValue(marketRuleId)) marketRuleRequestProto.set_marketruleid(marketRuleId);
    return marketRuleRequestProto;
}

protobuf::UserInfoRequest EClientUtils::createUserInfoRequestProto(int reqId) {
    protobuf::UserInfoRequest userInfoRequestProto;
    if (Utils::isValidValue(reqId)) userInfoRequestProto.set_reqid(reqId);
    return userInfoRequestProto;
}

protobuf::IdsRequest EClientUtils::createIdsRequestProto(int numIds) {
    protobuf::IdsRequest idsRequestProto;
    if (Utils::isValidValue(numIds)) idsRequestProto.set_numids(numIds);
    return idsRequestProto;
}

protobuf::CurrentTimeRequest EClientUtils::createCurrentTimeRequestProto() {
    protobuf::CurrentTimeRequest currentTimeRequestProto;
    return currentTimeRequestProto;
}

protobuf::CurrentTimeInMillisRequest EClientUtils::createCurrentTimeInMillisRequestProto() {
    protobuf::CurrentTimeInMillisRequest currentTimeInMillisRequestProto;
    return currentTimeInMillisRequestProto;
}

protobuf::StartApiRequest EClientUtils::createStartApiRequestProto(int clientId, const std::string& optionalCapabilities) {
    protobuf::StartApiRequest startApiRequestProto;
    if (Utils::isValidValue(clientId)) startApiRequestProto.set_clientid(clientId);
    if (!Utils::stringIsEmpty(optionalCapabilities)) startApiRequestProto.set_optionalcapabilities(optionalCapabilities);
    return startApiRequestProto;
}

protobuf::SetServerLogLevelRequest EClientUtils::createSetServerLogLevelRequestProto(int logLevel) {
    protobuf::SetServerLogLevelRequest setServerLogLevelRequestProto;
    if (Utils::isValidValue(logLevel)) setServerLogLevelRequestProto.set_loglevel(logLevel);
    return setServerLogLevelRequestProto;
}

protobuf::VerifyRequest EClientUtils::createVerifyRequestProto(const std::string& apiName, const std::string& apiVersion) {
    protobuf::VerifyRequest verifyRequestProto;
    if (!Utils::stringIsEmpty(apiName)) verifyRequestProto.set_apiname(apiName);
    if (!Utils::stringIsEmpty(apiVersion)) verifyRequestProto.set_apiversion(apiVersion);
    return verifyRequestProto;
}

protobuf::VerifyMessageRequest EClientUtils::createVerifyMessageRequestProto(const std::string& apiData) {
    protobuf::VerifyMessageRequest verifyMessageRequestProto;
    if (!Utils::stringIsEmpty(apiData)) verifyMessageRequestProto.set_apidata(apiData);
    return verifyMessageRequestProto;
}

protobuf::QueryDisplayGroupsRequest EClientUtils::createQueryDisplayGroupsRequestProto(int reqId) {
    protobuf::QueryDisplayGroupsRequest queryDisplayGroupsRequestProto;
    if (Utils::isValidValue(reqId)) queryDisplayGroupsRequestProto.set_reqid(reqId);
    return queryDisplayGroupsRequestProto;
}

protobuf::SubscribeToGroupEventsRequest EClientUtils::createSubscribeToGroupEventsRequestProto(int reqId, int groupId) {
    protobuf::SubscribeToGroupEventsRequest subscribeToGroupEventsRequestProto;
    if (Utils::isValidValue(reqId)) subscribeToGroupEventsRequestProto.set_reqid(reqId);
    if (Utils::isValidValue(groupId)) subscribeToGroupEventsRequestProto.set_groupid(groupId);
    return subscribeToGroupEventsRequestProto;
}

protobuf::UpdateDisplayGroupRequest EClientUtils::createUpdateDisplayGroupRequestProto(int reqId, const std::string& contractInfo) {
    protobuf::UpdateDisplayGroupRequest updateDisplayGroupRequestProto;
    if (Utils::isValidValue(reqId)) updateDisplayGroupRequestProto.set_reqid(reqId);
    if (!Utils::stringIsEmpty(contractInfo)) updateDisplayGroupRequestProto.set_contractinfo(contractInfo);
    return updateDisplayGroupRequestProto;
}

protobuf::UnsubscribeFromGroupEventsRequest EClientUtils::createUnsubscribeFromGroupEventsRequestProto(int reqId) {
    protobuf::UnsubscribeFromGroupEventsRequest unsubscribeFromGroupEventsRequestProto;
    if (Utils::isValidValue(reqId)) unsubscribeFromGroupEventsRequestProto.set_reqid(reqId);
    return unsubscribeFromGroupEventsRequestProto;
}

protobuf::MarketDepthExchangesRequest EClientUtils::createMarketDepthExchangesRequestProto() {
    protobuf::MarketDepthExchangesRequest marketDepthExchangesRequestProto;
    return marketDepthExchangesRequestProto;
}

protobuf::CancelContractData EClientUtils::createCancelContractDataProto(int reqId) {
    protobuf::CancelContractData cancelContractDataProto;
    if (Utils::isValidValue(reqId)) cancelContractDataProto.set_reqid(reqId);
    return cancelContractDataProto;
}

protobuf::CancelHistoricalTicks EClientUtils::createCancelHistoricalTicksProto(int reqId) {
    protobuf::CancelHistoricalTicks cancelHistoricalTicksProto;
    if (Utils::isValidValue(reqId)) cancelHistoricalTicksProto.set_reqid(reqId);
    return cancelHistoricalTicksProto;
}
