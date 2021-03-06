#include "stdafx.h"

#include <time.h>
#include "ServiceProxy.hpp"
#include "DataCenter.hpp"
#include "Module-Misc2/packagecache.h"
#include "Module-Misc2/SimpleWriteLog.h"
#include "PlatFormService.hpp"
#include "Module-Misc2/globalfunc.h"
#include "ISvr/IPlatformSingleSvr.h"
#include "ISvr/PlatformSvrMgr.h"
#include "Module-Misc2/GlobalConfigMgr.h"
#include "Module-Misc2/globalDefines.h"
#include "ReqOnRtnTrade.h"
#include "Tools_Win32.h"
#include "popmsgwin.h"
#include "easytime.h"
#include "tools_tools.h"

extern PlatformSvrMgr* g_pPlatformMgr;

//定义是否将BusinessData数据包存入文件，以备回放进行调试
//#ifdef _DEBUG
//#define SAVEBUSINESSDATA
//bool g_SaveBusinessData=true;              //是否保存底层业务数据
//bool g_PlayBusinessData=false;              //是否回放底层业务数据
//unsigned int g_PlayTimeDuration=3600*4; //回放多长时间的业务数据，单位为秒。表示回放当前时间减去这个值的时间以后的数据
//#endif

#define LOG_INFO(fmt, ...) \
    do{\
		if(m_pWriteLog)\
		{\
		m_pWriteLog->WriteLog_Fmt("CDataCenter", LOGLEVEL_DEBUGINFO, "[%s][%d]:"fmt, __CUSTOM_FILE__, __LINE__, __VA_ARGS__);\
		}\
    }while(0)

#define LOG_INFO2(fmt, ...) \
    do{\
		if(m_DataCenter.m_pWriteLog)\
		{\
		m_DataCenter.m_pWriteLog->WriteLog_Fmt("CDataCenter", LOGLEVEL_DEBUGINFO, "[%s][%d]:"fmt, __CUSTOM_FILE__, __LINE__, __VA_ARGS__);\
		}\
    }while(0)


#define INIT_OUTDATA(structname)\
    structname field;\
    memset(&field,0,sizeof(structname));\
    outData=field

int g_LockCount=0;
char LastFunction1[256]={0};
char LastFunction2[256]={0};
char LastFunction3[256]={0};
char LastFunction4[256]={0};
//#define MUTEX_LOCK()\
//	g_LockCount=0;\
//	while(m_mutex.TryLock()!=wxMUTEX_NO_ERROR)\
//	{\
//		g_LockCount++;\
//		if(g_LockCount==1000)\
//		{\
//				LOG_INFO("Lock Failed at (%s)--->(%s)--->(%s)--->(%s)--->(%s)",LastFunction1,LastFunction2,LastFunction3,LastFunction4,__FUNCTION__);\
//		}\
//		wxThread::Sleep(10);\
//	}\
//    m_mutex.Unlock();\
//	strcpy(LastFunction1,LastFunction2);\
//	strcpy(LastFunction2,LastFunction3);\
//	strcpy(LastFunction3,LastFunction4);\
//    strcpy(LastFunction4,__FUNCTION__);\
//    wxMutexLocker lock(m_mutex)


class CDataCenterWorkThread : public wxThread
{
public:
    CDataCenterWorkThread(CDataCenter& dc)
        :wxThread(),m_DataCenter(dc)
    {

    };

    // thread execution starts here
    virtual void *Entry()
    {
        char Buf[512];
        sprintf(Buf,"CDataCenter::ThreadId=%u\n", GetCurrentThreadId());
        OutputDebugString(Buf);

        CPackageCache& RefPackageCache=m_DataCenter.m_dataSource;
        char readBuf[10240];
        PackageHeadStru *pPackageHead=(PackageHeadStru *)readBuf;
        char *pData= readBuf+ sizeof(PackageHeadStru);

        unsigned int BeginTickCount=GetTickCount();
        unsigned int StartInitQryTickCount=0;

        while(!TestDestroy())
        {
            //调试时，回放businessdata.rec中的业务包
            unsigned int CurTickCount=GetTickCount();
            if(GlobalConfigManager::m_Test_bPlayBusinessData)
            {
                if(!StartInitQryTickCount&&m_DataCenter.m_PlatFormService->m_rServiceProxy.m_bInitQryStart)
                    StartInitQryTickCount=CurTickCount;

                if(StartInitQryTickCount>0&&
                    CurTickCount-StartInitQryTickCount>=3000/*&&
                    m_DataCenter.m_PlatFormService->m_rServiceProxy.m_bInitQryStart*/)
                {
                    static CPopMsgWin *pmsgwin1=NULL;
                    static CPopMsgWin *pmsgwin2=NULL;

                    if(!pmsgwin1||!pmsgwin2)
                    {
                        CPopMsgWin::Stru_Param msgwinpam;
                        strcpy(msgwinpam.szTitle,"OriPackage");
                        msgwinpam.MaxMsgLen=655360;
                        msgwinpam.InitPox=10;
                        msgwinpam.InitWid=400;
                        msgwinpam.InitHei=100;
                        pmsgwin1=new CPopMsgWin(msgwinpam);

                        strcpy(msgwinpam.szTitle,"PlaybackPackage");
                        msgwinpam.InitPox+=msgwinpam.InitWid;
                        pmsgwin2=new CPopMsgWin(msgwinpam);

                        while(!pmsgwin1->IsWndCreated()) Sleep(10);
                        while(!pmsgwin2->IsWndCreated()) Sleep(10);
                    }

                    char filename[256];
                    memset(filename,0,sizeof(filename));
                    CTools_Win32::GetMainPath(filename,sizeof(filename));
                    int len=strlen(filename);
                    strncpy(filename+len,"\\businessdata.rec",sizeof(filename)-1-len);

                    static char readBuf2[10240];
                    PackageHeadStru *pPackageHead2=(PackageHeadStru *)readBuf2;
                    char *pData2= readBuf2+ sizeof(PackageHeadStru);

                    HFILE hF;
                    char buf[512];

                    static bool bFirst=true;
                    static int RecNum=0;
                    if(bFirst)
                    {
                        bFirst=false;

                        //用于过滤掉不必要的行情
                        map<string,DataRtnDepthMarketData> mapDepthMarketData;

                        hF=_lopen(filename,OF_READ);
                        if(hF!=HFILE_ERROR)
                        {
                            int filelen=_llseek(hF,0,2);

                            //程序启动时，把businessdata.rec里的业务包内容显示到popmsgwin里去
                            int off=0;
                            _llseek(hF,0,0);
                            while(off<filelen)
                            {
                                _lread(hF,pPackageHead2,sizeof(PackageHeadStru));
                                _lread(hF,pData2,pPackageHead2->DataLen);

                                bool bFilter=false;
                                if(((AbstractBusinessData*)pData2)->BID==BID_RtnDepthMarketData)
                                {
                                    //过滤掉不必要的行情
                                    DataRtnDepthMarketData& MarketBusiness= *((DataRtnDepthMarketData*)pData2);
                                    string strInst=string(MarketBusiness.DepthMarketDataField.InstrumentID);
                                    map<string,DataRtnDepthMarketData>::const_iterator it=mapDepthMarketData.find(strInst);
                                    if(it!=mapDepthMarketData.end()&&
                                        it->second.DepthMarketDataField.LastPrice==MarketBusiness.DepthMarketDataField.LastPrice&&
                                        it->second.DepthMarketDataField.SettlementPrice==MarketBusiness.DepthMarketDataField.SettlementPrice&&
                                        it->second.DepthMarketDataField.PreSettlementPrice==MarketBusiness.DepthMarketDataField.PreSettlementPrice)
                                    {
                                        bFilter=true;
                                    }
                                    if(!bFilter) mapDepthMarketData[strInst]=MarketBusiness;
                                }

                                if(!bFilter)
                                {
                                    sprintf(buf,"%d: ",RecNum);
                                    CEasyTime((time_t)pPackageHead2->dwCRC).getstrshorttime(buf+strlen(buf),sizeof(buf)-strlen(buf));
                                    sprintf(buf+strlen(buf),":%s",CTools::BusinessID2String((int)(((AbstractBusinessData*)pData2)->BID)));
                                    pmsgwin1->AppendStr(CPopMsgWin::ForceShow,CPopMsgWin::NoDelOldContent,CPopMsgWin::AddCRLF,CPopMsgWin::NoInsertTime,buf);
                                }
                                RecNum++;
                                off+=sizeof(PackageHeadStru)+pPackageHead2->DataLen;
                            }
                            _lclose(hF);
                        }
                    }

                    //回放序号为StartNo-EndNo的业务包(StartNo <= No < EndNo)
                    static int StartNo=0;
                    static int EndNo=20000;
                    //EndNo=RecNum;
                    if(EndNo>StartNo)
                    {
                        hF=_lopen(filename,OF_READ);
                        if(hF!=HFILE_ERROR)
                        {
                            int filelen=_llseek(hF,0,2);

                            //用于过滤掉不必要的行情
                            map<string,DataRtnDepthMarketData> mapDepthMarketData;

                            int off=0;
                            int ind=0;
                            _llseek(hF,0,0);
                            while(off<filelen&&ind<StartNo)
                            {
                                _lread(hF,pPackageHead2,sizeof(PackageHeadStru));
                                _lread(hF,pData2,pPackageHead2->DataLen);
                                off+=sizeof(PackageHeadStru)+pPackageHead2->DataLen;
                                ind++;
                            }

                            while(off<filelen&&ind<EndNo)
                            {
                                _lread(hF,pPackageHead2,sizeof(PackageHeadStru));
                                _lread(hF,pData2,pPackageHead2->DataLen);

                                bool bFilter=false;
                                if(((AbstractBusinessData*)pData2)->BID==BID_RtnDepthMarketData)
                                {
                                    //过滤掉不必要的行情
                                    DataRtnDepthMarketData& MarketBusiness= *((DataRtnDepthMarketData*)pData2);
                                    string strInst=string(MarketBusiness.DepthMarketDataField.InstrumentID);
                                    map<string,DataRtnDepthMarketData>::const_iterator it=mapDepthMarketData.find(strInst);
                                    if(it!=mapDepthMarketData.end()&&
                                        it->second.DepthMarketDataField.LastPrice==MarketBusiness.DepthMarketDataField.LastPrice&&
                                        it->second.DepthMarketDataField.SettlementPrice==MarketBusiness.DepthMarketDataField.SettlementPrice&&
                                        it->second.DepthMarketDataField.PreSettlementPrice==MarketBusiness.DepthMarketDataField.PreSettlementPrice)
                                    {
                                        bFilter=true;
                                    }
                                    if(!bFilter) mapDepthMarketData[strInst]=MarketBusiness;
                                }

                                if(!bFilter)
                                {
                                    m_DataCenter.doProcessBusinessData(*((AbstractBusinessData*)pData2)); 

                                    if( ((AbstractBusinessData*)pData2)->BID==BID_RtnTrade||
                                        ((AbstractBusinessData*)pData2)->BID==BID_RtnOrder)
                                        Sleep(200);

                                    sprintf(buf,"%d: ",ind);
                                    CEasyTime((time_t)pPackageHead2->dwCRC).getstrshorttime(buf+strlen(buf),sizeof(buf)-strlen(buf));
                                    sprintf(buf+strlen(buf),":%s",CTools::BusinessID2String((int)(((AbstractBusinessData*)pData2)->BID)));
                                    //pmsgwin2->AppendStr(CPopMsgWin::ForceShow,CPopMsgWin::NoDelOldContent,CPopMsgWin::AddCRLF,CPopMsgWin::NoInsertTime,buf);
                                }

                                off+=sizeof(PackageHeadStru)+pPackageHead2->DataLen;
                                ind++;

                                //BID_RspQryAccountregister是初始化查询的最后一个
                                if(((AbstractBusinessData*)pData2)->BID==BID_RspQryAccountregister&&
                                    ((DataRspQryParkedOrderAction*)pData2)->bIsLast)
                                {
                                    Sleep(12000);
                                    //EndNo=ind;
                                }
                            }
                            StartNo=EndNo;

                            _lclose(hF);
                        }
                    }
                }
            }



            if(RefPackageCache.IsEmpty())
            {
                wxThread::Sleep(2);
                continue;
            }
           
            int GetOff=-1;
            if(!RefPackageCache.PeekFrom(pPackageHead, pData, sizeof(readBuf)-sizeof(PackageHeadStru),&GetOff))
                continue;
            
            //LOG_INFO2("CDataCenterWorkThread PeekFrom:CMDID=%d DataLen=%d GetOff=%d",pPackageHead->CmdID,pPackageHead->DataLen,GetOff);

            if(pPackageHead->DataLen<sizeof(AbstractBusinessData))
            {
                int OriGetOff,NewGetOff;
                RefPackageCache.DeleteCurPackage(&OriGetOff,&NewGetOff);
                LOG_INFO2("CDataCenterWorkThread DeleteCurPackage(长度不够):OriGetOff=%d NewGetOff=%d, (package CMDID=%d DataLen=%d)",
                    OriGetOff,NewGetOff,pPackageHead->CmdID,pPackageHead->DataLen);
                continue;
            }

            AbstractBusinessData tmpAbstractBusinessData=*((AbstractBusinessData*)pData);
            PackageHeadStru tmpPackageHead=*pPackageHead;
            int tmpGetOff=GetOff;

            if(GlobalConfigManager::m_Test_bSaveBusinessData)
            {
                //把底层业务数据保存到文件中去
                HFILE hF;
                char filename[256];
                memset(filename,0,sizeof(filename));
                CTools_Win32::GetMainPath(filename,sizeof(filename));
                int len=strlen(filename);
                strncpy(filename+len,"\\businessdata.rec",sizeof(filename)-1-len);

                //程序启动时，检查有没有旧的businessdata.rec，如果有，改名为businessdata_yyyymmdd_hhmmss.rec
                static bool bFirst=true;
                if(bFirst)
                {
                    hF=_lopen(filename,OF_READ);
                    if(hF!=HFILE_ERROR)
                    {
                        _lclose(hF);
                        char filename2[512];
                        SYSTEMTIME curTime;
                        GetLocalTime(&curTime);
                        memset(filename2,0,sizeof(filename2));
                        CTools_Win32::GetMainPath(filename2,sizeof(filename2));
                        int len2=strlen(filename2);
                        sprintf(filename2+len2,"\\businessdata_%04d%02d%02d_%02d%02d%02d.rec",curTime.wYear,curTime.wMonth,curTime.wDay,curTime.wHour,curTime.wMinute,curTime.wSecond);
                        rename(filename,filename2);
                    }
                    bFirst=false;
                }


                hF=_lopen(filename,OF_READWRITE);
                if(hF==HFILE_ERROR) hF=_lcreat(filename,0);
                _llseek(hF,0,2);
                pPackageHead->dwCRC=(unsigned int)time(NULL);
                _lwrite(hF,(char*)pPackageHead,sizeof(PackageHeadStru));
                _lwrite(hF,pData,pPackageHead->DataLen);
                _lclose(hF);
            }

            //DataCenter已经和底层同步上，可以开始初始化查询了。
            if(((AbstractBusinessData*)pData)->BID==BID_RspQrySettlementInfoConfirm)
                m_DataCenter.bRspQrySettlementInfoConfirm=true;

            if(!GlobalConfigManager::m_Test_bPlayBusinessData||
                ((AbstractBusinessData*)pData)->BID==BID_RspUserLogin)
            {
                m_DataCenter.doProcessBusinessData(*((AbstractBusinessData*)pData)); 
            }

            int OriGetOff,NewGetOff;
            RefPackageCache.DeleteCurPackage(&OriGetOff,&NewGetOff);
            //LOG_INFO2("CDataCenter DeleteCurPackage:OriGetOff=%d NewGetOff=%d",OriGetOff,NewGetOff);

            if(tmpAbstractBusinessData.BID!=BID_RtnDepthMarketData)
            {
                LOG_INFO2("CDataCenter peek package, (BID=%d, Size=%d, CMDID=%d DataLen=%d GetOff=%d) DeleteCurPackage(OriGetOff=%d NewGetOff=%d)", 
                    tmpAbstractBusinessData.BID, tmpAbstractBusinessData.Size,tmpPackageHead.CmdID,tmpPackageHead.DataLen,tmpGetOff,
                    OriGetOff,NewGetOff);
            }


        }

        return NULL;
    };

    // called when the thread exits - whether it terminates normally or is
    // stopped with Delete() (but not when it is Kill()ed!)
    virtual void OnExit()
    {
        m_DataCenter.m_pWorkThread = NULL;
    };

private:
    CDataCenter& m_DataCenter;
};



CDataCenter::CDataCenter(CPackageCache& dataSource,CPlatFormService* pHost,int PlatformID,int MaxPlatformID)
:   m_dataSource(dataSource),
    m_PlatFormService(pHost)
{
    //m_bStartInitQry=false;
    //m_bStopInitQry=false;
    bRspQrySettlementInfoConfirm=false;
    
    m_pWriteLog=new zqWriteLog();
    m_pWorkThread = NULL;
	m_pDataMgr=new CPlatformDataMgr(
        GlobalConfigManager::IsCalculate_OnRtnTrade(),
        GlobalConfigManager::m_bUseComb,
        GlobalConfigManager::m_DiscountCZCE,
        GlobalConfigManager::m_DiscountDCE,
        GlobalConfigManager::m_UnwindPrioritiesCZCE,
        GlobalConfigManager::m_UnwindPrioritiesDCE,
        GlobalConfigManager::m_PriceForMarginOfTodayPositionCZCE,
        GlobalConfigManager::m_PriceForMarginOfTodayPositionDCE,
        GlobalConfigManager::IsFreezeOrderUnknownStatus(),
        PlatformID,
        MaxPlatformID);

}

CDataCenter::~CDataCenter()
{
	SAFE_DELETE(m_pDataMgr);
    LOG_INFO("CDataCenter::~CDataCenter(析构CDataCenter对象)");
	SAFE_DELETE(m_pWriteLog);
}

///定制业务数据
int CDataCenter::SubscribeBusinessData(BusinessID BID, GUIModuleID GID, GuiCallBackFunc callback, bool sendCurrentInfo)
{
    if(BID>=BID_MAX || GID>=GID_MAX || callback==NULL)
        return -1;

    m_mutex.lock();
    int ret=0;
    try
    {
        m_subscribeInfo[BID][GID]=callback;
    }
    catch(...)
    {
        ret=-2;
    }
    m_mutex.unlock();

    if(ret<0) return ret;


    if(!sendCurrentInfo)
        return 0;

    switch(BID)
    {
        case BID_RtnDepthMarketData:
        {
            set<string> InstrumentSet;
            set<string>::iterator it;
            DataRtnDepthMarketData tmp;

            m_pDataMgr->m_Instruments.GetInstrumentList(InstrumentSet);
            if((int)InstrumentSet.size()>0)
            {
                for(it=InstrumentSet.begin();it!=InstrumentSet.end();it++)
                {
                    if(m_pDataMgr->m_Quots.GetQuotInfo(*it,tmp.DepthMarketDataField))
                    {
                        (callback)(GID, *(AbstractBusinessData*)(&tmp));
                    }
                }
            }
            break;
        }
        default:
            break;
    }

    return 0;
}

///取消业务数据的定制
int CDataCenter::UnSubscribeBusinessData(BusinessID BID, GUIModuleID GID)
{
    CeasymutexGuard guard(m_mutex);

    int ret=0;

    if(BID>=BID_MAX || GID>=GID_MAX)
        return -1;

    map<BusinessID, map<GUIModuleID, GuiCallBackFunc> >::iterator it1=m_subscribeInfo.find(BID);
    if(it1==m_subscribeInfo.end())
        ret=-3;
    else
    {
        map<GUIModuleID, GuiCallBackFunc>::iterator it2 = it1->second.find(GID);
        if(it2==it1->second.end())
            ret=-3;
        else
        {
            it1->second.erase(it2);
            if(it1->second.empty())
                m_subscribeInfo.erase(it1);
            ret=0;
        }
    }

    return ret;
}

///启动
int CDataCenter::Start()
{
    LOG_INFO("Start");
    if(m_pWorkThread)
        return 0;

    m_pWorkThread = new CDataCenterWorkThread(*this);
    if(m_pWorkThread->Create(512*1024) != wxTHREAD_NO_ERROR)
        LOG_INFO("Start:创建DataCenter的内部工作线程失败");
    else
        LOG_INFO("Start:创建DataCenter的内部工作线程成功");

    if(m_pWorkThread->Run() != wxTHREAD_NO_ERROR)
    {
         LOG_INFO("Start:运行DataCenter的内部工作线程失败!");
         return -1;
    }
    else
    {
         LOG_INFO("Start:运行DataCenter的内部工作线程成功!");
    }

    return 0;
}

///停止,定制信息保留，再次启动之后，定制依然有效
int CDataCenter::Stop()
{
    LOG_INFO("Stop");
    if(!m_pWorkThread)
        return 0;

    LOG_INFO("Stop:关闭DataCenter的内部工作线程");
    m_pWorkThread->Delete();
    m_pWorkThread=NULL; //will also be set NULL in calling CDataCenterWorkThread::OnExit()

    return 0;
}

int CDataCenter::doProcessBusinessData(AbstractBusinessData& data)
{
    //if(data.BID!=BID_RtnDepthMarketData)
    //    LOG_INFO("CDataCenter::doProcessBusinessData, BID=%d, Size=%d", data.BID, data.Size);

    DWORD t1,t2,t3;
    t1=GetTickCount();

    bool toBeContinue=true;
    switch(data.BID)
    {
        case BID_RtnDepthMarketData:    //深度行情
        {
            DataRtnDepthMarketData& NewBusiness= (DataRtnDepthMarketData&)data;   
            string strInstrument(NewBusiness.DepthMarketDataField.InstrumentID);
            if(strInstrument.empty()) toBeContinue=false;
            else 
            {
                //在结构中表明合约是否有有效持仓。此处只管单腿持仓，组合持仓不计算盈亏，不用管
                NewBusiness.bHavePosition=m_pDataMgr->m_PositionDetails.HavePositionDetail(strInstrument);
                m_pDataMgr->OnRtnDepthMarketData(NewBusiness.DepthMarketDataField);
            }
            break;
        }
        case BID_RspQryDepthMarketData://请求查询行情响应。当组合合约报单需要行情的昨结算价时，会触发查询单腿合约的行情
        {
            DataRspQryDepthMarketData& NewBusiness= (DataRspQryDepthMarketData&)data;
            if(NewBusiness.RspInfoField.ErrorID==0)
            {
                m_pDataMgr->OnRspQryDepthMarketData(NewBusiness.DepthMarketDataField);
            }
            break;
        }
        case BID_RtnOrder://报单回报
        {
			DataRtnOrder& NewBusiness= (DataRtnOrder&)data;

            string strInstrumentID(NewBusiness.OrderField.InstrumentID);
            if(strInstrumentID.empty()) break;
            PlatformStru_InstrumentInfo InstrumentInfo;
            if(!m_pDataMgr->m_Instruments.GetInstrumentInfo(strInstrumentID,InstrumentInfo))
		        break;

            m_pDataMgr->OnRtnOrder(NewBusiness.OrderField);

            //检查是否是组合合约报单
            string Leg1InstrumentID,Leg2InstrumentID;
            bool bComb = InstrumentInfo.ProductClass==THOST_FTDC_PC_Combination;
            if(bComb) m_pDataMgr->m_Instruments.GetLegInstrumentID(strInstrumentID,Leg1InstrumentID,Leg2InstrumentID);

            //根据需要触发查询费率。如果是组合合约，按分腿查询
            if(bComb)
            {
		        m_PlatFormService->m_rServiceProxy.GetCommissionRate_TriggerReq(Leg1InstrumentID);
		        m_PlatFormService->m_rServiceProxy.GetMarginRate_TriggerReq(Leg1InstrumentID);
		        m_PlatFormService->m_rServiceProxy.GetCommissionRate_TriggerReq(Leg2InstrumentID);
		        m_PlatFormService->m_rServiceProxy.GetMarginRate_TriggerReq(Leg2InstrumentID);
            }
            else
            {
		        m_PlatFormService->m_rServiceProxy.GetCommissionRate_TriggerReq(strInstrumentID);
		        m_PlatFormService->m_rServiceProxy.GetMarginRate_TriggerReq(strInstrumentID);
            }

            //组合报单计算冻结保证金和手续费需要用到行情里的昨结算价，如果没有行情，触发查询
            if(bComb)
            {
                if(util::isInvalidValue(NewBusiness.OrderField.freezeMargin)||
                   util::isInvalidValue(NewBusiness.OrderField.troubleMoney))
                {
                    PlatformStru_DepthMarketData LegQuotData;
                    if(!m_pDataMgr->m_Quots.GetQuotInfo(Leg1InstrumentID,LegQuotData))
                    {
                        PlatformStru_QryDepthMarketData QryDepthMarketData;
                        strncpy(QryDepthMarketData.Thost.InstrumentID,Leg1InstrumentID.c_str(),sizeof(QryDepthMarketData.Thost.InstrumentID)-1);
                        m_PlatFormService->m_rServiceProxy.ReqQryDepthMarketData(QryDepthMarketData);
                    }
                    if(!m_pDataMgr->m_Quots.GetQuotInfo(Leg2InstrumentID,LegQuotData))
                    {
                        PlatformStru_QryDepthMarketData QryDepthMarketData;
                        strncpy(QryDepthMarketData.Thost.InstrumentID,Leg2InstrumentID.c_str(),sizeof(QryDepthMarketData.Thost.InstrumentID)-1);
                        m_PlatFormService->m_rServiceProxy.ReqQryDepthMarketData(QryDepthMarketData);
                    }
                }
            }

            //针对未成交以及状态未知的报单订阅行情
            if(m_pDataMgr->m_Orders.IsOrderWorking(NewBusiness.OrderField))
            {
                m_PlatFormService->SetSubscribeStatus(strInstrumentID,GID_PlatformSvr);
                if(bComb)
                {
                    //组合单还要订阅双腿行情
                    m_PlatFormService->SetSubscribeStatus(Leg1InstrumentID,GID_PlatformSvr);
                    m_PlatFormService->SetSubscribeStatus(Leg2InstrumentID,GID_PlatformSvr);
                }
            }
            break;
        }
        case BID_RtnTrade://成交回报
        {
			DataRtnTrade& NewBusiness= (DataRtnTrade&)data;

            string strInstrumentID(NewBusiness.TradeField.InstrumentID);
            if(strInstrumentID.empty()) break;
            PlatformStru_InstrumentInfo InstrumentInfo;
            if(!m_pDataMgr->m_Instruments.GetInstrumentInfo(strInstrumentID,InstrumentInfo))
		        break;

            m_pDataMgr->OnRtnTrade(NewBusiness.TradeField);

            //触发查询费率
	        m_PlatFormService->m_rServiceProxy.GetCommissionRate_TriggerReq(strInstrumentID);
	        m_PlatFormService->m_rServiceProxy.GetMarginRate_TriggerReq(strInstrumentID);

            //如果配置指定了成交后需要查询相关信息，执行查询操作
            //如果在初始化查询过程中，如果有成交回报过来，也触发相关合约的查询，避免漏掉交易数据
            if(GlobalConfigManager::IsReq_OnRtnTrade()||
                m_PlatFormService->m_rServiceProxy.m_bInitQryStart)
            {
                CReqOnRtnTrade::OnRtnTrade(string(NewBusiness.TradeField.BrokerID),
                    string(NewBusiness.TradeField.InvestorID),
                    string(NewBusiness.TradeField.InstrumentID),
                    (NewBusiness.TradeField.TradeType==THOST_FTDC_TRDT_CombinationDerived?true:false),
                    m_pDataMgr->m_PositionDetails.GetRecordCountOfPositionDetail());
            }
            break;
        }
        case BID_RspQryInstrument://请求查询合约响应
        {
			DataRspQryInstrument& NewBusiness= (DataRspQryInstrument&)data; 
			if(NewBusiness.RspInfoField.ErrorID==0)
			{
				if(NewBusiness.InstrumentField.InstrumentID[0]!=0)
				{
                    bool bCTPSimulate=false;
                    if((int)m_pDataMgr->m_ServerParamCfg.vec.size()>0&&
                        m_pDataMgr->m_ServerParamCfg.current<(int)m_pDataMgr->m_ServerParamCfg.vec.size()&&
                        m_pDataMgr->m_ServerParamCfg.vec[m_pDataMgr->m_ServerParamCfg.current].CTPSimulate==1)
                        bCTPSimulate=true;

                    m_pDataMgr->OnRspQryInstrument(NewBusiness.InstrumentField,bCTPSimulate);
				}
            }
            break;
        }
        case BID_RspQryInvestorPosition://请求查询投资者持仓响应
        {
            DataRspQryInvestorPosition& NewBusiness= (DataRspQryInvestorPosition&)data; 
            if(NewBusiness.RspInfoField.ErrorID==0||NewBusiness.RspInfoField.ErrorID==CustomErrorID||NewBusiness.bIsLast)
            {
                //将要查询的合约ID拷贝到返回数据中去
                string QryInstrument;
                m_pDataMgr->m_Positions.GetQryPositionInstrument(QryInstrument);
                strncpy(NewBusiness.QryInstrumentID,QryInstrument.c_str(),sizeof(NewBusiness.QryInstrumentID)-1);

                m_pDataMgr->OnRspQryInvestorPosition(NewBusiness.InvestorPositionField,NewBusiness.RspInfoField.ErrorID,NewBusiness.bIsLast);
            }
            break;
        }
        case BID_RspQryInvestorPositionDetail://请求查询投资者持仓明细响应
        {
            DataRspQryInvestorPositionDetail& NewBusiness= (DataRspQryInvestorPositionDetail&)data; 
            if(NewBusiness.RspInfoField.ErrorID==0||NewBusiness.RspInfoField.ErrorID==CustomErrorID)
            {
                //将要查询的合约ID拷贝到返回数据中去
                string QryInstrument;
                m_pDataMgr->m_PositionDetails.GetQryPositionDetailInstrument(QryInstrument);
                strncpy(NewBusiness.QryInstrumentID,QryInstrument.c_str(),sizeof(NewBusiness.QryInstrumentID)-1);

                m_pDataMgr->OnRspQryInvestorPositionDetail(NewBusiness.InvestorPositionDetailField,NewBusiness.RspInfoField.ErrorID,NewBusiness.bIsLast);

                //根据需要触发费率查询。对于组合持仓的持仓明细，由于是分腿返回的，因此不需要做特殊处理。
                if(NewBusiness.bIsLast)
                {
                    set<string> setInstruments;
                    m_pDataMgr->m_PositionDetails.GetInstrumentSetOfQryRlt(setInstruments);
                    if(!setInstruments.empty())
                    {
                        set<string>::iterator it_Instrument;
                        for(it_Instrument=setInstruments.begin();it_Instrument!=setInstruments.end();it_Instrument++)
                        {
			                m_PlatFormService->m_rServiceProxy.GetCommissionRate_TriggerReq(*it_Instrument);
			                m_PlatFormService->m_rServiceProxy.GetMarginRate_TriggerReq(*it_Instrument);
                        }
                    }
                }

                m_PlatFormService->SetSubscribeStatus(NewBusiness.InvestorPositionDetailField.InstrumentID,GID_PlatformSvr);
            }
            break;
        }
        case BID_RspQryInvestorPositionCombDetail://请求查询投资者组合持仓明细响应
        {
            DataRspQryInvestorPositionCombDetail& NewBusiness= (DataRspQryInvestorPositionCombDetail&)data; 
			if (NewBusiness.RspInfoField.ErrorID == 0 || NewBusiness.RspInfoField.ErrorID == CustomErrorID)
			{
                //将要查询的合约ID拷贝到返回数据中去
                string QryInstrument;
                m_pDataMgr->m_PositionCombs.GetQryPositionCombInstrument(QryInstrument);
                strncpy(NewBusiness.QryInstrumentID,QryInstrument.c_str(),sizeof(NewBusiness.QryInstrumentID)-1);

                m_pDataMgr->OnRspQryInvestorPositionCombDetail(NewBusiness.InvestorPositionCombineDetailField,NewBusiness.RspInfoField.ErrorID,NewBusiness.bIsLast);
			}
			break;
        }
        case BID_RspQryTrade://请求查询成交响应
        {
            DataRspQryTrade& NewBusiness= (DataRspQryTrade&)data;
            if(NewBusiness.RspInfoField.ErrorID == 0||NewBusiness.RspInfoField.ErrorID==CustomErrorID||NewBusiness.bIsLast)
            {
                m_pDataMgr->OnRspQryTrade(NewBusiness.TradeField,NewBusiness.RspInfoField.ErrorID,NewBusiness.bIsLast);
            }
            break;
        }
        case BID_RspQryOrder://请求查询报单响应
        {
            DataRspQryOrder& NewBusiness= (DataRspQryOrder&)data;
            if(NewBusiness.RspInfoField.ErrorID==0||NewBusiness.RspInfoField.ErrorID==CustomErrorID)
            {
                m_pDataMgr->OnRspQryOrder(NewBusiness.OrderField,NewBusiness.RspInfoField.ErrorID,NewBusiness.bIsLast);

                if(NewBusiness.bIsLast)
                {
                    //按需要触发查询费率，订阅行情
                    set<string> setInstruments;
                    m_pDataMgr->m_Orders.GetInstrumentIDs_Working(setInstruments);
                    set<string>::iterator it_Instrument;
                    for(it_Instrument=setInstruments.begin();it_Instrument!=setInstruments.end();it_Instrument++)
                    {
                        m_PlatFormService->SetSubscribeStatus(*it_Instrument,GID_PlatformSvr);

                        //如果是组合合约，按分腿查询
                        string Leg1InstrumentID,Leg2InstrumentID;
                        if(GlobalFunc::IsCombInstrument(*it_Instrument,Leg1InstrumentID,Leg2InstrumentID))
                        {
		                    m_PlatFormService->m_rServiceProxy.GetCommissionRate_TriggerReq(Leg1InstrumentID);
		                    m_PlatFormService->m_rServiceProxy.GetMarginRate_TriggerReq(Leg1InstrumentID);
		                    m_PlatFormService->m_rServiceProxy.GetCommissionRate_TriggerReq(Leg2InstrumentID);
		                    m_PlatFormService->m_rServiceProxy.GetMarginRate_TriggerReq(Leg2InstrumentID);

                            //组合单还要订阅双腿行情
                            m_PlatFormService->SetSubscribeStatus(Leg1InstrumentID,GID_PlatformSvr);
                            m_PlatFormService->SetSubscribeStatus(Leg2InstrumentID,GID_PlatformSvr);
                        }
                        else
                        {
		                    m_PlatFormService->m_rServiceProxy.GetCommissionRate_TriggerReq(*it_Instrument);
		                    m_PlatFormService->m_rServiceProxy.GetMarginRate_TriggerReq(*it_Instrument);
                        }
                    }

                    //取等待行情进行冻结保证金和手续费计算的合约集，查询行情
                    m_pDataMgr->m_Orders.GetInstrumentIDs_WaitQuot(setInstruments);
                    for(it_Instrument=setInstruments.begin();it_Instrument!=setInstruments.end();it_Instrument++)
                    {
                        PlatformStru_QryDepthMarketData QryDepthMarketData;
                        strncpy(QryDepthMarketData.Thost.InstrumentID,it_Instrument->c_str(),sizeof(QryDepthMarketData.Thost.InstrumentID)-1);
                        m_PlatFormService->m_rServiceProxy.ReqQryDepthMarketData(QryDepthMarketData);
                    }
                }
            }
            break;        
        }
        case BID_RspOrderInsert://报单录入回报
        {
            DataRspOrderInsert& NewBusiness= (DataRspOrderInsert&)data;
      //      if(NewBusiness.RspInfoField.ErrorID!=0)
      //      {
			   //m_PlatFormService->SetSubscribeStatus(NewBusiness.InputOrderField.InstrumentID,S_STATUS_ORDERINSERT);
      //      }
            break;  
        }
		case BID_ErrRtnOrderInsert://报单录入回报
		{
			DataErrRtnOrderInsert& NewBusiness= (DataErrRtnOrderInsert&)data;
			//if(NewBusiness.RspInfoField.ErrorID!=0)
			//{
			//	m_PlatFormService->SetSubscribeStatus(NewBusiness.InputOrderField.InstrumentID,S_STATUS_ORDERINSERT);
			//}
		    break;  
		}
        case BID_RspQryTradingAccount://查询交易资金账户信息响应
        {
            DataRspQryTradingAccount& NewBusiness= (DataRspQryTradingAccount&)data;
            if(NewBusiness.RspInfoField.ErrorID==0)
            {
                m_pDataMgr->OnRspQryTradingAccount(NewBusiness.TradingAccountField);
            }
            break;
        }
        case BID_RspQryInvestor://请求查询投资者响应
        {
            DataRspQryInvestor& NewBusiness= (DataRspQryInvestor&)data;
            if(NewBusiness.RspInfoField.ErrorID==0)
            {
                m_pDataMgr->SetInvestorInfo(NewBusiness.InvestorField);
            }
            break;
        }
        case BID_RspQryInstrumentMarginRate://请求查询合约保证金率响应
        {
            DataRspQryInstrumentMarginRate& NewBusiness= (DataRspQryInstrumentMarginRate&)data;
            m_pDataMgr->OnRspQryInstrumentMarginRate(NewBusiness.InstrumentMarginRateField,NewBusiness.RspInfoField.ErrorID);
            break;
        }
        case BID_RspQryInstrumentCommissionRate://请求查询合约手续费率响应
        {
            DataRspQryInstrumentCommissionRate& NewBusiness= (DataRspQryInstrumentCommissionRate&)data;
            m_pDataMgr->OnRspQryInstrumentCommissionRate(NewBusiness.InstrumentCommissionRateField,NewBusiness.RspInfoField.ErrorID);
            break;
        }
        case BID_RspQrySettlementInfo://请求查询投资者结算结果响应
        {
            DataRspQrySettlementInfo& NewBusiness= (DataRspQrySettlementInfo&)data;
            if(NewBusiness.RspInfoField.ErrorID==0)
            {
                m_pDataMgr->AppendLastSettlementInfo(string(NewBusiness.SettlementInfoField.Content),NewBusiness.bIsLast);
            }
            else
            {
                m_pDataMgr->ClearLastSettlementInfo();
            }
            break;
        }
        case BID_RspUserLogin://登入响应，主要记录交易所的时间
        {
            DataRspUserLogin& NewBusiness= (DataRspUserLogin&)data;
            if(NewBusiness.RspInfoField.ErrorID==0 && NewBusiness.bTrader)
            {
                ExchangeTime exchangeTime;
                m_pDataMgr->m_Instruments.GetExchangeTime(exchangeTime);
                exchangeTime.CurTickCount = NewBusiness.CurTickCount;
                memcpy(&exchangeTime.TradingDay, &NewBusiness.UserLoginField.TradingDay, sizeof(exchangeTime.TradingDay));
                memcpy(&exchangeTime.SHFETime, &NewBusiness.UserLoginField.SHFETime, sizeof(exchangeTime.SHFETime));
                memcpy(&exchangeTime.DCETime, &NewBusiness.UserLoginField.DCETime, sizeof(exchangeTime.DCETime));
                memcpy(&exchangeTime.CZCETime, &NewBusiness.UserLoginField.CZCETime, sizeof(exchangeTime.CZCETime));
                memcpy(&exchangeTime.FFEXTime, &NewBusiness.UserLoginField.FFEXTime, sizeof(exchangeTime.FFEXTime));
                m_pDataMgr->m_Instruments.SetExchangeTime(exchangeTime);

                m_pDataMgr->m_LoginInfo.SetTradingLoginInfo(NewBusiness.UserLoginField.FrontID,NewBusiness.UserLoginField.SessionID,NewBusiness.UserLoginField.UserID);
            }
            break;
        }
		case BID_RspQryContractBank://请求查询签约银行响应
        {
            DataRspQryContractBank& pTmp= (DataRspQryContractBank&)data;
            //CThostFtdcContractBankField* pRawData= &pTmp->ContractBankField;
            if(pTmp.RspInfoField.ErrorID == 0)
            {   

                m_pDataMgr->SaveBankName(pTmp.ContractBankField);
				PlatformStru_QryAccountRegister  QryAccountregister;
				memset(&QryAccountregister,0,sizeof(QryAccountregister));
				if(pTmp.bIsLast==1)
				{
					if( m_PlatFormService )
						m_PlatFormService->m_rServiceProxy.ReqQryAccountregister(QryAccountregister);
				}
            }
            break;
        }
		
		case   BID_RspQryAccountregister://请求查询银期签约关系响应
        {
            DataRspQryAccountregister& pTmp= (DataRspQryAccountregister&)data;
            CThostFtdcAccountregisterField* pRawData= &pTmp.Accountregister;
            if(pTmp.RspInfoField.ErrorID == 0)
            {   
               
                m_pDataMgr->UpdateContractBank(pTmp.Accountregister);
            }
            break;
        }

        case BID_RspQryExchangeRate:        //请求查询汇率返回
        {
            DataRspQryExchangeRate& Tmp = (DataRspQryExchangeRate&)data;
            if(Tmp.RspInfoField.ErrorID == 0)
            {
                PlatformStru_ExchangeRate exchangerate;
                strncpy(exchangerate.BrokerID,Tmp.ExchangeRateField.BrokerID,sizeof(exchangerate.BrokerID)-1);
                strncpy(exchangerate.FromCurrencyID,Tmp.ExchangeRateField.FromCurrencyID,sizeof(exchangerate.FromCurrencyID)-1);
                strncpy(exchangerate.ToCurrencyID,Tmp.ExchangeRateField.ToCurrencyID,sizeof(exchangerate.ToCurrencyID)-1);
                exchangerate.FromCurrencyUnit=Tmp.ExchangeRateField.FromCurrencyUnit;
                exchangerate.ExchangeRate=Tmp.ExchangeRateField.ExchangeRate;
                m_pDataMgr->SaveExchangeRate(exchangerate);
            }
            break;
        }


        case BID_RspParkedOrderInsert:       //预埋单录入请求响应
        case BID_RspParkedOrderAction:       //预埋撤单录入请求响应
        case BID_RspRemoveParkedOrder:       //删除预埋单响应    
        case BID_RspRemoveParkedOrderAction: //删除预埋撤单响应
        case BID_RspQryParkedOrder:          //请求查询预埋单响应
        case BID_RspQryParkedOrderAction:    //请求查询预埋撤单响应
        {
            break;
        }

        default:
        break;                  
    }


    if(!toBeContinue)
        return -1;


    t2=GetTickCount();

    map<GUIModuleID,GuiCallBackFunc> CallbackFuncList;
    m_mutex.lock();
    map<BusinessID, map<GUIModuleID, GuiCallBackFunc> >::iterator it1=m_subscribeInfo.find(data.BID);
    if(it1!=m_subscribeInfo.end())
        CallbackFuncList=it1->second;
    m_mutex.unlock();

    if((int)CallbackFuncList.size()>0)
    {
        data.PlatformID = m_PlatFormService->m_rServiceProxy.GetPlatformID();

        map<GUIModuleID,GuiCallBackFunc>::iterator it_callback;
        for(it_callback=CallbackFuncList.begin();it_callback!=CallbackFuncList.end();it_callback++)
        {
            if(it_callback->second)
                (it_callback->second)(it_callback->first,data);
        }
    }


  //  if(m_pDataMgr->GetSubscribeBusinessData(data.BID,CallbackFuncList)==0)
  //  {
		//data.PlatformID = m_PlatFormService->GetPlatFormID();
  //      int i;
  //      for(i=0,it_callback=CallbackFuncList.begin();it_callback!=CallbackFuncList.end();it_callback++,i++)
  //      {
  //          if(it_callback->second)
  //              (it_callback->second)(it_callback->first,data);
  //      }
  //  }

    t3=GetTickCount();


    //char Buf[512];
    //sprintf(Buf,"doProcessBusinessData(%03d): - \t"
    //            "t1: %03u \t"
    //            "t2: %03u \t"
    //            "Total: %03u    ----", 
    //            data.BID,
    //            t2-t1,t3-t2,t3-t1);
//    OutputDebugString(Buf);

    return 0;  
}
///获取合约列表
int CDataCenter::GetInstrumentList(vector<InstrumentGroupInfo> &outData)
{
    m_pDataMgr->m_Instruments.GetInstrumentList(outData);
    return outData.size();
}
///获取合约列表
int CDataCenter::GetInstrumentList(set<string> &outData,string ExchangeID)
{
    m_pDataMgr->m_Instruments.GetInstrumentList(outData,ExchangeID);
    return outData.size();
}
//添加主力合约列表
void CDataCenter::AddMainInstrumentList(string instrument)
{
	 m_pDataMgr->m_Instruments.AddMainInstrumentList(instrument);
}
///获取主力合约列表
int CDataCenter::GetMainInstrumentList(set<string> &outData)
{
    return m_pDataMgr->m_Instruments.GetMainInstrumentList(outData);
}
///设置合约容差列表
void CDataCenter::SetInstrumentVarietyMap(map<string, string>& inData)
{
    m_pDataMgr->m_Instruments.SetInstrumentVarietyMap(inData);
}

// 获取合约容差列表
int CDataCenter::GetInstrumentVarietyMap(map<string, string>& outData)
{
    m_pDataMgr->m_Instruments.GetInstrumentVarietyMap(outData);
    return outData.size();
}

///获取指定合约信息
int CDataCenter::GetInstrumentInfo(const string& InstrumentID, PlatformStru_InstrumentInfo& outData)
{
    bool brlt=m_pDataMgr->m_Instruments.GetInstrumentInfo(InstrumentID,outData);
    return brlt?0:-1;
}
bool CDataCenter::GetLegInstrumentID(const string& strComInstrumentID,
                                     string& strLeg1InstrumentID,
                                     string& strLeg2InstrumentID)
{
    return m_pDataMgr->m_Instruments.GetLegInstrumentID(strComInstrumentID,strLeg1InstrumentID,strLeg2InstrumentID);
}
///设置指定合约信息
void CDataCenter::SetInstrumentInfo(const string& InstrumentID, PlatformStru_InstrumentInfo& outData)
{
    bool bCTPSimulate = false;
    if((int)m_pDataMgr->m_ServerParamCfg.vec.size()>0&&
        m_pDataMgr->m_ServerParamCfg.current<(int)m_pDataMgr->m_ServerParamCfg.vec.size()&&
        m_pDataMgr->m_ServerParamCfg.vec[m_pDataMgr->m_ServerParamCfg.current].CTPSimulate==1)
        bCTPSimulate=true;
    m_pDataMgr->m_Instruments.SetInstrumentInfo(InstrumentID,outData,bCTPSimulate);
    m_pDataMgr->m_Rates.SetNeedCheckCommissionRateAndMarginRate(true);
}

///获取合约的产品类型，失败返回-1
char CDataCenter::GetProductClassType(const string& InstrumentID)
{
    return m_pDataMgr->m_Instruments.GetProductClassType(InstrumentID);
}

///获取包含此单腿合约的组合合约列表
bool CDataCenter::GetCombInstrumentIDs_IncludeLeg(const string& LegInstrument,vector<string>& vecCombInstruments)
{
    return m_pDataMgr->m_Instruments.GetCombInstrumentIDs_IncludeLeg(LegInstrument,vecCombInstruments);
}

///获取合约品种代码
int CDataCenter::GetProductID(map<string, set<string> >& outData,string ExchangeID)
{
    m_pDataMgr->m_Instruments.GetProductID(outData,ExchangeID);
    return outData.size();
}

///获取合约手续费率
int CDataCenter::GetCommissionRate(const string& InstrumentID, PlatformStru_InstrumentCommissionRate& outData)
{
    return m_pDataMgr->m_Rates.GetCommissionRate(InstrumentID,outData);
}
///设置合约手续费率
int CDataCenter::SetCommissionRate(const string& InstrumentID, PlatformStru_InstrumentCommissionRate& outData)
{
    m_pDataMgr->m_Rates.SetCommissionRate(InstrumentID,outData);
    return 0;
}
///设置正在查询手续费率的合约
void CDataCenter::SetReqCommissionRateInstrumentID(const string& InstrumentID)
{
    m_pDataMgr->m_Rates.SetReqCommissionRateInstrumentID(InstrumentID);
}
///获取合约保证金率
int CDataCenter::GetMarginRate(const string& InstrumentID, PlatformStru_InstrumentMarginRate& outData)
{
    return m_pDataMgr->m_Rates.GetMarginRate(InstrumentID,outData);
}
///设置合约保证金率
int CDataCenter::SetMarginRate(const string& InstrumentID, PlatformStru_InstrumentMarginRate& outData)
{
    m_pDataMgr->m_Rates.SetMarginRate(InstrumentID,outData);
    return 0;
}

///获取合约乘数，成功返回合约乘数，失败返回-1
int CDataCenter::GetInstrumentMultiple(const string& InstrumentID)
{
    return m_pDataMgr->m_Instruments.GetInstrumentMultiple(InstrumentID);
}
///获取指定品种的合约列表,返回合约个数
int CDataCenter::GetInstrumentListByProductID(const string& ProductID,set<string> &outData)
{
    m_pDataMgr->m_Instruments.GetInstrumentListByProductID(ProductID,outData);
    return outData.size();
}



///获取指定合约行情
int CDataCenter::GetQuotInfo(const string& InstrumentID, PlatformStru_DepthMarketData& outData)
{
    return m_pDataMgr->m_Quots.GetQuotInfo(InstrumentID,outData)?0:-1;
}


///获取指定合约旧行情
int CDataCenter::GetOldQuotInfo(const string& InstrumentID, PlatformStru_DepthMarketData& outData)
{
    return m_pDataMgr->m_Quots.GetOldQuotInfo(InstrumentID,outData)?0:-1;
}

///获取指定合约的现价，失败返回无效值
double CDataCenter::GetCurPrice(const string& InstrumentID)
{
    return m_pDataMgr->m_Quots.GetCurPrice(InstrumentID);
}

int CDataCenter::GetPositions2(const string& strInstrument,
                               set<long>& setFTID,
                               vector<PlatformStru_Position>& outData,
                               long& lastseq)
{
    return m_pDataMgr->m_Positions.GetPositions2(strInstrument,setFTID,outData,lastseq);
}

int CDataCenter::GetPositions3(const string& strInstrument,
                               vector<PlatformStru_Position>& outData,
                               long& lastseq)
{
    return m_pDataMgr->m_Positions.GetPositions3(strInstrument,outData,lastseq);
}

int CDataCenter::GetPositionDetails3(const string& strInstrument,
                          set<long>& setFTID,
                          vector<PlatformStru_PositionDetail>& vecValue,
                          long& lastseq)
{
    return m_pDataMgr->m_PositionDetails.GetPositionDetails3(strInstrument,setFTID,vecValue,lastseq);
}

int CDataCenter::GetPositionCombs2(const string& strInstrument,vector<PlatformStru_Position>& outData)
{
    return m_pDataMgr->m_PositionCombs.GetPositionCombs2(strInstrument, outData);
}

int CDataCenter::GetPositions(vector<PlatformStru_Position>& outData)
{
    return m_pDataMgr->m_Positions.GetPositions(outData);
}

int CDataCenter::GetPositionDetails(vector<PlatformStru_PositionDetail>& outData,long& lastseq)
{
    return m_pDataMgr->m_PositionDetails.GetPositionDetails(outData,lastseq);
}


int CDataCenter::GetPositionCombs(vector<PlatformStru_Position>& outData)
{
    return m_pDataMgr->m_PositionCombs.GetPositionCombs(outData);
}



bool CDataCenter::HavePositionDetail(const string& strInstrumentID)
{
    return m_pDataMgr->m_PositionDetails.HavePositionDetail(strInstrumentID);
}
bool CDataCenter::HaveCombPositionDetail()
{
    return m_pDataMgr->m_PositionDetails.HaveCombPositionDetail();
}
void CDataCenter::GetDerivedInstrumentID_OnCloseTrade(set<string>& InstrumentIDs)
{
    m_pDataMgr->m_PositionDetails.GetDerivedInstrumentID_OnCloseTrade(InstrumentIDs);
}


//获取持仓记录键值列表,返回持仓记录的数量
int CDataCenter::GetPositionKeySet(set<PositionKey> &PositionKeySet)
{
    return m_pDataMgr->m_Positions.GetPositionKeySet(PositionKeySet);
}

//获取指定合约、方向、投保的持仓数据，成功返回0，失败返回-1
int CDataCenter::GetPositionData(const string& strAccount, const string& InstrumentID,const char Direction,const char HedgeFlag,PlatformStru_Position& PositionData)
{
    return m_pDataMgr->m_Positions.GetPositionData(strAccount,InstrumentID,Direction,HedgeFlag,PositionData);
}



string CDataCenter::GetTradingDay(const string& ExchangeID)
{
    return m_pDataMgr->m_Instruments.GetTradingDay();
}

bool CDataCenter::IsTodayPosition( const char* pOpenData )
{
	return m_pDataMgr->m_Instruments.IsToday( pOpenData );
}

///获取交易所时间
unsigned long CDataCenter::GetExchangeTime(const string& ExchangeID,string& time)
{
    return m_pDataMgr->m_Instruments.GetExchangeTime(ExchangeID,time);
}
    ///获取全部交易所
int CDataCenter::GetExchangeIDs(vector<string>& outData)
{
	return m_pDataMgr->m_Instruments.GetExchangeIDs(outData);
}

///设置当前显示的持仓内容。1:持仓；2:持仓明细；3:组合持仓
void CDataCenter::SetCurrentPositionContent(int PositionContentMode)
{
    switch(PositionContentMode)
    {
        case 1:CReqOnRtnTrade::SetQryPrior(1);      break;
        case 2:CReqOnRtnTrade::SetQryPrior(2);      break;
        case 3:CReqOnRtnTrade::SetQryPrior(3);      break;
    }
}



///获取全部报单
int CDataCenter::GetAllOrders(vector<PlatformStru_OrderInfo>& outData)
{
    return m_pDataMgr->m_Orders.GetAllOrders(outData);
}
///获取全部报单
int CDataCenter::GetAllOrders2(const string& strInstrument,vector<PlatformStru_OrderInfo>& outData)
{
    return m_pDataMgr->m_Orders.GetAllOrders2(strInstrument,outData);
}
///获取指定报单
bool CDataCenter::GetOrder(const OrderKey& inOrderKey,PlatformStru_OrderInfo& outData)
{
    return m_pDataMgr->m_Orders.GetOrder(inOrderKey,outData);
}
bool CDataCenter::GetOrder2(const string& strOrderSysID,PlatformStru_OrderInfo& outData)
{
    return m_pDataMgr->m_Orders.GetOrder2(strOrderSysID,outData);
}

int CDataCenter::GetTriggerOrders(vector<PlatformStru_OrderInfo>& outData)
{
    return m_pDataMgr->m_Orders.GetTriggerOrders(outData);
}

///获取合约相关的已触发的报单
int CDataCenter::GetTriggerOrders2(const string& strInstrument,vector<PlatformStru_OrderInfo>& outData)
{
    return m_pDataMgr->m_Orders.GetTriggerOrders2(strInstrument,outData);
}

///获取指定已触发的报单
bool CDataCenter::GetTriggerOrder(const OrderKey& orderkey,PlatformStru_OrderInfo& outData)
{
    return m_pDataMgr->m_Orders.GetTriggerOrder(orderkey,outData);
}

///获取已成交报单
int CDataCenter::GetTradedOrders(vector<PlatformStru_OrderInfo>& outData)
{
    return m_pDataMgr->m_Orders.GetTradedOrders(outData);
}

///获取已成交报单
int CDataCenter::GetTradedOrders2(const string& strInstrument,vector<PlatformStru_OrderInfo>& outData)
{
    return m_pDataMgr->m_Orders.GetTradedOrders2(strInstrument,outData);
}

///获取指定已成交报单
bool CDataCenter::GetTradedOrder(const OrderKey& orderkey,PlatformStru_OrderInfo& outData)
{
    return m_pDataMgr->m_Orders.GetTradedOrder(orderkey,outData);
}

///获取已撤单和错误报单
int CDataCenter::GetCanceledOrders(vector<PlatformStru_OrderInfo>& outData)
{
    return m_pDataMgr->m_Orders.GetCanceledOrders(outData);
}

///获取已撤单和错误报单
int CDataCenter::GetCanceledOrders2(const string& strInstrument,vector<PlatformStru_OrderInfo>& outData)
{
    return m_pDataMgr->m_Orders.GetCanceledOrders2(strInstrument,outData);
}

///获取指定已撤单和错误报单
bool CDataCenter::GetCanceledOrder(const OrderKey& orderkey,PlatformStru_OrderInfo& outData)
{
    return m_pDataMgr->m_Orders.GetCanceledOrder(orderkey,outData);
}

///获取未成交报单
int CDataCenter::GetWaitOrders(vector<PlatformStru_OrderInfo>& outData)
{
    return m_pDataMgr->m_Orders.GetWaitOrders(outData);
}

///获取未成交报单
int CDataCenter::GetWaitOrders3(const string& strInstrument,vector<PlatformStru_OrderInfo>& outData)
{
    return m_pDataMgr->m_Orders.GetWaitOrders3(strInstrument,outData);
}
///获取指定合约相关的未成交报单, 不包含手工审核中的报单，合约是单合约，报单是对应报单，或包含该合约的组合报单
int CDataCenter::GetWaitOrders2(const string& strInstrument,vector<PlatformStru_OrderInfo>& outData)
{
    return m_pDataMgr->m_Orders.GetWaitOrders2(strInstrument,outData);
}

///获取指定未成交报单
bool CDataCenter::GetWaitOrder(const OrderKey& orderkey,PlatformStru_OrderInfo& outData)
{
    return m_pDataMgr->m_Orders.GetWaitOrder(orderkey,outData);
}



//获取指定合约、指定方向、指定开平的挂单数量（手数）
int  CDataCenter::GetWaitOrderVolume(const string& strAccount, const string &strInstrumentID, const char Direction, char CloseType,const char HedgeFlag)
{
    return m_pDataMgr->m_Orders.GetWaitOrderVolume(strAccount,strInstrumentID,Direction,CloseType);
}

//获取指定合约指定方向的平仓挂单的平仓量，返回0表示成功，<0表示失败。成功时CloseVolume返回平仓单(含强平和本地强平)的手数，CloseTodayVolume返回平今单手数，CloseYesterdayVolume表示平昨单手数
int CDataCenter::GetCloseOrderVolume(const string& strAccount,const string &strInstrumentID, const char& Direction,int& CloseVolume,int& CloseTodayVolume,int& CloseYesterdayVolume,const char HedgeFlag)
{
    return m_pDataMgr->m_Orders.GetCloseOrderVolume(strAccount,strInstrumentID, Direction,CloseVolume,CloseTodayVolume,CloseYesterdayVolume);
}

///获取全部成交单
int CDataCenter::GetAllTrades(vector<PlatformStru_TradeInfo>& outData)
{
    return m_pDataMgr->m_Trades.GetAllTrades(outData);
}
///获取指定成交信息
bool CDataCenter::GetTradeInfo(const TradeKey& tradekey, PlatformStru_TradeInfo& outData)
{
    return m_pDataMgr->m_Trades.GetTradeInfo(tradekey,outData);
}
///获取指定成交信息
int CDataCenter::GetTradesOfInstrument(const string& strInstrument,vector<PlatformStru_TradeInfo>& outData)
{
    return m_pDataMgr->m_Trades.GetTradesOfInstrument(strInstrument,outData);
}

///获取全部成交统计记录
int CDataCenter::GetAllTradeTotals(vector<PlatformStru_TradeTotalInfo>& outData)
{
    return m_pDataMgr->m_TradeTotals.GetAllTradeTotals(outData);
}

///获取指定合约的成交统计记录成功返回0，失败返回-1
int CDataCenter::GetTradeTotalOfInstrument(const string& strInstrument, vector<PlatformStru_TradeTotalInfo>& outData)
{
    return m_pDataMgr->m_TradeTotals.GetTradeTotalOfInstrument(strInstrument,outData);
}

///获取成交统计
int CDataCenter::GetAllTradeTotalDatas(vector<TotalInfo>& outData)
{
    return 0;
}

///在成交统计查找参数rawData
int CDataCenter::FindIndexFromAllTradeTotalDatas(const PlatformStru_TradeInfo& rawData )
{
    return 0;
}
int CDataCenter::GetTradingAccountAvailable(double& fAvailable)
{
    return m_pDataMgr->m_TradingAccount.GetTradingAccountAvailable(fAvailable);
}
int CDataCenter::GetTradingAccountWithdrawQuota(double& fWithdrawQuota)
{
    return m_pDataMgr->m_TradingAccount.GetTradingAccountWithdrawQuota(fWithdrawQuota);
}
int CDataCenter::GetTradingAccountID(char* AccountID,int rltsize)
{
    return m_pDataMgr->m_TradingAccount.GetTradingAccountID(AccountID,rltsize);
}
///获取交易资金账户信息
int CDataCenter::GetTradingAccount(PlatformStru_TradingAccountInfo& outData)
{
    return m_pDataMgr->m_TradingAccount.GetTradingAccount(outData);
}
///获取账户资金文本信息
int CDataCenter::GetAccountText(string& outData,int language)
{
    return m_pDataMgr->m_TradingAccount.GetAccountText(outData,language);
}
    
///请求查询投资者结算结果响应
int CDataCenter::GetLastSettlementInfo(string& outData)
{
    return m_pDataMgr->GetLastSettlementInfo(outData);
}
 //获取客户签约银行信息
int CDataCenter::GetContractBank(map<string,  PlatformStru_ContractBank>& outData)
{
    return m_pDataMgr->GetContractBank(outData);
}

SERVER_PARAM_CFG&  CDataCenter::GetServerParamCfg()
{
    return m_pDataMgr->GetServerParamCfg();
}

void CDataCenter::ClearMapQryRltOrders()
{
    m_pDataMgr->m_Orders.ClearMapQryRltOrders();
}

void CDataCenter::ClearMapQryRltTrades()
{
    m_pDataMgr->m_Trades.ClearMapQryRltTrades();
//    m_pDataMgr->ClearMapQryRltTrades();
}

void CDataCenter::ClearMapQryRltPositionDetails(string& strQryInstrument)
{
    m_pDataMgr->m_PositionDetails.ClearMapQryRltPositionDetails(strQryInstrument);
}

void CDataCenter::ClearMapQryRltPositions(string& strQryInstrument)
{
    m_pDataMgr->m_Positions.ClearMapQryRltPositions(strQryInstrument);
    m_pDataMgr->m_PositionCombs.ClearMapQryRltPositionCombs(strQryInstrument);
}

void CDataCenter::ClearMapQryRltPositionDetailComb(string& strQryInstrument)
{
    m_pDataMgr->m_PositionDetailCombs.ClearMapQryRltPositionDetailCombs(strQryInstrument);
}

//UI层通知底层，初始化查询开始或完成
void CDataCenter::NotifyInitQryStart()
{
    LOG_INFO("CDataCenter::NotifyInitQryStart - %s","初始化查询开始");

    //if(bStart)
    //    m_bStartInitQry=true;
    //else
    //    m_bStopInitQry=true;

  //  if(!bStart)
  //  {
  //      //初始化查询完成，根据日初持仓明细、日内成交记录，重新计算成交的平今量、平仓盈亏、手续费
  //      m_pDataMgr->m_PositionDetails.ReCalcuTradeInfo_OnInitQryCompleted();

  //      //根据组合持仓明细和持仓明细，更新组合持仓的持仓成本、开仓成本、持仓均价和开仓均价。
  //      m_pDataMgr->m_PositionCombs.UpdateCostByPositionDetialComb_AfterInitQry();

  //      //更新资金
		//m_pDataMgr->m_TradingAccount.UpdateTradingAccount_RtnTrade_RspQryPosition_RspQryTrade_InitQryCompleted();
  //  }
}
//UI层通知底层，初始化查询开始或完成
void CDataCenter::NotifyInitQrySucc()
{
    LOG_INFO("CDataCenter::NotifyInitQrySucc - %s","初始化查询成功");

    //if(bStart)
    //    m_bStartInitQry=true;
    //else
    //    m_bStopInitQry=true;

    //if(!bStart)
    //{
    //初始化查询完成，根据日初持仓明细、日内成交记录，重新计算成交的平今量、平仓盈亏、手续费
    m_pDataMgr->m_PositionDetails.ReCalcuTradeInfo_OnInitQryCompleted();

    //根据组合持仓明细和持仓明细，更新组合持仓的持仓成本、开仓成本、持仓均价和开仓均价。
    m_pDataMgr->m_PositionCombs.UpdateCostByPositionDetialComb_AfterInitQry();

    //更新资金
	m_pDataMgr->m_TradingAccount.UpdateTradingAccount_RtnTrade_RspQryPosition_RspQryTrade_InitQryCompleted();
    //}
}

int CDataCenter::GetAllOrderFTIDList( vector<long>& vec )
{
	return m_pDataMgr->m_Orders.GetAllOrderFTIDList(vec);
}

int CDataCenter::GetAllOrderInfo( long lFTID, PlatformStru_OrderInfo& outData )
{
	return m_pDataMgr->m_Orders.GetAllOrderInfo(lFTID, outData);
}

int CDataCenter::GetAllOrderInfo( OrderKey key, PlatformStru_OrderInfo& outData )
{
	return m_pDataMgr->m_Orders.GetAllOrderInfo(key, outData);
}

int CDataCenter::GetPositionFTIDList( vector<long>& vec )
{
	return m_pDataMgr->m_Positions.GetPositionFTIDList(vec);
}

int CDataCenter::GetPositionInfo( long lFTID, PlatformStru_Position& outData )
{
	return m_pDataMgr->m_Positions.GetPositionInfo(lFTID, outData);
}

int CDataCenter::GetPositionInfo( PositionKey key, PlatformStru_Position& outData )
{
	return m_pDataMgr->m_Positions.GetPositionInfo(key, outData);
}

int CDataCenter::GetPositionCombFTIDList( vector<long>& vec )
{
	return m_pDataMgr->m_PositionCombs.GetPositionCombFTIDList(vec);
}

int CDataCenter::GetPositionCombInfo( long lFTID, PlatformStru_Position& outData )
{
	return m_pDataMgr->m_PositionCombs.GetPositionCombInfo(lFTID, outData);
}

//获取交易连接的FrontID和SessionID。这两个值在交易登录时由ctp返回
void CDataCenter::GetTradingLoginInfo(int& FrontID,int& SessionID,string& UserID)
{
    m_pDataMgr->m_LoginInfo.GetTradingLoginInfo(FrontID,SessionID,UserID);
}

//获取币种信息
vector<string> CDataCenter::GetCurrencys(const string& BankName)
{
    return m_pDataMgr->GetCurrencys(BankName);
}
