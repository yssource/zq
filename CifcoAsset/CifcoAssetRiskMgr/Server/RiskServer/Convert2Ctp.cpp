#include "StdAfx.h"
#include "Convert2Ctp.h"
#ifdef SAFESTRCOPY
#undef SAFESTRCOPY
#endif
#define SAFESTRCOPY(tag)	safestrcpy(ctp.tag,sizeof(risk.tag),risk.tag)
#ifdef VALUEEQU
#undef VALUEEQU
#endif
#define VALUEEQU(tag)		ctp.tag=risk.tag
Convert2Ctp::Convert2Ctp(void)
{
}

Convert2Ctp::~Convert2Ctp(void)
{
}
void Convert2Ctp::GetBizNoticeField(const BizNoticeField& risk,CShfeFtdcBizNoticeField& ctp)	
{	
	///�¼���������
	SAFESTRCOPY(TradingDay);		
	///֪ͨ�¼��ڵ�������
	VALUEEQU(SequenceNo);		
	///����֪ͨ;��
	VALUEEQU(Method);		
	///�¼�����ʱ��
	SAFESTRCOPY(EventTime);		
	///���͹�˾����
	SAFESTRCOPY(BrokerID);		
	///�û�����
	SAFESTRCOPY(UserID);		
	///Ͷ���ߴ���
	SAFESTRCOPY(InvestorID);		
	///ҵ������
	SAFESTRCOPY(BizType);		
	///֪ͨ״̬
	VALUEEQU(Status);		
	///֪ͨ����
	SAFESTRCOPY(Message);		
	///������Ϣ
	SAFESTRCOPY(ErrorMsg);		
}	
void Convert2Ctp::GetBrokerInvestorField(const BrokerInvestorField& risk,CShfeFtdcBrokerInvestorField& ctp)	
{	
	///���͹�˾����
	SAFESTRCOPY(BrokerID);		
	///Ͷ���ߴ���
	SAFESTRCOPY(InvestorID);		
}	
void Convert2Ctp::GetForceCloseListField(const ForceCloseListField& risk,CShfeFtdcForceCloseListField& ctp)	
{	
	///���͹�˾����
	SAFESTRCOPY(BrokerID);		
	///Ͷ���ߴ���
	SAFESTRCOPY(InvestorID);		
}	
void Convert2Ctp::GetForceClosePositionField(const ForceClosePositionField& risk,CShfeFtdcForceClosePositionField& ctp)	
{	
	///��Լ����
	SAFESTRCOPY(InstrumentID);		
	///���͹�˾����
	SAFESTRCOPY(BrokerID);		
	///Ͷ���ߴ���
	SAFESTRCOPY(InvestorID);		
	///�ֲֶ�շ���
	VALUEEQU(PosiDirection);		
	///Ͷ���ױ���־
	VALUEEQU(HedgeFlag);		
	///�ֲ�����
	VALUEEQU(PositionDate);		
	///��ǰ�ֲ�����
	VALUEEQU(Position);		
	///��ǿƽ�ֲ�����
	VALUEEQU(FCPosition);		
	///ǿƽ�۸�����
	VALUEEQU(FCPriceType);		
	///�޼۵�������
	VALUEEQU(PriceTick);		
	///ǿƽ�۸�
	VALUEEQU(FCPrice);		
	///ƽ���ͷŵı�֤��
	VALUEEQU(ReleaseMargin);		
	///ƽ��ӯ��
	VALUEEQU(CloseProfit);		
	///ƽ���ͷŵĽ�������֤��
	VALUEEQU(ExchReleaseMargin);		
}	
void Convert2Ctp::GetForceClosePositionRuleField(const ForceClosePositionRuleField& risk,CShfeFtdcForceClosePositionRuleField& ctp)	
{	
	///��Ʒ���Լ����
	SAFESTRCOPY(ProductInstrumentID);		
	///ǿƽ�ֲַ���
	VALUEEQU(FCPosiDirection);		
	///ǿƽͶ���ױ���־
	VALUEEQU(FCHedgeFlag);		
	///ǿƽ��ϳֱֲ�־
	VALUEEQU(FCCombPosiFlag);		
	///ǿƽ��ʷ�ֲ�˳��
	VALUEEQU(FCHistoryPosiOrder);		
	///ǿƽ�۸�����
	VALUEEQU(FCPrice);		
	///�޼۵�������
	VALUEEQU(PriceTick);		
	///����ǿƽ����������ȼ�
	SAFESTRCOPY(FCRulePriority);		
}	
void Convert2Ctp::GetForceCloseStandardField(const ForceCloseStandardField& risk,CShfeFtdcForceCloseStandardField& ctp)	
{	
	///ǿƽ��׼
	VALUEEQU(ForceCloseLevel);		
	///ǿƽ�ʽ��ͷű�׼
	VALUEEQU(ForceCloseRelease);		
	///��ͣ�巽��ֲ�����
	VALUEEQU(FCNonLimitFirst);		
}	
void Convert2Ctp::GetIndexNPPField(const IndexNPPField& risk,CShfeFtdcIndexNPPField& ctp)	
{	
	///���͹�˾����
	SAFESTRCOPY(BrokerID);		
	///Ͷ���ߴ���
	SAFESTRCOPY(InvestorID);		
	///��Ʒ����
	SAFESTRCOPY(ProductIDs);		
	///����ֵ(�ڲ������Ǿ�ʾֵ,��ָ��FullIndexNPP����ָ��ֵ)
	VALUEEQU(WarnLevel);		
}	
void Convert2Ctp::GetInputOrderActionField(const InputOrderActionField& risk,CShfeFtdcInputOrderActionField& ctp)	
{	
	///���͹�˾����
	SAFESTRCOPY(BrokerID);		
	///Ͷ���ߴ���
	SAFESTRCOPY(InvestorID);		
	///������������
	VALUEEQU(OrderActionRef);		
	///��������
	SAFESTRCOPY(OrderRef);		
	///������
	VALUEEQU(RequestID);		
	///ǰ�ñ��
	VALUEEQU(FrontID);		
	///�Ự���
	VALUEEQU(SessionID);		
	///����������
	SAFESTRCOPY(ExchangeID);		
	///�������
	SAFESTRCOPY(OrderSysID);		
	///������־
	VALUEEQU(ActionFlag);		
	///�۸�
	VALUEEQU(LimitPrice);		
	///�����仯
	VALUEEQU(VolumeChange);		
	///�û�����
	SAFESTRCOPY(UserID);		
	///��Լ����
	SAFESTRCOPY(InstrumentID);		
}	
void Convert2Ctp::GetInstrumentPriceField(const InstrumentPriceField& risk,CShfeFtdcInstrumentPriceField& ctp)	
{	
	///��Լ����
	SAFESTRCOPY(InstrumentID);		
	///�۸�����
	VALUEEQU(PriceType);		
	///�۸�
	VALUEEQU(Price);		
}	
void Convert2Ctp::GetInvestorIDRangeField(const InvestorIDRangeField& risk,CShfeFtdcInvestorIDRangeField& ctp)	
{	
	///Ͷ���ߴ���(Ϊ�մ�������Ͷ���ߴ���)
	SAFESTRCOPY(InvestorIDBeg);		
	///Ͷ���ߴ���(Ϊ�մ�������Ͷ���ߴ���)
	SAFESTRCOPY(InvestorIDEnd);		
}	
void Convert2Ctp::GetInvestorPatternField(const InvestorPatternField& risk,CShfeFtdcInvestorPatternField& ctp)	
{	
	///���͹�˾����
	SAFESTRCOPY(BrokerID);		
	///Ͷ���ߴ���
	SAFESTRCOPY(InvestorID);		
	///ҵ������
	SAFESTRCOPY(BizType);		
	///֪ͨ;��
	VALUEEQU(Method);		
	///ģ�����
	VALUEEQU(PatternID);		
	///�Ƿ�����
	VALUEEQU(IsActive);		
}	
void Convert2Ctp::GetNormalRiskQueryField(const NormalRiskQueryField& risk,CShfeFtdcNormalRiskQueryField& ctp)	
{	
	///���͹�˾����
	SAFESTRCOPY(BrokerID);		
	///�û�����
	SAFESTRCOPY(UserID);		
}	
void Convert2Ctp::GetNoticePatternField(const NoticePatternField& risk,CShfeFtdcNoticePatternField& ctp)	
{	
	///���͹�˾����
	SAFESTRCOPY(BrokerID);		
	///ҵ������
	SAFESTRCOPY(BizType);		
	///֪ͨ;��
	VALUEEQU(Method);		
	///ҵ������
	SAFESTRCOPY(BizName);		
	///����޸�ģ����û�����
	SAFESTRCOPY(UserID);		
	///�Ƿ�����
	VALUEEQU(IsActive);		
	///֪ͨģ������
	SAFESTRCOPY(Pattern);		
}	
void Convert2Ctp::GetNotifySequenceField(const NotifySequenceField& risk,CShfeFtdcNotifySequenceField& ctp)	
{	
	///���
	VALUEEQU(SequenceNo);		
}	
void Convert2Ctp::GetPredictRiskParamField(const PredictRiskParamField& risk,CShfeFtdcPredictRiskParamField& ctp)	
{	
	///D1����
	VALUEEQU(D1);		
	///�Ƿ������Ч��¼
	VALUEEQU(IsFilter);		
}	
void Convert2Ctp::GetPriceRangeField(const PriceRangeField& risk,CShfeFtdcPriceRangeField& ctp)	
{	
	///��Լ����
	SAFESTRCOPY(InstrumentID);		
	///�۸�1
	VALUEEQU(Price1);		
	///�۸�2
	VALUEEQU(Price2);		
}	
void Convert2Ctp::GetPriceVaryParamField(const PriceVaryParamField& risk,CShfeFtdcPriceVaryParamField& ctp)	
{	
	///��Լ���
	SAFESTRCOPY(InstrumentID);		
	///�۸񲨶�����
	VALUEEQU(Direction);		
	///�۸񲨶�����(>=0��Ч)
	VALUEEQU(Pecent);		
	///�۸񲨶��Ļ�׼������
	VALUEEQU(BasePriceType);		
	///�Զ����׼��
	VALUEEQU(BasePrice);		
}	
void Convert2Ctp::GetProductLimitsField(const ProductLimitsField& risk,CShfeFtdcProductLimitsField& ctp)	
{	
	///Ʒ�ֻ��Լ����
	SAFESTRCOPY(ProductID);		
	///D1�ǵ���
	VALUEEQU(Limit1);		
	///D2�ǵ���
	VALUEEQU(Limit2);		
	///D3�ǵ���
	VALUEEQU(Limit3);		
	///D4�ǵ���
	VALUEEQU(Limit4);		
	///���֤����
	VALUEEQU(MaxMarginRate1);		
	///�����
	VALUEEQU(Price);		
}	
void Convert2Ctp::GetQryInstPositionRateField(const QryInstPositionRateField& risk,CShfeFtdcQryInstPositionRateField& ctp)	
{	
	///���͹�˾����
	SAFESTRCOPY(BrokerID);		
	///��ʼ��Լ����
	SAFESTRCOPY(InstIDStart);		
	///������Լ����
	SAFESTRCOPY(InstIDEnd);		
	///ɢ����ֵ����ֲ�������
	VALUEEQU(hbtotal_little);		
	///�л���ֵ����ֲ�������
	VALUEEQU(hbtotal_medium);		
	///ɢ����ֵ�����ֲ�������
	VALUEEQU(hstotal_little);		
	///�л���ֵ�����ֲ�������
	VALUEEQU(hstotal_medium);		
	///ɢ����ֵ�ֲ�������
	VALUEEQU(htotal_little);		
	///�л���ֵ�ֲ�������
	VALUEEQU(htotal_medium);		
	///ɢ��Ͷ������ֲ�������
	VALUEEQU(sbtotal_little);		
	///�л�Ͷ������ֲ�������
	VALUEEQU(sbtotal_medium);		
	///ɢ��Ͷ�������ֲ�������
	VALUEEQU(sstotal_little);		
	///�л�Ͷ�������ֲ�������
	VALUEEQU(sstotal_medium);		
	///ɢ��Ͷ���ֲ�������
	VALUEEQU(stotal_little);		
	///�л�Ͷ���ֲ�������
	VALUEEQU(stotal_medium);		
	///ɢ������ֲ�������
	VALUEEQU(buytotal_little);		
	///�л�����ֲ�������
	VALUEEQU(buytotal_medium);		
	///ɢ�������ֲ�������
	VALUEEQU(selltotal_little);		
	///�л������ֲ�������
	VALUEEQU(selltotal_medium);		
	///ɢ���ֲܳ�������
	VALUEEQU(total_little);		
	///�л��ֲܳ�������
	VALUEEQU(total_medium);		
	///ȡֵ��ʽ
	VALUEEQU(ValueMode);		
}	
void Convert2Ctp::GetQryInvestorMarginRateField(const QryInvestorMarginRateField& risk,CShfeFtdcQryInvestorMarginRateField& ctp)	
{	
	///���͹�˾����
	SAFESTRCOPY(BrokerID);		
	///Ͷ���ߴ���
	SAFESTRCOPY(InvestorID);		
	///��Լ����
	SAFESTRCOPY(InstrumentID);		
	///Ͷ���ױ���־
	VALUEEQU(HedgeFlag);		
}	
void Convert2Ctp::GetQryPriceVaryEffectField(const QryPriceVaryEffectField& risk,CShfeFtdcQryPriceVaryEffectField& ctp)	
{	
	///���͹�˾����
	SAFESTRCOPY(BrokerID);		
	///Ͷ���ߴ���
	SAFESTRCOPY(InvestorID);		
	///��������
	VALUEEQU(RiskLevel);		
}	
void Convert2Ctp::GetQryProductPositionRateField(const QryProductPositionRateField& risk,CShfeFtdcQryProductPositionRateField& ctp)	
{	
	///���͹�˾����
	SAFESTRCOPY(BrokerID);		
	///��Ʒ����
	SAFESTRCOPY(ProductID);		
	///ɢ����ֵ����ֲ�������
	VALUEEQU(hbtotal_little);		
	///�л���ֵ����ֲ�������
	VALUEEQU(hbtotal_medium);		
	///ɢ����ֵ�����ֲ�������
	VALUEEQU(hstotal_little);		
	///�л���ֵ�����ֲ�������
	VALUEEQU(hstotal_medium);		
	///ɢ����ֵ�ֲ�������
	VALUEEQU(htotal_little);		
	///�л���ֵ�ֲ�������
	VALUEEQU(htotal_medium);		
	///ɢ��Ͷ������ֲ�������
	VALUEEQU(sbtotal_little);		
	///�л�Ͷ������ֲ�������
	VALUEEQU(sbtotal_medium);		
	///ɢ��Ͷ�������ֲ�������
	VALUEEQU(sstotal_little);		
	///�л�Ͷ�������ֲ�������
	VALUEEQU(sstotal_medium);		
	///ɢ��Ͷ���ֲ�������
	VALUEEQU(stotal_little);		
	///�л�Ͷ���ֲ�������
	VALUEEQU(stotal_medium);		
	///ɢ������ֲ�������
	VALUEEQU(buytotal_little);		
	///�л�����ֲ�������
	VALUEEQU(buytotal_medium);		
	///ɢ�������ֲ�������
	VALUEEQU(selltotal_little);		
	///�л������ֲ�������
	VALUEEQU(selltotal_medium);		
	///ɢ���ֲܳ�������
	VALUEEQU(total_little);		
	///�л��ֲܳ�������
	VALUEEQU(total_medium);		
	///ȡֵ��ʽ
	VALUEEQU(ValueMode);		
}	
void Convert2Ctp::GetQrySafePriceRangeField(const QrySafePriceRangeField& risk,CShfeFtdcQrySafePriceRangeField& ctp)	
{	
	///���͹�˾����
	SAFESTRCOPY(BrokerID);		
	///Ͷ���ߴ���
	SAFESTRCOPY(InvestorID);		
	///��Լ�۸񲨶�����
	VALUEEQU(PriceVaryAlgo);		
	///�۸񲨶��Ļ�׼������(v5.1.2֮�����)
	VALUEEQU(BasePriceType);		
	///��������
	VALUEEQU(RiskLevel);		
	///����Լ˳�򲨶�ʱ���������ͣ�����
	VALUEEQU(MaxLimitDay);		
}	
void Convert2Ctp::GetQryStatField(const QryStatField& risk,CShfeFtdcQryStatField& ctp)	
{	
	///���͹�˾����
	SAFESTRCOPY(BrokerID);		
	///������Ʒ�ֺ�Լ�������(����������ʹ��ǰ׺e:����ʽ��cu,e:SHFE,cu1105)
	SAFESTRCOPY(ExchangeProductInstID);		
	///��������
	VALUEEQU(SortType);		
	///�����������ؽ��
	VALUEEQU(ResultCount);		
	///���������ؽ��
	VALUEEQU(ResultRatio);		
}	
void Convert2Ctp::GetQueryBrokerDepositField(const QueryBrokerDepositField& risk,CShfeFtdcQueryBrokerDepositField& ctp)	
{	
	///���͹�˾����
	SAFESTRCOPY(BrokerID);		
	///����������
	SAFESTRCOPY(ExchangeID);		
}	
void Convert2Ctp::GetRemoveRiskParkedOrderField(const RemoveRiskParkedOrderField& risk,CShfeFtdcRemoveRiskParkedOrderField& ctp)	
{	
	///���͹�˾����
	SAFESTRCOPY(BrokerID);		
	///�û�����
	SAFESTRCOPY(UserID);		
	///Ͷ���ߴ���
	SAFESTRCOPY(InvestorID);		
	///Ԥ�񱨵����
	SAFESTRCOPY(ParkedOrderID);		
}	
void Convert2Ctp::GetReqRiskUserLoginField(const ReqRiskUserLoginField& risk,CShfeFtdcReqRiskUserLoginField& ctp)	
{	
	///���͹�˾����
	SAFESTRCOPY(BrokerID);		
	///�û�����
	SAFESTRCOPY(UserID);		
	///����
	SAFESTRCOPY(Password);		
	///�ͻ��˰汾,20091230��Version=1,֮ǰ�汾Version=0
	VALUEEQU(Version);		
	///����ǰ���пͻ������ӵ�SessionID
	VALUEEQU(LocalSessionID);		
}	
void Convert2Ctp::GetRiskForceCloseOrderField(const RiskForceCloseOrderField& risk,CShfeFtdcRiskForceCloseOrderField& ctp)	
{	
	///���ǿƽ����
	VALUEEQU(FCType);		
	///����ǿƽ��������ʱ��
	SAFESTRCOPY(Time1);		
	///����ǿƽ��������ʱ�䣨���룩
	VALUEEQU(Millisec1);		
	///ǿƽ�����ύʱ��
	SAFESTRCOPY(Time2);		
	///ǿƽ�����ύʱ�䣨���룩
	VALUEEQU(Millisec2);		
	///ǿƽ�������
	SAFESTRCOPY(FCSceneId);		
	///���͹�˾����
	SAFESTRCOPY(BrokerID);		
	///Ͷ���ߴ���
	SAFESTRCOPY(InvestorID);		
	///��Լ����
	SAFESTRCOPY(InstrumentID);		
	///��������
	SAFESTRCOPY(OrderRef);		
	///�û�����
	SAFESTRCOPY(UserID);		
	///�����۸�����
	VALUEEQU(OrderPriceType);		
	///��������
	VALUEEQU(Direction);		
	///��Ͽ�ƽ��־
	SAFESTRCOPY(CombOffsetFlag);		
	///���Ͷ���ױ���־
	SAFESTRCOPY(CombHedgeFlag);		
	///�۸�
	VALUEEQU(LimitPrice);		
	///����
	VALUEEQU(VolumeTotalOriginal);		
	///��Ч������
	VALUEEQU(TimeCondition);		
	///GTD����
	SAFESTRCOPY(GTDDate);		
	///�ɽ�������
	VALUEEQU(VolumeCondition);		
	///��С�ɽ���
	VALUEEQU(MinVolume);		
	///��������
	VALUEEQU(ContingentCondition);		
	///ֹ���
	VALUEEQU(StopPrice);		
	///ǿƽԭ��
	VALUEEQU(ForceCloseReason);		
	///�Զ������־
	VALUEEQU(IsAutoSuspend);		
	///ҵ��Ԫ
	SAFESTRCOPY(BusinessUnit);		
	///������
	VALUEEQU(RequestID);		
	///�û�ǿ����־
	VALUEEQU(UserForceClose);		
	///ǰ�ñ��
	VALUEEQU(FrontID);		
	///�Ự���
	VALUEEQU(SessionID);		
}	
void Convert2Ctp::GetRiskInvestorParamField(RiskInvestorParamField& risk,CShfeFtdcRiskInvestorParamField& ctp)	
{	
	///��������
	VALUEEQU(ParamID);		
	///���͹�˾����
	SAFESTRCOPY(BrokerID);		
	///Ͷ���ߴ���
	SAFESTRCOPY(InvestorID);		
	///����ֵ
	SAFESTRCOPY(ParamValue);		
}	
void Convert2Ctp::GetRiskLoginInfoField(RiskLoginInfoField& risk,CShfeFtdcRiskLoginInfoField& ctp)	
{	
	///���͹�˾����
	SAFESTRCOPY(BrokerID);		
	///�û�����
	SAFESTRCOPY(UserID);		
	///����ǰ�ûỰ���
	VALUEEQU(LocalSessionID);		
	///���ǰ�ûỰ���
	VALUEEQU(SessionID);		
	///���ǰ�ñ��
	VALUEEQU(FrontID);		
}	
void Convert2Ctp::GetRiskPatternField(RiskPatternField& risk,CShfeFtdcRiskPatternField& ctp)	
{	
	///���͹�˾����
	SAFESTRCOPY(BrokerID);		
	///ҵ������
	SAFESTRCOPY(BizType);		
	///ģ�����
	VALUEEQU(PatternID);		
	///ģ������
	SAFESTRCOPY(PatternName);		
	///֪ͨģ������
	SAFESTRCOPY(Pattern);		
}	
void Convert2Ctp::GetSTPriceField(STPriceField& risk,CShfeFtdcSTPriceField& ctp)	
{	
	///���͹�˾����
	SAFESTRCOPY(BrokerID);		
	///Ͷ���߷�Χ
	VALUEEQU(InvestorRange);		
	///Ͷ���ߴ����ģ�����
	SAFESTRCOPY(InvestorID);		
	///��Լ����
	SAFESTRCOPY(InstrumentID);		
	///�۸�����
	VALUEEQU(PriceType);		
	///�۸�
	VALUEEQU(Price);		
}	
void Convert2Ctp::GetRiskNotifyAField(const RiskNotifyAField& risk,CShfeFtdcRiskNotifyAField& ctp)	
{	
	///����֪ͨ�¼��ڵ�������
	VALUEEQU(SequenceNo);		
	///�¼���������
	SAFESTRCOPY(EventDate);		
	///�¼�����ʱ��
	SAFESTRCOPY(EventTime);		
	///���͹�˾����
	SAFESTRCOPY(BrokerID);		
	///�����͸÷���֪ͨ���û�����
	SAFESTRCOPY(UserID);		
	///Ͷ���ߴ���
	SAFESTRCOPY(InvestorID);		
	///֪ͨ����
	VALUEEQU(NotifyClass);		
	///����֪ͨ;��
	VALUEEQU(NotifyMethod);		
	///����֪ͨ״̬
	VALUEEQU(NotifyStatus);		
	///֪ͨ����
	SAFESTRCOPY(Message);		
	///Ԥ���ֶ�(�˹�֪ͨ-֪ͨ����Ա������Ϊ����ԭ��)
	SAFESTRCOPY(Reserve);		
}	
void Convert2Ctp::GetRiskNotifyCommandField(const RiskNotifyCommandField& risk,CShfeFtdcRiskNotifyCommandField& ctp)	
{	
	///���͹�˾����
	SAFESTRCOPY(BrokerID);		
	///�û�����
	SAFESTRCOPY(UserID);		
	///֪ͨ����
	VALUEEQU(NotifyClass);		
	///Ͷ���ߴ���
	SAFESTRCOPY(InvestorID);		
	///�Ƿ���ϵͳ֪ͨ
	VALUEEQU(IsAutoSystem);		
	///�Ƿ��Ͷ���֪ͨ
	VALUEEQU(IsAutoSMS);		
	///�Ƿ����ʼ�֪ͨ
	VALUEEQU(IsAutoEmail);		
	///�����ֶ�
	SAFESTRCOPY(Reserve);		
	///֪ͨģ������
	SAFESTRCOPY(Pattern);		
	///�Ƿ�������������֪ͨ
	VALUEEQU(IsNormal);		
	///�Ƿ��������;�ʾ֪ͨ
	VALUEEQU(IsWarn);		
}	
void Convert2Ctp::GetRiskNtfSequenceField(const RiskNtfSequenceField& risk,CShfeFtdcRiskNtfSequenceField& ctp)	
{	
	///���
	VALUEEQU(SequenceNo);		
	///ҵ����������
	SAFESTRCOPY(DataType);		
}	
void Convert2Ctp::GetRiskParkedOrderField(const RiskParkedOrderField& risk,CShfeFtdcRiskParkedOrderField& ctp)	
{	
	///Ԥ�񱨵����
	SAFESTRCOPY(ParkedOrderID);		
	///Ԥ�񵥱��ر��
	SAFESTRCOPY(LocalID);		
	///����û�����
	VALUEEQU(UserType);		
	///Ԥ��״̬
	VALUEEQU(Status);		
	///Ԥ��״̬��Ϣ
	SAFESTRCOPY(StatusMsg);		
	///��������
	VALUEEQU(TriggerType);		
	///���׽׶�
	VALUEEQU(TradeSegment);		
	///����������
	SAFESTRCOPY(ExchangeID);		
	///���ǿƽ����
	VALUEEQU(FCType);		
	///����ǿƽ��������ʱ��
	SAFESTRCOPY(Time1);		
	///����ǿƽ��������ʱ�䣨���룩
	VALUEEQU(Millisec1);		
	///ǿƽ�����ύʱ��
	SAFESTRCOPY(Time2);		
	///ǿƽ�����ύʱ�䣨���룩
	VALUEEQU(Millisec2);		
	///ǿƽ�������
	SAFESTRCOPY(FCSceneId);		
	///���͹�˾����
	SAFESTRCOPY(BrokerID);		
	///Ͷ���ߴ���
	SAFESTRCOPY(InvestorID);		
	///��Լ����
	SAFESTRCOPY(InstrumentID);		
	///��������
	SAFESTRCOPY(OrderRef);		
	///�û�����
	SAFESTRCOPY(UserID);		
	///�����۸�����
	VALUEEQU(OrderPriceType);		
	///��������
	VALUEEQU(Direction);		
	///��Ͽ�ƽ��־
	SAFESTRCOPY(CombOffsetFlag);		
	///���Ͷ���ױ���־
	SAFESTRCOPY(CombHedgeFlag);		
	///�۸�
	VALUEEQU(LimitPrice);		
	///����
	VALUEEQU(VolumeTotalOriginal);		
	///��Ч������
	VALUEEQU(TimeCondition);		
	///GTD����
	SAFESTRCOPY(GTDDate);		
	///�ɽ�������
	VALUEEQU(VolumeCondition);		
	///��С�ɽ���
	VALUEEQU(MinVolume);		
	///��������
	VALUEEQU(ContingentCondition);		
	///ֹ���
	VALUEEQU(StopPrice);		
	///ǿƽԭ��
	VALUEEQU(ForceCloseReason);		
	///�Զ������־
	VALUEEQU(IsAutoSuspend);		
	///ҵ��Ԫ
	SAFESTRCOPY(BusinessUnit);		
	///������
	VALUEEQU(RequestID);		
	///�û�ǿ����־
	VALUEEQU(UserForceClose);		
	///�����ύ״̬
	VALUEEQU(OrderSubmitStatus);		
	///����״̬
	VALUEEQU(OrderStatus);		
	///����״̬��Ϣ
	SAFESTRCOPY(OrderStatusMsg);		
	///�������
	VALUEEQU(ErrorID);		
	///������Ϣ
	SAFESTRCOPY(ErrorMsg);		
	///Ԥ��ʱ��
	SAFESTRCOPY(ParkedTime);		
	///Ԥ����
	VALUEEQU(OriginalParkedVol);		
	///Ԥ��ʱ��ƽ��
	VALUEEQU(MaxCloseVol1);		
	///����ʱ��ƽ��
	VALUEEQU(MaxCloseVol2);		
	///Ԥ��ʱ׷��
	VALUEEQU(Call1);		
	///����ʱ׷��
	VALUEEQU(Call2);		
	///Ԥ��ʱ�����
	VALUEEQU(MoneyIO1);		
	///����ʱ�����
	VALUEEQU(MoneyIO2);		
	///ɾ��ԭ��
	SAFESTRCOPY(DeleteReason);		
	///ǿƽ�ʽ��ͷű�׼
	VALUEEQU(ForceCloseRelease);		
}	
void Convert2Ctp::GetRiskUserEventField(const RiskUserEventField& risk,CShfeFtdcRiskUserEventField& ctp)	
{	
	///�û��¼���������
	VALUEEQU(SequenceNo);		
	///�¼���������
	SAFESTRCOPY(EventDate);		
	///�¼�����ʱ��
	SAFESTRCOPY(EventTime);		
	///���͹�˾����
	SAFESTRCOPY(BrokerID);		
	///�û�����
	SAFESTRCOPY(UserID);		
	///�¼�����
	VALUEEQU(EventType);		
	///�¼���Ϣ
	SAFESTRCOPY(EventInfo);		
}	
void  Convert2Ctp::GetSTMarginRateField(const STMarginRateField& risk,CShfeFtdcSTMarginRateField& ctp)	
{	
	///��Լ����
	SAFESTRCOPY(InstrumentID);		
	///Ͷ���߷�Χ
	VALUEEQU(InvestorRange);		
	///���͹�˾����
	SAFESTRCOPY(BrokerID);		
	///Ͷ���ߴ���
	SAFESTRCOPY(InvestorID);		
	///Ͷ���ױ���־
	VALUEEQU(HedgeFlag);		
	///��ͷ��֤����
	VALUEEQU(LongMarginRatioByMoney);		
	///��ͷ��֤���
	VALUEEQU(LongMarginRatioByVolume);		
	///��ͷ��֤����
	VALUEEQU(ShortMarginRatioByMoney);		
	///��ͷ��֤���
	VALUEEQU(ShortMarginRatioByVolume);		
}	
void Convert2Ctp::GetSubMarketDataField(const SubMarketDataField& risk,CShfeFtdcSubMarketDataField& ctp)	
{	
	///��Լ���
	SAFESTRCOPY(InstrumentID);		
}	
void Convert2Ctp::GetUserPasswordUpdateField(const UserPasswordUpdateField& risk,CShfeFtdcUserPasswordUpdateField& ctp)	
{	
	///���͹�˾����
	SAFESTRCOPY(BrokerID);		
	///�û�����
	SAFESTRCOPY(UserID);		
	///ԭ���Ŀ���
	SAFESTRCOPY(OldPassword);		
	///�µĿ���
	SAFESTRCOPY(NewPassword);		
}	