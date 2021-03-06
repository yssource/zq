#include "stdafx.h"
#include "CMemCache.h"

#define  NULL_CHAR 0
#define  NULL_CHAR_STR '-'

std::string CMemCache::OrderToString(const PlatformStru_OrderInfo & nOrder)
{
	char buf[2048] = {0};
	sprintf(buf,"<O BrokerID=\"%s\" InvestorID=\"%s\" InstrumentID=\"%s\" OrderRef=\"%s\" UserID=\"%s\" \
				OrderPriceType=\"%c\" Direction=\"%c\" CombOffsetFlag=\"%s\" CombHedgeFlag=\"%s\" LimitPrice=\"%g\" \
				VolumeTotalOriginal=\"%d\" TimeCondition=\"%d\" GTDDate=\"%s\" VolumeCondition=\"%c\" MinVolume=\"%d\" \
				ContingentCondition=\"%c\" StopPrice=\"%g\" ForceCloseReason=\"%c\" IsAutoSuspend=\"%d\" BusinessUnit=\"%s\" \
				RequestID=\"%d\" OrderLocalID=\"%s\" ExchangeID=\"%s\" ParticipantID=\"%s\" ClientID=\"%s\" \
				ExchangeInstID=\"%s\" TraderID=\"%s\" InstallID=\"%d\" OrderSubmitStatus=\"%c\" NotifySequence=\"%d\" \
				TradingDay=\"%s\" SettlementID=\"%d\" OrderSysID=\"%s\" OrderSource=\"%c\" OrderStatus=\"%c\" \
				OrderType=\"%c\" VolumeTraded=\"%d\" VolumeTotal=\"%d\" InsertDate=\"%s\" InsertTime=\"%s\" \
				ActiveTime=\"%s\" SuspendTime=\"%s\" UpdateTime=\"%s\" CancelTime=\"%s\" ActiveTraderID=\"%s\" \
				ClearingPartID=\"%s\" SequenceNo=\"%d\" FrontID=\"%d\" SessionID=\"%d\" UserProductInfo=\"%s\" \
				StatusMsg=\"%s\" UserForceClose=\"%d\" ActiveUserID=\"%s\" BrokerOrderSeq=\"%d\" RelativeOrderSysID=\"%s\" />",

				nOrder.BrokerID,nOrder.InvestorID,nOrder.InstrumentID,nOrder.OrderRef,nOrder.UserID,
				nOrder.OrderPriceType == NULL_CHAR ? NULL_CHAR_STR :nOrder.OrderPriceType, nOrder.Direction == NULL_CHAR ? NULL_CHAR_STR :nOrder.Direction,
				nOrder.CombOffsetFlag,nOrder.CombHedgeFlag,nOrder.LimitPrice,
				nOrder.VolumeTotalOriginal== NULL_CHAR ? NULL_CHAR_STR :nOrder.VolumeTotalOriginal,
				nOrder.TimeCondition,nOrder.GTDDate,nOrder.VolumeCondition==NULL_CHAR ? NULL_CHAR_STR:nOrder.VolumeCondition,nOrder.MinVolume,
				nOrder.ContingentCondition,nOrder.StopPrice,nOrder.ForceCloseReason==NULL_CHAR ? NULL_CHAR_STR:nOrder.ForceCloseReason,nOrder.IsAutoSuspend,nOrder.BusinessUnit,
				nOrder.RequestID,nOrder.OrderLocalID,nOrder.ExchangeID,nOrder.ParticipantID,nOrder.ClientID,
				nOrder.ExchangeInstID,nOrder.TraderID,nOrder.InstallID,nOrder.OrderSubmitStatus ==NULL_CHAR ? NULL_CHAR_STR:nOrder.OrderSubmitStatus,nOrder.NotifySequence,
				nOrder.TradingDay,nOrder.SettlementID,nOrder.OrderSysID,nOrder.OrderSource ==NULL_CHAR ? NULL_CHAR_STR:nOrder.OrderSource,nOrder.OrderStatus==NULL_CHAR ? NULL_CHAR_STR:nOrder.OrderStatus,
				nOrder.OrderType == NULL_CHAR ? NULL_CHAR_STR :nOrder.OrderType,nOrder.VolumeTraded,nOrder.VolumeTotal,nOrder.InsertDate,nOrder.InsertTime,
				nOrder.ActiveTime,nOrder.SuspendTime,nOrder.UpdateTime,nOrder.CancelTime,nOrder.ActiveTraderID,
				nOrder.ClearingPartID,nOrder.SequenceNo,nOrder.FrontID,nOrder.SessionID,nOrder.UserProductInfo,
				nOrder.StatusMsg,nOrder.UserForceClose,nOrder.ActiveUserID,nOrder.BrokerOrderSeq,nOrder.RelativeOrderSysID
				);

	
	return buf;
}

std::string CMemCache::OrderInputToString(const PlatformStru_OrderInfo & nOrder)
{
	char buf[2048] = {0};
	sprintf(buf,"<OI BrokerID=\"%s\" InvestorID=\"%s\" InstrumentID=\"%s\" OrderRef=\"%s\" UserID=\"%s\" \
				OrderPriceType=\"%c\" Direction=\"%c\" CombOffsetFlag=\"%s\" CombHedgeFlag=\"%s\" LimitPrice=\"%g\" \
				VolumeTotalOriginal=\"%d\" TimeCondition=\"%d\" GTDDate=\"%s\" VolumeCondition=\"%c\" MinVolume=\"%d\" \
				ContingentCondition=\"%c\" StopPrice=\"%g\" ForceCloseReason=\"%c\" IsAutoSuspend=\"%d\" BusinessUnit=\"%s\" \
				RequestID=\"%d\" OrderLocalID=\"%s\" ExchangeID=\"%s\" ParticipantID=\"%s\" ClientID=\"%s\" \
				ExchangeInstID=\"%s\" TraderID=\"%s\" InstallID=\"%d\" OrderSubmitStatus=\"%c\" NotifySequence=\"%d\" \
				TradingDay=\"%s\" SettlementID=\"%d\" OrderSysID=\"%s\" OrderSource=\"%c\" OrderStatus=\"%c\" \
				OrderType=\"%c\" VolumeTraded=\"%d\" VolumeTotal=\"%d\" InsertDate=\"%s\" InsertTime=\"%s\" \
				ActiveTime=\"%s\" SuspendTime=\"%s\" UpdateTime=\"%s\" CancelTime=\"%s\" ActiveTraderID=\"%s\" \
				ClearingPartID=\"%s\" SequenceNo=\"%d\" FrontID=\"%d\" SessionID=\"%d\" UserProductInfo=\"%s\" \
				StatusMsg=\"%s\" UserForceClose=\"%d\" ActiveUserID=\"%s\" BrokerOrderSeq=\"%d\" RelativeOrderSysID=\"%s\" />",

				nOrder.BrokerID,nOrder.InvestorID,nOrder.InstrumentID,nOrder.OrderRef,nOrder.UserID,
				nOrder.OrderPriceType == NULL_CHAR ? NULL_CHAR_STR :nOrder.OrderPriceType, nOrder.Direction == NULL_CHAR ? NULL_CHAR_STR :nOrder.Direction,
				nOrder.CombOffsetFlag,nOrder.CombHedgeFlag,nOrder.LimitPrice,
				nOrder.VolumeTotalOriginal== NULL_CHAR ? NULL_CHAR_STR :nOrder.VolumeTotalOriginal,
				nOrder.TimeCondition,nOrder.GTDDate,nOrder.VolumeCondition==NULL_CHAR ? NULL_CHAR_STR:nOrder.VolumeCondition,nOrder.MinVolume,
				nOrder.ContingentCondition,nOrder.StopPrice,nOrder.ForceCloseReason==NULL_CHAR ? NULL_CHAR_STR:nOrder.ForceCloseReason,nOrder.IsAutoSuspend,nOrder.BusinessUnit,
				nOrder.RequestID,nOrder.OrderLocalID,nOrder.ExchangeID,nOrder.ParticipantID,nOrder.ClientID,
				nOrder.ExchangeInstID,nOrder.TraderID,nOrder.InstallID,nOrder.OrderSubmitStatus ==NULL_CHAR ? NULL_CHAR_STR:nOrder.OrderSubmitStatus,nOrder.NotifySequence,
				nOrder.TradingDay,nOrder.SettlementID,nOrder.OrderSysID,nOrder.OrderSource ==NULL_CHAR ? NULL_CHAR_STR:nOrder.OrderSource,nOrder.OrderStatus==NULL_CHAR ? NULL_CHAR_STR:nOrder.OrderStatus,
				nOrder.OrderType == NULL_CHAR ? NULL_CHAR_STR :nOrder.OrderType,nOrder.VolumeTraded,nOrder.VolumeTotal,nOrder.InsertDate,nOrder.InsertTime,
				nOrder.ActiveTime,nOrder.SuspendTime,nOrder.UpdateTime,nOrder.CancelTime,nOrder.ActiveTraderID,
				nOrder.ClearingPartID,nOrder.SequenceNo,nOrder.FrontID,nOrder.SessionID,nOrder.UserProductInfo,
				nOrder.StatusMsg,nOrder.UserForceClose,nOrder.ActiveUserID,nOrder.BrokerOrderSeq,nOrder.RelativeOrderSysID
				);


	return buf;
}
std::string  CMemCache::TraderToString(const PlatformStru_TradeInfo & nTrade)
{
	char buf[1024] = {0};
	sprintf(buf,"<T  BrokerID=\"%s\" InvestorID=\"%s\" InstrumentID=\"%s\" OrderRef=\"%s\" UserID=\"%s\" \
		ExchangeID=\"%s\" TradeID=\"%s\" Direction=\"%c\" OrderSysID=\"%s\" ParticipantID=\"%s\" \
		ClientID=\"%s\" TradingRole=\"%c\" ExchangeInstID=\"%s\" OffsetFlag=\"%c\" HedgeFlag=\"%c\" \
		Price=\"%g\" Volume=\"%d\" TradeDate=\"%s\" TradeTime=\"%s\" TradeType=\"%c\" \
		PriceSource=\"%c\" TraderID=\"%s\" OrderLocalID=\"%s\" ClearingPartID=\"%s\" BusinessUnit=\"%s\" \
		SequenceNo=\"%d\" TradingDay=\"%s\" SettlementID=\"%d\" BrokerOrderSeq=\"%d\" />",		
		nTrade.BrokerID,nTrade.InvestorID,nTrade.InstrumentID,nTrade.OrderRef,nTrade.UserID,
		nTrade.ExchangeID,nTrade.TradeID,nTrade.Direction == NULL_CHAR ? NULL_CHAR_STR :nTrade.Direction,nTrade.OrderSysID,nTrade.ParticipantID,
		nTrade.ClientID,nTrade.TradingRole== NULL_CHAR ? NULL_CHAR_STR:nTrade.TradingRole,nTrade.ExchangeInstID,nTrade.OffsetFlag == NULL_CHAR ? NULL_CHAR_STR :nTrade.OffsetFlag,nTrade.HedgeFlag == NULL_CHAR ? NULL_CHAR_STR :nTrade.HedgeFlag,
		nTrade.Price,nTrade.Volume,nTrade.TradeDate,nTrade.TradeTime,nTrade.TradeType== NULL_CHAR ? NULL_CHAR_STR :nTrade.TradeType,
		nTrade.PriceSource== NULL_CHAR ? NULL_CHAR_STR :nTrade.PriceSource,nTrade.TraderID,nTrade.OrderLocalID,nTrade.ClearingPartID,nTrade.BusinessUnit,
		nTrade.SequenceNo,nTrade.TradingDay,nTrade.SettlementID,nTrade.BrokerOrderSeq,nTrade.TradeSource
		);
	return buf;
}
std::string CMemCache::PositionToString(const PlatformStru_Position & nPos)
{
	char buf[2048] = {0};
	sprintf(buf,"<P \
		 InstrumentID=\"%s\" BrokerID=\"%s\" InvestorID=\"%s\" PosiDirection=\"%c\" HedgeFlag=\"%c\" \
		 PositionDate=\"%d\" YdPosition=\"%d\" Position=\"%d\" LongFrozen=\"%d\" ShortFrozen=\"%d\" \
		 LongFrozenAmount=\"%g\"  ShortFrozenAmount=\"%g\" OpenVolume=\"%d\" CloseVolume=\"%d\" OpenAmount=\"%g\" \
		 CloseAmount=\"%g\" PositionCost=\"%g\" PreMargin=\"%g\" UseMargin=\"%g\" FrozenMargin=\"%g\" \
		 FrozenCash=\"%g\" FrozenCommission=\"%g\" CashIn=\"%g\" Commission=\"%g\" CloseProfit=\"%g\" \
		 PositionProfit=\"%g\" PreSettlementPrice=\"%g\" SettlementPrice=\"%g\" TradingDay=\"%s\" SettlementID=\"%d\" \
		 OpenCost=\"%g\" ExchangeMargin=\"%g\" CombPosition=\"%d\" CombLongFrozen=\"%d\" CombShortFrozen=\"%d\" \
		 CloseProfitByDate=\"%g\" CloseProfitByTrade=\"%g\" TodayPosition=\"%d\" MarginRateByMoney=\"%g\" \
		 MarginRateByVolume=\"%g\" ExchangeID=\"%s\" PositionProfitByTrade=\"%g\" TotalPositionProfitByDate=\"%g\" \
		 CanCloseVolume=\"%d\" CanCloseTodayVolume=\"%d\" CanCloseydVolume=\"%d\" AveragePositionPrice=\"%g\" \
		 TodayCloseYdVolume=\"%d\" YdPositionRemain=\"%d\" AverageOpenPrice=\"%g\"/>",	
		nPos.InstrumentID, nPos.BrokerID,nPos.InvestorID,nPos.PosiDirection== NULL_CHAR ? NULL_CHAR_STR :nPos.PosiDirection,nPos.HedgeFlag== NULL_CHAR ? NULL_CHAR_STR :nPos.HedgeFlag,
		nPos.PositionDate, nPos.YdPosition,nPos.Position,nPos.LongFrozen,nPos.ShortFrozen,
		nPos.LongFrozenAmount, nPos.ShortFrozenAmount,nPos.OpenVolume,nPos.CloseVolume,nPos.OpenAmount,
		nPos.CloseAmount, nPos.PositionCost,nPos.PreMargin,nPos.UseMargin,nPos.FrozenMargin,
		nPos.FrozenCash, nPos.FrozenCommission,nPos.CashIn,nPos.Commission,nPos.CloseProfit,
		nPos.PositionProfit, nPos.PreSettlementPrice,nPos.SettlementPrice,nPos.TradingDay,nPos.SettlementID,
		nPos.OpenCost, nPos.ExchangeMargin,nPos.CombPosition,nPos.CombLongFrozen,nPos.CombShortFrozen,
		nPos.CloseProfitByDate, nPos.CloseProfitByTrade,nPos.TodayPosition,nPos.MarginRateByMoney,
		nPos.MarginRateByVolume,nPos.ExchangeID,nPos.PositionProfitByTrade,nPos.TotalPositionProfitByDate,
		nPos.CanCloseVolume,nPos.CanCloseTodayVolume,nPos.CanCloseydVolume,nPos.AveragePositionPrice,
		nPos.TodayCloseYdVolume,nPos.YdPositionRemain,nPos.AverageOpenPrice);
	return buf;
}
std::string CMemCache::FundToString(const PlatformStru_TradingAccountInfo & nFund)
{
	char buf[1024] = {0};
	sprintf(buf,"<Fund BrokerID=\"%s\" AccountID=\"%s\" PreMortgage=\"%g\" PreCredit=\"%g\" PreDeposit=\"%g\" \
				PreBalance=\"%g\" PreMargin=\"%g\" InterestBase=\"%g\" Interest=\"%g\" Deposit=\"%g\" \
				Withdraw=\"%g\" FrozenMargin=\"%g\" FrozenCash=\"%g\" FrozenCommission=\"%g\" CurrMargin=\"%g\" \
				CashIn=\"%g\" Commission=\"%g\" CloseProfit=\"%g\" PositionProfit=\"%g\" Balance=\"%g\" \
				Available=\"%g\" WithdrawQuota=\"%g\" Reserve=\"%g\" TradingDay=\"%s\" SettlementID=\"%d\" \
				Credit=\"%g\" Mortgage=\"%g\" ExchangeMargin=\"%g\" DeliveryMargin=\"%g\" ExchangeDeliveryMargin=\"%g\" \
				StaticProfit=\"%g\" DynamicProfit=\"%g\" RiskDegree=\"%g\"/>",
				nFund.BrokerID,nFund.AccountID,nFund.PreMortgage,nFund.PreCredit,nFund.PreDeposit, 
				nFund.PreBalance,nFund.PreMargin,nFund.InterestBase,nFund.Interest,nFund.Deposit, 
				nFund.Withdraw,nFund.FrozenMargin,nFund.FrozenCash,nFund.FrozenCommission,nFund.CurrMargin, 
				nFund.CashIn,nFund.Commission,nFund.CloseProfit,nFund.PositionProfit,nFund.Balance, 
				nFund.Available,nFund.WithdrawQuota,nFund.Reserve,nFund.TradingDay,nFund.SettlementID, 
				nFund.Credit,nFund.Mortgage,nFund.ExchangeMargin,nFund.DeliveryMargin,nFund.ExchangeDeliveryMargin,
				nFund.StaticProfit,nFund.DynamicProfit,nFund.RiskDegree);
	return buf;
}
std::string CMemCache::OrderActionToString(const CThostFtdcOrderActionField & nOA)
{
	char buf[1024] = {0};
	sprintf(buf,"<OA BrokerID=\"%s\" InvestorID=\"%s\" OrderActionRef=\"%d\" OrderRef=\"%s\" RequestID=\"%d\" \
		 FrontID=\"%d\" SessionID=\"%d\" ExchangeID=\"%s\" OrderSysID=\"%s\" ActionFlag=\"%c\" \
		 LimitPrice=\"%g\" VolumeChange=\"%d\" ActionDate=\"%s\" ActionTime=\"%s\" TraderID=\"%s\" \
		 InstallID=\"%d\" OrderLocalID=\"%s\" ActionLocalID=\"%s\" ParticipantID=\"%s\" ClientID=\"%s\" \
		 BusinessUnit=\"%s\" UserID=\"%s\" StatusMsg=\"%s\" InstrumentID=\"%s\" />",	
		nOA.BrokerID, nOA.InvestorID, nOA.OrderActionRef, nOA.OrderRef,nOA.RequestID,
		nOA.FrontID, nOA.SessionID, nOA.ExchangeID, nOA.OrderSysID,nOA.ActionFlag== NULL_CHAR ? NULL_CHAR_STR :nOA.ActionFlag,
		nOA.LimitPrice, nOA.VolumeChange, nOA.ActionDate, nOA.ActionTime,nOA.TraderID,
		nOA.InstallID, nOA.OrderLocalID, nOA.ActionLocalID, nOA.ParticipantID,nOA.ClientID,
		nOA.BusinessUnit,  nOA.UserID, nOA.StatusMsg,nOA.InstrumentID);

	return buf;
}
std::string CMemCache::PositionDetailToString(const PlatformStru_PositionDetail & nOrder)
{

	return "";
}
std::string CMemCache::QuotToString(const PlatformStru_DepthMarketData & nQuot)
{
	char buf[2048] = {0};

	sprintf(buf,"<Q TradingDay=\"%s\" InstrumentID=\"%s\" ExchangeID=\"%s\" ExchangeInstID=\"%s\" LastPrice=\"%g\" \
				PreSettlementPrice=\"%g\" PreClosePrice=\"%g\" PreOpenInterest=\"%g\" OpenPrice=\"%g\" HighestPrice=\"%g\" \
				LowestPrice=\"%g\" Volume=\"%d\" Turnover=\"%g\" OpenInterest=\"%g\" ClosePrice=\"%g\" \
				SettlementPrice=\"%g\" UpperLimitPrice=\"%g\" LowerLimitPrice=\"%g\" PreDelta=\"%g\" CurrDelta=\"%g\" \
				UpdateTime=\"%s\" UpdateMillisec=\"%d\" BidPrice1=\"%g\" BidVolume1=\"%d\" AskPrice1=\"%g\" AskVolume1=\"%d\" \
				BidPrice2=\"%g\" AskVolume2=\"%d\" BidVolume2=\"%d\" AskPrice2=\"%g\" \
				BidPrice3=\"%g\" AskVolume3=\"%d\" BidVolume3=\"%d\" AskPrice3=\"%g\" \
				BidPrice4=\"%g\" AskVolume4=\"%d\" BidVolume4=\"%d\" AskPrice4=\"%g\" \
				BidPrice5=\"%g\" AskVolume5=\"%d\" BidVolume5=\"%d\" AskPrice5=\"%g\" AveragePrice=\"%g\" \
				BidPrice6=\"%g\" AskVolume6=\"%d\" BidVolume6=\"%d\" AskPrice6=\"%g\" \
				BidPrice7=\"%g\" AskVolume7=\"%d\" BidVolume7=\"%d\" AskPrice7=\"%g\" \
				BidPrice8=\"%g\" AskVolume8=\"%d\" BidVolume8=\"%d\" AskPrice8=\"%g\" \
				BidPrice9=\"%g\" AskVolume9=\"%d\" BidVolume9=\"%d\" AskPrice9=\"%g\" \
				BidPrice10=\"%g\" AskVolume10=\"%d\" BidVolume10=\"%d\" AskPrice10=\"%g\" \
				NewVolume=\"%d\" />",	
				nQuot.TradingDay,nQuot.InstrumentID,nQuot.ExchangeID,nQuot.ExchangeInstID,nQuot.LastPrice,
				nQuot.PreSettlementPrice,nQuot.PreClosePrice,nQuot.PreOpenInterest,nQuot.OpenPrice,nQuot.HighestPrice,
				nQuot.LowestPrice,nQuot.Volume,nQuot.Turnover,nQuot.OpenInterest,nQuot.ClosePrice,
				nQuot.SettlementPrice,nQuot.UpperLimitPrice,nQuot.LowerLimitPrice,nQuot.PreDelta,nQuot.CurrDelta,
				nQuot.UpdateTime,nQuot.UpdateMillisec,nQuot.BidPrice1,nQuot.BidVolume1,nQuot.AskPrice1,nQuot.AskVolume1,
				nQuot.BidPrice2,nQuot.AskVolume2,nQuot.BidVolume2,nQuot.AskPrice2,
				nQuot.BidPrice3,nQuot.AskVolume3,nQuot.BidVolume3,nQuot.AskPrice3,
				nQuot.BidPrice4,nQuot.AskVolume4,nQuot.BidVolume4,nQuot.AskPrice4,
				nQuot.BidPrice5,nQuot.AskVolume5,nQuot.BidVolume5,nQuot.AskPrice5,nQuot.AveragePrice,
				nQuot.BidPrice6,nQuot.AskVolume6,nQuot.BidVolume6,nQuot.AskPrice6,
				nQuot.BidPrice7,nQuot.AskVolume7,nQuot.BidVolume7,nQuot.AskPrice7,
				nQuot.BidPrice8,nQuot.AskVolume8,nQuot.BidVolume8,nQuot.AskPrice8,
				nQuot.BidPrice9,nQuot.AskVolume9,nQuot.BidVolume9,nQuot.AskPrice9,
				nQuot.BidPrice10,nQuot.AskVolume10,nQuot.BidVolume10,nQuot.AskPrice10,
				nQuot.NewVolume	);

	return buf;
}
bool CMemCache::CachePosition(const PlatformStru_Position & nPos )
{
    if(mbSeverQuit)
        return false;

	if(mConnect == NULL || 1==mConnect->err)
    {
        printf("mbSeverQuit = %s, mConnect->errstr = %s",mbSeverQuit?"true":"false",mConnect->errstr);
		return false;
    }

	//Hset p:88002 poskey.tostring position
    PositionKey lKey(nPos);
	std::string ls;
	ls = "p:" ;
	ls += nPos.InvestorID;
	char lKeyBuf[256] = {0};
	lKey.tostring(lKeyBuf,256);

	std::string lxml =PositionToString(nPos);
	redisReply* lp = (redisReply*)redisCommand(mConnect,"HSET %s %s %s", ls.c_str(),lKeyBuf, lxml.c_str());
	// printf("SET: %s\n", reply->str);
	if(lp)
        freeReplyObject(lp);
	return true;
}
bool CMemCache::CachePositionDetail(const PlatformStru_PositionDetail & nPosDetail )
{
    if(mbSeverQuit)
        return false;

	if(mConnect == NULL || 1==mConnect->err)
    {
        printf("mbSeverQuit = %s, mConnect->errstr = %s",mbSeverQuit?"true":"false",mConnect->errstr);
        return false;
    }

	//Hset pd:88002 posdetailkey.tostring positiondetail
	PositionDetailKey lKey(nPosDetail);
	std::string ls;
	ls = "pd:" ;
	ls += nPosDetail.InvestorID;
	char lKeyBuf[256] = {0};
	lKey.tostring(lKeyBuf,256);
	std::string lxml = PositionDetailToString(nPosDetail);
	redisReply* lp = (redisReply*)redisCommand(mConnect,"HSET %s %s %s", ls.c_str(), lKeyBuf,lxml.c_str());
	// printf("SET: %s\n", reply->str);
	if(lp)
        freeReplyObject(lp);
	return true;
	
}
bool CMemCache::CacheFund(const PlatformStru_TradingAccountInfo & nFundInfo)
{
    if(mbSeverQuit)
        return false;

	if(mConnect == NULL || 1==mConnect->err)
    {
        printf("mbSeverQuit = %s, mConnect->errstr = %s",mbSeverQuit?"true":"false",mConnect->errstr);
        return false;
    }

	//Hset fund userid fund
	std::string ls;
	ls = "fund" ;
	std::string lxml = FundToString(nFundInfo);
	redisReply* lp = (redisReply*)redisCommand(mConnect,"HSET %s %s %s", ls.c_str(), nFundInfo.AccountID,lxml.c_str());
	// printf("SET: %s\n", reply->str);
	if(lp)
        freeReplyObject(lp);
	return true;
}
bool CMemCache::CacheOrder(const PlatformStru_OrderInfo & nOrder)
{
    if(mbSeverQuit)
        return false;

	if(mConnect == NULL || 1==mConnect->err)
    {
        printf("mbSeverQuit = %s, mConnect->errstr = %s",mbSeverQuit?"true":"false",mConnect->errstr);
        return false;
    }

	//Hset o:88002 order.tostring nOrder
	OrderKey lKey(nOrder);
	std::string ls;
	ls = "o:" ;
	ls += nOrder.InvestorID;
	char lKeyBuf[256] = {0};
	lKey.tostring(lKeyBuf,256);
	if( nOrder.OrderStatus == THOST_FTDC_OST_PartTradedQueueing ||                   //部分成交还在队列中 '1'
		nOrder.OrderStatus == THOST_FTDC_OST_NoTradeQueueing ||
		nOrder.OrderStatus == THOST_FTDC_OST_Unknown)	                   //未成交还在队列中 '3'
	{
		std::string lxml = OrderToString(nOrder);
		redisReply* lp = (redisReply*)redisCommand(mConnect,"HSET %s %s %s", ls.c_str(), lKeyBuf,lxml.c_str());
		// printf("SET: %s\n", reply->str);
		if(lp)
            freeReplyObject(lp);
	}
	else
	{
		redisReply* lp = (redisReply*)redisCommand(mConnect,"HDEL %s %s", ls.c_str(), lKeyBuf);
		// printf("SET: %s\n", reply->str);
		if(lp)
            freeReplyObject(lp);
	}

	return true;
}

bool CMemCache::PushQuot(const PlatformStru_DepthMarketData& nQuot)
{
    if(mbSeverQuit)
        return false;

	if(mConnect == NULL || 1==mConnect->err)
    {
        printf("mbSeverQuit = %s, mConnect->errstr = %s",mbSeverQuit?"true":"false",mConnect->errstr);
        return false;
    }

	//Publish q:IF1406 nQuot
	std::string ls;
	ls = "cq:" ;
	ls += nQuot.InstrumentID;
	std::string lxml= QuotToString(nQuot);
	redisReply* lp = (redisReply*)redisCommand(mConnect,"Publish %s  %s", ls.c_str(),lxml.c_str());
	// printf("SET: %s\n", reply->str);
	if(lp)
        freeReplyObject(lp);
	return true;
}
bool CMemCache::PushOrder(const PlatformStru_OrderInfo& nOrder)
{
    if(mbSeverQuit)
        return false;

	if(mConnect == NULL || 1==mConnect->err)
    {
        printf("mbSeverQuit = %s, mConnect->errstr = %s",mbSeverQuit?"true":"false",mConnect->errstr);
        return false;
    }

	//Publish co:888002 nQuot
	std::string ls;
	ls = "co:" ;
	ls += nOrder.InvestorID;
	std::string lxml = OrderToString(nOrder);
	redisReply* lp = (redisReply*)redisCommand(mConnect,"Publish %s %s", ls.c_str(),lxml.c_str());
	// printf("SET: %s\n", reply->str);
    if(lp)
	    freeReplyObject(lp);
	return true;
}
bool CMemCache::PushTrader(const PlatformStru_TradeInfo& nTrader)
{
    if(mbSeverQuit)
        return false;

	if(mConnect == NULL || 1==mConnect->err)
    {
        printf("mbSeverQuit = %s, mConnect->errstr = %s",mbSeverQuit?"true":"false",mConnect->errstr);
        return false;
    }

	//Publish ct:888002 nQuot
	std::string ls;
	ls = "ct:" ;
	ls += nTrader.InvestorID;
	std::string lxml =	TraderToString(nTrader);
	redisReply* lp = (redisReply*)redisCommand(mConnect,"Publish %s %s", ls.c_str(),lxml.c_str());
	// printf("SET: %s\n", reply->str);
    if(lp)
	    freeReplyObject(lp);
	return true;
}
bool CMemCache::PushOrderInputError(const PlatformStru_OrderInfo & nOrder)
{
    if(mbSeverQuit)
        return false;

	if(mConnect == NULL || 1==mConnect->err)
    {
        printf("mbSeverQuit = %s, mConnect->errstr = %s",mbSeverQuit?"true":"false",mConnect->errstr);
        return false;
    }

	//Publish coi:888002 nQuot
	std::string ls;
	ls = "coi:" ;
	ls += nOrder.InvestorID;
	std::string lxml =	OrderInputToString(nOrder);
	redisReply* lp = (redisReply*)redisCommand(mConnect,"Publish %s %s", ls.c_str(),lxml.c_str());
	// printf("SET: %s\n", reply->str);
	if(lp)
        freeReplyObject(lp);
	return true;
}
bool CMemCache::PushOrderActionError(const CThostFtdcOrderActionField & nOrder)
{
    if(mbSeverQuit)
        return false;

	if(mConnect == NULL || 1==mConnect->err)
    {
        printf("mbSeverQuit = %s, mConnect->errstr = %s",mbSeverQuit?"true":"false",mConnect->errstr);
        return false;
    }

	//Publish coa:888002 nQuot
	std::string ls;
	ls = "coa:" ;
	ls += nOrder.InvestorID;
	std::string lxml = OrderActionToString(nOrder);
	redisReply* lp = (redisReply*)redisCommand(mConnect,"Publish %s  %s", ls.c_str(),lxml.c_str());
	// printf("SET: %s\n", reply->str);
	if(lp)
        freeReplyObject(lp);
	return true;
}

bool CMemCache::PushStrategyStopOrRun(const std::string & nsName,bool nsStop)
{
    if(mbSeverQuit)
        return false;

	if(mConnect == NULL || 1==mConnect->err)
    {
        printf("mbSeverQuit = %s, mConnect->errstr = %s",mbSeverQuit?"true":"false",mConnect->errstr);
        return false;
    }

	//hset uol 888002 off
	redisReply* lp1 = (redisReply*)redisCommand(mConnect,"HSET StretegyRun %s %s", nsName.c_str(),nsStop?"off":"on");
	if(lp1)
        freeReplyObject(lp1);


	//Publish sa:Traend stop
	std::string ls;
	ls = "csr:" ;
	ls += nsName;

    std::string szbool = (nsStop?"stop":"run");

    char szValue[128]={0};
    sprintf(szValue,"<SR run=\"%s\" />",szbool.c_str());

	redisReply* lp = (redisReply*)redisCommand(mConnect,"Publish %s %s", ls.c_str(),szValue);
	// printf("SET: %s\n", reply->str);
	if(lp)
        freeReplyObject(lp);
	return true;
}
bool  CMemCache::PushStrategyInstanceStopOrRun(const std::string & nsUserName,const std::string& nStrategyName,bool nsStop)
{
    if(mbSeverQuit)
        return false;

	if(mConnect == NULL || 1==mConnect->err)
    {
        printf("mbSeverQuit = %s, mConnect->errstr = %s",mbSeverQuit?"true":"false",mConnect->errstr);
        return false;
    }

	//hset uol 888002 off
	redisReply* lp1 = (redisReply*)redisCommand(mConnect,"HSET StretegyInstanceRun %s:%s %s", nsUserName.c_str(),
		nStrategyName.c_str(),nsStop?"off":"on");

	if(lp1)
        freeReplyObject(lp1);


	//Publish sa:Traend stop
	std::string ls;
	ls = "csir:" ;
	ls += nsUserName;
	ls += ":" ;
	ls += nStrategyName;

    std::string szbool = (nsStop?"stop":"run");

    char szValue[128]={0};
    sprintf(szValue,"<SIR run=\"%s\" />",szbool.c_str());

	redisReply* lp = (redisReply*)redisCommand(mConnect,"Publish %s %s", ls.c_str(),szValue);
	//printf("SET: %s\n", lp->str);
	if(lp)
        freeReplyObject(lp);
	return true;
}
bool CMemCache::PushStrategyTimer(const std::string & nsName,bool nsStop)
{
    if(mbSeverQuit)
        return false;

    if(mConnect == NULL || 1==mConnect->err)
    {
        printf("mbSeverQuit = %s, mConnect->errstr = %s",mbSeverQuit?"true":"false",mConnect->errstr);
        return false;
    }

    //hset st 888002 off
    redisReply* lp1 = (redisReply*)redisCommand(mConnect,"HSET StrategyTimer %s %s", nsName.c_str(),nsStop?"off":"on");
    if(lp1)
        freeReplyObject(lp1);


    //Publish st:888002 <ST run=off>
    std::string ls;
    ls = "st:" ;
    ls += nsName;

    std::string szbool = (nsStop?"off":"on");

    char szValue[128]={0};
    sprintf(szValue,"<ST run=\"%s\" />",szbool.c_str());

    redisReply* lp = (redisReply*)redisCommand(mConnect,"Publish %s %s", ls.c_str(),szValue);
    // printf("SET: %s\n", reply->str);
    if(lp)
        freeReplyObject(lp);
    return true;
}
// bool CMemCache::PushTraderUseroffLine(const std::string & nUserName,bool bOffline)
// {
// 	if(mConnect == NULL || 1==mConnect->err)
// 		return false;
// 
// 	//hset uol 888002 off
// 	std::string ls;
// 	ls = "OnlineUser" ;
// 	redisReply* lp1 = (redisReply*)redisCommand(mConnect,"HSET %s %s %s", ls.c_str(),nUserName.c_str(),bOffline?"off":"on");
// 	if(lp1)
//         freeReplyObject(lp1);
// 
// 	//if(bOffline)
// 	{
// 		//Publish coa:888002 nQuot
// 		std::string ls;
// 		ls = "cuof:" ;
// 		ls += nUserName;
// 
//         std::string szbool = (bOffline?"off":"on");
// 
//         char szValue[128]={0};
//         sprintf(szValue,"<UR online=\"%s\" />",szbool.c_str());
// 
// 		redisReply* lp = (redisReply*)redisCommand(mConnect,"Publish %s %s", ls.c_str(),szValue);
// 		// printf("SET: %s\n", reply->str);
// 		if(lp)
//             freeReplyObject(lp);
// 	}
// 	return true;
// }


bool CMemCache::SubscibleRead(DealCommandFun pFun)
{
	if(mConnect == NULL || 1==mConnect->err)
    {
        printf("mbSeverQuit = %s, mConnect->errstr = %s",mbSeverQuit?"true":"false",mConnect->errstr);
        return false;
    }

	redisReply*  reply = (redisReply*)redisCommand(mConnect,"SUBSCRIBE OrderInput OrderAction HeartBeat");
	if(reply)
        freeReplyObject(reply);
	while(redisGetReply(mConnect,(void**)&reply) == REDIS_OK)
	{
        if(mbSeverQuit)
            return false;

		printf( "Received[%s] channel %s: %s\n",
			reply->element[0]->str,
			reply->element[1]->str,
			reply->element[2]->str );
		// consume message
		if(reply->element[2]->str)
			pFun(reply->element[2]->str);
		if(reply)
            freeReplyObject(reply);
	}
	return true;
}