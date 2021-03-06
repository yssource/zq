
#pragma once
#include "CommonErrorCode.h"
#include "KernelStruct.h"
#include "PlatformDataMgr_Instruments.h"
#include "PlatformDataMgr_Quots.h"
#include "PlatformDataMgr_Rates.h"
#include "CommonStruct.h"



class IInterfaceAccountOp
{
public:	
	
	virtual CPlatformDataMgr_Rates* GetPlatFormRates()= 0;
public:

	//设置行情
	virtual void SetPlatFormQutos(CPlatformDataMgr_Quots * npQutos)= 0;

	//设置合约
	virtual void SetPlatFormInstrument(CPlatformDataMgr_Instruments * npInstrument)= 0;
	///设置合约保证金率
	virtual  void SetMarginRate(const std::string& InstrumentID,
		PlatformStru_InstrumentMarginRate& outData) = 0;

	virtual int GetMarginRate(const std::string& InstrumentID, 
		PlatformStru_InstrumentMarginRate& outData) = 0;

	///设置合约手续费率
	virtual void SetCommissionRate(const std::string& InstrumentID, 
		PlatformStru_InstrumentCommissionRate& outData) = 0;

	virtual int GetCommissionRate(const std::string& InstrumentID, 
		PlatformStru_InstrumentCommissionRate& outData) = 0;

	virtual eCommissionMarginReady GetCommissonMarginReady() = 0;

	virtual void SetCommissionMarginReady(const eCommissionMarginReady & nReady) = 0;

public://行情操作
	virtual bool CalulateByNewQuto(const PlatformStru_DepthMarketData & nQutosData,bool nbSettlement) = 0;

public: //报单操作

	//查询报单的到来
	virtual CF_ERROR QryOrderReach(PlatformStru_OrderInfo& OrderInfo,
		const int ErrID,
		const bool bIsLast) = 0;

	//追加一个报单
	virtual CF_ERROR PushOrder( PlatformStru_OrderInfo & nOrder) = 0;

	///获取指定报单
	virtual CF_ERROR GetOrderByKey(const OrderKey &nOrderKey,
		PlatformStru_OrderInfo& outData) = 0;

	///获取全部报单
	virtual CF_ERROR GetAllOrders(std::vector<PlatformStru_OrderInfo>& outData) = 0;

	/////获取全部已触发的报单
	virtual CF_ERROR GetTriggerOrders(std::vector<PlatformStru_OrderInfo>& outData) = 0;

	/////获取已成交报单
	virtual CF_ERROR GetTradedOrders(std::vector<PlatformStru_OrderInfo>& outData) = 0;

	/////获取已撤单和错误报单
	virtual CF_ERROR GetCanceledOrders(std::vector<PlatformStru_OrderInfo>& outData) = 0;

	/////获取未成交报单
	virtual CF_ERROR GetWaitOrders(std::vector<PlatformStru_OrderInfo>& outData) = 0;


	/////获取未知状态的报单
	virtual CF_ERROR GetUnkownOrders(std::vector<PlatformStru_OrderInfo>& outData) = 0;

public:// 成交操作
	//查询成交的到来
	virtual CF_ERROR QryTradeReach(PlatformStru_TradeInfo& OrderInfo,
		const int ErrID,
		const bool bIsLast) = 0;

	//追加一个成交
	virtual CF_ERROR PushTrader(PlatformStru_TradeInfo & nTrader) = 0;

	//获取全部成交单，返回数据项的数量
	virtual CF_ERROR GetAllTrades(std::vector<PlatformStru_TradeInfo>& outData) = 0;


	//测试成交是否已存在
	virtual bool TestTradeInfoExist(const TradeKey &key) = 0;

public://持仓汇总操作

	//日初查询持仓的到来
	virtual CF_ERROR  QryPositionReach( 
		PlatformStru_Position& PositionInfo,
		int ErrID,
		bool bIsLast) = 0;
	//获取持仓汇总，返回数据项的数量
	virtual CF_ERROR  GetPositions(
		const std::string & nInstrumentID,
		std::vector<PlatformStru_Position>& outData,
		bool bNeedZeroVolumePosition = false) = 0 ;


	//判断是否有持仓
	virtual bool HasPositions() = 0;

public: //持仓明细汇总

	//日初查询持仓明细的到来
	virtual CF_ERROR  QryPositionDetailReach( 
		PlatformStru_PositionDetail& PositionDetailInfo,
		int ErrID,
		bool bIsLast) = 0;
	//获取持仓明细，返回数据项的数量
	virtual CF_ERROR  GetPositionDetail(
		const std::string & nInstrumentID,
		std::vector<PlatformStru_PositionDetail>& outData) = 0;

	//获取持仓明细，返回数据项的数量
	virtual CF_ERROR  GetPositionDetail(const PositionDetailKey & nKey,
		PlatformStru_PositionDetail& outData) = 0;


	//获取某个时间段的持仓明细
	virtual CF_ERROR  GetPositionDetail(int nLowtime,int nUpperTime,
		std::vector<PlatformStru_PositionDetail>& outData)  = 0;


	
public: //资金操作
	virtual CF_ERROR SetQryFundInfo(const PlatformStru_TradingAccountInfo& inData) =0 ;
	//获取交易员资金信息
	virtual CF_ERROR  GetFundInfo(PlatformStru_TradingAccountInfo& outData) = 0;

public:

	//设置用户的CTP链接状态
	virtual CF_ERROR SetAccountCTPStatus(const EnumAccountStatus & nStatus) = 0;

	//获取用户的CTP链接状态
	//获取用户的CTP链接状态
	//nDiffTime 返回进入该状态的时间
	virtual EnumAccountStatus GetAccountCTPStatus(double & nDifftime) = 0; 	
	//获取用户初始化状态
	virtual eUserInitStatus GetAccountInitStatus() = 0;

	//设置用户初始化状态
	virtual void SetAccountInitStatus(eUserInitStatus neStatus) = 0;
	virtual void SetEqualAccountInitStatus(eUserInitStatus neStatus) = 0;

	virtual void clear() = 0;

public://汇总操作
	//获取所有的持仓明细
	virtual CF_ERROR  GetAllPositionDetail(
		std::vector<PlatformStru_PositionDetail> & outData) = 0;

	//获取所有的持仓
	virtual CF_ERROR  GetAllPosition(
		std::vector<PlatformStru_Position>& outData) = 0;

public:
	//清楚所有交易数据
	virtual void ClearAll() = 0;

	//清楚持仓，持仓明细，资金和成交的交易数据
	virtual void ClearTraderAndPosition() = 0;


public:
	//结算时将所有的未成交，部分成交，待审核的报单全部改为已撤单
	virtual void CancelAllUnkonwnOrders() = 0;

	//查询完成后，更新所有挂单对应的持仓冻结项
	virtual void UpdateQryWaitOrderPosition() = 0;

public:
	virtual void SetLoadOrderUpdateSeq(int nSeq)= 0;
	virtual void SetLoadTraderUpdateSeq(int nSeq)= 0;

	virtual void GetLoadOrderUpdateSeq(int& nSeq)= 0;
	virtual void GetLoadTraderUpdateSeq(int& nSeq)= 0;

	virtual void GetOrderAndTradeGeSeq( int nOrderSeq,
		int nTraderSeq,
		std::vector<PlatformStru_OrderInfo> & nvecOrder,
		std::vector<PlatformStru_TradeInfo> & nvecTrader) = 0;

	virtual bool IsSyncFinished()  = 0;
	virtual void SetSyncFinished(bool bVal) = 0;

};