
#pragma warning(disable : 4996)
#pragma warning(disable : 4786)



#define WRITELOGID

#include "SvrDBOpr.h"
#include "easymutex.h"
#include "CommonPkg.h"
#include <iostream>
#include "WriteLog.h"
#include "Guard.h"
#include "CommonErrorCode.h"
#include "CommonDef.h"
#include "SvrDBOprImpl.h"
#include <sstream>
#include "CommonStruct.h"
#include "CommonMacro.h"
using namespace std;
using namespace oracle::occi;
#define  CHUNKSIZE 10240
CSvrDBOprImpl* CSvrDBOprImpl::m_pObj=NULL;
const int nMaxInsertRow = 10000;
const int nMaxRow = 30000;
#define NEW_ARRAY_PTR(type,ptrname)  type* ptrname=new type[nMaxInsertRow];CSafeArrayPtr<type> ap_##ptrname(ptrname) 
#define safestrcpy(dest,destlen,src) strncpy_s(dest,destlen,src,(destlen)-1)
#define SETDBBUF_INT(tag)	do{m_pStmt->setDataBuffer(++index,tag,OCCIINT, sizeof(int), len_int);}while(0)
#define SETDBBUF_DOUBLE(tag)	do{m_pStmt->setDataBuffer(++index,tag,OCCIFLOAT, sizeof(double), len_double);}while(0)
#define SETDBBUF_CHAR(tag) do{m_pStmt->setDataBuffer(++index,tag,OCCI_SQLT_CHR, sizeof(char), len_char);}while(0)
#define SETDBBUF_LONG(tag) do{m_pStmt->setDataBuffer(++index,tag,OCCI_SQLT_LNG, sizeof(long), len_long);}while(0)
#define SETDBBUF_STR(tag,type,lenarray)	do{m_pStmt->setDataBuffer(++index,tag,OCCI_SQLT_STR, sizeof(type), lenarray);}while(0)
#define FILLDB_EQUVAL(tag) tag[nRecordNum] = field.tag
#define FILLDB_STR(tag,lenarray) \
    do{\
    safestrcpy(tag[nRecordNum],sizeof(field.tag),field.tag);\
    lenarray[nRecordNum]=strlen(field.tag)+1;\
    }while(0)




void PushColumnDataToVector(std::vector<ColumeData>& ltest,oracle::occi::Type nType,int nColumnMaxLen,int nColunmMemOffset)
{
    ColumeData ls;
    ls.eColumeType = nType; //InstrumentID
    ls.nColumeMaxLen = nColumnMaxLen;
    ls.nOffset = nColunmMemOffset;
    ltest.push_back(ls);
}

void PushColumnDataToVectorEx(std::vector<ColumeDataEx>& ltest,oracle::occi::Type nType,int nColumnMaxLen,int nColunmMemOffset,
                              const std::string nsColumeName)
{
    ColumeDataEx ls;
    ls.eColumeType = nType; //InstrumentID
    ls.nColumeMaxLen = nColumnMaxLen;
    ls.nOffset = nColunmMemOffset;
    ls.msColumeName = nsColumeName;
    ltest.push_back(ls);
}
CSvrDBOprImpl::CSvrDBOprImpl()
: m_pEnvironment(NULL)
, m_pCon(NULL)
, m_pStmt(NULL)
, m_pRes(NULL)
, m_strUserName("")
, m_strPwd("")
, m_strDBName("")
, m_bConn(false)
, m_pWriteLog(NULL)
{
    m_pWriteLog = new CWriteLog(WriteLogMode_LOCALFILE, "CSvrDBOprImpl.log");
	m_nQuotSeq=0;
}

CSvrDBOprImpl::~CSvrDBOprImpl()
{
    DisConnected();

    if ( m_pWriteLog != NULL )
    {
        delete m_pWriteLog;
        m_pWriteLog = NULL;
    }
}

CSvrDBOprImpl& CSvrDBOprImpl::getObj(void)
{
    CGuard guard(&g_mutex);
    if(!m_pObj)
        m_pObj=new CSvrDBOprImpl();
    return *m_pObj;
}

//void CSvrDBOprImpl::deleteObj()
//{
//	CGuard guard(&g_mutex);
//	if( NULL == m_pObj)
//	{
//		delete m_pObj;
//		m_pObj = NULL;
//	}
//}

void CSvrDBOprImpl::InitDB( const string& strUserName, const string& strPwd, const string& strDBName )
{
    CGuard guard(&g_mutex);
    m_strUserName = strUserName;
    m_strPwd = strPwd;
    m_strDBName = strDBName;
}

bool CSvrDBOprImpl::Conncect()
{
    WriteLog("Conncect", "Connecting");
    CGuard guard(&g_mutex);
    char szBuffer[1024];
    memset(szBuffer, 0, sizeof(szBuffer));
    sprintf(szBuffer, "%s, %s, %s", m_strUserName.c_str(), m_strPwd.c_str(), m_strDBName.c_str());
    WriteLog("", szBuffer);
    if ( m_strUserName.empty() || m_strPwd.empty() || m_strDBName.empty() )
    {
        return false;
    }

    try{
        m_pEnvironment = Environment::createEnvironment(Environment::DEFAULT);
        if ( NULL == m_pEnvironment )
        {
            return false;
        }

        m_pCon = m_pEnvironment->createConnection(m_strUserName, m_strPwd, m_strDBName);
        if ( NULL == m_pCon )
        {
            return false;
        }
        else
        {
            m_pCon->setTAFNotify(&NotifyFn, NULL);
        }

    }catch(SQLException &e){
        std::cout<<e.what()<<endl;
        WriteLog("Conncect Exception", e.what());
        return false;
    }
    WriteLog("Conncect", "Connected success");
    m_bConn = true;
    return true;
}

bool CSvrDBOprImpl::IsConnected()
{
     CGuard guard(&g_mutex);	
     if( m_pCon == NULL)
         return false;
 
     try
     {
         m_pStmt = m_pCon->createStatement();
         m_pStmt->execute( "select 1 from dual" );
         m_pCon->commit();
         m_pCon->terminateStatement(m_pStmt);
     }catch(oracle::occi::SQLException &e){
         RollBack();
         //std::cout<<e.what()<<endl;
         //std::cout<<pSql<<endl;
         //nErrorCode = GetErrorCode(e.what());
         WriteLog("Test connect", e.what());
         WriteLog("IsConnected", "Is not Connected");
         return false;
     }
     WriteLog("IsConnected", "Is Connected");
     return true;

    //return m_bConn;
}

void CSvrDBOprImpl::DisConnected()
{
    WriteLog("Disconnect","Disconnect");
    if ( !m_bConn )
    {
        return;
    }

    try
    {
        if ( NULL != m_pCon && NULL != m_pEnvironment )
        {
            m_pEnvironment->terminateConnection(m_pCon);
        }
    }
    catch (SQLException& e)
    {
        std::cout << e.what() << endl;
        WriteLog("Disconnect",e.what());
    }

    m_bConn = false;
}

int CSvrDBOprImpl::NotifyFn(oracle::occi::Environment *env, oracle::occi::Connection *conn, 
                            void *ctx, oracle::occi::Connection::FailOverType foType, 
                            oracle::occi::Connection::FailOverEventType foEvent)
{
    CGuard guard(&g_mutex);
    //cout << "TAF callback FailOverEventType " << foEvent << endl;
    //WriteLog("","TAF callback FailOverEventType ",);
    switch(foEvent)
    {
    case Connection::FO_BEGIN:
        m_pObj->WriteLog("FO_BEGIN","TAF callback start reconnecting...");
        //cout << "TAF callback start reconnecting..." << endl;
        m_pObj->m_bConn = false;
        break;
    case Connection::FO_END:
    case Connection::FO_ABORT:
        m_pObj->WriteLog("FO_ABORT FO_END","TAF callback reconnected successful");
        //cout << "TAF callback reconnected successful" << endl;
        m_pObj->m_bConn = true;
        break;
    case Connection::FO_REAUTH:
        m_pObj->WriteLog("FO_REAUTH","TAF callback FO_REAUTH ");
        break;
    case Connection::FO_ERROR:
        m_pObj->WriteLog("FO_ERROR","TAF callback Retrying ");
		printf("数据库连接错误，请检查配置文件和数据库服务器是否正常");

        return FO_RETRY;
    default:
        break;
    }

    return 0; //continue failover

}

void CSvrDBOprImpl::RollBack()
{
    try
    {
        if ( NULL != m_pCon  )
        {
            m_pCon->rollback();
            if ( NULL != m_pStmt )
            {
                m_pCon->terminateStatement(m_pStmt);
            }
        }
    }
    catch (SQLException& e)
    {
        std::cout << e.what() << endl;
        WriteLog("RollBack",e.what());

    }
}

void CSvrDBOprImpl::WriteLog(const string& strSql, const string& strError)
{
    if ( NULL != m_pWriteLog )
    {
        char szBuffer[MAX_SQL_LENGTH];
        memset(szBuffer, 0, sizeof(szBuffer));
        sprintf_s(szBuffer, MAX_USABLE_SQL_LENGTH, "%s\n%s", strError.c_str(), strSql.c_str());
        m_pWriteLog->WriteLog_Fmt("", WriteLogLevel_DEBUGINFO, szBuffer);
    }
}

/**
* @功能描述: 执行更新、删除SQL语句，返回影响的记录条数
* @参数列表: pSql :要执行的SQL语句
* @参数列表: nNum :SQL语句影响的记录条数
* @参数列表: nErrorCode :错误码
* @返 回 值: true,执行成功 false,执行失败
**/
bool CSvrDBOprImpl::ExcuteUpdate( const char* pSql, int& nNum, int& nErrorCode )
{
    CGuard guard(&g_mutex);
    nErrorCode = CF_ERROR_SUCCESS;
    if ( NULL == pSql )
    {
        nErrorCode = CF_ERROR_DATABASE_OTHER_ERROR;
        return false;
    }

    if ( !IsConnected() && !Conncect())
    {
        nErrorCode = CF_ERROR_DATABASE_OTHER_ERROR;
        return false;
    }

    try
    {
        m_pStmt = m_pCon->createStatement();
        nNum = m_pStmt->executeUpdate( pSql );
        m_pCon->commit();

        m_pCon->terminateStatement(m_pStmt);
    }catch(oracle::occi::SQLException &e){
        RollBack();
        std::cout<<e.what()<<endl;
        std::cout<<pSql<<endl;
        nErrorCode = GetErrorCode(e.what());
        WriteLog(pSql, e.what());
        return false;
    }

    return true;
}

/**
* @功能描述: 执行任意SQL语句
* @参数列表: pSql :要执行的SQL语句
* @参数列表: nErrorCode :错误码
* @返 回 值: true,执行成功 false,执行失败
**/
bool CSvrDBOprImpl::Excute( const char* pSql, int& nErrorCode )
{
    CGuard guard(&g_mutex);
    nErrorCode = CF_ERROR_SUCCESS;
    if ( NULL == pSql )
    {
        nErrorCode = CF_ERROR_DATABASE_OTHER_ERROR;
        return false;
    }

    if ( !IsConnected() && !Conncect())
    {
        nErrorCode = CF_ERROR_DATABASE_OTHER_ERROR;
        return false;
    }

    try
    {
        m_pStmt = m_pCon->createStatement();
        m_pStmt->execute( pSql );
        m_pCon->commit();

        m_pCon->terminateStatement(m_pStmt);
    }catch(oracle::occi::SQLException &e){
        RollBack();
        std::cout<<e.what()<<endl;
        std::cout<<pSql<<endl;
        nErrorCode = GetErrorCode(e.what());
        WriteLog(pSql, e.what());
        return false;
    }

    return true;
}

/**
* @功能描述: 执行查询SQL语句，返回查询的记录条数
* @参数列表: pSql :要执行的SQL语句
* @参数列表: nRecordNum :查询到的记录条数
* @参数列表: nErrorCode :错误码
* @返 回 值: true,执行成功 false,执行失败
**/
bool CSvrDBOprImpl::ExcuteSelect( const char* pSql, int& nRecordNum, int& nErrorCode )
{
    CGuard guard(&g_mutex);
    nErrorCode = CF_ERROR_SUCCESS;
    if ( NULL == pSql )
    {
        nErrorCode = CF_ERROR_DATABASE_OTHER_ERROR;
        return false;
    }

    if ( !IsConnected() && !Conncect())
    {
        nErrorCode = CF_ERROR_DATABASE_OTHER_ERROR;
        return false;
    }

    try
    {
        m_pStmt = m_pCon->createStatement( pSql );
        m_pRes = m_pStmt->executeQuery();
        if ( m_pRes->next())
        {
            nRecordNum = m_pRes->getInt(1);
        }

        m_pStmt->closeResultSet(m_pRes);
        m_pCon->terminateStatement(m_pStmt);
    }catch(oracle::occi::SQLException &e){
        RollBack();
        std::cout<<e.what()<<endl;
        std::cout<<pSql<<endl;
        nErrorCode = GetErrorCode(e.what());
        WriteLog(pSql, e.what());
        return false;
    }

    return true;
}

/**
* @功能描述: 插入一条数据，并返回插入记录的ID
* @参数列表: pSql :插入数据SQL语句
* @参数列表: pIDSql :查询可用sequnce id的SQL语句
* @参数列表: nPKID :插入的记录ID
* @参数列表: nErrorCode :错误码
* @返 回 值: true,执行成功 false,执行失败
**/
bool CSvrDBOprImpl::InsertAndReturnID( const char* pSql, const char* pIDSql, int& nPKID, int& nErrorCode )
{
    CGuard guard(&g_mutex);
    nErrorCode = CF_ERROR_SUCCESS;
    if ( NULL == pSql || (NULL != pSql && strlen(pSql) == 0) )
    {
        nErrorCode = CF_ERROR_DATABASE_OTHER_ERROR;
        return false;
    }

    if ( NULL == pIDSql || (NULL != pIDSql && strlen(pIDSql) == 0) )
    {
        nErrorCode = CF_ERROR_DATABASE_OTHER_ERROR;
        return false;
    }

    if ( !IsConnected() && !Conncect())
    {
        nErrorCode = CF_ERROR_DATABASE_OTHER_ERROR;
        return false;
    }

    try
    {
        m_pStmt = m_pCon->createStatement( );
        m_pStmt->executeUpdate( pSql );
        m_pRes = m_pStmt->executeQuery( pIDSql );
        if ( m_pRes->next())
        {
            nPKID = m_pRes->getInt(1);
        }

        m_pCon->commit();
        m_pStmt->closeResultSet(m_pRes);
        m_pCon->terminateStatement(m_pStmt);
    }catch(oracle::occi::SQLException &e){
        RollBack();
        std::cout<<e.what()<<endl;
        std::cout<<pSql<<endl;
        nErrorCode = GetErrorCode(e.what());
        WriteLog(pSql, e.what());
        return false;
    }

    return true;
}

/**
* @功能描述: 执行查询SQL语句，并返回结果集
* @参数列表: pSql :查询SQL语句
* @参数列表: vec :结果集
* @参数列表: nErrorCode :错误码
* @返 回 值: true,执行成功 false,执行失败
**/
bool CSvrDBOprImpl::QueryData( const char* pSql, std::vector<std::vector<_variant_t>>& vec, int& nErrorCode )
{
    CGuard guard(&g_mutex);
    nErrorCode = CF_ERROR_SUCCESS;
    if ( NULL == pSql || (NULL != pSql && strlen(pSql) == 0) )
    {
        nErrorCode = CF_ERROR_DATABASE_OTHER_ERROR;
        return false;
    }

    if ( !IsConnected() && !Conncect())
    {
        nErrorCode = CF_ERROR_DATABASE_OTHER_ERROR;
        return false;
    }

    try
    {
        m_pStmt = m_pCon->createStatement( );
        m_pRes = m_pStmt->executeQuery( pSql );
        std::vector<MetaData> vMetaData = m_pRes->getColumnListMetaData();
        while ( m_pRes->next())
        {
            //OutputDebugString("new line\n");
            std::vector<_variant_t> vColumn;
            for(size_t i = 0; i < vMetaData.size(); i++)
            {
                /*char s[50] = {0};
                sprintf(s,"new colume %d\n",i+1);
                OutputDebugString(s);*/
                _variant_t var;
                MetaData data = vMetaData[i];
                int nType = data.getInt(MetaData::ATTR_DATA_TYPE);
                switch(nType)
                {

                case OCCIIBDOUBLE:
                    {
                        var.vt = VT_R8;
                        var.dblVal = m_pRes->getBDouble(i+1).value;
                        break;
                    }


                case OCCI_SQLT_AFC: //ansi char
                    {
                        std::string strValue = m_pRes->getString(i+1);
                        var.vt = VT_I1;
                        var.cVal = strValue[0];
                        break;
                    }
                case OCCI_SQLT_CHR: //char string
                    {						
                        std::string strValue = m_pRes->getString(i+1);
                        var.SetString(strValue.c_str());
                    }
                    break;
                case OCCI_SQLT_NUM:
                    {
                        int nScale = data.getInt(MetaData::ATTR_SCALE);
                        if ( nScale == 0 )
                        {
                            //INT
                            var.vt = VT_INT;
                            var.intVal = m_pRes->getInt(i+1);
                        }
                        else if ( nScale == -127)
                        {
                            //DOUBLE 
                            var.vt = VT_R8;
                            var.dblVal = m_pRes->getDouble(i+1);
                        }
                        else
                        {

                        }
                        break;
                    }
                case OCCI_SQLT_DAT:
                    {//日期
                        //var.vt = VT_DATE;						
                        Date dd = m_pRes->getDate(i+1);

                        int nYear;
                        unsigned int nMonth, nDay, nHour, nMinute, nSecond;
                        dd.getDate(nYear, nMonth, nDay, nHour, nMinute, nSecond);
                        char szDate[256];
                        memset(szDate, 0, sizeof(szDate));
                        sprintf(szDate,"%4d-%02d-%02d %02d:%02d:%02d", nYear, nMonth, nDay, nHour, nMinute, nSecond);

                        var.SetString(szDate);					
                    }
                    break;
                case OCCI_SQLT_DATE:
                    //Date类型必须被格式化成字符串后返回
                    var.SetString("");
                    break;
                default:
                    break;
                }

                vColumn.push_back(var);
            }

            vec.push_back(vColumn);
        }

        m_pCon->commit();
        m_pStmt->closeResultSet(m_pRes);
        m_pCon->terminateStatement(m_pStmt);
    }catch(oracle::occi::SQLException &e){
        RollBack();
        std::cout<<e.what()<<endl;
        std::cout<<pSql<<endl;
        nErrorCode = GetErrorCode(e.what());
        WriteLog(pSql, e.what());
        return false;
    }

    return true;
}

int CSvrDBOprImpl::GetErrorCode( const std::string& strError )
{
    if ( strError.find("ORA-00001") != std::string::npos)
    {
        return CF_ERROR_DATABASE_DUPLICATE_FIELD;
    }
    else if ( strError.find("ORA-00291") != std::string::npos )
    {
        return CF_ERROR_DATABASE_NO_DEPENDENT;
    }
    else if ( strError.find("ORA-00292") != std::string::npos )
    {
        return CF_ERROR_DATABASE_RECODE_USED;
    }
    else
    {
        return CF_ERROR_DATABASE_OTHER_ERROR;
    }
}

bool CSvrDBOprImpl::SaveRolePrivilegeRelation(int nRoleID, std::vector<int>& vecPrivilegeID, int& nErrorCode )
{
    CGuard guard(&g_mutex);
    nErrorCode = CF_ERROR_SUCCESS;
    if ( !IsConnected() && !Conncect())
    {
        nErrorCode = CF_ERROR_DATABASE_OTHER_ERROR;
        return false;
    }

    char szBuffer[MAX_SQL_LENGTH];
    try
    {
        memset(szBuffer, 0, sizeof(szBuffer));
        sprintf(szBuffer, "delete RELA_ROLE_PRIVELEGE t where t.roleid = %d", nRoleID);

        m_pStmt = m_pCon->createStatement();
        m_pStmt->execute(szBuffer);

        std::vector<int>::iterator it = vecPrivilegeID.begin();
        for ( ; it != vecPrivilegeID.end(); it++ )
        {
            memset(szBuffer, 0, sizeof(szBuffer));
            sprintf(szBuffer, "insert into RELA_ROLE_PRIVELEGE values(%d, %d, %d)",nRoleID, *it, UNDELETE_FLAG);
            m_pStmt->execute( szBuffer );
        }

        m_pCon->commit();
        m_pCon->terminateStatement(m_pStmt);
    }catch(oracle::occi::SQLException &e){
        RollBack();
        std::cout<<e.what()<<endl;
        std::cout<<szBuffer<<endl;
        nErrorCode = GetErrorCode(e.what());
        WriteLog(szBuffer, e.what());
        return false;
    }

    return true;
}

bool CSvrDBOprImpl::SaveUserRole( int nUserID, std::vector<int>& vRoleID, int& nErrorCode )
{
    CGuard guard(&g_mutex);
    nErrorCode = CF_ERROR_SUCCESS;
    if ( !IsConnected() && !Conncect())
    {
        nErrorCode = CF_ERROR_DATABASE_OTHER_ERROR;
        return false;
    }

    char szBuffer[MAX_SQL_LENGTH];
    try
    {
        memset(szBuffer, 0, sizeof(szBuffer));
        sprintf(szBuffer, "delete RELA_USER_ROLE t where t.userid = %d", nUserID);

        m_pStmt = m_pCon->createStatement();
        m_pStmt->execute(szBuffer);

        std::vector<int>::iterator it = vRoleID.begin();
        for ( ; it != vRoleID.end(); it++ )
        {
            memset(szBuffer, 0, sizeof(szBuffer));
            sprintf(szBuffer, "insert into RELA_USER_ROLE values(%d, %d, %d)",
                nUserID, (*it), UNDELETE_FLAG);
            m_pStmt->execute( szBuffer );
        }

        m_pCon->commit();
        m_pCon->terminateStatement(m_pStmt);
    }catch(oracle::occi::SQLException &e){
        RollBack();
        std::cout<<e.what()<<endl;
        std::cout<<szBuffer<<endl;
        nErrorCode = GetErrorCode(e.what());
        WriteLog(szBuffer, e.what());
        return false;
    }

    return true;
}

bool CSvrDBOprImpl::SaveProductTraderRelation(int nRelationType,int nID, std::vector<int>& vID, int& nErrorCode )
{
    CGuard guard(&g_mutex);
    nErrorCode = CF_ERROR_SUCCESS;
    if ( !IsConnected() && !Conncect())
    {
        nErrorCode = CF_ERROR_DATABASE_OTHER_ERROR;
        return false;
    }

    char szBuffer[MAX_SQL_LENGTH];
    try
    {
        if (PRODUCT_ASSGIN_TRADER_TYPE == nRelationType)
        {
            memset(szBuffer, 0, sizeof(szBuffer));
            sprintf(szBuffer, "delete RELA_TRADER_PRODUCT t where t.financeproductid = %d", nID);
        } 
        else if(TRADER_ASSGIN_PRODUCT_TYPE == nRelationType)
        {
            memset(szBuffer, 0, sizeof(szBuffer));
            sprintf(szBuffer, "delete RELA_TRADER_PRODUCT t where t.traderid = %d", nID);
        }

        m_pStmt = m_pCon->createStatement();
        m_pStmt->execute(szBuffer);

        std::vector<int>::iterator it = vID.begin();
        for ( ; it != vID.end(); it++ )
        {
            if (PRODUCT_ASSGIN_TRADER_TYPE == nRelationType)
            {
                memset(szBuffer, 0, sizeof(szBuffer));
                sprintf(szBuffer, "insert into RELA_TRADER_PRODUCT values(%d, %d, %d)", nID, (*it), UNDELETE_FLAG);
            }
            else if(TRADER_ASSGIN_PRODUCT_TYPE == nRelationType)
            {
                memset(szBuffer, 0, sizeof(szBuffer));
                sprintf(szBuffer, "insert into RELA_TRADER_PRODUCT values(%d, %d, %d)", (*it), nID, UNDELETE_FLAG);
            }

            m_pStmt->execute( szBuffer );
        }

        m_pCon->commit();
        m_pCon->terminateStatement(m_pStmt);
    }catch(oracle::occi::SQLException &e){
        RollBack();
        std::cout<<e.what()<<endl;
        std::cout<<szBuffer<<endl;
        nErrorCode = GetErrorCode(e.what());
        WriteLog(szBuffer, e.what());
        return false;
    }

    return true;
}

bool CSvrDBOprImpl::SaveStrategyOrganizationRelation(int nStrategyID, std::vector<int>& vOrgID, int& nErrorCode )
{
    CGuard guard(&g_mutex);
    nErrorCode = CF_ERROR_SUCCESS;
    if ( !IsConnected() && !Conncect())
    {
        nErrorCode = CF_ERROR_DATABASE_OTHER_ERROR;
        return false;
    }

    char szBuffer[MAX_SQL_LENGTH];
    try
    {
        memset(szBuffer, 0, sizeof(szBuffer));
        sprintf(szBuffer, "delete RELA_ORG_STRATEGY t where t.strategyid = %d", nStrategyID);

        m_pStmt = m_pCon->createStatement();
        m_pStmt->execute(szBuffer);

        std::vector<int>::iterator it = vOrgID.begin();
        for ( ; it != vOrgID.end(); it++ )
        {
            memset(szBuffer, 0, sizeof(szBuffer));
            sprintf(szBuffer, "insert into RELA_ORG_STRATEGY values(%d, %d, %d)",nStrategyID, (*it), UNDELETE_FLAG);
            m_pStmt->execute( szBuffer );
        }

        m_pCon->commit();
        m_pCon->terminateStatement(m_pStmt);
    }catch(oracle::occi::SQLException &e){
        RollBack();
        std::cout<<e.what()<<endl;
        std::cout<<szBuffer<<endl;
        nErrorCode = GetErrorCode(e.what());
        WriteLog(szBuffer, e.what());
        return false;
    }

    return true;
}

bool CSvrDBOprImpl::SaveUserOrganizationRelation( int nOrgID, int nUserType, int nUserID, int nRelationType, int& nErrorCode)
{
    CGuard guard(&g_mutex);
    nErrorCode = CF_ERROR_SUCCESS;
    if ( !IsConnected() && !Conncect())
    {
        nErrorCode = CF_ERROR_DATABASE_OTHER_ERROR;
        return false;
    }

    char szBuffer[MAX_SQL_LENGTH];   
    m_pStmt = m_pCon->createStatement();

    try
    {
        //         std::vector<int>::iterator it_InAttch = vInID.begin();
        //         for ( ; it_InAttch != vInID.end(); it_InAttch++ )
        //         {
        memset(szBuffer, 0, sizeof(szBuffer));
        sprintf(szBuffer, "update RELA_USER_ORG t set t.orgid = %d where t.userid = %d and t.usertype = %d and t.relationtype = %d", nOrgID,nUserID, nUserType, nRelationType);
        m_pStmt->execute( szBuffer );
        //         }

        //默认的都移到总部ID下
        //         int nOutOrgID = ORGNIZATION_ROOT;
        //         std::vector<int>::iterator it_OutAttch = vOutID.begin();
        //         for ( ; it_OutAttch != vOutID.end(); it_OutAttch++ )
        //         {
        //             memset(szBuffer, 0, sizeof(szBuffer));
        //             sprintf(szBuffer, "update RELA_USER_ORG t set t.orgid = %d where t.userid = %d and t.usertype = %d and t.relationtype = %d", nOutOrgID,(*it_OutAttch), nUserType, nRelationType);
        //             m_pStmt->execute( szBuffer );
        //         }

        m_pCon->commit();
        m_pCon->terminateStatement(m_pStmt);

    }catch(oracle::occi::SQLException &e){
        RollBack();
        std::cout<<e.what()<<endl;
        std::cout<<szBuffer<<endl;
        nErrorCode = GetErrorCode(e.what());
        WriteLog(szBuffer, e.what());
        return false;
    }

    return true;
}

bool CSvrDBOprImpl::SaveUserOrganizationRelation( int nOrgID, int nUserType, std::vector<int>& vInID, std::vector<int>& vOutID, int nRelationType, int& nErrorCode)
{
    CGuard guard(&g_mutex);
    nErrorCode = CF_ERROR_SUCCESS;
    if ( !IsConnected() && !Conncect())
    {
        nErrorCode = CF_ERROR_DATABASE_OTHER_ERROR;
        return false;
    }

    char szBuffer[MAX_SQL_LENGTH];   
    m_pStmt = m_pCon->createStatement();

    try
    {
        std::vector<int>::iterator it_InAttch = vInID.begin();
        for ( ; it_InAttch != vInID.end(); it_InAttch++ )
        {
            memset(szBuffer, 0, sizeof(szBuffer));
            sprintf(szBuffer, "update RELA_USER_ORG t set t.orgid = %d where t.userid = %d and t.usertype = %d and t.relationtype = %d", nOrgID,(*it_InAttch), nUserType, nRelationType);
            m_pStmt->execute( szBuffer );
        }

        //默认的都移到总部ID下
        int nOutOrgID = ORGNIZATION_ROOT;
        std::vector<int>::iterator it_OutAttch = vOutID.begin();
        for ( ; it_OutAttch != vOutID.end(); it_OutAttch++ )
        {
            memset(szBuffer, 0, sizeof(szBuffer));
            sprintf(szBuffer, "update RELA_USER_ORG t set t.orgid = %d where t.userid = %d and t.usertype = %d and t.relationtype = %d", nOutOrgID,(*it_OutAttch), nUserType, nRelationType);
            m_pStmt->execute( szBuffer );
        }

        m_pCon->commit();
        m_pCon->terminateStatement(m_pStmt);

    }catch(oracle::occi::SQLException &e){
        RollBack();
        std::cout<<e.what()<<endl;
        std::cout<<szBuffer<<endl;
        nErrorCode = GetErrorCode(e.what());
        WriteLog(szBuffer, e.what());
        return false;
    }

    return true;
}

bool CSvrDBOprImpl::SaveUserOrganizationRelation( int nRiskID, int nAttachOrg, std::vector<int>& vMonitorOrg, int& nErrorCode)
{
    CGuard guard(&g_mutex);
    nErrorCode = CF_ERROR_SUCCESS;
    if ( !IsConnected() && !Conncect())
    {
        nErrorCode = CF_ERROR_DATABASE_OTHER_ERROR;
        return false;
    }

    char szBuffer[MAX_SQL_LENGTH];   
    m_pStmt = m_pCon->createStatement();

    try
    {
        memset(szBuffer, 0, sizeof(szBuffer));
        sprintf(szBuffer, "update RELA_USER_ORG t set t.orgid = %d where t.userid = %d and t.usertype = %d and t.relationtype = %d", nAttachOrg,nRiskID, USER_TYPE_RISK_CONTROL, USER_ORG_RELATION_ATTACH);
        m_pStmt->execute(szBuffer);

        memset(szBuffer, 0, sizeof(szBuffer));
        sprintf(szBuffer, "delete RELA_USER_ORG t where t.userid = %d and t.relationtype = %d", nRiskID, USER_ORG_RELATION_MONITOR);
        m_pStmt->execute(szBuffer);

        std::vector<int>::iterator it = vMonitorOrg.begin();
        for ( ; it != vMonitorOrg.end(); it++ )
        {
            memset(szBuffer, 0, sizeof(szBuffer));
            sprintf(szBuffer, "insert into RELA_USER_ORG values(%d, %d, %d, %d, %d)", nRiskID, (*it), USER_ORG_RELATION_MONITOR, USER_TYPE_RISK_CONTROL, UNDELETE_FLAG);
            m_pStmt->execute( szBuffer );
        }

        m_pCon->commit();
        m_pCon->terminateStatement(m_pStmt);

    }catch(oracle::occi::SQLException &e){
        RollBack();
        std::cout<<e.what()<<endl;
        std::cout<<szBuffer<<endl;
        nErrorCode = GetErrorCode(e.what());
        WriteLog(szBuffer, e.what());
        return false;
    }

    return true;
}

//保存合约
bool CSvrDBOprImpl::SaveInstruments(const std::string& strTime, 
                                    const std::vector<PlatformStru_InstrumentInfo>& nVecInstruemnts ,
                                    int& nErrorCode ,
									const std::string& nsTableName)
{

    PlatformStru_InstrumentInfo lInfo;
    std::vector<ColumeData> ltest;
    ColumeData ls;
    ls.eColumeType = OCCI_SQLT_STR; //InstanceID
    ls.nColumeMaxLen = sizeof(InstrumentIDType);
    ls.nOffset = (int)&lInfo.InstrumentID - (int)&lInfo;
    ltest.push_back(ls);

    ls.eColumeType = OCCI_SQLT_STR;//ExchangeID
    ls.nColumeMaxLen = sizeof(TThostFtdcExchangeIDType);
    ls.nOffset = (int)&lInfo.ExchangeID- (int)&lInfo;
    ltest.push_back(ls);

    ls.eColumeType = OCCI_SQLT_STR;//InstrumentName
    ls.nColumeMaxLen = sizeof(InstrumentIDType);
    ls.nOffset = (int)&lInfo.InstrumentName- (int)&lInfo;
    ltest.push_back(ls);

    ls.eColumeType = OCCI_SQLT_STR;//ExchangeInstID
    ls.nColumeMaxLen = sizeof(TThostFtdcExchangeInstIDType);
    ls.nOffset = (int)&lInfo.ExchangeInstID- (int)&lInfo;
    ltest.push_back(ls);

    ls.eColumeType = OCCI_SQLT_STR;//PriductID
    ls.nColumeMaxLen = sizeof(InstrumentIDType);
    ls.nOffset = (int)&lInfo.ProductID- (int)&lInfo;
    ltest.push_back(ls);

    ls.eColumeType = OCCI_SQLT_CHR;//ProductClass
    ls.nColumeMaxLen = sizeof(TThostFtdcProductClassType);
    ls.nOffset = (int)&lInfo.ProductClass- (int)&lInfo;
    ltest.push_back(ls);

    ls.eColumeType = OCCIINT; //DeliverYear
    ls.nColumeMaxLen = sizeof(TThostFtdcYearType);
    ls.nOffset = (int)&lInfo.DeliveryYear- (int)&lInfo;
    ltest.push_back(ls);

    ls.eColumeType = OCCIINT;//DeliverMonth
    ls.nColumeMaxLen = sizeof(TThostFtdcMonthType);
    ls.nOffset = (int)&lInfo.DeliveryMonth- (int)&lInfo;
    ltest.push_back(ls);

    ls.eColumeType = OCCIINT;//MaxMarkedOrderVolume
    ls.nColumeMaxLen = sizeof(TThostFtdcVolumeType);
    ls.nOffset = (int)&lInfo.MaxMarketOrderVolume- (int)&lInfo;
    ltest.push_back(ls);

    ls.eColumeType = OCCIINT;//MinMarketOrderVolume
    ls.nColumeMaxLen = sizeof(TThostFtdcVolumeType);
    ls.nOffset = (int)&lInfo.MinMarketOrderVolume- (int)&lInfo;
    ltest.push_back(ls);

    ls.eColumeType = OCCIINT;//MaxLimitOrderVolume
    ls.nColumeMaxLen = sizeof(TThostFtdcVolumeType);
    ls.nOffset = (int)&lInfo.MaxLimitOrderVolume- (int)&lInfo;
    ltest.push_back(ls);	

    ls.eColumeType = OCCIINT;//MinLimitOrderVolume
    ls.nColumeMaxLen = sizeof(TThostFtdcVolumeType);
    ls.nOffset = (int)&lInfo.MinLimitOrderVolume- (int)&lInfo;
    ltest.push_back(ls);		

    ls.eColumeType = OCCIINT;//VolumeMultiple
    ls.nColumeMaxLen = sizeof(TThostFtdcVolumeMultipleType);
    ls.nOffset = (int)&lInfo.VolumeMultiple- (int)&lInfo;
    ltest.push_back(ls);	

    ls.eColumeType = OCCIBDOUBLE;//PriceTick
    ls.nColumeMaxLen = sizeof(TThostFtdcPriceType);
    ls.nOffset = (int)&lInfo.PriceTick- (int)&lInfo;
    ltest.push_back(ls);	

    //创建日
    ls.eColumeType = OCCI_SQLT_STR;//CreateDate
    ls.nColumeMaxLen = sizeof(TThostFtdcDateType);
    ls.nOffset = (int)&lInfo.CreateDate- (int)&lInfo;
    ltest.push_back(ls);	

    ls.eColumeType = OCCI_SQLT_STR;//OpenDate
    ls.nColumeMaxLen = sizeof(TThostFtdcDateType);
    ls.nOffset = (int)&lInfo.OpenDate- (int)&lInfo;
    ltest.push_back(ls);	

    ls.eColumeType = OCCI_SQLT_STR;//ExpireDate
    ls.nColumeMaxLen = sizeof(TThostFtdcDateType);
    ls.nOffset = (int)&lInfo.ExpireDate- (int)&lInfo;
    ltest.push_back(ls);	

    ls.eColumeType = OCCI_SQLT_STR;//StartDelivDate
    ls.nColumeMaxLen = sizeof(TThostFtdcDateType);
    ls.nOffset = (int)&lInfo.StartDelivDate- (int)&lInfo;
    ltest.push_back(ls);	

    ls.eColumeType = OCCI_SQLT_STR;//EndDeliveDate
    ls.nColumeMaxLen = sizeof(TThostFtdcDateType);
    ls.nOffset = (int)&lInfo.EndDelivDate- (int)&lInfo;
    ltest.push_back(ls);	

    ls.eColumeType = OCCI_SQLT_CHR;//InstLifePhase
    ls.nColumeMaxLen = sizeof(TThostFtdcInstLifePhaseType);
    ls.nOffset = (int)&lInfo.InstLifePhase- (int)&lInfo;
    ltest.push_back(ls);	

    ls.eColumeType = OCCIINT;//IsTrading
    ls.nColumeMaxLen = sizeof(TThostFtdcBoolType);
    ls.nOffset = (int)&lInfo.IsTrading- (int)&lInfo;
    ltest.push_back(ls);	

    ls.eColumeType = OCCI_SQLT_CHR; //PositionType
    ls.nColumeMaxLen = sizeof(TThostFtdcPositionTypeType);
    ls.nOffset = (int)&lInfo.PositionType- (int)&lInfo;
    ltest.push_back(ls);	

    ls.eColumeType = OCCI_SQLT_CHR;//PositionDateType
    ls.nColumeMaxLen = sizeof(TThostFtdcPositionDateTypeType);
    ls.nOffset = (int)&lInfo.PositionDateType- (int)&lInfo;
    ltest.push_back(ls);	

    ls.eColumeType = OCCIBDOUBLE;//LongMarginRatio
    ls.nColumeMaxLen = sizeof(TThostFtdcRatioType);
    ls.nOffset = (int)&lInfo.LongMarginRatio- (int)&lInfo;
    ltest.push_back(ls);	

    ls.eColumeType = OCCIBDOUBLE;//ShortMarginRatio
    ls.nColumeMaxLen = sizeof(TThostFtdcRatioType);
    ls.nOffset = (int)&lInfo.ShortMarginRatio- (int)&lInfo;
    ltest.push_back(ls);	

    ls.eColumeType = OCCI_SQLT_CHR;//MaxMarginSideAlgorithm
    ls.nColumeMaxLen = sizeof(TThostFtdcMaxMarginSideAlgorithmType);
    ls.nOffset = (int)&lInfo.MaxMarginSideAlgorithm - (int)&lInfo;
    ltest.push_back(ls);

    ls.eColumeType = OCCIINT;//CombMarginDiscountMode
    ls.nColumeMaxLen = sizeof(int);
    ls.nOffset = (int)&lInfo.CombMarginDiscountMode - (int)&lInfo;
    ltest.push_back(ls);

    ls.eColumeType = OCCIINT;//UnwindPriorities
    ls.nColumeMaxLen = sizeof(int);
    ls.nOffset = (int)&lInfo.UnwindPriorities - (int)&lInfo;
    ltest.push_back(ls);

    ls.eColumeType = OCCIINT;//PriceForMarginOfTodayPosition
    ls.nColumeMaxLen = sizeof(int);
    ls.nOffset = (int)&lInfo.PriceForMarginOfTodayPosition - (int)&lInfo;
    ltest.push_back(ls);	

    ls.eColumeType = OCCIINT;//CloseTodayInstructionSupport
    ls.nColumeMaxLen = sizeof(int);
    ls.nOffset = (int)&lInfo.CloseTodayInstructionSupport - (int)&lInfo;
    ltest.push_back(ls);

    ls.eColumeType = OCCIINT;//CloseInstructionSupport
    ls.nColumeMaxLen = sizeof(int);
    ls.nOffset = (int)&lInfo.CloseInstructionSupport - (int)&lInfo;
    ltest.push_back(ls);

    ls.eColumeType = OCCI_SQLT_STR;//Currency
    ls.nColumeMaxLen = sizeof(char[11]);
    ls.nOffset = (int)&lInfo.Currency - (int)&lInfo;
    ltest.push_back(ls);

    ls.eColumeType = OCCIINT;//TicksPerPoint
    ls.nColumeMaxLen = sizeof(int);
    ls.nOffset = (int)&lInfo.TicksPerPoint - (int)&lInfo;
    ltest.push_back(ls);

    ls.eColumeType = OCCI_SQLT_STR;//TickSize
    ls.nColumeMaxLen = sizeof(char[10]);
    ls.nOffset = (int)&lInfo.TickSize - (int)&lInfo;
    ltest.push_back(ls);

    std::string lsDeleteSql = 
        "delete from " + nsTableName + " t where t.ValidateDate = \'" + strTime + "\'" ;

    return BatchInsert(	nsTableName,
        lsDeleteSql,
        strTime,
        ltest,
        nVecInstruemnts,
        nErrorCode);

}

//保存费率
bool CSvrDBOprImpl::SaveAccountCommissionRate(const std::string& nsTableName,
                                              const std::string& strTime,
                                              const std::string& nsBrokerID,
                                              const std::string& nsAccountID,
                                              const std::vector<PlatformStru_InstrumentCommissionRate>& nCommissions,
                                              int& nErrorCode ) 
{
    PlatformStru_InstrumentCommissionRate lInfo;
    std::vector<ColumeData> ltest;
    ColumeData ls;
    ls.eColumeType = OCCI_SQLT_STR; //InstrumentID
    ls.nColumeMaxLen = sizeof(TThostFtdcInstrumentIDType);
    ls.nOffset = (int)&lInfo.InstrumentID - (int)&lInfo;
    ltest.push_back(ls);

    ls.eColumeType = OCCI_SQLT_CHR;//InvestorRange
    ls.nColumeMaxLen = sizeof(TThostFtdcInvestorRangeType);
    ls.nOffset = (int)&lInfo.InvestorRange - (int)&lInfo;
    ltest.push_back(ls);

    ls.eColumeType = OCCI_SQLT_STR;//BrokerID
    ls.nColumeMaxLen = sizeof(TThostFtdcBrokerIDType);
    ls.nOffset = (int)&lInfo.BrokerID - (int)&lInfo;
    ltest.push_back(ls);

    ls.eColumeType = OCCI_SQLT_STR;//InvestorID
    ls.nColumeMaxLen = sizeof(TThostFtdcInvestorIDType);
    ls.nOffset = (int)&lInfo.InvestorID - (int)&lInfo;
    ltest.push_back(ls);

    ls.eColumeType = OCCIBDOUBLE;//OpenRatiosByMoney
    ls.nColumeMaxLen = sizeof(TThostFtdcRatioType);
    ls.nOffset = (int)&lInfo.OpenRatioByMoney - (int)&lInfo;
    ltest.push_back(ls);

    ls.eColumeType = OCCIBDOUBLE;//OpenRatiosByVolume
    ls.nColumeMaxLen = sizeof(TThostFtdcRatioType);
    ls.nOffset = (int)&lInfo.OpenRatioByVolume - (int)&lInfo;
    ltest.push_back(ls);

    ls.eColumeType = OCCIBDOUBLE; //CloseRatioByMoney
    ls.nColumeMaxLen = sizeof(TThostFtdcRatioType);
    ls.nOffset = (int)&lInfo.CloseRatioByMoney - (int)&lInfo;
    ltest.push_back(ls);

    ls.eColumeType = OCCIBDOUBLE;//CloseRatioByVolume
    ls.nColumeMaxLen = sizeof(TThostFtdcRatioType);
    ls.nOffset = (int)&lInfo.CloseRatioByVolume - (int)&lInfo;
    ltest.push_back(ls);

    ls.eColumeType = OCCIBDOUBLE;//CloseTodayRatioByMoney
    ls.nColumeMaxLen = sizeof(TThostFtdcRatioType);
    ls.nOffset = (int)&lInfo.CloseTodayRatioByMoney - (int)&lInfo;
    ltest.push_back(ls);

    ls.eColumeType = OCCIBDOUBLE;//CloseTodayRatioByVolume
    ls.nColumeMaxLen = sizeof(TThostFtdcRatioType);
    ls.nOffset = (int)&lInfo.CloseTodayRatioByVolume - (int)&lInfo;
    ltest.push_back(ls);

    std::string lsDeleteSql = "delete from " +nsTableName  +" t where t.ValidateDate = \'" 
        + strTime   
        +"\' AND BrokerID = \'" 
        + nsBrokerID + 
        +"\' AND InvestorID = \'" 
        + nsAccountID + "\'";

    if(nsBrokerID == "*")
    {
        lsDeleteSql.clear();
        lsDeleteSql = "delete from " +nsTableName  +" t where t.ValidateDate = \'" 	+ strTime +  "\'";
    }
    return BatchInsert(	nsTableName,
        lsDeleteSql,
        strTime,
        ltest,
        nCommissions,
        nErrorCode);


}


//保证金率
bool CSvrDBOprImpl::SaveAccountMarginRate(const std::string& nsTableName,
                                          const std::string& strTime,
                                          const std::string& nsBrokerID,
                                          const std::string& nsAccountID,
                                          const std::vector<PlatformStru_InstrumentMarginRate>& nMargins ,
                                          int& nErrorCode ) 
{
    PlatformStru_InstrumentMarginRate lInfo;
    std::vector<ColumeData> ltest;
    ColumeData ls;
    ls.eColumeType = OCCI_SQLT_STR; //InstrumentID
    ls.nColumeMaxLen = sizeof(TThostFtdcInstrumentIDType);
    ls.nOffset = (int)&lInfo.InstrumentID - (int)&lInfo;
    ltest.push_back(ls);

    ls.eColumeType = OCCI_SQLT_CHR;//InvestorRange
    ls.nColumeMaxLen = sizeof(TThostFtdcInvestorRangeType);
    ls.nOffset = (int)&lInfo.InvestorRange - (int)&lInfo;
    ltest.push_back(ls);

    ls.eColumeType = OCCI_SQLT_STR;//BrokerID
    ls.nColumeMaxLen = sizeof(TThostFtdcBrokerIDType);
    ls.nOffset = (int)&lInfo.BrokerID - (int)&lInfo;
    ltest.push_back(ls);

    ls.eColumeType = OCCI_SQLT_STR;//InvestorID
    ls.nColumeMaxLen = sizeof(TThostFtdcInvestorIDType);
    ls.nOffset = (int)&lInfo.InvestorID - (int)&lInfo;
    ltest.push_back(ls);

    ls.eColumeType = OCCI_SQLT_CHR;//hedeflag
    ls.nColumeMaxLen = sizeof(TThostFtdcHedgeFlagType	);
    ls.nOffset = (int)&lInfo.HedgeFlag - (int)&lInfo;
    ltest.push_back(ls);

    ls.eColumeType = OCCIBDOUBLE;//LongMarginRatioByMoney;
    ls.nColumeMaxLen = sizeof(TThostFtdcRatioType);
    ls.nOffset = (int)&lInfo.LongMarginRatioByMoney - (int)&lInfo;
    ltest.push_back(ls);

    ls.eColumeType = OCCIBDOUBLE; //LongMarginRatioByVolume;
    ls.nColumeMaxLen = sizeof(TThostFtdcRatioType);
    ls.nOffset = (int)&lInfo.LongMarginRatioByVolume - (int)&lInfo;
    ltest.push_back(ls);

    ls.eColumeType = OCCIBDOUBLE;//ShortMarginRatioByMoney;
    ls.nColumeMaxLen = sizeof(TThostFtdcRatioType);
    ls.nOffset = (int)&lInfo.ShortMarginRatioByMoney - (int)&lInfo;
    ltest.push_back(ls);

    ls.eColumeType = OCCIBDOUBLE;//ShortMarginRatioByVolume;
    ls.nColumeMaxLen = sizeof(TThostFtdcRatioType);
    ls.nOffset = (int)&lInfo.ShortMarginRatioByVolume- (int)&lInfo;
    ltest.push_back(ls);

    ls.eColumeType = OCCIINT;//IsRelative;
    ls.nColumeMaxLen = sizeof(TThostFtdcBoolType);
    ls.nOffset = (int)&lInfo.IsRelative - (int)&lInfo;
    ltest.push_back(ls);


    std::string lsDeleteSql = "delete from " +nsTableName  +" t where t.ValidateDate = \'" 
        + strTime   
        +"\' AND BrokerID = \'" 
        + nsBrokerID + 
        +"\' AND InvestorID = \'" 
        + nsAccountID + "\'";

    if(nsBrokerID == "*")
    {
        lsDeleteSql.clear();
        lsDeleteSql = "delete from " +nsTableName  +" t where t.ValidateDate = \'" 	+ strTime +  "\'";
    }

    return BatchInsert(	nsTableName,
        lsDeleteSql,
        strTime,
        ltest,
        nMargins,
        nErrorCode);
    return false;
}
//报单
bool CSvrDBOprImpl::SaveUserOrderInfos(const std::string& nsTableName,
                                       bool nbDelete,
                                       const std::string& strTime,
                                       const std::string& nsUserName,
                                       const std::vector<PlatformStru_OrderInfo>& nOrders ,
                                       int& nErrorCode )
{
    //return MergeOrderInfos(nsTableName,strTime,nOrders,nErrorCode);

    //先删除今天该账户所有的，然后再添加
    PlatformStru_OrderInfo lInfo;
    std::vector<ColumeData> ltest;

    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcBrokerIDType),(int)&lInfo.BrokerID - (int)&lInfo); //BrokerID
    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcInvestorIDType),(int)&lInfo.InvestorID - (int)&lInfo); //InvestorID
    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcInstrumentIDType),(int)&lInfo.InstrumentID - (int)&lInfo); //InstrumentID
    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcOrderRefType),(int)&lInfo.OrderRef - (int)&lInfo);//Orderref
    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcUserIDType),(int)&lInfo.UserID - (int)&lInfo);    //UserID
    PushColumnDataToVector(ltest,OCCI_SQLT_CHR,sizeof(TThostFtdcOrderPriceTypeType),(int)&lInfo.OrderPriceType - (int)&lInfo);	//OrderPriceType
    PushColumnDataToVector(ltest,OCCI_SQLT_CHR,sizeof(TThostFtdcDirectionType),(int)&lInfo.Direction - (int)&lInfo);            //Driection
    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcCombOffsetFlagType),(int)&lInfo.CombOffsetFlag - (int)&lInfo);  //ComboffsetFlag
    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcCombHedgeFlagType),(int)&lInfo.CombHedgeFlag - (int)&lInfo);	//combhedgeflag
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.LimitPrice - (int)&lInfo);       //LimitPrice
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.VolumeTotalOriginal - (int)&lInfo); //VolumeTotalOriginal
    PushColumnDataToVector(ltest,OCCI_SQLT_CHR,sizeof(TThostFtdcTimeConditionType),(int)&lInfo.TimeCondition - (int)&lInfo);	 //TimeCondition
    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcDateType),(int)&lInfo.GTDDate - (int)&lInfo); //GTData
    PushColumnDataToVector(ltest,OCCI_SQLT_CHR,sizeof(TThostFtdcVolumeConditionType),(int)&lInfo.VolumeCondition - (int)&lInfo); //VolumeCondition
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.MinVolume - (int)&lInfo); //MinVolumn
    PushColumnDataToVector(ltest,OCCI_SQLT_CHR,sizeof(TThostFtdcContingentConditionType),(int)&lInfo.ContingentCondition - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.StopPrice - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCI_SQLT_CHR,sizeof(TThostFtdcForceCloseReasonType),(int)&lInfo.ForceCloseReason - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcBoolType),(int)&lInfo.IsAutoSuspend - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcBusinessUnitType),(int)&lInfo.BusinessUnit - (int)&lInfo);

    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcRequestIDType),(int)&lInfo.RequestID - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcOrderLocalIDType),(int)&lInfo.OrderLocalID - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcExchangeIDType),(int)&lInfo.ExchangeID - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcParticipantIDType),(int)&lInfo.ParticipantID - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcClientIDType),(int)&lInfo.ClientID - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcExchangeInstIDType),(int)&lInfo.ExchangeInstID - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcTraderIDType),(int)&lInfo.TraderID - (int)&lInfo);

    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcInstallIDType),(int)&lInfo.InstallID - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCI_SQLT_CHR,sizeof(TThostFtdcOrderSubmitStatusType),(int)&lInfo.OrderSubmitStatus - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcSequenceNoType),(int)&lInfo.NotifySequence - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcDateType),(int)&lInfo.TradingDay - (int)&lInfo);

    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcSettlementIDType),(int)&lInfo.SettlementID - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcOrderSysIDType),(int)&lInfo.OrderSysID - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCI_SQLT_CHR,sizeof(TThostFtdcOrderSourceType),(int)&lInfo.OrderSource - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCI_SQLT_CHR,sizeof(TThostFtdcOrderStatusType),(int)&lInfo.OrderStatus - (int)&lInfo);

    PushColumnDataToVector(ltest,OCCI_SQLT_CHR,sizeof(TThostFtdcOrderTypeType),(int)&lInfo.OrderType - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.VolumeTraded - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.VolumeTotal - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcDateType),(int)&lInfo.InsertDate - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcTimeType),(int)&lInfo.InsertTime - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcTimeType),(int)&lInfo.ActiveTime - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcTimeType),(int)&lInfo.SuspendTime - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcTimeType),(int)&lInfo.UpdateTime - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcTimeType),(int)&lInfo.CancelTime - (int)&lInfo);

    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcTraderIDType),(int)&lInfo.ActiveTraderID - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcParticipantIDType),(int)&lInfo.ClearingPartID - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcSequenceNoType),(int)&lInfo.SequenceNo - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcFrontIDType),(int)&lInfo.FrontID - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcSessionIDType),(int)&lInfo.SessionID - (int)&lInfo);

    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcProductInfoType),(int)&lInfo.UserProductInfo - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcErrorMsgType),(int)&lInfo.StatusMsg - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcBoolType),(int)&lInfo.UserForceClose - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcUserIDType),(int)&lInfo.ActiveUserID - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcSequenceNoType),(int)&lInfo.BrokerOrderSeq - (int)&lInfo);

    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcOrderSysIDType),(int)&lInfo.RelativeOrderSysID - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.AvgPrice - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(int),(int)&lInfo.ExStatus - (int)&lInfo);

	PushColumnDataToVector(ltest,OCCIINT,sizeof(int),(int)&lInfo.FTID - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIINT,sizeof(int),(int)&lInfo.UpdateSeq - (int)&lInfo);

    std::string lsDeleteSql;
    if(nbDelete)
    {
        if(nsUserName.empty())
            lsDeleteSql = "delete from "+nsTableName+" t where t.ValidateDate = \'" 
            + strTime + "\'";
        else
            lsDeleteSql = "delete from "+nsTableName+" t where t.ValidateDate = \'" 
            + strTime
            + "\' AND InvestorID = \'"
            + nsUserName + "\'";
    }

    return BatchInsert(	nsTableName,
        lsDeleteSql,
        strTime,
        ltest,
        nOrders,
        nErrorCode);
    return false;


}
//成交
bool CSvrDBOprImpl::SaveUserTraderInfos(const std::string& nsTableName,
                                        bool nbDelete,
                                        const std::string& strTime,
                                        const std::string& nsUserName,
                                        const std::vector<PlatformStru_TradeInfo>& nTraders ,
                                        int& nErrorCode )
{
    PlatformStru_TradeInfo lInfo;
    std::vector<ColumeData> ltest;

    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcBrokerIDType),(int)&lInfo.BrokerID - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcInvestorIDType),(int)&lInfo.InvestorID - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcInstrumentIDType),(int)&lInfo.InstrumentID - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcOrderRefType),(int)&lInfo.OrderRef - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcUserIDType),(int)&lInfo.UserID - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcExchangeIDType),(int)&lInfo.ExchangeID - (int)&lInfo);	
    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcTradeIDType),(int)&lInfo.TradeID - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCI_SQLT_CHR,sizeof(TThostFtdcDirectionType),(int)&lInfo.Direction - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcOrderSysIDType),(int)&lInfo.OrderSysID - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcParticipantIDType),(int)&lInfo.ParticipantID - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcClientIDType),(int)&lInfo.ClientID - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCI_SQLT_CHR,sizeof(TThostFtdcDirectionType),(int)&lInfo.TradingRole - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcExchangeInstIDType),(int)&lInfo.ExchangeInstID - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCI_SQLT_CHR,sizeof(TThostFtdcOffsetFlagType),(int)&lInfo.OffsetFlag - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCI_SQLT_CHR,sizeof(TThostFtdcHedgeFlagType),(int)&lInfo.HedgeFlag - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.Price - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.Volume - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcDateType),(int)&lInfo.TradeDate - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcTimeType),(int)&lInfo.TradeTime - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCI_SQLT_CHR,sizeof(TThostFtdcTradeTypeType),(int)&lInfo.TradeType - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCI_SQLT_CHR,sizeof(TThostFtdcPriceSourceType),(int)&lInfo.PriceSource - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcTraderIDType),(int)&lInfo.TraderID - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcOrderLocalIDType),(int)&lInfo.OrderLocalID - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcParticipantIDType),(int)&lInfo.ClearingPartID - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcBusinessUnitType),(int)&lInfo.BusinessUnit - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcSequenceNoType),(int)&lInfo.SequenceNo - (int)&lInfo);	
    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcDateType),(int)&lInfo.TradingDay - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcSettlementIDType),(int)&lInfo.SettlementID - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcSequenceNoType),(int)&lInfo.BrokerOrderSeq - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCI_SQLT_CHR,sizeof(TThostFtdcTradeSourceType),(int)&lInfo.TradeSource - (int)&lInfo);	
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.CloseProfitByDate - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.CloseProfitByTrade - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.TradeCommission - (int)&lInfo);

	PushColumnDataToVector(ltest,OCCIINT,sizeof(int),(int)&lInfo.FTID - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIINT,sizeof(int),(int)&lInfo.UpdateSeq - (int)&lInfo);

    std::string lsDeleteSql ;
    if(nbDelete)
    {
        if(nsUserName.empty())
            lsDeleteSql = "delete from "+nsTableName+" t where t.ValidateDate = \'" 
            + strTime + "\'";		
        else
            lsDeleteSql = "delete from "+nsTableName+" t where t.ValidateDate = \'" 
            + strTime
            + "\' AND InvestorID = \'"
            + nsUserName + "\'";
    }

    return BatchInsert(	nsTableName,
        lsDeleteSql,
        strTime,
        ltest,
        nTraders,
        nErrorCode);
    return false;
}
//持仓
bool CSvrDBOprImpl::SaveUserPositionInfos(const std::string& nsTableName,
                                          bool nbDelete,
                                          const std::string& strTime,										 
                                          const std::string& nsUserName,
                                          const std::vector<PlatformStru_Position>& nPositions ,
                                          int& nErrorCode )
{
    PlatformStru_Position lInfo;
    std::vector<ColumeData> ltest;
    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcInstrumentIDType),(int)&lInfo.InstrumentID - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcBrokerIDType),(int)&lInfo.BrokerID - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcInvestorIDType),(int)&lInfo.InvestorID - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCI_SQLT_CHR,sizeof(TThostFtdcPosiDirectionType),(int)&lInfo.PosiDirection - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCI_SQLT_CHR,sizeof(TThostFtdcHedgeFlagType),(int)&lInfo.HedgeFlag - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcDateType),(int)&lInfo.TradingDay - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCI_SQLT_CHR,sizeof(TThostFtdcPositionDateType),(int)&lInfo.PositionDate - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcSettlementIDType),(int)&lInfo.SettlementID - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.Position - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.TodayPosition - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.YdPosition - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.OpenVolume - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.CloseVolume - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.OpenAmount - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.CloseAmount - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.PositionCost - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.OpenCost - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.LongFrozen - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.ShortFrozen - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.LongFrozenAmount - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.ShortFrozenAmount - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.FrozenMargin - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.FrozenCommission - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.FrozenCash- (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.Commission - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.PreMargin - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.UseMargin - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.ExchangeMargin - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcRatioType),(int)&lInfo.MarginRateByMoney - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcRatioType),(int)&lInfo.MarginRateByVolume - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.CashIn - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.PositionProfit - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.CloseProfit - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.CloseProfitByDate - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.CloseProfitByTrade - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.PreSettlementPrice - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.SettlementPrice - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.CombPosition - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.CombLongFrozen - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.CombShortFrozen - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.PositionProfitByTrade - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.TotalPositionProfitByDate - (int)&lInfo);

	PushColumnDataToVector(ltest,OCCIINT,sizeof(int),(int)&lInfo.FTID - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIINT,sizeof(int),(int)&lInfo.UpdateSeq - (int)&lInfo);


    std::string lsDeleteSql;
    if(nbDelete)
    {
        if(nsUserName.empty())
            lsDeleteSql = "delete from "+nsTableName+" t where t.ValidateDate = \'" 
            + strTime	+ "\'";
        else
            lsDeleteSql = "delete from "+nsTableName+" t where t.ValidateDate = \'" 
            + strTime
            + "\' AND InvestorID = \'"
            + nsUserName + "\'";
    }

    return BatchInsert(	nsTableName,
        lsDeleteSql,
        strTime,
        ltest,
        nPositions,
        nErrorCode);
    return false;
}

//持仓明细
bool CSvrDBOprImpl::SaveUserPositionDetailInfos( const std::string& nsTableName,
                                                bool nbDelete,
                                                const std::string& strTime,
                                                const std::string& nsUserName,
                                                const std::vector<PlatformStru_PositionDetail>& nPositionDetails ,
                                                int& nErrorCode )
{
    PlatformStru_PositionDetail lInfo;
    std::vector<ColumeData> ltest;
    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcInstrumentIDType),(int)&lInfo.InstrumentID - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcBrokerIDType),(int)&lInfo.BrokerID - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcInvestorIDType),(int)&lInfo.InvestorID - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCI_SQLT_CHR,sizeof(TThostFtdcHedgeFlagType),(int)&lInfo.HedgeFlag - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCI_SQLT_CHR,sizeof(TThostFtdcDirectionType),(int)&lInfo.Direction - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcDateType),(int)&lInfo.OpenDate - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcTradeIDType),(int)&lInfo.TradeID - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.Volume - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.OpenPrice - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcDateType),(int)&lInfo.TradingDay - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcSettlementIDType),(int)&lInfo.SettlementID - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCI_SQLT_CHR,sizeof(TThostFtdcTradeTypeType),(int)&lInfo.TradeType - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcInstrumentIDType),(int)&lInfo.CombInstrumentID - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcExchangeIDType),(int)&lInfo.ExchangeID - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.CloseProfitByDate - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.CloseProfitByTrade - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.PositionProfitByDate - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.PositionProfitByTrade - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.Margin- (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.ExchMargin - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcRatioType),(int)&lInfo.MarginRateByMoney - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcRatioType),(int)&lInfo.MarginRateByVolume - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.LastSettlementPrice - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.SettlementPrice - (int)&lInfo);	
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.CloseVolume - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.CloseAmount - (int)&lInfo);


	PushColumnDataToVector(ltest,OCCIINT,sizeof(int),(int)&lInfo.FTID - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIINT,sizeof(int),(int)&lInfo.UpdateSeq - (int)&lInfo);


    std::string lsDeleteSql;
    if(nbDelete)
    {
        if(nsUserName.empty())
            lsDeleteSql = "delete from "+nsTableName+" t where t.ValidateDate = \'" 
            + strTime	+ "\'";
        else
            lsDeleteSql = "delete from "+nsTableName+" t where t.ValidateDate = \'" 
            + strTime
            + "\' AND InvestorID = \'"
            + nsUserName + "\'";
    }

    return BatchInsert(	nsTableName,
        lsDeleteSql,
        strTime,
        ltest,
        nPositionDetails,
        nErrorCode);
    return false;
}

//资金
bool CSvrDBOprImpl::SaveUserFundInfos(const std::string& nsTableName,
                                      const std::string& strTime,
                                      const std::string& nsUserName,
                                      const PlatformStru_TradingAccountInfo& nFundInfos,
                                      int& nErrorCode )
{
    std::vector<PlatformStru_TradingAccountInfo> nVecFundInfos;
    nVecFundInfos.push_back(nFundInfos);

    PlatformStru_TradingAccountInfo lInfo;
    std::vector<ColumeData> ltest;
    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcBrokerIDType),(int)&lInfo.BrokerID - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcInvestorIDType),(int)&lInfo.AccountID - (int)&lInfo);	
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.PreMortgage - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.PreCredit - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.PreDeposit - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.PreBalance - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.PreMargin- (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.InterestBase - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.Interest - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.Deposit - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.Withdraw - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.FrozenMargin - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.FrozenCash- (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.FrozenCommission- (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.CurrMargin - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.CashIn - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.Commission - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.CloseProfit - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.PositionProfit - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.Balance - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.Available - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.WithdrawQuota - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.Reserve - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcDateType),(int)&lInfo.TradingDay - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcSettlementIDType),(int)&lInfo.SettlementID - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.Credit - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.Mortgage - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.ExchangeMargin - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.DeliveryMargin - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.ExchangeDeliveryMargin - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.StaticProfit - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.DynamicProfit - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.RiskDegree - (int)&lInfo);


    std::string lsDeleteSql = "delete from " +nsTableName +" t where t.ValidateDate = \'" 
        + strTime
        + "\' AND ACCOUNTID = \'"
        + nsUserName + "\'";

    return BatchInsert(	nsTableName,
        lsDeleteSql,
        strTime,
        ltest,
        nVecFundInfos,
        nErrorCode);
    return false;
}

//出入金
bool CSvrDBOprImpl::SaveFundInOut(const sFundInOut & s)
{

    bool lbRet = false;
    char szBuffer[MAX_SQL_LENGTH];	
    sprintf(szBuffer, "insert into TradeData_FundInOut values(\
                      \'%s\',%d,%f,\'%s\',\'%s\',\'%s\',\'%s\',sysdate)",
                      s.mUserID, s.meInOut,s.mdbVolume,s.mOpAdminID,s.msDesc,s.msDay,s.msTime);
    int nErrorCode;
    if( CInterface_SvrDBOpr::getObj().Excute(szBuffer, nErrorCode))
    {
        lbRet = true;
    }


    return lbRet;
}

//写交易日
bool CSvrDBOprImpl::SaveOneTradingDay(const std::string& nsTableName,
                                      const std::string& strTradingDay,	
                                      int nInitStatus,
                                      int& nErrorCode )
{
    bool lbRet = false;
    char szBuffer[MAX_SQL_LENGTH];	
    sprintf(szBuffer, "insert into %s values(\'%s\',sysdate,%d)",
        nsTableName.c_str(),strTradingDay.c_str(),nInitStatus);


    if( CInterface_SvrDBOpr::getObj().Excute(szBuffer, nErrorCode))
    {
        lbRet = true;
    }

    /*std::vector<std::string> nVecTradingDay;
    nVecTradingDay.push_back(strTradingDay);

    std::vector<ColumeData> ltest;
    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcDateType),0);	

    return BatchInsert(	nsTableName,
    "",
    ltest,
    nVecTradingDay,
    nErrorCode);*/
    return false;
}


//更新交易日状态
bool CSvrDBOprImpl::UpdateTradingDayInitStatus(const std::string& nsTableName,
                                               const std::string& strTradingDay,
                                               int nInitStatus,
                                               int& nErrorCode ) 
{
    bool lbRet = false;
    char szBuffer[MAX_SQL_LENGTH];	
    sprintf(szBuffer, "update %s t set t.validateDate= sysdate ,t.InitStatus=%d  where t.TradingDay=\'%s\'",
        nsTableName.c_str(),nInitStatus,strTradingDay.c_str());


    int nNum = 0;
    if( CInterface_SvrDBOpr::getObj().ExcuteUpdate(szBuffer,nNum, nErrorCode))
    {
        lbRet = true;
    }
    return lbRet;
}

bool CSvrDBOprImpl::SaveOneSettlementDay(const std::string& nsTableName,
                                         const std::string& strTradingDay,	
                                         int& nErrorCode ) 
{
    bool lbRet = false;
    char szBuffer[MAX_SQL_LENGTH];	
    sprintf(szBuffer, "insert into %s values(\'%s\',sysdate)",
        nsTableName.c_str(),strTradingDay.c_str());

    if( CInterface_SvrDBOpr::getObj().Excute(szBuffer, nErrorCode))
    {
        lbRet = true;
    }
    return lbRet;
}
//报单
bool CSvrDBOprImpl::MergeOrderInfos(
                                    const std::string& nsTableName,							
                                    const std::string& strTime,		
                                    const std::vector<PlatformStru_OrderInfo>& nOrders ,
                                    int& nErrorCode ) 
{
    //先删除今天该账户所有的，然后再添加
    PlatformStru_OrderInfo lInfo;
    std::vector<ColumeDataEx> ltest;

    PushColumnDataToVectorEx(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcBrokerIDType),(int)&lInfo.BrokerID - (int)&lInfo,"BROKERID"); //BrokerID
    PushColumnDataToVectorEx(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcInvestorIDType),(int)&lInfo.InvestorID - (int)&lInfo,"INVESTORID"); //InvestorID
    PushColumnDataToVectorEx(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcInstrumentIDType),(int)&lInfo.InstrumentID - (int)&lInfo,"INSTRUMENTID"); //InstrumentID
    PushColumnDataToVectorEx(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcOrderRefType),(int)&lInfo.OrderRef - (int)&lInfo,"ORDERREF");//Orderref
    PushColumnDataToVectorEx(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcUserIDType),(int)&lInfo.UserID - (int)&lInfo,"USERID");    //UserID
    PushColumnDataToVectorEx(ltest,OCCI_SQLT_CHR,sizeof(TThostFtdcOrderPriceTypeType),(int)&lInfo.OrderPriceType - (int)&lInfo,"ORDERPRICETYPE");	//OrderPriceType
    PushColumnDataToVectorEx(ltest,OCCI_SQLT_CHR,sizeof(TThostFtdcDirectionType),(int)&lInfo.Direction - (int)&lInfo,"DIRECTION");            //Driection
    PushColumnDataToVectorEx(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcCombOffsetFlagType),(int)&lInfo.CombOffsetFlag - (int)&lInfo,"COMBOFFSETFLAG");  //ComboffsetFlag
    PushColumnDataToVectorEx(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcCombHedgeFlagType),(int)&lInfo.CombHedgeFlag - (int)&lInfo,"COMBHEDGEFLAG");	//combhedgeflag
    PushColumnDataToVectorEx(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.LimitPrice - (int)&lInfo,"LIMITPRICE");       //LimitPrice
    PushColumnDataToVectorEx(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.VolumeTotalOriginal - (int)&lInfo,"VOLUMETOTALORIGINAL"); //VolumeTotalOriginal
    PushColumnDataToVectorEx(ltest,OCCI_SQLT_CHR,sizeof(TThostFtdcTimeConditionType),(int)&lInfo.TimeCondition - (int)&lInfo,"TIMECONDITION");	 //TimeCondition
    PushColumnDataToVectorEx(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcDateType),(int)&lInfo.GTDDate - (int)&lInfo,"GTDDATE"); //GTData
    PushColumnDataToVectorEx(ltest,OCCI_SQLT_CHR,sizeof(TThostFtdcVolumeConditionType),(int)&lInfo.VolumeCondition - (int)&lInfo,"VOLUMECONDITION"); //VolumeCondition
    PushColumnDataToVectorEx(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.MinVolume - (int)&lInfo,"MINVOLUME"); //MinVolumn
    PushColumnDataToVectorEx(ltest,OCCI_SQLT_CHR,sizeof(TThostFtdcContingentConditionType),(int)&lInfo.ContingentCondition - (int)&lInfo,"CONTINGENTCONDITION");
    PushColumnDataToVectorEx(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.StopPrice - (int)&lInfo,"STOPPRICE");
    PushColumnDataToVectorEx(ltest,OCCI_SQLT_CHR,sizeof(TThostFtdcForceCloseReasonType),(int)&lInfo.ForceCloseReason - (int)&lInfo,"FORCECLOSEREASON");
    PushColumnDataToVectorEx(ltest,OCCIINT,sizeof(TThostFtdcBoolType),(int)&lInfo.IsAutoSuspend - (int)&lInfo,"ISAUTOSUSPEND");
    PushColumnDataToVectorEx(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcBusinessUnitType),(int)&lInfo.BusinessUnit - (int)&lInfo,"BUSINESSUNIT");

    PushColumnDataToVectorEx(ltest,OCCIINT,sizeof(TThostFtdcRequestIDType),(int)&lInfo.RequestID - (int)&lInfo,"REQUESTID");
    PushColumnDataToVectorEx(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcOrderLocalIDType),(int)&lInfo.OrderLocalID - (int)&lInfo,"ORDERLOCALID");
    PushColumnDataToVectorEx(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcExchangeIDType),(int)&lInfo.ExchangeID - (int)&lInfo,"EXCHANGEID");
    PushColumnDataToVectorEx(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcParticipantIDType),(int)&lInfo.ParticipantID - (int)&lInfo,"PARTICIPANTID");
    PushColumnDataToVectorEx(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcClientIDType),(int)&lInfo.ClientID - (int)&lInfo,"CLIENTID");
    PushColumnDataToVectorEx(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcExchangeInstIDType),(int)&lInfo.ExchangeInstID - (int)&lInfo,"EXCHANGEINSTID");
    PushColumnDataToVectorEx(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcTraderIDType),(int)&lInfo.TraderID - (int)&lInfo,"TRADERID");

    PushColumnDataToVectorEx(ltest,OCCIINT,sizeof(TThostFtdcInstallIDType),(int)&lInfo.InstallID - (int)&lInfo,"INSTALLID");
    PushColumnDataToVectorEx(ltest,OCCI_SQLT_CHR,sizeof(TThostFtdcOrderSubmitStatusType),(int)&lInfo.OrderSubmitStatus - (int)&lInfo,"ORDERSUBMITSTATUS");
    PushColumnDataToVectorEx(ltest,OCCIINT,sizeof(TThostFtdcSequenceNoType),(int)&lInfo.NotifySequence - (int)&lInfo,"NOTIFYSEQUENCE");
    PushColumnDataToVectorEx(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcDateType),(int)&lInfo.TradingDay - (int)&lInfo,"TRADINGDAY");

    PushColumnDataToVectorEx(ltest,OCCIINT,sizeof(TThostFtdcSettlementIDType),(int)&lInfo.SettlementID - (int)&lInfo,"SETTLEMENTID");
    PushColumnDataToVectorEx(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcOrderSysIDType),(int)&lInfo.OrderSysID - (int)&lInfo,"ORDERSYSID");
    PushColumnDataToVectorEx(ltest,OCCI_SQLT_CHR,sizeof(TThostFtdcOrderSourceType),(int)&lInfo.OrderSource - (int)&lInfo,"ORDERSOURCE");
    PushColumnDataToVectorEx(ltest,OCCI_SQLT_CHR,sizeof(TThostFtdcOrderStatusType),(int)&lInfo.OrderStatus - (int)&lInfo,"ORDERSTATUS");

    PushColumnDataToVectorEx(ltest,OCCI_SQLT_CHR,sizeof(TThostFtdcOrderTypeType),(int)&lInfo.OrderType - (int)&lInfo,"ORDERTYPE");
    PushColumnDataToVectorEx(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.VolumeTraded - (int)&lInfo,"VOLUMETRADED");
    PushColumnDataToVectorEx(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.VolumeTotal - (int)&lInfo,"VOLUMETOTAL");
    PushColumnDataToVectorEx(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcDateType),(int)&lInfo.InsertDate - (int)&lInfo,"INSERTDATE");
    PushColumnDataToVectorEx(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcTimeType),(int)&lInfo.InsertTime - (int)&lInfo,"INSERTTIME");
    PushColumnDataToVectorEx(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcTimeType),(int)&lInfo.ActiveTime - (int)&lInfo,"ACTIVETIME");
    PushColumnDataToVectorEx(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcTimeType),(int)&lInfo.SuspendTime - (int)&lInfo,"SUSPENDTIME");
    PushColumnDataToVectorEx(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcTimeType),(int)&lInfo.UpdateTime - (int)&lInfo,"UPDATETIME");
    PushColumnDataToVectorEx(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcTimeType),(int)&lInfo.CancelTime - (int)&lInfo,"CANCELTIME");

    PushColumnDataToVectorEx(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcTraderIDType),(int)&lInfo.ActiveTraderID - (int)&lInfo,"ACTIVETRADERID");
    PushColumnDataToVectorEx(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcParticipantIDType),(int)&lInfo.ClearingPartID - (int)&lInfo,"CLEARINGPARTID");
    PushColumnDataToVectorEx(ltest,OCCIINT,sizeof(TThostFtdcSequenceNoType),(int)&lInfo.SequenceNo - (int)&lInfo,"SEQUENCENO");
    PushColumnDataToVectorEx(ltest,OCCIINT,sizeof(TThostFtdcFrontIDType),(int)&lInfo.FrontID - (int)&lInfo,"FRONTID");
    PushColumnDataToVectorEx(ltest,OCCIINT,sizeof(TThostFtdcSessionIDType),(int)&lInfo.SessionID - (int)&lInfo,"SESSIONID");

    PushColumnDataToVectorEx(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcProductInfoType),(int)&lInfo.UserProductInfo - (int)&lInfo,"USERPRODUCTINFO");
    PushColumnDataToVectorEx(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcErrorMsgType),(int)&lInfo.StatusMsg - (int)&lInfo,"STATUSMSG");
    PushColumnDataToVectorEx(ltest,OCCIINT,sizeof(TThostFtdcBoolType),(int)&lInfo.UserForceClose - (int)&lInfo,"USERFORCECLOSE");
    PushColumnDataToVectorEx(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcUserIDType),(int)&lInfo.ActiveUserID - (int)&lInfo,"ACTIVEUSERID");
    PushColumnDataToVectorEx(ltest,OCCIINT,sizeof(TThostFtdcSequenceNoType),(int)&lInfo.BrokerOrderSeq - (int)&lInfo,"BROKERORDERSEQ");

    PushColumnDataToVectorEx(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcOrderSysIDType),(int)&lInfo.RelativeOrderSysID - (int)&lInfo,"RELATIVEORDERSYSID");
    PushColumnDataToVectorEx(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.AvgPrice - (int)&lInfo,"AVGPRICE");
    PushColumnDataToVectorEx(ltest,OCCIINT,sizeof(int),(int)&lInfo.ExStatus - (int)&lInfo,"EXSTATUS");

    std::string lsConditionSql;
    lsConditionSql = "T.INVESTORID = :v2 AND T.InstrumentID = :v3 AND T.Orderref = :v4 AND T.FRONTID = :v48 AND T.SESSIONID = :v49";

    return BatchMerge(	nsTableName,
        lsConditionSql,
        strTime,
        ltest,
        nOrders,
        nErrorCode);

}

template <class T>
bool CSvrDBOprImpl::BatchInsert(const std::string& nsTableName,
                                const std::string& nDeleteSql,
                                const std::string& nValidateDate,
                                std::vector<ColumeData>& nVecColume ,
                                const std::vector<T>& nVecData ,
                                int& nErrorCode )
{

    CGuard guard(&g_mutex);
    nErrorCode = CF_ERROR_SUCCESS;
    if ( !IsConnected() && !Conncect())
    {
        return false;
    }

    if(nVecColume.size() == 0)
        return false;
    std::string lSql;


    bool bRet = true;
    try
    {
        m_pStmt = m_pCon->createStatement();
        //先删除今天已存储的记录
        if(!nDeleteSql.empty())
            m_pStmt->execute( nDeleteSql);

        //生成Sql语句		
        if(nVecData.size() != 0)
        {

            lSql = "insert into "  + nsTableName + " values(" ;
            for(unsigned int k = 1; k <= nVecColume.size();k++)
            {	

                std::stringstream lTemp;
                lTemp << k;
                lSql += ":v" + lTemp.str() + ",";	  
            }	
            lSql += "\'" + nValidateDate +"\',sysdate)";
            //批量存储，每次最多存储10000条		

            m_pStmt->setSQL(lSql);
            int nCount = nVecData.size() / nMaxInsertRow;
            if ( nVecData.size() % nMaxInsertRow != 0 )
            {
                nCount++;
            }

            for ( int i = 0; i < nCount; i++ )
            {
                int index=0,nRecordNum = 0;
                int lnRemindNum = nVecData.size() - i*nMaxInsertRow;
                int lnMaxNum =(nMaxInsertRow > lnRemindNum) ? lnRemindNum : nMaxInsertRow;


                //分配内存
                std::vector<char*> lpVecColumnBuf;
                std::vector<unsigned short*> lVecColumnRelLen;
                std::vector<ColumeData>::iterator lIter = nVecColume.begin();
                for(;lIter != nVecColume.end();lIter++)
                {
                    char * lpBuf = (char*)malloc(lnMaxNum *lIter->nColumeMaxLen);
                    memset(lpBuf,0,lnMaxNum *lIter->nColumeMaxLen);
                    unsigned short * lpLenBuf = (unsigned short *)malloc(lnMaxNum*sizeof(unsigned short));
                    memset(lpLenBuf,0,lnMaxNum*sizeof(unsigned short));
                    lpVecColumnBuf.push_back(lpBuf);	
                    lVecColumnRelLen.push_back(lpLenBuf);

                }



                //填充内存
                for ( int j = 0; j < lnMaxNum; j++)
                {
                    const T* field = &nVecData[i*nMaxInsertRow+j];		

                    for(unsigned int k = 0; k < nVecColume.size();k++)
                    {
                        char * lpBuf = (char*)field+nVecColume[k].nOffset;	

                        memcpy(lpVecColumnBuf[k]+j*nVecColume[k].nColumeMaxLen,
                            lpBuf,
                            nVecColume[k].nColumeMaxLen);


                        lVecColumnRelLen[k][j]  = nVecColume[k].nColumeMaxLen;

                        if(nVecColume[k].eColumeType == oracle::occi::OCCI_SQLT_STR)
                            lVecColumnRelLen[k][j]  = strlen((char*)lpBuf) + 1;
                    }

                    nRecordNum++;
                }

                //设置内存到oracle
                for(unsigned int k = 0; k < lpVecColumnBuf.size();k++)
                {		

                    m_pStmt->setDataBuffer(k+1, (void*)lpVecColumnBuf[k], 
                        nVecColume[k].eColumeType,
                        nVecColume[k].nColumeMaxLen, 
                        lVecColumnRelLen[k]); 

                }

                try
                {
                    m_pStmt->executeArrayUpdate(nRecordNum); 
                }			
                catch (oracle::occi::SQLException &e)
                {
                    //释放内存
                    for(unsigned int k = 0; k < lpVecColumnBuf.size();k++)
                    {
                        free(lpVecColumnBuf[k]);
                        free(lVecColumnRelLen[k]);
                    }	
                    throw e;
                }
                //释放内存
                for(unsigned int k = 0; k < lpVecColumnBuf.size();k++)
                {
                    free(lpVecColumnBuf[k]);
                    free(lVecColumnRelLen[k]);
                }	

            }
        }

        m_pCon->commit();
        m_pCon->terminateStatement(m_pStmt);
    }
    catch(oracle::occi::SQLException &e)
    {
        RollBack();
        std::cout<<e.what()<<endl;
        std::cout<<lSql<<endl;
        nErrorCode = GetErrorCode(e.what());
        WriteLog(lSql.c_str(), e.what());
        bRet = false;
    }

    return bRet;

}

bool CSvrDBOprImpl::CreatInsTable_PreDay(const std::string& nsTableName,
										 int& nErrorCode )
{
	CGuard guard(&g_mutex);
	nErrorCode = CF_ERROR_SUCCESS;
	if ( !IsConnected() && !Conncect())
	{
		return false;
	}

	std::string lSql;

	int nRecordNum = 0; //保存查询表的个数
	bool bRet = true;

	//查询是否有INSTRUMENT表
	lSql = "select count(*) from user_tables where table_name='INSTRUMENT'";

	try
	{
		m_pStmt = m_pCon->createStatement( lSql );
		m_pRes = m_pStmt->executeQuery();
		if ( m_pRes->next())
		{
			nRecordNum = m_pRes->getInt(1);
		}

		m_pStmt->closeResultSet(m_pRes);
		m_pCon->terminateStatement(m_pStmt);
	}catch(oracle::occi::SQLException &e){
		RollBack();
		std::cout<<e.what()<<endl;
		std::cout<<lSql<<endl;
		nErrorCode = GetErrorCode(e.what());
		WriteLog(lSql, e.what());
		return false;
	}

	if (nRecordNum>0)//有该表 修改表名
	{ 
		std::string strDate;
		std::string strTableDate=nsTableName.substr(11,8);
		lSql = "select t.validatedate from INSTRUMENT t where rownum = 1";
		try
		{
			m_pStmt = m_pCon->createStatement( lSql );
			m_pRes = m_pStmt->executeQuery();
			if ( m_pRes->next())
			{
				strDate = m_pRes->getString(1);
			}

			m_pStmt->closeResultSet(m_pRes);
			m_pCon->terminateStatement(m_pStmt);
		}catch(oracle::occi::SQLException &e){
			RollBack();
			std::cout<<e.what()<<endl;
			std::cout<<lSql<<endl;
			nErrorCode = GetErrorCode(e.what());
			WriteLog(lSql, e.what());
			return false;
		}
		if (strDate == strTableDate || strDate.empty())
		{
			return true;//已有当日合约表
		}
		else
		{
			//修改表名
			lSql = "ALTER TABLE INSTRUMENT rename to "+nsTableName;
			try
			{
				m_pStmt = m_pCon->createStatement();
				m_pStmt->execute( lSql );
				m_pCon->commit();

				m_pCon->terminateStatement(m_pStmt);
			}catch(oracle::occi::SQLException &e){
				RollBack();
				std::cout<<e.what()<<endl;
				std::cout<<lSql<<endl;
				nErrorCode = GetErrorCode(e.what());
				WriteLog(lSql, e.what());
				return false;
			}
		}
	}

	//新建表
	lSql = "";
	lSql = "CREATE TABLE INSTRUMENT \
		( \
		instrumentid                  VARCHAR2(64),\
		exchangeid                    VARCHAR2(9),\
		instrumentname                VARCHAR2(64),\
		exchangeinstid                VARCHAR2(31),\
		productid                     VARCHAR2(64),\
		productclass                  CHAR(1),\
		deliveryyear                  INTEGER,\
		deliverymonth                 INTEGER,\
		maxmarketordervolume          INTEGER,\
		minmarketordervolume          INTEGER,\
		maxlimitordervolume           INTEGER,\
		minlimitordervolume           INTEGER,\
		volumemultiple                INTEGER,\
		pricetick                     BINARY_DOUBLE,\
		createdate                    VARCHAR2(9),\
		opendate                      VARCHAR2(9),\
		expiredate                    VARCHAR2(9),\
		startdelivdate                VARCHAR2(9),\
		enddelivdate                  VARCHAR2(9),\
		instlifephase                 CHAR(1),\
		istrading                     INTEGER,\
		positiontype                  CHAR(1),\
		positiondatetype              CHAR(1),\
		longmarginratio               BINARY_DOUBLE,\
		shortmarginratio              BINARY_DOUBLE,\
		maxmarginsidealgorithm        CHAR(1),\
		combmargindiscountmode        INTEGER,\
		unwindpriorities              INTEGER,\
		priceformarginoftodayposition INTEGER,\
		closetodayinstructionsupport  INTEGER,\
		closeinstructionsupport       INTEGER,\
		currency                      VARCHAR2(11),\
		ticksperpoint                 INTEGER,\
		ticksize                      VARCHAR2(10),\
		validatedate                  VARCHAR2(11),\
		insertdbtime                  DATE\
		)" ;            

	try
	{
		m_pStmt = m_pCon->createStatement();
		m_pStmt->execute( lSql );
		m_pCon->commit();

		m_pCon->terminateStatement(m_pStmt);
	}catch(oracle::occi::SQLException &e){
		RollBack();
		std::cout<<e.what()<<endl;
		std::cout<<lSql<<endl;
		nErrorCode = GetErrorCode(e.what());
		WriteLog(lSql, e.what());
		return false;
	}

	return bRet;
}

template <class T>
bool CSvrDBOprImpl::BatchInsert_PerDay(const std::string& nsTableName,
                                       const std::string& nDeleteSql,
                                       const std::string& nValidateDate,
                                       std::vector<ColumeData>& nVecColume ,
                                       const std::vector<T>& nVecData ,
                                       int& nErrorCode )
{

    CGuard guard(&g_mutex);
    nErrorCode = CF_ERROR_SUCCESS;
    if ( !IsConnected() && !Conncect())
    {
        return false;
    }

    if(nVecColume.size() == 0)
        return false;
    std::string lSql;

    int nRecordNum = 0; //保存查询表的个数
    bool bRet = true;

    try
    {
        //查询是否有该表
        lSql = "select count(*) from user_tables where table_name=\'" + nsTableName + "\'";

        try
        {
            m_pStmt = m_pCon->createStatement( lSql );
            m_pRes = m_pStmt->executeQuery();
            if ( m_pRes->next())
            {
                nRecordNum = m_pRes->getInt(1);
            }

            m_pStmt->closeResultSet(m_pRes);
            m_pCon->terminateStatement(m_pStmt);
        }catch(oracle::occi::SQLException &e){
            RollBack();
            std::cout<<e.what()<<endl;
            std::cout<<lSql<<endl;
            nErrorCode = GetErrorCode(e.what());
            WriteLog(lSql, e.what());
            return false;
        }

        if (nRecordNum==0)//无该表 创建该表
        {   

            lSql = "";
            lSql = "CREATE TABLE " + nsTableName +
                "( \
				seq                INTEGER,\
                tradingday         VARCHAR2(9),\
                instrumentid       VARCHAR2(64),\
                exchangeid         VARCHAR2(9),\
                exchangeinstid     VARCHAR2(31),\
                lastprice          BINARY_DOUBLE,\
                presettlementprice BINARY_DOUBLE,\
                precloseprice      BINARY_DOUBLE,\
                preopeninterest    BINARY_DOUBLE,\
                openprice          BINARY_DOUBLE,\
                highestprice       BINARY_DOUBLE,\
                lowestprice        BINARY_DOUBLE,\
                volume             INTEGER,\
                turnover           BINARY_DOUBLE,\
                openinterest       BINARY_DOUBLE,\
                closeprice         BINARY_DOUBLE,\
                settlementprice    BINARY_DOUBLE,\
                upperlimitprice    BINARY_DOUBLE,\
                lowerlimitprice    BINARY_DOUBLE,\
                predelta           BINARY_DOUBLE,\
                currdelta          BINARY_DOUBLE,\
                updatetime         VARCHAR2(9),\
                updatemillisec     INTEGER,\
                bidprice1          BINARY_DOUBLE,\
                bidvolume1         INTEGER,\
                askprice1          BINARY_DOUBLE,\
                askvolume1         INTEGER,\
                bidprice2          BINARY_DOUBLE,\
                bidvolume2         INTEGER,\
                askprice2          BINARY_DOUBLE,\
                askvolume2         INTEGER,\
                bidprice3          BINARY_DOUBLE,\
                bidvolume3         INTEGER,\
                askprice3          BINARY_DOUBLE,\
                askvolume3         INTEGER,\
                bidprice4          BINARY_DOUBLE,\
                bidvolume4         INTEGER,\
                askprice4          BINARY_DOUBLE,\
                askvolume4         INTEGER,\
                bidprice5          BINARY_DOUBLE,\
                bidvolume5         INTEGER,\
                askprice5          BINARY_DOUBLE,\
                askvolume5         INTEGER,\
                averageprice       BINARY_DOUBLE,\
                bidprice6          BINARY_DOUBLE,\
                bidvolume6         INTEGER,\
                askprice6          BINARY_DOUBLE,\
                askvolume6         INTEGER,\
                bidprice7          BINARY_DOUBLE,\
                bidvolume7         INTEGER,\
                askprice7          BINARY_DOUBLE,\
                askvolume7         INTEGER,\
                bidprice8          BINARY_DOUBLE,\
                bidvolume8         INTEGER,\
                askprice8          BINARY_DOUBLE,\
                askvolume8         INTEGER,\
                bidprice9          BINARY_DOUBLE,\
                bidvolume9         INTEGER,\
                askprice9          BINARY_DOUBLE,\
                askvolume9         INTEGER,\
                bidprice10         BINARY_DOUBLE,\
                bidvolume10        INTEGER,\
                askprice10         BINARY_DOUBLE,\
                askvolume10        INTEGER,\
                newvolume          INTEGER,\
                validatedate       VARCHAR2(11),\
                insertdbtime       DATE\
                )" ;            

            try
            {
                m_pStmt = m_pCon->createStatement();
                m_pStmt->execute( lSql );
                m_pCon->commit();

                m_pCon->terminateStatement(m_pStmt);
            }catch(oracle::occi::SQLException &e){
                RollBack();
                std::cout<<e.what()<<endl;
                std::cout<<lSql<<endl;
                nErrorCode = GetErrorCode(e.what());
                WriteLog(lSql, e.what());
                return false;
            }

			//每月第一天重建SEQ
			if(nVecData.size() != 0)
			{

			
			string strdate(nVecData[0].TradingDay);
			strdate.erase(0,6);
			if (strdate =="01")
			{


				//查找序列号是否存在
				nRecordNum =0;
				lSql = "";
				lSql =  "select count(*) FROM All_Sequences where sequence_name='QUOT_SEQ'" ;

				try
				{
					m_pStmt = m_pCon->createStatement( lSql );
					m_pRes = m_pStmt->executeQuery();
					if ( m_pRes->next())
					{
						nRecordNum = m_pRes->getInt(1);
					}

					m_pStmt->closeResultSet(m_pRes);
					m_pCon->terminateStatement(m_pStmt);
				}catch(oracle::occi::SQLException &e){
					RollBack();
					std::cout<<e.what()<<endl;
					std::cout<<lSql<<endl;
					nErrorCode = GetErrorCode(e.what());
					WriteLog(lSql, e.what());
					return false;
				}

				if (nRecordNum != 0)//已存在的话先删掉
				{
					lSql = "";
					lSql =  "drop sequence QUOT_SEQ" ;

					try
					{
						m_pStmt = m_pCon->createStatement();
						m_pStmt->execute( lSql );
						m_pCon->commit();

						m_pCon->terminateStatement(m_pStmt);
					}catch(oracle::occi::SQLException &e){
						RollBack();
						std::cout<<e.what()<<endl;
						std::cout<<lSql<<endl;
						nErrorCode = GetErrorCode(e.what());
						WriteLog(lSql, e.what());
						return false;
					}  
				}

				//创建序列
				lSql = "";
				lSql =  "create sequence QUOT_SEQ  MINVALUE 1 MAXVALUE 1000000000000 INCREMENT BY 1 START WITH 1 NOCACHE  NOORDER  NOCYCLE" ;

				try
				{
					m_pStmt = m_pCon->createStatement();
					m_pStmt->execute( lSql );
					m_pCon->commit();

					m_pCon->terminateStatement(m_pStmt);
				}catch(oracle::occi::SQLException &e){
					RollBack();
					std::cout<<e.what()<<endl;
					std::cout<<lSql<<endl;
					nErrorCode = GetErrorCode(e.what());
					WriteLog(lSql, e.what());
					return false;
				}  
			}
			}

            //创建索引
            lSql = "";
            lSql =  "create index INDEX_INSERTDBTIME_" + nValidateDate + " on " + nsTableName + 
                " (INSERTDBTIME)";

            try
            {
                m_pStmt = m_pCon->createStatement();
                m_pStmt->execute( lSql );
                m_pCon->commit();

                m_pCon->terminateStatement(m_pStmt);
            }catch(oracle::occi::SQLException &e){
                RollBack();
                std::cout<<e.what()<<endl;
                std::cout<<lSql<<endl;
                nErrorCode = GetErrorCode(e.what());
                WriteLog(lSql, e.what());
                return false;
            }  

			//创建索引
			lSql = "";
			lSql =  "create index INDEX_INSTRUMENTID_" + nValidateDate + " on " + nsTableName + 
				" (INSTRUMENTID)";

			try
			{
				m_pStmt = m_pCon->createStatement();
				m_pStmt->execute( lSql );
				m_pCon->commit();

				m_pCon->terminateStatement(m_pStmt);
			}catch(oracle::occi::SQLException &e){
				RollBack();
				std::cout<<e.what()<<endl;
				std::cout<<lSql<<endl;
				nErrorCode = GetErrorCode(e.what());
				WriteLog(lSql, e.what());
				return false;
			}  
            lSql =  "create index INDEX_UPDATETIME_" + nValidateDate + " on " + nsTableName + 
                " (UPDATETIME)" ;

            try
            {
                m_pStmt = m_pCon->createStatement();
                m_pStmt->execute( lSql );
                m_pCon->commit();

                m_pCon->terminateStatement(m_pStmt);
            }catch(oracle::occi::SQLException &e){
                RollBack();
                std::cout<<e.what()<<endl;
                std::cout<<lSql<<endl;
                nErrorCode = GetErrorCode(e.what());
                WriteLog(lSql, e.what());
                return false;
            } 
        }


        m_pStmt = m_pCon->createStatement();
        //先删除今天已存储的记录
        if(!nDeleteSql.empty())
            m_pStmt->execute( nDeleteSql);


        //生成Sql语句		
        if(nVecData.size() != 0)
        {
			char szBuffer[MAX_SQL_LENGTH];
			memset(szBuffer, 0, sizeof(szBuffer));
			sprintf_s(szBuffer, "%d",nVecData.size() );

			WriteLog(szBuffer, "");

            lSql = "insert /*+Append*/  into "  + nsTableName + " values(QUOT_SEQ.NEXTVAL ,";
            for(unsigned int k = 1; k <= nVecColume.size();k++)
            {	
                std::stringstream lTemp;
                lTemp << k;
                lSql += ":v" + lTemp.str() + ",";			

            }	
            lSql += "\'" + nValidateDate +"\',sysdate)";
            //批量存储，每次最多存储10000条		

            m_pStmt->setSQL(lSql);
            int nCount = nVecData.size() / nMaxInsertRow;
            if ( nVecData.size() % nMaxInsertRow != 0 )
            {
                nCount++;
            }

            for ( int i = 0; i < nCount; i++ )
            {
                int index=0,nRecordNum = 0;
                int lnRemindNum = nVecData.size() - i*nMaxInsertRow;
                int lnMaxNum =(nMaxInsertRow > lnRemindNum) ? lnRemindNum : nMaxInsertRow;


                //分配内存
                std::vector<char*> lpVecColumnBuf;
                std::vector<unsigned short*> lVecColumnRelLen;
                std::vector<ColumeData>::iterator lIter = nVecColume.begin();
                for(;lIter != nVecColume.end();lIter++)
                {
                    char * lpBuf = (char*)malloc(lnMaxNum *lIter->nColumeMaxLen);
                    memset(lpBuf,0,lnMaxNum *lIter->nColumeMaxLen);
                    unsigned short * lpLenBuf = (unsigned short *)malloc(lnMaxNum*sizeof(unsigned short));
                    memset(lpLenBuf,0,lnMaxNum*sizeof(unsigned short));
                    lpVecColumnBuf.push_back(lpBuf);	
                    lVecColumnRelLen.push_back(lpLenBuf);

                }


				
                //填充内存
                for ( int j = 0; j < lnMaxNum; j++)
                {
                    const T* field = &nVecData[i*nMaxInsertRow+j];		

                    for(unsigned int k = 0; k < nVecColume.size();k++)
                    {
					
						
                        char * lpBuf = (char*)field+nVecColume[k].nOffset;	

                        memcpy(lpVecColumnBuf[k]+j*nVecColume[k].nColumeMaxLen,
                            lpBuf,
                            nVecColume[k].nColumeMaxLen);


                        lVecColumnRelLen[k][j]  = nVecColume[k].nColumeMaxLen;

                        if(nVecColume[k].eColumeType == oracle::occi::OCCI_SQLT_STR)
                            lVecColumnRelLen[k][j]  = strlen((char*)lpBuf) + 1;
                    }

					


                    nRecordNum++;
                }

                //设置内存到oracle
                for(unsigned int k = 0; k < lpVecColumnBuf.size();k++)
                {		

                    m_pStmt->setDataBuffer(k+1, (void*)lpVecColumnBuf[k], 
                        nVecColume[k].eColumeType,
                        nVecColume[k].nColumeMaxLen, 
                        lVecColumnRelLen[k]); 

                }

				int n = sizeof(PlatformStru_DepthMarketData);
                try
                {
                    m_pStmt->executeArrayUpdate(nRecordNum); 
                }			
                catch (oracle::occi::SQLException &e)
                {
					 WriteLog("释放内存  出错！！！","");
                    //释放内存
                    for(unsigned int k = 0; k < lpVecColumnBuf.size();k++)
                    {
                        free(lpVecColumnBuf[k]);
                        free(lVecColumnRelLen[k]);
                    }	
                    throw e;
                }
                //释放内存
                for(unsigned int k = 0; k < lpVecColumnBuf.size();k++)
                {
                    free(lpVecColumnBuf[k]);
                    free(lVecColumnRelLen[k]);
                }	

            }
        }

        m_pCon->commit();
        m_pCon->terminateStatement(m_pStmt);
    }
    catch(oracle::occi::SQLException &e)
    {
        RollBack();
        std::cout<<e.what()<<endl;
        std::cout<<lSql<<endl;
        nErrorCode = GetErrorCode(e.what());
        WriteLog(lSql.c_str(), e.what());
        bRet = false;
    }

    return bRet;

}

template <class T>
bool CSvrDBOprImpl::BatchMerge(							
                               const std::string& nsTableName,
                               const std::string& nsConditionStr,
                               const std::string& nValidateDate,
                               std::vector<ColumeDataEx>& nVecColume ,
                               const std::vector<T>& nVecData ,
                               int& nErrorCode )
{
    CGuard guard(&g_mutex);
    nErrorCode = CF_ERROR_SUCCESS;
    if ( !IsConnected() && !Conncect())
    {
        return false;
    }

    if(nVecColume.size() == 0)
        return false;
    std::string lSql;


    bool bRet = true;
    try
    {
        m_pStmt = m_pCon->createStatement();


        std::string lsUpdate;
        lsUpdate = " when matched then update set " ;
        for(unsigned int k = 1; k <= nVecColume.size();k++)
        {	
            std::stringstream lTemp;
            lTemp << k;
            lsUpdate += "T."+nVecColume[k-1].msColumeName + " = :v" + lTemp.str() + ",";			

        }	
        lsUpdate += "T.ValidateDate = \'" + nValidateDate +"\',T.InsertDBTime=sysdate ";

        std::string lsInsert;
        lsInsert = " when not matched then insert values(" ;
        for(unsigned int k = 1; k <= nVecColume.size();k++)
        {	
            std::stringstream lTemp;
            lTemp << k;
            lsInsert += ":v" + lTemp.str() + ",";			

        }	
        lsInsert += "\'" + nValidateDate +"\',sysdate)";
        //生成Sql语句
        lSql = "merge into "  + nsTableName + " T using Dual On (" + nsConditionStr +")" + lsUpdate + lsInsert;

        //批量存储，每次最多存储10000条		


        m_pStmt->setSQL(lSql);
        int nCount = nVecData.size() / nMaxInsertRow;
        if ( nVecData.size() % nMaxInsertRow != 0 )
        {
            nCount++;
        }

        for ( int i = 0; i < nCount; i++ )
        {
            int index=0,nRecordNum = 0;
            int lnRemindNum = nVecData.size() - i*nMaxInsertRow;
            int lnMaxNum = (nMaxInsertRow > lnRemindNum) ? lnRemindNum : nMaxInsertRow;


            //分配内存
            std::vector<char*> lpVecColumnBuf;
            std::vector<unsigned short*> lVecColumnRelLen;
            std::vector<ColumeDataEx>::iterator lIter = nVecColume.begin();
            for(;lIter != nVecColume.end();lIter++)
            {
                char * lpBuf = (char*)malloc(lnMaxNum *lIter->nColumeMaxLen);
                memset(lpBuf,0,lnMaxNum *lIter->nColumeMaxLen);
                unsigned short * lpLenBuf = (unsigned short *)malloc(lnMaxNum*sizeof(unsigned short));
                memset(lpLenBuf,0,lnMaxNum*sizeof(unsigned short));
                lpVecColumnBuf.push_back(lpBuf);	
                lVecColumnRelLen.push_back(lpLenBuf);

            }



            //填充内存
            for ( int j = i*nMaxInsertRow; j < lnMaxNum; j++)
            {
                const T* field = &nVecData[j];		

                for(unsigned int k = 0; k < nVecColume.size();k++)
                {
                    char * lpBuf = (char*)field+nVecColume[k].nOffset;	

                    memcpy(lpVecColumnBuf[k]+j*nVecColume[k].nColumeMaxLen,
                        lpBuf,
                        nVecColume[k].nColumeMaxLen);


                    lVecColumnRelLen[k][j]  = nVecColume[k].nColumeMaxLen;

                    if(nVecColume[k].eColumeType == oracle::occi::OCCI_SQLT_STR)
                        lVecColumnRelLen[k][j]  = strlen((char*)lpBuf) + 1;
                }

                nRecordNum++;
            }

            //设置内存到oracle
            for(unsigned int k = 0; k < lpVecColumnBuf.size();k++)
            {		

                m_pStmt->setDataBuffer(k+1, (void*)lpVecColumnBuf[k], 
                    nVecColume[k].eColumeType,
                    nVecColume[k].nColumeMaxLen, 
                    lVecColumnRelLen[k]); 

            }

            try
            {
                m_pStmt->executeArrayUpdate(nRecordNum); 
            }			
            catch (oracle::occi::SQLException &e)
            {
                //释放内存
                for(unsigned int k = 0; k < lpVecColumnBuf.size();k++)
                {
                    free(lpVecColumnBuf[k]);
                    free(lVecColumnRelLen[k]);
                }	
                throw e;
            }
            //释放内存
            for(unsigned int k = 0; k < lpVecColumnBuf.size();k++)
            {
                free(lpVecColumnBuf[k]);
                free(lVecColumnRelLen[k]);
            }	

        }

        m_pCon->commit();
        m_pCon->terminateStatement(m_pStmt);
    }
    catch(oracle::occi::SQLException &e)
    {
        RollBack();
        std::cout<<e.what()<<endl;
        //std::cout<<lSql<<endl;
        nErrorCode = GetErrorCode(e.what());
        WriteLog(lSql.c_str(), e.what());
        bRet = false;
    }

    return bRet;
}

template <class T>
bool CSvrDBOprImpl::BatchDelete(							
				 const std::string& nsTableName,
				 const std::string& nsConditionStr,
				 std::vector<ColumeDataEx>& nVecColume ,
				 const std::vector<T>& nVecData ,
				 int& nErrorCode )
{
	CGuard guard(&g_mutex);
	nErrorCode = CF_ERROR_SUCCESS;
	if ( !IsConnected() && !Conncect())
	{
		return false;
	}

	std::string lSql;

	bool bRet = true;
	try
	{
		m_pStmt = m_pCon->createStatement();

		//生成Sql语句
		lSql = "delete from " + nsTableName + nsConditionStr;


		//批量删除，每次最多存储10000条		
		m_pStmt->setSQL(lSql);
		int nCount = nVecData.size() / nMaxInsertRow;
		if ( nVecData.size() % nMaxInsertRow != 0 )
		{
			nCount++;
		}

		for ( int i = 0; i < nCount; i++ )
		{
			int index=0,nRecordNum = 0;
			int lnRemindNum = nVecData.size() - i*nMaxInsertRow;
			int lnMaxNum = (nMaxInsertRow > lnRemindNum) ? lnRemindNum : nMaxInsertRow;


			//分配内存
			std::vector<char*> lpVecColumnBuf;
			std::vector<unsigned short*> lVecColumnRelLen;
			std::vector<ColumeDataEx>::iterator lIter = nVecColume.begin();
			for(;lIter != nVecColume.end();lIter++)
			{
				char * lpBuf = (char*)malloc(lnMaxNum *lIter->nColumeMaxLen);
				memset(lpBuf,0,lnMaxNum *lIter->nColumeMaxLen);
				unsigned short * lpLenBuf = (unsigned short *)malloc(lnMaxNum*sizeof(unsigned short));
				memset(lpLenBuf,0,lnMaxNum*sizeof(unsigned short));
				lpVecColumnBuf.push_back(lpBuf);	
				lVecColumnRelLen.push_back(lpLenBuf);
			}



			//填充内存
			for ( int j = i*nMaxInsertRow; j < lnMaxNum; j++)
			{
				const T* field = &nVecData[j];		

				for(unsigned int k = 0; k < nVecColume.size();k++)
				{
					char * lpBuf = (char*)field+nVecColume[k].nOffset;	

					memcpy(lpVecColumnBuf[k]+j*nVecColume[k].nColumeMaxLen,
						lpBuf,
						nVecColume[k].nColumeMaxLen);


					lVecColumnRelLen[k][j]  = nVecColume[k].nColumeMaxLen;

					if(nVecColume[k].eColumeType == oracle::occi::OCCI_SQLT_STR)
						lVecColumnRelLen[k][j]  = strlen((char*)lpBuf) + 1;
				}

				nRecordNum++;
			}

			//设置内存到oracle
			for(unsigned int k = 0; k < lpVecColumnBuf.size();k++)
			{		

				m_pStmt->setDataBuffer(k+1, (void*)lpVecColumnBuf[k], 
					nVecColume[k].eColumeType,
					nVecColume[k].nColumeMaxLen, 
					lVecColumnRelLen[k]); 

			}

			try
			{
				m_pStmt->executeArrayUpdate(nRecordNum); 
			}			
			catch (oracle::occi::SQLException &e)
			{
				//释放内存
				for(unsigned int k = 0; k < lpVecColumnBuf.size();k++)
				{
					free(lpVecColumnBuf[k]);
					free(lVecColumnRelLen[k]);
				}	
				throw e;
			}
			//释放内存
			for(unsigned int k = 0; k < lpVecColumnBuf.size();k++)
			{
				free(lpVecColumnBuf[k]);
				free(lVecColumnRelLen[k]);
			}	

		}

		m_pCon->commit();
		m_pCon->terminateStatement(m_pStmt);
	}
	catch(oracle::occi::SQLException &e)
	{
		RollBack();
		std::cout<<e.what()<<endl;
		//std::cout<<lSql<<endl;
		nErrorCode = GetErrorCode(e.what());
		WriteLog(lSql.c_str(), e.what());
		bRet = false;
	}

	return bRet;
}

bool CSvrDBOprImpl::SaveOrderTransfer(const std::vector<SOrderTransfer>& vecOrderTransfer ,
                                      int& nErrorCode )
{
    CGuard guard(&g_mutex);

    char szBuffer[MAX_SQL_LENGTH];
    memset(szBuffer, 0, sizeof(szBuffer));
    OTACCOUNTTYPE* szAccountID	= new OTACCOUNTTYPE[nMaxInsertRow];
    int *nFrontID						= new int[nMaxInsertRow];
    int *nSessionID						= new int[nMaxInsertRow];
    int *nOrderRef						= new int[nMaxInsertRow];
    OTACCOUNTTYPE* szInvestorID			= new OTACCOUNTTYPE[nMaxInsertRow];
    int *nCTPOrderRef					= new int[nMaxInsertRow];
    int *nCTPFrontID					= new int[nMaxInsertRow];
    int *nCTPSessionID					= new int[nMaxInsertRow];
    OTACCOUNTTYPE* szBrokerID			= new OTACCOUNTTYPE[nMaxInsertRow];
    ORDERSYSID* orderSysID				= new ORDERSYSID[nMaxInsertRow];
    TExchangeIDType* ExchangeID			= new TExchangeIDType[nMaxInsertRow];


    bool bRet = true;
    try
    {
        m_pStmt = m_pCon->createStatement();

        //批量存储，每次最多存储10000条
        memset(szBuffer, 0, sizeof(szBuffer));
        sprintf(szBuffer, "insert into OFFER_ORDERREFTRANSFER values(:v1,:v2,:v3,:v4,:v5,:v6,:v7,sysdate,:v8,:v9, :v10, :v11)");
        m_pStmt->setSQL(szBuffer);
        int nCount = vecOrderTransfer.size() / nMaxInsertRow;
        if ( vecOrderTransfer.size() % nMaxInsertRow != 0 )
        {
            nCount++;
        }

        for ( int i = 0; i < nCount; i++ )
        {
            ub2 len_int[nMaxInsertRow];
            ub2 len_string[nMaxInsertRow];
            ub2 len_stringInvestorID[nMaxInsertRow];
            ub2 len_stringszBrokerID[nMaxInsertRow];
            ub2 len_stringorderSysID[nMaxInsertRow];
            ub2 len_stringExchangeID[nMaxInsertRow];

            int nRecordNum = 0;
            for ( int j = i*nMaxInsertRow; j < static_cast<int>(vecOrderTransfer.size()) && nRecordNum < nMaxInsertRow; j++)
            {
                SOrderTransfer info = vecOrderTransfer[j];
                len_int[nRecordNum] = sizeof(int);	

                strcpy(szAccountID[nRecordNum], info.szAccountID);	
                len_string[nRecordNum] = strlen(szAccountID[nRecordNum])+1;
                nFrontID[nRecordNum]		 = info.nFrontID;
                nSessionID[nRecordNum]		 = info.nSessionID;	
                nOrderRef[nRecordNum]		 = info.nOrderRef;			
                nCTPOrderRef[nRecordNum]	 = info.nCTPOrderRef;				
                nCTPFrontID[nRecordNum]		 = info.nCTPFrontID;
                nCTPSessionID[nRecordNum]    = info.nCTPSessionID;	

                strcpy(szInvestorID[nRecordNum], info.szInvestorID);	
                len_stringInvestorID[nRecordNum] = strlen(szInvestorID[nRecordNum])+1;

                strcpy(szBrokerID[nRecordNum], info.szBrokerID);	
                len_stringszBrokerID[nRecordNum] = strlen(szBrokerID[nRecordNum])+1;

                strcpy(orderSysID[nRecordNum], info.orderSysID);	
                len_stringorderSysID[nRecordNum] = strlen(orderSysID[nRecordNum])+1;

                strcpy(ExchangeID[nRecordNum], info.ExchangeID);	
                len_stringExchangeID[nRecordNum] = strlen(ExchangeID[nRecordNum])+1;
                nRecordNum++;
            }

            m_pStmt->setDataBuffer(1, szAccountID, OCCI_SQLT_STR, sizeof(OTACCOUNTTYPE), len_string); 
            m_pStmt->setDataBuffer(2, nFrontID, OCCIINT, sizeof(int), len_int); 
            m_pStmt->setDataBuffer(3, nSessionID, OCCIINT, sizeof(int), len_int);
            m_pStmt->setDataBuffer(4, nOrderRef, OCCIINT, sizeof(int), len_int); 
            m_pStmt->setDataBuffer(5, nCTPOrderRef, OCCIINT, sizeof(int), len_int); 
            m_pStmt->setDataBuffer(6, nCTPFrontID, OCCIINT, sizeof(int), len_int); 
            m_pStmt->setDataBuffer(7, nCTPSessionID, OCCIINT, sizeof(int), len_int); 
            m_pStmt->setDataBuffer(8, szInvestorID, OCCI_SQLT_STR, sizeof(OTACCOUNTTYPE), len_stringInvestorID); 
            m_pStmt->setDataBuffer(9, szBrokerID, OCCI_SQLT_STR, sizeof(OTACCOUNTTYPE), len_stringszBrokerID); 
            m_pStmt->setDataBuffer(10, orderSysID, OCCI_SQLT_STR, sizeof(ORDERSYSID), len_stringorderSysID); 
            m_pStmt->setDataBuffer(11, ExchangeID, OCCI_SQLT_STR, sizeof(TExchangeIDType), len_stringExchangeID); 

            m_pStmt->executeArrayUpdate(nRecordNum); 
        }

        m_pCon->commit();
        m_pCon->terminateStatement(m_pStmt);
    }
    catch(oracle::occi::SQLException &e)
    {
        RollBack();
        std::cout<<e.what()<<endl;
        std::cout<<szBuffer<<endl;
        nErrorCode = GetErrorCode(e.what());
        WriteLog(szBuffer, e.what());
        //return false;
    }

    delete [] szAccountID;
    delete [] nFrontID;
    delete [] nSessionID;
    delete [] nOrderRef;
    delete [] nCTPOrderRef;
    delete [] nCTPFrontID;
    delete [] nCTPSessionID;
    delete [] szInvestorID;
    delete [] szBrokerID;
    delete [] orderSysID;
    delete [] ExchangeID;

    return bRet;
}
bool CSvrDBOprImpl::GetOrderTransfer(std::string strTime,  std::vector<SOrderTransfer>& vecOrderTransfer)
{
    CGuard guard(&g_mutex);

    if ( !IsConnected() && !Conncect())
    {
        return false;
    }

    char szBuffer[MAX_SQL_LENGTH];
    memset(szBuffer, 0, sizeof(szBuffer));
    try
    {
        sprintf(szBuffer, "select t1.ACCOUNTID,t1.FRONTID,t1.SESSIONID,t1.ORDERREF,t1.CTP_ORDERREF,\
                          t1.CTP_FRONTID,t1.CTP_SESSIONID,to_char(t1.TIME, 'YYYY-MM-DD'), t1.INVESTORID, t1.BROKERID, t1.ORDERSYSID, t1.EXCHANGEID\
                          from OFFER_ORDERREFTRANSFER t1 where to_char(t1.TIME, 'YYYY-MM-DD') = '%s'",
                          strTime.c_str());
        m_pStmt = m_pCon->createStatement( szBuffer );
        m_pRes = m_pStmt->executeQuery();
        while ( m_pRes->next())
        {
            SOrderTransfer data;
            memset(&data, 0, sizeof(data));

            std::string strValue = m_pRes->getString(1);
            strcpy(data.szAccountID, strValue.c_str());

            data.nFrontID = m_pRes->getInt(2);
            data.nSessionID = m_pRes->getInt(3);
            data.nOrderRef = m_pRes->getInt(4);
            data.nCTPOrderRef = m_pRes->getInt(5);
            data.nCTPFrontID = m_pRes->getInt(6);
            data.nCTPSessionID = m_pRes->getInt(7);		
            strValue = m_pRes->getString(8);
            strcpy(data.szUpdateDate, strValue.c_str());
            strValue = m_pRes->getString(9);
            strcpy(data.szInvestorID, strValue.c_str());
            strValue = m_pRes->getString(10);
            strcpy(data.szBrokerID, strValue.c_str());

            strValue = m_pRes->getString(11);
            strcpy(data.orderSysID, strValue.c_str());

            strValue = m_pRes->getString(12);
            strcpy(data.ExchangeID, strValue.c_str());
            vecOrderTransfer.push_back(data);
        }

        m_pStmt->closeResultSet(m_pRes);
        m_pCon->terminateStatement(m_pStmt);
    }catch(oracle::occi::SQLException &e){
        std::cout<<e.what()<<endl;
        std::cout<<szBuffer<<endl;

        WriteLog(szBuffer, e.what());
        return false;
    }

    return true;
}
bool CSvrDBOprImpl::UpdateOrderTransfer(SOrderTransfer& sOrderTransfer)
{
    char strTime[64];
    memset(&strTime, 0, sizeof(strTime));
    SYSTEMTIME st;
    GetLocalTime(&st);	
    sprintf(strTime,"%4d-%02d-%02d",st.wYear, st.wMonth, st.wDay);	

    CGuard guard(&g_mutex);

    if ( !IsConnected() && !Conncect())
    {
        return false;
    }

    char szBuffer[MAX_SQL_LENGTH];
    memset(szBuffer, 0, sizeof(szBuffer));
    try
    {
        sprintf(szBuffer, "update OFFER_ORDERREFTRANSFER  set ordersysid ='%s' , EXCHANGEID = '%s' where orderref = %d \
                          and accountid = '%s' and frontid = %d and sessionid = %d \
                          and  to_char(TIME, 'YYYY-MM-DD') = '%s'",
                          sOrderTransfer.orderSysID, sOrderTransfer.ExchangeID, sOrderTransfer.nOrderRef, 
                          sOrderTransfer.szAccountID, sOrderTransfer.nFrontID,
                          sOrderTransfer.nSessionID, strTime);
        m_pStmt = m_pCon->createStatement( szBuffer );
        m_pRes = m_pStmt->executeQuery();

        m_pStmt->closeResultSet(m_pRes);
        m_pCon->commit();
        m_pCon->terminateStatement(m_pStmt);
    }catch(oracle::occi::SQLException &e){
        std::cout<<e.what()<<endl;
        std::cout<<szBuffer<<endl;

        WriteLog(szBuffer, e.what());
        return false;
    }

    return true;
}
bool CSvrDBOprImpl::AddMessage( const MessageInfo& msgInfo, int& nPKID, int& nErrorCode )
{
    CGuard guard(&g_mutex);
    nErrorCode = CF_ERROR_SUCCESS;
    if ( !IsConnected() && !Conncect())
    {
        nErrorCode = CF_ERROR_DATABASE_OTHER_ERROR;
        return false;
    }

    char szBuffer[MAX_SQL_LENGTH];

    try
    {
        memset(szBuffer, 0, sizeof(szBuffer));
        char szOwner[50];
        memset(szOwner, 0, sizeof(szOwner));
        if ( msgInfo.nOwner == 0 )
        {
            strcpy(szOwner, "NULL");
        }
        else
        {
            sprintf(szOwner, "%d", msgInfo.nOwner);
        }
        //先建立id自增 create sequence SEQ_MESSAGEID   minvalue 1 maxvalue 99999999 start with 1 increment by 1 nocache;
        sprintf(szBuffer, "insert into RISK_MESSAGEINFO values(SEQ_MESSAGEID.NEXTVAL, \
                          '%s','%s', to_date('%s','YYYY-MM-DD'), %s, sysdate,'%s')", 
                          msgInfo.szTitle, msgInfo.szContent, msgInfo.szExpiredDate, 
                          szOwner, msgInfo.szOwner);

        m_pStmt = m_pCon->createStatement();
        m_pStmt->execute(szBuffer);

        m_pRes = m_pStmt->executeQuery( "select SEQ_MESSAGEID.currval from dual" );
        if ( m_pRes->next())
        {
            nPKID = m_pRes->getInt(1);
        }
        m_pStmt->closeResultSet(m_pRes);

        m_pCon->commit();
        m_pCon->terminateStatement(m_pStmt);
    }catch(oracle::occi::SQLException &e)
    {
        RollBack();
        std::cout<<e.what()<<endl;
        std::cout<<szBuffer<<endl;	
        nErrorCode = GetErrorCode(e.what());
        WriteLog(szBuffer,  e.what());
        return false;
    }

    return true;
}
bool CSvrDBOprImpl::SaveDepthMarketData( const std::string& nsTableName,
                                        const std::string& strTime,
                                        const std::vector<PlatformStru_DepthMarketData>& vecData, int& nErrorCode)
{
    CGuard guard(&g_mutex);

    PlatformStru_DepthMarketData lInfo;
    std::vector<ColumeData> ltest;
    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcDateType),(int)&lInfo.TradingDay - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(InstrumentIDType),(int)&lInfo.InstrumentID - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcExchangeIDType),(int)&lInfo.ExchangeID - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcExchangeInstIDType),(int)&lInfo.ExchangeInstID - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.LastPrice - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.PreSettlementPrice - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.PreClosePrice - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcLargeVolumeType),(int)&lInfo.PreOpenInterest - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.OpenPrice- (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.HighestPrice - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.LowestPrice - (int)&lInfo);

    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.Volume - (int)&lInfo);

    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.Turnover - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcLargeVolumeType),(int)&lInfo.OpenInterest - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.ClosePrice - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.SettlementPrice- (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.UpperLimitPrice- (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.LowerLimitPrice - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcRatioType),(int)&lInfo.PreDelta - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcRatioType),(int)&lInfo.CurrDelta - (int)&lInfo);

    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcTimeType),(int)&lInfo.UpdateTime - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcMillisecType),(int)&lInfo.UpdateMillisec - (int)&lInfo);

    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.BidPrice1 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.BidVolume1 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.AskPrice1 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcSettlementIDType),(int)&lInfo.AskVolume1 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.BidPrice2 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.BidVolume2 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.AskPrice2 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.AskVolume2 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.BidPrice3 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.BidVolume3 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.AskPrice3 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.AskVolume3 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.BidPrice4 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.BidVolume4 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.AskPrice4 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.AskVolume4 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.BidPrice5 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.BidVolume5 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.AskPrice5 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.AskVolume5 - (int)&lInfo);

    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.AveragePrice - (int)&lInfo);

    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.BidPrice6 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.BidVolume6 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.AskPrice6 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.AskVolume6 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.BidPrice7 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.BidVolume7 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.AskPrice7 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.AskVolume7 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.BidPrice8 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.BidVolume8 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.AskPrice8 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.AskVolume8 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.BidPrice9 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.BidVolume9 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.AskPrice9 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.AskVolume9 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.BidPrice10 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.BidVolume10 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.AskPrice10 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.AskVolume10 - (int)&lInfo);

    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.NewVolume - (int)&lInfo);



    return BatchInsert(	nsTableName,
        "",
        strTime,
        ltest,
        vecData,
        nErrorCode);
    return false;
}

bool CSvrDBOprImpl::CreatKLineTable_PerDay(const std::string& strDate)
{
	std::string lSql;
	int nErrorCode;

	int nRecordNum = 0; //保存查询表的个数
	bool bRet = true;

	char tradedate[16];
	memset(tradedate,0,sizeof(tradedate)-1);
	strncpy(tradedate,strDate.c_str(),6);
	string strdate(tradedate);

	std::vector<std::string>  vecTableName;
	vecTableName.push_back( "KLINEMINI1");
	vecTableName.push_back( "KLINEMINI5");
	vecTableName.push_back( "KLINEMINI15");
	vecTableName.push_back( "KLINEMINI30");
	vecTableName.push_back( "KLINEMINI60");
	vecTableName.push_back( "KLINEMINIDAY");
	vecTableName.push_back( "KLINEMINIWEEK");
	vecTableName.push_back( "KLINEMINIMONTH");

	std::string strName;

	for ( int i = 0; i< vecTableName.size();i++)
	{

		strName.clear();
		if ( i> 4)
		{
			//strdate.substr(0,4);
			strName = vecTableName[i]; 
		}
		else
		{
            strName = vecTableName[i] + "_" + strdate; 
		}
		

		//查询是否有该表
		lSql = "select count(*) from user_tables where table_name=\'" + strName + "\'";

		try
		{
			m_pStmt = m_pCon->createStatement( lSql );
			m_pRes = m_pStmt->executeQuery();
			if ( m_pRes->next())
			{
				nRecordNum = m_pRes->getInt(1);
			}

			m_pStmt->closeResultSet(m_pRes);
			m_pCon->terminateStatement(m_pStmt);
		}catch(oracle::occi::SQLException &e){
			RollBack();
			std::cout<<e.what()<<endl;
			std::cout<<lSql<<endl;
			nErrorCode = GetErrorCode(e.what());
			WriteLog(lSql, e.what());
			return false;
		}

		if (nRecordNum==0)//无该表 创建该表
		{   

			lSql = "";
			lSql = "CREATE TABLE " + strName +
				"( \
				instrumentid       VARCHAR2(64),\
				EnumPhrase         INTEGER,\
				dwTime             DATE,\
				OpenPrice          BINARY_DOUBLE,\
				HighPrice          BINARY_DOUBLE,\
				fLowPrice          BINARY_DOUBLE,\
				fClosePrice        BINARY_DOUBLE,\
				fSumTradeVolume    BINARY_DOUBLE,\
				dwVolume           INTEGER,\
				dwHoldVolume       INTEGER,\
				dJieSuan           BINARY_DOUBLE,\
				insertdbtime       DATE\
				)" +" NOLOGGING" ;            

			try
			{
				m_pStmt = m_pCon->createStatement();
				m_pStmt->execute( lSql );
				m_pCon->commit();

				m_pCon->terminateStatement(m_pStmt);
			}catch(oracle::occi::SQLException &e){
				RollBack();
				std::cout<<e.what()<<endl;
				std::cout<<lSql<<endl;
				nErrorCode = GetErrorCode(e.what());
				WriteLog(lSql, e.what());
				return false;
			}
	} 
		////创建索引
		//lSql = "";
		//lSql =  "create index INDEX_" + strName + " on " + strName + 
		//	" (INSTRUMENTID,DWTIME)";

		//try
		//{
		//	m_pStmt = m_pCon->createStatement();
		//	m_pStmt->execute( lSql );
		//	m_pCon->commit();

		//	m_pCon->terminateStatement(m_pStmt);
		//}catch(oracle::occi::SQLException &e){
		//	RollBack();
		//	std::cout<<e.what()<<endl;
		//	std::cout<<lSql<<endl;
		//	nErrorCode = GetErrorCode(e.what());
		//	WriteLog(lSql, e.what());
		//	return false;
		//} 
	}


	return true;
}

bool CSvrDBOprImpl::CreatDepthMarketTable_PerDay(const std::string& nsTableName)
{
	std::string lSql;
	int nErrorCode;

	int nRecordNum = 0; //保存查询表的个数
	bool bRet = true;

	char tradedate[16];
	memset(tradedate,0,sizeof(tradedate)-1);
	strncpy(tradedate,nsTableName.c_str()+10,8);
	string strdate(tradedate);
	//查询是否有该表
	lSql = "select count(*) from user_tables where table_name=\'" + nsTableName + "\'";

	try
	{
		m_pStmt = m_pCon->createStatement( lSql );
		m_pRes = m_pStmt->executeQuery();
		if ( m_pRes->next())
		{
			nRecordNum = m_pRes->getInt(1);
		}

		m_pStmt->closeResultSet(m_pRes);
		m_pCon->terminateStatement(m_pStmt);
	}catch(oracle::occi::SQLException &e){
		RollBack();
		std::cout<<e.what()<<endl;
		std::cout<<lSql<<endl;
		 nErrorCode = GetErrorCode(e.what());
		WriteLog(lSql, e.what());
		return false;
	}

	if (nRecordNum==0)//无该表 创建该表
	{   

		lSql = "";
		lSql = "CREATE TABLE " + nsTableName +
			"( \
			seq                INTEGER primary key,\
			tradingday         VARCHAR2(9),\
			instrumentid       VARCHAR2(64),\
			exchangeid         VARCHAR2(9),\
			exchangeinstid     VARCHAR2(31),\
			lastprice          BINARY_DOUBLE,\
			presettlementprice BINARY_DOUBLE,\
			precloseprice      BINARY_DOUBLE,\
			preopeninterest    BINARY_DOUBLE,\
			openprice          BINARY_DOUBLE,\
			highestprice       BINARY_DOUBLE,\
			lowestprice        BINARY_DOUBLE,\
			volume             INTEGER,\
			turnover           BINARY_DOUBLE,\
			openinterest       BINARY_DOUBLE,\
			closeprice         BINARY_DOUBLE,\
			settlementprice    BINARY_DOUBLE,\
			upperlimitprice    BINARY_DOUBLE,\
			lowerlimitprice    BINARY_DOUBLE,\
			predelta           BINARY_DOUBLE,\
			currdelta          BINARY_DOUBLE,\
			updatetime         VARCHAR2(9),\
			updatemillisec     INTEGER,\
			bidprice1          BINARY_DOUBLE,\
			bidvolume1         INTEGER,\
			askprice1          BINARY_DOUBLE,\
			askvolume1         INTEGER,\
			bidprice2          BINARY_DOUBLE,\
			bidvolume2         INTEGER,\
			askprice2          BINARY_DOUBLE,\
			askvolume2         INTEGER,\
			bidprice3          BINARY_DOUBLE,\
			bidvolume3         INTEGER,\
			askprice3          BINARY_DOUBLE,\
			askvolume3         INTEGER,\
			bidprice4          BINARY_DOUBLE,\
			bidvolume4         INTEGER,\
			askprice4          BINARY_DOUBLE,\
			askvolume4         INTEGER,\
			bidprice5          BINARY_DOUBLE,\
			bidvolume5         INTEGER,\
			askprice5          BINARY_DOUBLE,\
			askvolume5         INTEGER,\
			averageprice       BINARY_DOUBLE,\
			bidprice6          BINARY_DOUBLE,\
			bidvolume6         INTEGER,\
			askprice6          BINARY_DOUBLE,\
			askvolume6         INTEGER,\
			bidprice7          BINARY_DOUBLE,\
			bidvolume7         INTEGER,\
			askprice7          BINARY_DOUBLE,\
			askvolume7         INTEGER,\
			bidprice8          BINARY_DOUBLE,\
			bidvolume8         INTEGER,\
			askprice8          BINARY_DOUBLE,\
			askvolume8         INTEGER,\
			bidprice9          BINARY_DOUBLE,\
			bidvolume9         INTEGER,\
			askprice9          BINARY_DOUBLE,\
			askvolume9         INTEGER,\
			bidprice10         BINARY_DOUBLE,\
			bidvolume10        INTEGER,\
			askprice10         BINARY_DOUBLE,\
			askvolume10        INTEGER,\
			newvolume          INTEGER,\
			validatedate       VARCHAR2(11),\
			insertdbtime       DATE\
			)" +" NOLOGGING" ;            

		try
		{
			m_pStmt = m_pCon->createStatement();
			m_pStmt->execute( lSql );
			m_pCon->commit();

			m_pCon->terminateStatement(m_pStmt);
		}catch(oracle::occi::SQLException &e){
			RollBack();
			std::cout<<e.what()<<endl;
			std::cout<<lSql<<endl;
			nErrorCode = GetErrorCode(e.what());
			WriteLog(lSql, e.what());
			return false;
		}



		//创建索引
		lSql = "";
		lSql =  "create index INDEX_INSERTDBTIME_" + strdate + " on " + nsTableName + 
			" (INSERTDBTIME)";

		try
		{
			m_pStmt = m_pCon->createStatement();
			m_pStmt->execute( lSql );
			m_pCon->commit();

			m_pCon->terminateStatement(m_pStmt);
		}catch(oracle::occi::SQLException &e){
			RollBack();
			std::cout<<e.what()<<endl;
			std::cout<<lSql<<endl;
			nErrorCode = GetErrorCode(e.what());
			WriteLog(lSql, e.what());
			return false;
		}  

		//创建索引
		lSql = "";
		lSql =  "create index INDEX_INSTRUMENTID_" + strdate + " on " + nsTableName + 
			" (INSTRUMENTID)";

		try
		{
			m_pStmt = m_pCon->createStatement();
			m_pStmt->execute( lSql );
			m_pCon->commit();

			m_pCon->terminateStatement(m_pStmt);
		}catch(oracle::occi::SQLException &e){
			RollBack();
			std::cout<<e.what()<<endl;
			std::cout<<lSql<<endl;
			nErrorCode = GetErrorCode(e.what());
			WriteLog(lSql, e.what());
			return false;
		}  
		lSql =  "create index INDEX_UPDATETIME_" + strdate + " on " + nsTableName + 
			" (UPDATETIME)" ;

		try
		{
			m_pStmt = m_pCon->createStatement();
			m_pStmt->execute( lSql );
			m_pCon->commit();

			m_pCon->terminateStatement(m_pStmt);
		}catch(oracle::occi::SQLException &e){
			RollBack();
			std::cout<<e.what()<<endl;
			std::cout<<lSql<<endl;
			nErrorCode = GetErrorCode(e.what());
			WriteLog(lSql, e.what());
			return false;
		} 
	}

		//每月第一天重建SEQ


		//string strdate = nsTableName.erase(0,10);
			strdate.erase(0,6);

		//查找序列号是否存在
			nRecordNum =0;
			lSql = "";
			lSql =  "select count(*) FROM All_Sequences where sequence_name='QUOT_SEQ' and sequence_owner='TS_INSTITUTIONDB_TEST'" ;

			try
			{
				m_pStmt = m_pCon->createStatement( lSql );
				m_pRes = m_pStmt->executeQuery();
				if ( m_pRes->next())
				{
					nRecordNum = m_pRes->getInt(1);
				}

				m_pStmt->closeResultSet(m_pRes);
				m_pCon->terminateStatement(m_pStmt);
			}catch(oracle::occi::SQLException &e){
				RollBack();
				std::cout<<e.what()<<endl;
				std::cout<<lSql<<endl;
				nErrorCode = GetErrorCode(e.what());
				WriteLog(lSql, e.what());
				return false;
			}
	if (strdate =="26"|| nRecordNum == 0 )
	{
			if (nRecordNum != 0)//已存在的话先删掉
			{
				lSql = "";
				lSql =  "drop sequence QUOT_SEQ" ;

				try
				{
					m_pStmt = m_pCon->createStatement();
					m_pStmt->execute( lSql );
					m_pCon->commit();

					m_pCon->terminateStatement(m_pStmt);
				}catch(oracle::occi::SQLException &e){
					RollBack();
					std::cout<<e.what()<<endl;
					std::cout<<lSql<<endl;
					nErrorCode = GetErrorCode(e.what());
					WriteLog(lSql, e.what());
					return false;
				}  
			}

			//创建序列
			lSql = "";
			lSql =  "create sequence QUOT_SEQ  MINVALUE 1 NOMAXVALUE INCREMENT BY 1 START WITH 1 NOCACHE  NOORDER  NOCYCLE" ;

			try
			{
				m_pStmt = m_pCon->createStatement();
				m_pStmt->execute( lSql );
				m_pCon->commit();

				m_pCon->terminateStatement(m_pStmt);
			}catch(oracle::occi::SQLException &e){
				RollBack();
				std::cout<<e.what()<<endl;
				std::cout<<lSql<<endl;
				nErrorCode = GetErrorCode(e.what());
				WriteLog(lSql, e.what());
				return false;
			}  
		}
	
	return true;
}

bool CSvrDBOprImpl::SaveDepthMarketData_PerDay( const std::string& nsTableName,
                                               const std::string& strTime,
                                               const std::vector<PlatformStru_DepthMarketData>& vecData, int& nErrorCode)
{
    CGuard guard(&g_mutex);

    PlatformStru_DepthMarketData lInfo;
    std::vector<ColumeData> ltest;
	//PushColumnDataToVector(ltest,OCCIINT,sizeof(int),sizeof(int));
    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcDateType),(int)&lInfo.TradingDay - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(InstrumentIDType),(int)&lInfo.InstrumentID - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcExchangeIDType),(int)&lInfo.ExchangeID - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcExchangeInstIDType),(int)&lInfo.ExchangeInstID - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.LastPrice - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.PreSettlementPrice - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.PreClosePrice - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcLargeVolumeType),(int)&lInfo.PreOpenInterest - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.OpenPrice- (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.HighestPrice - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.LowestPrice - (int)&lInfo);

    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.Volume - (int)&lInfo);

    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.Turnover - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcLargeVolumeType),(int)&lInfo.OpenInterest - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.ClosePrice - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.SettlementPrice- (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.UpperLimitPrice- (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.LowerLimitPrice - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcRatioType),(int)&lInfo.PreDelta - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcRatioType),(int)&lInfo.CurrDelta - (int)&lInfo);

    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcTimeType),(int)&lInfo.UpdateTime - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcMillisecType),(int)&lInfo.UpdateMillisec - (int)&lInfo);

    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.BidPrice1 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.BidVolume1 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.AskPrice1 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcSettlementIDType),(int)&lInfo.AskVolume1 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.BidPrice2 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.BidVolume2 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.AskPrice2 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.AskVolume2 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.BidPrice3 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.BidVolume3 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.AskPrice3 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.AskVolume3 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.BidPrice4 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.BidVolume4 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.AskPrice4 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.AskVolume4 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.BidPrice5 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.BidVolume5 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.AskPrice5 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.AskVolume5 - (int)&lInfo);

    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.AveragePrice - (int)&lInfo);

    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.BidPrice6 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.BidVolume6 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.AskPrice6 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.AskVolume6 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.BidPrice7 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.BidVolume7 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.AskPrice7 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.AskVolume7 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.BidPrice8 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.BidVolume8 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.AskPrice8 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.AskVolume8 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.BidPrice9 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.BidVolume9 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.AskPrice9 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.AskVolume9 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.BidPrice10 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.BidVolume10 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.AskPrice10 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.AskVolume10 - (int)&lInfo);

    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.NewVolume - (int)&lInfo);



    return BatchInsert_PerDay(	nsTableName,
        "",
        strTime,
        ltest,
        vecData,
        nErrorCode);
    return false;
}

bool CSvrDBOprImpl::SaveSettlementPrice2DB(std::string strTime, std::vector<SettlementPriceField>& vSettlementPrice ,int& nErrorCode)
{
    CGuard guard(&g_mutex);
    nErrorCode = CF_ERROR_SUCCESS;
    if ( !IsConnected() && !Conncect())
    {
        nErrorCode = CF_ERROR_DATABASE_OTHER_ERROR;
        return false;
    }

    char szBuffer[MAX_SQL_LENGTH];
    try
    {
        memset(szBuffer, 0, sizeof(szBuffer));
        sprintf(szBuffer, "delete from SETTLEMENT_SETTLEMENTPRICE t where to_char(t.VALIDATEDATE,'YYYY-MM-DD')  = '%s'",strTime.c_str());
        m_pStmt = m_pCon->createStatement();
        m_pStmt->execute(szBuffer);

        std::vector<SettlementPriceField>::iterator it=vSettlementPrice.begin();
        while(it !=vSettlementPrice.end())	
        {
            memset(szBuffer, 0, sizeof(szBuffer));
            sprintf(szBuffer,"insert into SETTLEMENT_SETTLEMENTPRICE values('%s','%s','%s',%f,%f,'%s',sysdate)",(*it).InstrumentID,\
                (*it).ExchangeID,(*it).ProductID,(*it).SettlementPrice,(*it).LastSettlementPrice,(*it).SettlementDate);
            m_pStmt->execute( szBuffer );
            it++;
        }
        m_pCon->commit();
        m_pCon->terminateStatement(m_pStmt);
    }catch(oracle::occi::SQLException &e){
        RollBack();
        std::cout<<e.what()<<endl;
        std::cout<<szBuffer<<endl;
        nErrorCode = GetErrorCode(e.what());
        WriteLog(szBuffer, e.what());
        return false;
    }

    return true;

}

bool CSvrDBOprImpl::SaveDepthMarketDataWithDelete( const std::string& nsTableName,
                                                  const std::string& strTime,
                                                  const std::vector<PlatformStru_DepthMarketData>& vecData, 
                                                  int& nErrorCode )
{
    CGuard guard(&g_mutex);

    PlatformStru_DepthMarketData lInfo;
    std::vector<ColumeData> ltest;
    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcDateType),(int)&lInfo.TradingDay - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(InstrumentIDType),(int)&lInfo.InstrumentID - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcExchangeIDType),(int)&lInfo.ExchangeID - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcExchangeInstIDType),(int)&lInfo.ExchangeInstID - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.LastPrice - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.PreSettlementPrice - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.PreClosePrice - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcLargeVolumeType),(int)&lInfo.PreOpenInterest - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.OpenPrice- (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.HighestPrice - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.LowestPrice - (int)&lInfo);

    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.Volume - (int)&lInfo);

    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.Turnover - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcLargeVolumeType),(int)&lInfo.OpenInterest - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.ClosePrice - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.SettlementPrice- (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.UpperLimitPrice- (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.LowerLimitPrice - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcRatioType),(int)&lInfo.PreDelta - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcRatioType),(int)&lInfo.CurrDelta - (int)&lInfo);

    PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcTimeType),(int)&lInfo.UpdateTime - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcMillisecType),(int)&lInfo.UpdateMillisec - (int)&lInfo);

    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.BidPrice1 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.BidVolume1 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.AskPrice1 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcSettlementIDType),(int)&lInfo.AskVolume1 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.BidPrice2 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.BidVolume2 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.AskPrice2 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.AskVolume2 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.BidPrice3 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.BidVolume3 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.AskPrice3 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.AskVolume3 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.BidPrice4 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.BidVolume4 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.AskPrice4 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.AskVolume4 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.BidPrice5 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.BidVolume5 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.AskPrice5 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.AskVolume5 - (int)&lInfo);

    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.AveragePrice - (int)&lInfo);

    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.BidPrice6 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.BidVolume6 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.AskPrice6 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.AskVolume6 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.BidPrice7 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.BidVolume7 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.AskPrice7 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.AskVolume7 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.BidPrice8 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.BidVolume8 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.AskPrice8 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.AskVolume8 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.BidPrice9 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.BidVolume9 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.AskPrice9 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.AskVolume9 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.BidPrice10 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.BidVolume10 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.AskPrice10 - (int)&lInfo);
    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.AskVolume10 - (int)&lInfo);

    PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.NewVolume - (int)&lInfo);


    std::string lsDeleteSql = 
        "delete from \"TRADEDATA_INSTRUEMENTS\" t where t.ValidateDate = \'" + strTime + "\'" ;

    return BatchInsert(	nsTableName,
        lsDeleteSql,
        strTime,
        ltest,
        vecData,
        nErrorCode);

}

bool CSvrDBOprImpl::SaveMessageSendStatus( std::vector<MsgSendStatus> vMsgStatus )
{
    CGuard guard(&g_mutex);
    if ( !IsConnected() && !Conncect())
    {
        return false;
    }

    char szBuffer[MAX_SQL_LENGTH];

    try
    {
        m_pStmt = m_pCon->createStatement();
        for ( UINT i = 0; i < vMsgStatus.size(); i++ )
        {
            memset(szBuffer, 0, sizeof(szBuffer));
            sprintf(szBuffer, "insert into RISK_MESSAGETARGET values(%d,%d,'%s',%d)",
                vMsgStatus[i].nRiskMgmtUserID, vMsgStatus[i].nMessageID, 
                vMsgStatus[i].szAccount, vMsgStatus[i].nSendStatus);
            m_pStmt->execute( szBuffer );
        }

        m_pCon->commit();
        m_pCon->terminateStatement(m_pStmt);
    }catch(oracle::occi::SQLException &e){
        RollBack();
        std::cout<<e.what()<<endl;
        std::cout<<szBuffer<<endl;

        WriteLog(szBuffer,  e.what());
        return false;
    }

    return true;
}
bool CSvrDBOprImpl::InsertRiskPlan(std::vector<RiskPlan>& vecRiskPlan, int& nErrorCode)
{
    CGuard guard(&g_mutex);
    nErrorCode = CF_ERROR_SUCCESS;
    if ( !IsConnected() && !Conncect())
    {
        nErrorCode = CF_ERROR_DATABASE_OTHER_ERROR;
        return false;
    }

    char szBuffer[MAX_SQL_LENGTH];

    try
    {
        if(vecRiskPlan.size() == 0)
            return false;

        m_pStmt = m_pCon->createStatement();

        RiskPlan& rPlan = vecRiskPlan[0];

        memset(szBuffer, 0, sizeof(szBuffer));
        sprintf(szBuffer, "delete RISK_RISKWARING t where t.orgid = %d and t.RISKINDICATORID = %d and t.BEGINTIME = '%s' and t.ENDTIME ='%s'",
			rPlan.OrgIDPlanRelation.nOrgID, rPlan.OrgIDPlanRelation.nRiskIndicatorID, rPlan.OrgIDPlanRelation.cTimeBegin, rPlan.OrgIDPlanRelation.cTimeEnd );
        m_pStmt->execute(szBuffer);

        memset(szBuffer, 0, sizeof(szBuffer));
        sprintf(szBuffer, "delete RISK_ORGIDPLANRELATION t where t.orgid = %d and t.RISKINDICATORID = %d and t.BEGINTIME = '%s' and t.ENDTIME ='%s'",
			rPlan.OrgIDPlanRelation.nOrgID, rPlan.OrgIDPlanRelation.nRiskIndicatorID, rPlan.OrgIDPlanRelation.cTimeBegin, rPlan.OrgIDPlanRelation.cTimeEnd  );
        m_pStmt->execute(szBuffer);

        std::vector<RiskPlan>::iterator it = vecRiskPlan.begin();
        for ( ; it != vecRiskPlan.end(); it++ )
        {			
            RiskPlan& riskPlan = *it;
            memset(szBuffer, 0, sizeof(szBuffer));
            if(it == vecRiskPlan.begin())
            {
                sprintf(szBuffer, "insert into RISK_ORGIDPLANRELATION values(%d, %d, %d, '%s', %d, %d, '%s', '%s', '%s', %f, %f, %f, %f, '%s' )",
                    riskPlan.OrgIDPlanRelation.nOrgID, riskPlan.OrgIDPlanRelation.nRiskIndicatorID, riskPlan.OrgIDPlanRelation.bAllProduct,
                    riskPlan.OrgIDPlanRelation.ProductID, riskPlan.OrgIDPlanRelation.bUse,  riskPlan.OrgIDPlanRelation.nRiskType,
					riskPlan.OrgIDPlanRelation.cTimeBegin, riskPlan.OrgIDPlanRelation.cTimeEnd,	riskPlan.OrgIDPlanRelation.cInstruments, 
					riskPlan.OrgIDPlanRelation.dbMarginDividDynamic, riskPlan.OrgIDPlanRelation.dbMarginUse, riskPlan.OrgIDPlanRelation.dbLossAmount, riskPlan.OrgIDPlanRelation.dbLossPercent,
					riskPlan.OrgIDPlanRelation.szPlan);
                m_pStmt->execute( szBuffer );
            }

            memset(szBuffer, 0, sizeof(szBuffer));
            sprintf(szBuffer, "insert into RISK_RISKWARING values(%d, %d, %d, %f, %d, %d, '%s','%s')",
                riskPlan.WaringLevel.nOrgID, riskPlan.WaringLevel.nRiskIndicatorID, riskPlan.WaringLevel.nRiskLevelID,
                riskPlan.WaringLevel.fThresholdValue, riskPlan.WaringLevel.nResponseType, riskPlan.WaringLevel.nColor, riskPlan.WaringLevel.cTimeBegin, riskPlan.WaringLevel.cTimeEnd);
            m_pStmt->execute( szBuffer );

            if(riskPlan.OrgIDPlanRelation.nRiskIndicatorID ==RI_FundNetValue && it == vecRiskPlan.begin())
            {
                memset(szBuffer, 0, sizeof(szBuffer));
                sprintf(szBuffer, "delete RISK_FUNDNETPARAM t where t.orgid = %d ", rPlan.OrgIDPlanRelation.nOrgID);
                m_pStmt->execute(szBuffer);

                memset(szBuffer, 0, sizeof(szBuffer));
                sprintf(szBuffer, "insert into RISK_FUNDNETPARAM values(%d, %f, %f, %f, %d)",
                    riskPlan.WaringLevel.nOrgID, riskPlan.netFundParam.dInnerVolumn, riskPlan.netFundParam.dOuterVolumn, riskPlan.netFundParam.dOuterNetAsset, riskPlan.netFundParam.nInnerNetAssetOption );
                m_pStmt->execute( szBuffer );
            }
        }

        m_pCon->commit();
        m_pCon->terminateStatement(m_pStmt);
    }catch(oracle::occi::SQLException &e)
    {
        RollBack();
        std::cout<<e.what()<<endl;
        std::cout<<szBuffer<<endl;
        nErrorCode = GetErrorCode(e.what());
        WriteLog(szBuffer, e.what());
        return false;
    }

    return true;
}
bool CSvrDBOprImpl::GetRiskPlan(const char* pSql, std::vector<RiskPlan>& vecRiskPlan, int& nErrorCode )
{
    CGuard guard(&g_mutex);
    nErrorCode = CF_ERROR_SUCCESS;
    if ( !IsConnected() && !Conncect())
    {
        nErrorCode = CF_ERROR_DATABASE_OTHER_ERROR;
        return false;
    }

    //	char szBuffer[MAX_SQL_LENGTH];
    //	memset(szBuffer, 0, sizeof(szBuffer));
    try
    {
        //	strcpy_s(szBuffer,MAX_SQL_LENGTH, "select t2.orgid,t2.riskindicatorid,t2.allproduct,t2.riskproduct,t2.use, t1.riskindicatorid, t1.risklevelid,t1.thresholdvalue, t1.responsetype, t1.color  from RISK_RISKWARING t1, RISK_ORGIDPLANRELATION t2 where t1.RISKINDICATORID = t2.RISKINDICATORID and t1.orgid = t2.orgid");
        m_pStmt = m_pCon->createStatement( pSql );
        m_pRes = m_pStmt->executeQuery();
        while ( m_pRes->next())
        {
            RiskPlan plan;
            memset(&plan, 0, sizeof(plan));

            plan.OrgIDPlanRelation.nOrgID		= m_pRes->getInt(1);
            plan.OrgIDPlanRelation.nRiskIndicatorID = (RiskIndicatorType)(m_pRes->getInt(2));
            plan.OrgIDPlanRelation.bAllProduct      = m_pRes->getInt(3)==0?false:true;
            std::string strValue = m_pRes->getString(4);
            strcpy(plan.OrgIDPlanRelation.ProductID, strValue.c_str());
            plan.OrgIDPlanRelation.bUse       = m_pRes->getInt(5)==0?false:true;

            plan.WaringLevel.nOrgID		= m_pRes->getInt(1);
            plan.WaringLevel.nRiskIndicatorID = (RiskIndicatorType)(m_pRes->getInt(2));
            plan.WaringLevel.nRiskLevelID  = m_pRes->getInt(7);
            plan.WaringLevel.nResponseType = (RiskWarningType)(m_pRes->getInt(9)); 
            plan.WaringLevel.fThresholdValue  = (float)(m_pRes->getDouble(8));
            plan.WaringLevel.nColor			  = m_pRes->getInt(10);

            if(plan.OrgIDPlanRelation.nRiskIndicatorID == RI_FundNetValue)
            {
                plan.netFundParam.nOrgID = plan.OrgIDPlanRelation.nOrgID;
                plan.netFundParam.dInnerVolumn = (float)(m_pRes->getDouble(11));
                plan.netFundParam.dOuterVolumn = (float)(m_pRes->getDouble(12));
                plan.netFundParam.dOuterNetAsset = (float)(m_pRes->getDouble(13));
                plan.netFundParam.nInnerNetAssetOption = m_pRes->getInt(14);
            }
			plan.OrgIDPlanRelation.nRiskType = m_pRes->getInt(15);

			strValue = m_pRes->getString(16);
			strcpy(plan.OrgIDPlanRelation.cTimeBegin, strValue.c_str());	
			strcpy(plan.WaringLevel.cTimeBegin, strValue.c_str());		
			strValue = m_pRes->getString(17);
			strcpy(plan.OrgIDPlanRelation.cTimeEnd, strValue.c_str());
			strcpy(plan.WaringLevel.cTimeEnd, strValue.c_str());		
			strValue = m_pRes->getString(18);
			strcpy(plan.OrgIDPlanRelation.cInstruments, strValue.c_str());

			plan.OrgIDPlanRelation.dbMarginDividDynamic = (float)(m_pRes->getDouble(19));
			plan.OrgIDPlanRelation.dbMarginUse = (float)(m_pRes->getDouble(20));
			plan.OrgIDPlanRelation.dbLossAmount = (float)(m_pRes->getDouble(21));
			plan.OrgIDPlanRelation.dbLossPercent = (float)(m_pRes->getDouble(22));
			
			strValue = m_pRes->getString(23);
			strcpy(plan.OrgIDPlanRelation.szPlan, strValue.c_str());

            vecRiskPlan.push_back(plan);	
        }
        m_pStmt->closeResultSet(m_pRes);
        m_pCon->terminateStatement(m_pStmt);
    }
    catch(oracle::occi::SQLException &e)
    {
        std::cout<<e.what()<<endl;
        std::cout<<pSql<<endl;
        nErrorCode = GetErrorCode(e.what());
        WriteLog(pSql, e.what());
        return false;
    }

    return true;

}
bool CSvrDBOprImpl::QueryData( const char* pSql, std::vector<EventMessageTemplate>& vec )
{
    CGuard guard(&g_mutex);
    if ( NULL == pSql )
    {
        return false;
    }

    if ( !IsConnected() && !Conncect())
    {
        return false;
    }

    try
    {
        m_pStmt = m_pCon->createStatement( pSql );
        m_pRes = m_pStmt->executeQuery();
        while ( m_pRes->next())
        {
            EventMessageTemplate info;
            memset(&info, 0, sizeof(info));

            info.nRiskIndicatorID = (RiskIndicatorType)m_pRes->getInt(1);
            info.nRiskLevelID = m_pRes->getInt(2);
            string strValue = m_pRes->getString(3);
            strcpy(info.szTitle, strValue.c_str());
            strValue = m_pRes->getString(4);
            strcpy(info.szContent, strValue.c_str());

            vec.push_back(info);
        }

        m_pStmt->closeResultSet(m_pRes);
        m_pCon->terminateStatement(m_pStmt);
    }catch(oracle::occi::SQLException &e){
        std::cout<<e.what()<<endl;
        std::cout<<pSql<<endl;

        WriteLog(pSql, e.what());
        return false;
    }

    return true;
}
bool CSvrDBOprImpl::SaveMsgTemplate( int nIndicatorID, std::vector<EventMessageTemplate>& vec )
{
    CGuard guard(&g_mutex);
    if ( !IsConnected() && !Conncect())
    {
        return false;
    }

    char szBuffer[MAX_SQL_LENGTH];

    try
    {
        memset(szBuffer, 0, sizeof(szBuffer));
        sprintf(szBuffer, "delete RISK_EVENTMESSAGETEMPLATE t where t.riskindicatorid = %d", nIndicatorID);

        m_pStmt = m_pCon->createStatement();
        m_pStmt->execute(szBuffer);

        std::vector<EventMessageTemplate>::iterator it = vec.begin();
        for ( ; it != vec.end(); it++ )
        {
            memset(szBuffer, 0, sizeof(szBuffer));
            sprintf(szBuffer, "insert into RISK_EVENTMESSAGETEMPLATE values(%d, %d,'%s','%s')",
                nIndicatorID, (*it).nRiskLevelID, (*it).szTitle, (*it).szContent);
            m_pStmt->execute( szBuffer );
        }

        m_pCon->commit();
        m_pCon->terminateStatement(m_pStmt);
    }catch(oracle::occi::SQLException &e){
        RollBack();
        std::cout<<e.what()<<endl;
        std::cout<<szBuffer<<endl;

        WriteLog(szBuffer, e.what());
        return false;
    }

    return true;
}
bool CSvrDBOprImpl::QueryData( const char* pSql, std::vector<NetFundParam>& vec )
{
    CGuard guard(&g_mutex);
    if ( NULL == pSql )
    {
        return false;
    }

    if ( !IsConnected() && !Conncect())
    {
        return false;
    }

    try
    {
        m_pStmt = m_pCon->createStatement( pSql );
        m_pRes = m_pStmt->executeQuery();
        while ( m_pRes->next())
        {
            NetFundParam info;
            memset(&info, 0, sizeof(info));

            info.nOrgID = m_pRes->getInt(1);
            info.dInnerVolumn = m_pRes->getDouble(2);
            info.dOuterVolumn = m_pRes->getDouble(3);
            info.dOuterNetAsset = m_pRes->getDouble(4);
            info.nInnerNetAssetOption = m_pRes->getInt(5);

            vec.push_back(info);
        }

        m_pStmt->closeResultSet(m_pRes);
        m_pCon->terminateStatement(m_pStmt);
    }catch(oracle::occi::SQLException &e){
        std::cout<<e.what()<<endl;
        std::cout<<pSql<<endl;

        WriteLog(pSql, e.what());
        return false;
    }

    return true;
}
bool CSvrDBOprImpl::SaveRiskEvent( std::string strTime, std::vector<RiskEvent>& vRiskEvent, int& nErrorCode, bool deleteToday/* = true*/ )
{
    CGuard guard(&g_mutex);
    nErrorCode = CF_ERROR_SUCCESS;
    if ( !IsConnected() && !Conncect())
    {
        nErrorCode = CF_ERROR_DATABASE_OTHER_ERROR;
        return false;
    }

    char szBuffer[MAX_SQL_LENGTH];
    memset(szBuffer, 0, sizeof(szBuffer));

    int* RiskEventID = new int[nMaxInsertRow];
    int* RiskEventSubID = new int[nMaxInsertRow];
    INT64* EventTime = new INT64[nMaxInsertRow];
    int* TradeAccountID = new int[nMaxInsertRow];	
    int* RiskIndicatorID = new int[nMaxInsertRow];
    int* RiskLevelID = new int[nMaxInsertRow];
    double* IndicatorValue = new double[nMaxInsertRow];
    int* MsgSendStatus = new int[nMaxInsertRow];
    int* IsValid = new int[nMaxInsertRow];
    InstrumentIDType* InstrumentID = new InstrumentIDType[nMaxInsertRow];
	RiskTime* TimeBegin = new RiskTime[nMaxInsertRow];
	RiskTime* TimeEnd   = new RiskTime[nMaxInsertRow];
	BrokerIDType* BrokerID = new BrokerIDType[nMaxInsertRow];
	double* IndicatorCurrentValue = new double[nMaxInsertRow];
	int* ResponseType = new int[nMaxInsertRow];
	double* IndicatorValue2 = new double[nMaxInsertRow];
	double* IndicatorCurrentValue2 = new double[nMaxInsertRow];

    bool bRet = true;
    try
    {
        m_pStmt = m_pCon->createStatement();
        if(deleteToday)
        {
            //先删除今天已存储的记录
            sprintf(szBuffer, "delete from RISK_RISKEVENT t where \
                              to_char(to_date('1970-01-01','yyyy-MM-dd')+t.eventtime/86400,'YYYYMMDD') = '%s'",
                              strTime.c_str());
            m_pStmt->execute( szBuffer );
        }

        //批量存储，每次最多存储10000条
        memset(szBuffer, 0, sizeof(szBuffer));
        sprintf(szBuffer, "insert into RISK_RISKEVENT values(:v1,:v2,:v3,:v4,:v5,:v6, \
                          :v7,:v8,:v9,:v10)");
        m_pStmt->setSQL(szBuffer);
        int nCount = vRiskEvent.size() / nMaxInsertRow;
        if ( vRiskEvent.size() % nMaxInsertRow != 0 )
        {
            nCount++;
        }

        for ( int i = 0; i < nCount; i++ )
        {
            ub2 len_int[nMaxInsertRow];
            ub2 len_double[nMaxInsertRow];
            ub2 len_int64[nMaxInsertRow];
            ub2 len_InstrumentID[nMaxInsertRow];
			ub2 len_TimeBegin[nMaxInsertRow];
			ub2 len_TimeEnd[nMaxInsertRow];
			ub2 len_BrokerID[nMaxInsertRow];


            int nRecordNum = 0;
            for ( int j = i*nMaxInsertRow; j < static_cast<int>(vRiskEvent.size()) && nRecordNum < nMaxInsertRow; j++)
            {
                RiskEvent info = vRiskEvent[j];
                len_int[nRecordNum] = sizeof(int);
                len_int64[nRecordNum] = sizeof(INT64);
                len_double[nRecordNum] = sizeof(double);

                RiskEventID[nRecordNum] = info.nRiskEventID;
                RiskEventSubID[nRecordNum] = info.nRiskEventSubID;
                EventTime[nRecordNum] = info.lEventTime;
                TradeAccountID[nRecordNum] = info.nTradeInvestorID;
                RiskIndicatorID[nRecordNum] = info.nRiskIndicatorID;
                RiskLevelID[nRecordNum] = info.nRiskLevelID;
                IndicatorValue[nRecordNum] = info.dblIndicatorValue;
                MsgSendStatus[nRecordNum] = info.nMsgSendStatus;
                IsValid[nRecordNum] = info.nIsValid;
                strcpy(InstrumentID[nRecordNum], info.InstrumentID);
                len_InstrumentID[nRecordNum] = strlen(InstrumentID[nRecordNum]) + 1;

				strcpy(TimeBegin[nRecordNum], info.cTimeBegin);
				len_TimeBegin[nRecordNum] = strlen(TimeBegin[nRecordNum]) + 1;			
				strcpy(TimeEnd[nRecordNum], info.cTimeEnd);
				len_TimeEnd[nRecordNum] = strlen(TimeEnd[nRecordNum]) + 1;
				strcpy(BrokerID[nRecordNum], info.BrokerID);
				len_BrokerID[nRecordNum] = strlen(BrokerID[nRecordNum]) + 1;
				IndicatorCurrentValue[nRecordNum] = info.dblIndicatorCurrentValue;
				ResponseType[nRecordNum] = info.nResponseType;				
				IndicatorValue2[nRecordNum] = info.dblIndicatorValue2;
				IndicatorCurrentValue2[nRecordNum] = info.dblIndicatorCurrentValue2;

                nRecordNum++;
            }

            m_pStmt->setDataBuffer(1, RiskEventID, OCCIINT, sizeof(int), len_int); 
            m_pStmt->setDataBuffer(2, RiskEventSubID, OCCIINT, sizeof(int), len_int); 
            m_pStmt->setDataBuffer(3, TradeAccountID, OCCIINT, sizeof(int), len_int);
            m_pStmt->setDataBuffer(4, EventTime, OCCIINT, sizeof(INT64), len_int64); 			
            m_pStmt->setDataBuffer(5, RiskIndicatorID, OCCIINT, sizeof(int), len_int); 
            m_pStmt->setDataBuffer(6, RiskLevelID, OCCIINT, sizeof(int), len_int); 
            m_pStmt->setDataBuffer(7, IndicatorValue, OCCIFLOAT, sizeof(double), len_double); 
            m_pStmt->setDataBuffer(8, MsgSendStatus, OCCIINT, sizeof(int), len_int); 
            m_pStmt->setDataBuffer(9, IsValid, OCCIINT, sizeof(int), len_int); 
            m_pStmt->setDataBuffer(10, InstrumentID, OCCI_SQLT_STR, sizeof(InstrumentIDType), len_InstrumentID); 
			m_pStmt->setDataBuffer(11, TimeBegin, OCCI_SQLT_STR, 6, len_TimeBegin); 
			m_pStmt->setDataBuffer(12, TimeEnd, OCCI_SQLT_STR, 6, len_TimeEnd); 
			m_pStmt->setDataBuffer(13, BrokerID, OCCI_SQLT_STR, sizeof(BrokerIDType), len_BrokerID); 
			m_pStmt->setDataBuffer(14, IndicatorCurrentValue, OCCIFLOAT, sizeof(double), len_double); 
			m_pStmt->setDataBuffer(15, ResponseType, OCCIINT, sizeof(int), len_int); 
			m_pStmt->setDataBuffer(16, IndicatorValue2, OCCIFLOAT, sizeof(double), len_double); 
			m_pStmt->setDataBuffer(17, IndicatorCurrentValue2, OCCIFLOAT, sizeof(double), len_double); 



            m_pStmt->executeArrayUpdate(nRecordNum); 
        }

        m_pCon->commit();
        m_pCon->terminateStatement(m_pStmt);
    }catch(oracle::occi::SQLException &e){
        RollBack();
        std::cout<<e.what()<<endl;
        std::cout<<szBuffer<<endl;
        nErrorCode = GetErrorCode(e.what());
        WriteLog(szBuffer, e.what());
        bRet = false;
    }

    delete [] RiskEventID;
    delete [] RiskEventSubID;
    delete [] EventTime;
    delete [] TradeAccountID;
    delete [] RiskIndicatorID;
    delete [] RiskLevelID;
    delete [] IndicatorValue;
    delete [] MsgSendStatus;
    delete [] IsValid;
    delete [] InstrumentID;
	delete [] TimeBegin;
	delete [] TimeEnd;
	delete [] BrokerID;
	delete [] IndicatorCurrentValue;
	delete [] ResponseType;
	delete [] IndicatorValue2;
	delete [] IndicatorCurrentValue2;
    return bRet;
}
bool CSvrDBOprImpl::SaveNetFundCalcResult( std::string strTime, std::vector<NetFundCalcResult>& vResult , int& nErrorCode)
{
	nErrorCode = CF_ERROR_SUCCESS;
	if ( !IsConnected() && !Conncect())
	{
		nErrorCode = CF_ERROR_DATABASE_OTHER_ERROR;
		return false;
	}
    CGuard guard(&g_mutex);
    if ( !IsConnected() && !Conncect())
    {
        return false;
    }

    char szBuffer[MAX_SQL_LENGTH];
    memset(szBuffer, 0, sizeof(szBuffer));

    int* TradeAccountID = new int[nMaxInsertRow];
    double* InnerVolumn = new double[nMaxInsertRow];
    double* OuterVolumn = new double[nMaxInsertRow];
    double* InnerNetAsset = new double[nMaxInsertRow];
    double* OuterNetAsset = new double[nMaxInsertRow];
    double* InnerPerNet = new double[nMaxInsertRow];
    double* OuterPerNet = new double[nMaxInsertRow];
    double* TotalNetAsset = new double[nMaxInsertRow];

    bool bRet = true;
    try
    {
        m_pStmt = m_pCon->createStatement();
        //先删除今天已存储的记录
        sprintf(szBuffer, "delete from RISK_FUNDNETCALCRESULT t where \
                          to_char(t.updatedate,'YYYY-MM-DD') = '%s'",
                          strTime.c_str());
        m_pStmt->execute( szBuffer );

        //批量存储，每次最多存储10000条
        memset(szBuffer, 0, sizeof(szBuffer));
        sprintf(szBuffer, "insert into RISK_FUNDNETCALCRESULT values(:v1,:v2,:v3,:v4,:v5,:v6, \
                          :v7,:v8,sysdate)");
        m_pStmt->setSQL(szBuffer);
        int nCount = vResult.size() / nMaxInsertRow;
        if ( vResult.size() % nMaxInsertRow != 0 )
        {
            nCount++;
        }

        for ( int i = 0; i < nCount; i++ )
        {
            ub2 len_int[nMaxInsertRow];
            ub2 len_double[nMaxInsertRow];

            int nRecordNum = 0;
            for ( int j = i*nMaxInsertRow; j < static_cast<int>(vResult.size()) && nRecordNum < nMaxInsertRow; j++)
            {
                NetFundCalcResult info = vResult[j];
                len_int[nRecordNum] = sizeof(int);
                len_double[nRecordNum] = sizeof(double);

                TradeAccountID[nRecordNum] = info.nTradeAccountID;
                InnerVolumn[nRecordNum] = info.dInnerVolumn;
                OuterVolumn[nRecordNum] = info.dOuterVolumn;
                InnerNetAsset[nRecordNum] = info.dInnerNetAsset;
                OuterNetAsset[nRecordNum] = info.dOuterNetAsset;
                InnerPerNet[nRecordNum] = info.dInnerPerNet;
                OuterPerNet[nRecordNum] = info.dOuterPerNet;
                TotalNetAsset[nRecordNum] = info.dTotalNetAsset;

                nRecordNum++;
            }

            m_pStmt->setDataBuffer(1, TradeAccountID, OCCIINT, sizeof(int), len_int); 
            m_pStmt->setDataBuffer(2, InnerVolumn, OCCIFLOAT, sizeof(double), len_double); 
            m_pStmt->setDataBuffer(3, OuterVolumn, OCCIFLOAT, sizeof(double), len_double);
            m_pStmt->setDataBuffer(4, InnerNetAsset, OCCIFLOAT, sizeof(double), len_double); 
            m_pStmt->setDataBuffer(5, OuterNetAsset, OCCIFLOAT, sizeof(double), len_double); 
            m_pStmt->setDataBuffer(6, InnerPerNet, OCCIFLOAT, sizeof(double), len_double); 
            m_pStmt->setDataBuffer(7, OuterPerNet, OCCIFLOAT, sizeof(double), len_double); 
            m_pStmt->setDataBuffer(8, TotalNetAsset, OCCIFLOAT, sizeof(double), len_double); 

            m_pStmt->executeArrayUpdate(nRecordNum); 
        }

        m_pCon->commit();
        m_pCon->terminateStatement(m_pStmt);
    }catch(oracle::occi::SQLException &e){
        RollBack();
        std::cout<<e.what()<<endl;
        std::cout<<szBuffer<<endl;
        nErrorCode = GetErrorCode(e.what());
        WriteLog(szBuffer,  e.what());
        bRet = false;
    }

    delete [] TradeAccountID;
    delete [] InnerVolumn;
    delete [] OuterVolumn;
    delete [] InnerNetAsset;
    delete [] OuterNetAsset;
    delete [] InnerPerNet;
    delete [] OuterPerNet;
    delete [] TotalNetAsset;

    return bRet;
}

//////////////////////////////////////////////////////////////////////////
//写初始资金
bool CSvrDBOprImpl::SaveInitFund(const std::string & nsTableName,
                                 const std::string & nsUserName,
                                 const double & ndbVal) 
{


    CGuard guard(&g_mutex);
    if ( !IsConnected() && !Conncect())
    {
        return false;
    }

    char szBuffer[MAX_SQL_LENGTH];

    try
    {
        m_pStmt = m_pCon->createStatement();

        memset(szBuffer, 0, sizeof(szBuffer));
        sprintf(szBuffer, "delete from %s where userid = \'%s\'",
            nsTableName.c_str(),nsUserName.c_str(),ndbVal);
        m_pStmt->execute( szBuffer );

        memset(szBuffer, 0, sizeof(szBuffer));
        sprintf(szBuffer, "insert into %s values(\'%s\',%f,sysdate)",
            nsTableName.c_str(),nsUserName.c_str(),ndbVal);
        m_pStmt->execute( szBuffer );


        m_pCon->commit();
        m_pCon->terminateStatement(m_pStmt);
    }catch(oracle::occi::SQLException &e){
        RollBack();
        std::cout<<e.what()<<endl;
        std::cout<<szBuffer<<endl;

        WriteLog(szBuffer,  e.what());
        return false;
    }

    return true;

}
bool CSvrDBOprImpl::QueryData( const char* pSql, std::vector<RiskIndicator>& vec, int& nErrorCode  )
{
    CGuard guard(&g_mutex);
    nErrorCode = CF_ERROR_SUCCESS;
    if ( NULL == pSql )
    {
        nErrorCode = CF_ERROR_DATABASE_OTHER_ERROR;
        return false;
    }

    if ( !IsConnected() && !Conncect())
    {
        nErrorCode = CF_ERROR_DATABASE_OTHER_ERROR;
        return false;
    }

    try
    {
        m_pStmt = m_pCon->createStatement( pSql );
        m_pRes = m_pStmt->executeQuery();
        while ( m_pRes->next())
        {
            RiskIndicator info;
            memset(&info, 0, sizeof(info));

            info.nRiskIndicatorID = (RiskIndicatorType)m_pRes->getInt(1);
            string strValue = m_pRes->getString(2);
            strcpy(info.szName, strValue.c_str());
            strValue = m_pRes->getString(3);
            strcpy(info.szAlgorithmDesc, strValue.c_str());

			strValue = m_pRes->getString(4);
			strcpy(info.szTypeName, strValue.c_str());
            vec.push_back(info);
        }

        m_pStmt->closeResultSet(m_pRes);
        m_pCon->terminateStatement(m_pStmt);
    }catch(oracle::occi::SQLException &e)
    {
        std::cout<<e.what()<<endl;
        std::cout<<pSql<<endl;
        nErrorCode = GetErrorCode(e.what());
        WriteLog(pSql,  e.what());
        return false;
    }

    return true;
}
bool CSvrDBOprImpl::QueryData( const char* pSql, std::vector<RiskResponse>& vec, int& nErrorCode )
{
    CGuard guard(&g_mutex);
    nErrorCode = CF_ERROR_SUCCESS;
    if ( NULL == pSql )
    {
        nErrorCode = CF_ERROR_DATABASE_OTHER_ERROR;
        return false;
    }

    if ( !IsConnected() && !Conncect())
    {
        nErrorCode = CF_ERROR_DATABASE_OTHER_ERROR;
        return false;
    }

    try
    {
        m_pStmt = m_pCon->createStatement( pSql );
        m_pRes = m_pStmt->executeQuery();
        while ( m_pRes->next())
        {
            RiskResponse info;
            memset(&info, 0, sizeof(info));

            info.nResponseType = (RiskWarningType)m_pRes->getInt(1);
            string strValue = m_pRes->getString(2);
            strcpy(info.szName, strValue.c_str());
            strValue = m_pRes->getString(3);
            strcpy(info.szDesc, strValue.c_str());

            vec.push_back(info);
        }

        m_pStmt->closeResultSet(m_pRes);
        m_pCon->terminateStatement(m_pStmt);
    }catch(oracle::occi::SQLException &e)
    {
        std::cout<<e.what()<<endl;
        std::cout<<pSql<<endl;
        nErrorCode = GetErrorCode(e.what());
        WriteLog(pSql,  e.what());
        return false;
    }

    return true;
}
bool CSvrDBOprImpl::QueryData( const char* pSql, std::vector<RiskEvent>& vec, int& nErrorCode )
{
    CGuard guard(&g_mutex);
    nErrorCode = CF_ERROR_SUCCESS;
    if ( NULL == pSql )
    {
        nErrorCode = CF_ERROR_DATABASE_OTHER_ERROR;
        return false;
    }

    if ( !IsConnected() && !Conncect())
    {
        nErrorCode = CF_ERROR_DATABASE_OTHER_ERROR;
        return false;
    }

    try
    {
        m_pStmt = m_pCon->createStatement( pSql );
        m_pRes = m_pStmt->executeQuery();
        while ( m_pRes->next())
        {
            RiskEvent info;
            memset(&info, 0, sizeof(info));

            info.nRiskEventID = m_pRes->getInt(1);	
            info.nRiskEventSubID = m_pRes->getInt(2);
            info.nTradeInvestorID = m_pRes->getInt(3);
            info.lEventTime = m_pRes->getInt(4);
            info.nRiskIndicatorID = (RiskIndicatorType)m_pRes->getInt(5);
            info.nRiskLevelID = m_pRes->getInt(6);
            info.dblIndicatorValue = m_pRes->getDouble(7);
            info.nMsgSendStatus = (MsgStatusType)m_pRes->getInt(8);
            info.nIsValid = (RiskEventType)m_pRes->getInt(9);
            string strValue = m_pRes->getString(10);
            strcpy(info.InstrumentID, strValue.c_str());

			string strTime = m_pRes->getString(11);
			strcpy(info.cTimeBegin, strTime.c_str());
			strTime = m_pRes->getString(12);
			strcpy(info.cTimeEnd, strTime.c_str());			
			string BROKERID = m_pRes->getString(13);
			strcpy(info.BrokerID, BROKERID.c_str());
			info.dblIndicatorCurrentValue = m_pRes->getDouble(14);
			info.nResponseType = m_pRes->getInt(15);			
			info.dblIndicatorValue2 = m_pRes->getDouble(16);
			info.dblIndicatorCurrentValue2 = m_pRes->getDouble(17);

            vec.push_back(info);
        }

        m_pStmt->closeResultSet(m_pRes);
        m_pCon->terminateStatement(m_pStmt);
    }catch(oracle::occi::SQLException &e){
        std::cout<<e.what()<<endl;
        std::cout<<pSql<<endl;
        nErrorCode = GetErrorCode(e.what());
        WriteLog(pSql,  e.what());
        return false;
    }

    return true;

}
bool CSvrDBOprImpl::AddMessage( const MessageInfo& msgInfo, const std::vector<TargetAccount> vAccount, int& nPKID, int& nErrorCode )
{
    CGuard guard(&g_mutex);
    nErrorCode = CF_ERROR_SUCCESS;
    if ( !IsConnected() && !Conncect())
    {
        nErrorCode = CF_ERROR_DATABASE_OTHER_ERROR;
        return false;
    }

    char szBuffer[MAX_SQL_LENGTH];

    try
    {
        memset(szBuffer, 0, sizeof(szBuffer));
        char szOwner[50];
        memset(szOwner, 0, sizeof(szOwner));
        if ( msgInfo.nOwner == 0 )
        {
            strcpy(szOwner, "NULL");
        }
        else
        {
            sprintf(szOwner, "%d", msgInfo.nOwner);
        }
        //先建立id自增 create sequence SEQ_MESSAGEID   minvalue 1 maxvalue 99999999 start with 1 increment by 1 nocache;

        sprintf(szBuffer, "insert into RISK_MESSAGEINFO values(SEQ_MESSAGEID.NEXTVAL, \
                          '%s','%s', to_date('%s','YYYY-MM-DD'), %s, sysdate,'%s')", 
                          msgInfo.szTitle, msgInfo.szContent, msgInfo.szExpiredDate, 
                          szOwner, msgInfo.szOwner);

        m_pStmt = m_pCon->createStatement();
        m_pStmt->execute(szBuffer);

        m_pRes = m_pStmt->executeQuery( "select SEQ_MESSAGEID.currval from dual" );
        if ( m_pRes->next())
        {
            nPKID = m_pRes->getInt(1);
        }
        m_pStmt->closeResultSet(m_pRes);

        m_pCon->commit();
        m_pCon->terminateStatement(m_pStmt);
    }catch(oracle::occi::SQLException &e){
        RollBack();
        std::cout<<e.what()<<endl;
        std::cout<<szBuffer<<endl;
        nErrorCode = GetErrorCode(e.what());
        WriteLog(szBuffer,  e.what());
        return false;
    }

    return true;
}
bool CSvrDBOprImpl::QueryData( const char* pSql, std::vector<MessageInfo>& vec, int& nErrorCode  )
{
    CGuard guard(&g_mutex);
    nErrorCode = CF_ERROR_SUCCESS;
    if ( NULL == pSql )
    {
        nErrorCode = CF_ERROR_DATABASE_OTHER_ERROR;
        return false;
    }

    if ( !IsConnected() && !Conncect())
    {
        nErrorCode = CF_ERROR_DATABASE_OTHER_ERROR;
        return false;
    }

    try
    {
        m_pStmt = m_pCon->createStatement( pSql );
        m_pRes = m_pStmt->executeQuery();
        while ( m_pRes->next())
        {
            MessageInfo info;
            memset(&info, 0, sizeof(info));

            info.nMessageID = m_pRes->getInt(1);
            string strValue = m_pRes->getString(2);
            strcpy(info.szTitle, strValue.c_str());
            strValue = m_pRes->getString(3);
            strcpy(info.szContent, strValue.c_str());
            strValue = m_pRes->getString(4);
            strcpy(info.szExpiredDate, strValue.c_str());
            info.nOwner = m_pRes->getInt(5);
            strValue = m_pRes->getString(6);
            strcpy(info.szCreateDate, strValue.c_str());
            strValue = m_pRes->getString(7);
            strcpy(info.szOwner, strValue.c_str());

            vec.push_back(info);
        }

        m_pStmt->closeResultSet(m_pRes);
        m_pCon->terminateStatement(m_pStmt);
    }catch(oracle::occi::SQLException &e){
        std::cout<<e.what()<<endl;
        std::cout<<pSql<<endl;
        nErrorCode = GetErrorCode(e.what());
        WriteLog(pSql,  e.what());
        return false;
    }

    return true;
}
bool CSvrDBOprImpl::QueryData( const char* pSql, std::vector<MsgSendStatus>& vec, int& nErrorCode )
{
    CGuard guard(&g_mutex);
    nErrorCode = CF_ERROR_SUCCESS;
    if ( NULL == pSql )
    {
        nErrorCode = CF_ERROR_DATABASE_OTHER_ERROR;
        return false;
    }

    if ( !IsConnected() && !Conncect())
    {
        nErrorCode = CF_ERROR_DATABASE_OTHER_ERROR;
        return false;
    }

    try
    {
        m_pStmt = m_pCon->createStatement( pSql );
        m_pRes = m_pStmt->executeQuery();
        while ( m_pRes->next())
        {
            MsgSendStatus info;
            memset(&info, 0, sizeof(info));
            info.nRiskMgmtUserID = m_pRes->getInt(1);
            info.nMessageID = m_pRes->getInt(2);
            std::string strValue = m_pRes->getString(3);
            strcpy(info.szAccount, strValue.c_str());
            info.nSendStatus = m_pRes->getInt(4);

            vec.push_back(info);
        }

        m_pStmt->closeResultSet(m_pRes);
        m_pCon->terminateStatement(m_pStmt);
    }catch(oracle::occi::SQLException &e){
        std::cout<<e.what()<<endl;
        std::cout<<pSql<<endl;
        nErrorCode = GetErrorCode(e.what());
        WriteLog(pSql,  e.what());
        return false;
    }

    return true;
}
bool CSvrDBOprImpl::QueryData( const char* pSql, std::vector<NetFundCalcResult>& vec, int& nErrorCode )
{
    CGuard guard(&g_mutex);
    nErrorCode = CF_ERROR_SUCCESS;
    if ( NULL == pSql )
    {
        nErrorCode = CF_ERROR_DATABASE_OTHER_ERROR;
        return false;
    }

    if ( !IsConnected() && !Conncect())
    {
        nErrorCode = CF_ERROR_DATABASE_OTHER_ERROR;
        return false;
    }

    try
    {
        m_pStmt = m_pCon->createStatement( pSql );
        m_pRes = m_pStmt->executeQuery();
        while ( m_pRes->next())
        {
            NetFundCalcResult info;
            memset(&info, 0, sizeof(info));

            info.nTradeAccountID = m_pRes->getInt(1);
            info.dInnerVolumn = m_pRes->getDouble(2);
            info.dOuterVolumn = m_pRes->getDouble(3);
            info.dInnerNetAsset = m_pRes->getDouble(4);
            info.dOuterNetAsset = m_pRes->getDouble(5);
            info.dInnerPerNet = m_pRes->getDouble(6);
            info.dOuterPerNet = m_pRes->getDouble(7);
            info.dTotalNetAsset = m_pRes->getDouble(8);
            string strValue = m_pRes->getString(9);
            strcpy(info.szUpdateDate, strValue.c_str());

            vec.push_back(info);
        }

        m_pStmt->closeResultSet(m_pRes);
        m_pCon->terminateStatement(m_pStmt);
    }catch(oracle::occi::SQLException &e){
        std::cout<<e.what()<<endl;
        std::cout<<pSql<<endl;
        nErrorCode = GetErrorCode(e.what());
        WriteLog(pSql,  e.what());
        return false;
    }

    return true;
}
bool CSvrDBOprImpl::SaveForceCloseOrder(PlatformStru_InputOrder& order, InputOrderKey& lKey, bool bForceCloseType, std::string strRiskName, int& nErrorCode)
{	
    CGuard guard(&g_mutex);
    nErrorCode = CF_ERROR_SUCCESS;
    if ( !IsConnected() && !Conncect())
    {
        nErrorCode = CF_ERROR_DATABASE_OTHER_ERROR;
        return false;
    }

    char szBuffer[MAX_SQL_LENGTH];

    try
    {
        m_pStmt = m_pCon->createStatement();


        sprintf(szBuffer, "insert into RISK_FORCECLOSE_ORDER values('%s', '%s', '%s', '%s', '%s', '%c', '%c', '%s',  '%s'\
                          , %f, %d, '%c', '%s', '%c', %d, '%c',%f, '%c', %d, '%s', %d, %d, '%c', '%s', '%s', '%s',\
                          '%s', %d, %d, '%s', %d, '%s', sysdate)",
                          order.BrokerID, order.InvestorID, order.InstrumentID, order.OrderRef, order.UserID,
                          order.OrderPriceType, order.Direction, order.CombOffsetFlag, order.CombHedgeFlag,
                          order.LimitPrice, order.VolumeTotalOriginal, order.TimeCondition, order.GTDDate,
                          order.VolumeCondition, order.MinVolume, order.ContingentCondition, order.StopPrice,
                          order.ForceCloseReason, order.IsAutoSuspend, order.BusinessUnit, order.RequestID,
                          order.UserForceClose, order.OrderClass, order.strAccount, order.strLocalRequestID,
                          order.strLocalRefID, order.strExchange, lKey.nFrontID, lKey.nSessionID, lKey.szOrderRef,
                          bForceCloseType, strRiskName.c_str()
                          );
        m_pStmt->execute( szBuffer );

        m_pCon->commit();
        m_pCon->terminateStatement(m_pStmt);
    }catch(oracle::occi::SQLException &e)
    {
        RollBack();
        std::cout<<e.what()<<endl;
        std::cout<<szBuffer<<endl;
        nErrorCode = GetErrorCode(e.what());
        WriteLog(szBuffer, e.what());
        return false;
    }

    return true;	
}
bool CSvrDBOprImpl::SaveForceCloseOrderAction(PlatformStru_InputOrderAction& order, bool bForceCloseType, std::string strRiskName, int& nErrorCode)
{
    CGuard guard(&g_mutex);
    nErrorCode = CF_ERROR_SUCCESS;
    if ( !IsConnected() && !Conncect())
    {
        nErrorCode = CF_ERROR_DATABASE_OTHER_ERROR;
        return false;
    }

    char szBuffer[MAX_SQL_LENGTH];

    try
    {
        m_pStmt = m_pCon->createStatement();

        sprintf(szBuffer, "insert into RISK_FORCECLOSE_ORDERACTION values('%s', '%s', %d, '%s', %d, %d, %d, '%s', '%s', '%c', %f, %d, '%s', '%s', sysdate, '%s',  %d)",
            order.Thost.BrokerID, order.Thost.InvestorID, order.Thost.OrderActionRef,
            order.Thost.OrderRef, order.Thost.RequestID, order.Thost.FrontID, order.Thost.SessionID,
            order.Thost.ExchangeID, order.Thost.OrderSysID, order.Thost.ActionFlag, order.Thost.LimitPrice,
            order.Thost.VolumeChange, order.Thost.UserID, order.Thost.InstrumentID, strRiskName.c_str(), bForceCloseType
            );
        m_pStmt->execute( szBuffer );

        m_pCon->commit();
        m_pCon->terminateStatement(m_pStmt);
    }catch(oracle::occi::SQLException &e)
    {
        RollBack();
        std::cout<<e.what()<<endl;
        std::cout<<szBuffer<<endl;
        nErrorCode = GetErrorCode(e.what());
        WriteLog(szBuffer, e.what());
        return false;
    }

    return true;
}

//基础数据操作入库
bool CSvrDBOprImpl::WriteLogToDB( const LogBaseData& logData, int& nErrorCode )
{
    CGuard guard(&g_mutex);
    nErrorCode = CF_ERROR_SUCCESS;
    if ( !IsConnected() && !Conncect())
    {
        nErrorCode = CF_ERROR_DATABASE_OTHER_ERROR;
        return false;
    }

    char szBuffer[MAX_SQL_LENGTH];

    try
    {
        m_pStmt = m_pCon->createStatement();

        sprintf(szBuffer, "insert into %s values('%s', %d, %d, %d, '%s', %d)",
            logData.logTable,logData.logDateTime,logData.logOperatorID,logData.logTargetorID,logData.logAction,logData.logContent,logData.logResult);
        m_pStmt->execute( szBuffer );

        m_pCon->commit();
        m_pCon->terminateStatement(m_pStmt);
    }catch(oracle::occi::SQLException &e)
    {
        RollBack();
        std::cout<<e.what()<<endl;
        std::cout<<szBuffer<<endl;
        nErrorCode = GetErrorCode(e.what());
        WriteLog(szBuffer, e.what());
        return false;
    }

    return true;
}



//报单
bool CSvrDBOprImpl::SaveAccountOrderInfos(const std::string& nsTableName,
									   bool nbDelete,
									   const std::string& strTime,
									   const std::string& nsBrokerID,
									   const std::string& nsAccountID,
									   const std::vector<PlatformStru_OrderInfo>& nOrders ,
									   int& nErrorCode )
{
	//return MergeOrderInfos(nsTableName,strTime,nOrders,nErrorCode);

	//先删除今天该账户所有的，然后再添加
	PlatformStru_OrderInfo lInfo;
	std::vector<ColumeData> ltest;

	PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcBrokerIDType),(int)&lInfo.BrokerID - (int)&lInfo); //BrokerID
	PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcInvestorIDType),(int)&lInfo.InvestorID - (int)&lInfo); //InvestorID
	PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcInstrumentIDType),(int)&lInfo.InstrumentID - (int)&lInfo); //InstrumentID
	PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcOrderRefType),(int)&lInfo.OrderRef - (int)&lInfo);//Orderref
	PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcUserIDType),(int)&lInfo.UserID - (int)&lInfo);    //UserID
	PushColumnDataToVector(ltest,OCCI_SQLT_CHR,sizeof(TThostFtdcOrderPriceTypeType),(int)&lInfo.OrderPriceType - (int)&lInfo);	//OrderPriceType
	PushColumnDataToVector(ltest,OCCI_SQLT_CHR,sizeof(TThostFtdcDirectionType),(int)&lInfo.Direction - (int)&lInfo);            //Driection
	PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcCombOffsetFlagType),(int)&lInfo.CombOffsetFlag - (int)&lInfo);  //ComboffsetFlag
	PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcCombHedgeFlagType),(int)&lInfo.CombHedgeFlag - (int)&lInfo);	//combhedgeflag
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.LimitPrice - (int)&lInfo);       //LimitPrice
	PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.VolumeTotalOriginal - (int)&lInfo); //VolumeTotalOriginal
	PushColumnDataToVector(ltest,OCCI_SQLT_CHR,sizeof(TThostFtdcTimeConditionType),(int)&lInfo.TimeCondition - (int)&lInfo);	 //TimeCondition
	PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcDateType),(int)&lInfo.GTDDate - (int)&lInfo); //GTData
	PushColumnDataToVector(ltest,OCCI_SQLT_CHR,sizeof(TThostFtdcVolumeConditionType),(int)&lInfo.VolumeCondition - (int)&lInfo); //VolumeCondition
	PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.MinVolume - (int)&lInfo); //MinVolumn
	PushColumnDataToVector(ltest,OCCI_SQLT_CHR,sizeof(TThostFtdcContingentConditionType),(int)&lInfo.ContingentCondition - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.StopPrice - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCI_SQLT_CHR,sizeof(TThostFtdcForceCloseReasonType),(int)&lInfo.ForceCloseReason - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcBoolType),(int)&lInfo.IsAutoSuspend - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcBusinessUnitType),(int)&lInfo.BusinessUnit - (int)&lInfo);

	PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcRequestIDType),(int)&lInfo.RequestID - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcOrderLocalIDType),(int)&lInfo.OrderLocalID - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcExchangeIDType),(int)&lInfo.ExchangeID - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcParticipantIDType),(int)&lInfo.ParticipantID - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcClientIDType),(int)&lInfo.ClientID - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcExchangeInstIDType),(int)&lInfo.ExchangeInstID - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcTraderIDType),(int)&lInfo.TraderID - (int)&lInfo);

	PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcInstallIDType),(int)&lInfo.InstallID - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCI_SQLT_CHR,sizeof(TThostFtdcOrderSubmitStatusType),(int)&lInfo.OrderSubmitStatus - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcSequenceNoType),(int)&lInfo.NotifySequence - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcDateType),(int)&lInfo.TradingDay - (int)&lInfo);

	PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcSettlementIDType),(int)&lInfo.SettlementID - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcOrderSysIDType),(int)&lInfo.OrderSysID - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCI_SQLT_CHR,sizeof(TThostFtdcOrderSourceType),(int)&lInfo.OrderSource - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCI_SQLT_CHR,sizeof(TThostFtdcOrderStatusType),(int)&lInfo.OrderStatus - (int)&lInfo);

	PushColumnDataToVector(ltest,OCCI_SQLT_CHR,sizeof(TThostFtdcOrderTypeType),(int)&lInfo.OrderType - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.VolumeTraded - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.VolumeTotal - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcDateType),(int)&lInfo.InsertDate - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcTimeType),(int)&lInfo.InsertTime - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcTimeType),(int)&lInfo.ActiveTime - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcTimeType),(int)&lInfo.SuspendTime - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcTimeType),(int)&lInfo.UpdateTime - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcTimeType),(int)&lInfo.CancelTime - (int)&lInfo);

	PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcTraderIDType),(int)&lInfo.ActiveTraderID - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcParticipantIDType),(int)&lInfo.ClearingPartID - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcSequenceNoType),(int)&lInfo.SequenceNo - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcFrontIDType),(int)&lInfo.FrontID - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcSessionIDType),(int)&lInfo.SessionID - (int)&lInfo);

	PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcProductInfoType),(int)&lInfo.UserProductInfo - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcErrorMsgType),(int)&lInfo.StatusMsg - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcBoolType),(int)&lInfo.UserForceClose - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcUserIDType),(int)&lInfo.ActiveUserID - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcSequenceNoType),(int)&lInfo.BrokerOrderSeq - (int)&lInfo);

	PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcOrderSysIDType),(int)&lInfo.RelativeOrderSysID - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.AvgPrice - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIINT,sizeof(int),(int)&lInfo.ExStatus - (int)&lInfo);

	PushColumnDataToVector(ltest,OCCIINT,sizeof(int),(int)&lInfo.FTID - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIINT,sizeof(int),(int)&lInfo.UpdateSeq - (int)&lInfo);

	std::string lsDeleteSql;
	if(nbDelete)
	{
		if(nsBrokerID.empty() && nsAccountID.empty())
			lsDeleteSql = "delete from "+nsTableName+" t where t.ValidateDate = \'" 
			+ strTime + "\'";
		else
			lsDeleteSql = "delete from "+nsTableName+" t where t.ValidateDate = \'" 
			+ strTime
			+ "\' AND BrokerID = \'" + nsBrokerID 
			+ "\' AND InvestorID = \'" + nsAccountID + "\'";
	}

	return BatchInsert(	nsTableName,
		lsDeleteSql,
		strTime,
		ltest,
		nOrders,
		nErrorCode);
	return false;


}
//成交
bool CSvrDBOprImpl::SaveAccountTraderInfos(const std::string& nsTableName,
										bool nbDelete,
										const std::string& strTime,
										const std::string& nsBrokerID,
										const std::string& nsAccountID,
										const std::vector<PlatformStru_TradeInfo>& nTraders ,
										int& nErrorCode )
{
	PlatformStru_TradeInfo lInfo;
	std::vector<ColumeData> ltest;

	PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcBrokerIDType),(int)&lInfo.BrokerID - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcInvestorIDType),(int)&lInfo.InvestorID - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcInstrumentIDType),(int)&lInfo.InstrumentID - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcOrderRefType),(int)&lInfo.OrderRef - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcUserIDType),(int)&lInfo.UserID - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcExchangeIDType),(int)&lInfo.ExchangeID - (int)&lInfo);	
	PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcTradeIDType),(int)&lInfo.TradeID - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCI_SQLT_CHR,sizeof(TThostFtdcDirectionType),(int)&lInfo.Direction - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcOrderSysIDType),(int)&lInfo.OrderSysID - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcParticipantIDType),(int)&lInfo.ParticipantID - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcClientIDType),(int)&lInfo.ClientID - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCI_SQLT_CHR,sizeof(TThostFtdcDirectionType),(int)&lInfo.TradingRole - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcExchangeInstIDType),(int)&lInfo.ExchangeInstID - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCI_SQLT_CHR,sizeof(TThostFtdcOffsetFlagType),(int)&lInfo.OffsetFlag - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCI_SQLT_CHR,sizeof(TThostFtdcHedgeFlagType),(int)&lInfo.HedgeFlag - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.Price - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.Volume - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcDateType),(int)&lInfo.TradeDate - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcTimeType),(int)&lInfo.TradeTime - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCI_SQLT_CHR,sizeof(TThostFtdcTradeTypeType),(int)&lInfo.TradeType - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCI_SQLT_CHR,sizeof(TThostFtdcPriceSourceType),(int)&lInfo.PriceSource - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcTraderIDType),(int)&lInfo.TraderID - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcOrderLocalIDType),(int)&lInfo.OrderLocalID - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcParticipantIDType),(int)&lInfo.ClearingPartID - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcBusinessUnitType),(int)&lInfo.BusinessUnit - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcSequenceNoType),(int)&lInfo.SequenceNo - (int)&lInfo);	
	PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcDateType),(int)&lInfo.TradingDay - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcSettlementIDType),(int)&lInfo.SettlementID - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcSequenceNoType),(int)&lInfo.BrokerOrderSeq - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCI_SQLT_CHR,sizeof(TThostFtdcTradeSourceType),(int)&lInfo.TradeSource - (int)&lInfo);	
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.CloseProfitByDate - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.CloseProfitByTrade - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.TradeCommission - (int)&lInfo);

	PushColumnDataToVector(ltest,OCCIINT,sizeof(int),(int)&lInfo.FTID - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIINT,sizeof(int),(int)&lInfo.UpdateSeq - (int)&lInfo);

	std::string lsDeleteSql;
	if(nbDelete)
	{
		if(nsBrokerID.empty() && nsAccountID.empty())
			lsDeleteSql = "delete from "+nsTableName+" t where t.ValidateDate = \'" 
			+ strTime + "\'";
		else
			lsDeleteSql = "delete from "+nsTableName+" t where t.ValidateDate = \'" 
			+ strTime
			+ "\' AND BrokerID = \'" + nsBrokerID 
			+ "\' AND InvestorID = \'" + nsAccountID + "\'";
	}

	return BatchInsert(	nsTableName,
		lsDeleteSql,
		strTime,
		ltest,
		nTraders,
		nErrorCode);
	return false;
}
//持仓
bool CSvrDBOprImpl::SaveAccountPositionInfos(const std::string& nsTableName,
										  bool nbDelete,
										  const std::string& strTime,										 
										  const std::string& nsBrokerID,
										  const std::string& nsAccountID,
										  const std::vector<PlatformStru_Position>& nPositions ,
										  int& nErrorCode )
{
	PlatformStru_Position lInfo;
	std::vector<ColumeData> ltest;
	PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcInstrumentIDType),(int)&lInfo.InstrumentID - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcBrokerIDType),(int)&lInfo.BrokerID - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcInvestorIDType),(int)&lInfo.InvestorID - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCI_SQLT_CHR,sizeof(TThostFtdcPosiDirectionType),(int)&lInfo.PosiDirection - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCI_SQLT_CHR,sizeof(TThostFtdcHedgeFlagType),(int)&lInfo.HedgeFlag - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcDateType),(int)&lInfo.TradingDay - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCI_SQLT_CHR,sizeof(TThostFtdcPositionDateType),(int)&lInfo.PositionDate - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcSettlementIDType),(int)&lInfo.SettlementID - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.Position - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.TodayPosition - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.YdPosition - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.OpenVolume - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.CloseVolume - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.OpenAmount - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.CloseAmount - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.PositionCost - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.OpenCost - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.LongFrozen - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.ShortFrozen - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.LongFrozenAmount - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.ShortFrozenAmount - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.FrozenMargin - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.FrozenCommission - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.FrozenCash- (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.Commission - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.PreMargin - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.UseMargin - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.ExchangeMargin - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcRatioType),(int)&lInfo.MarginRateByMoney - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcRatioType),(int)&lInfo.MarginRateByVolume - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.CashIn - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.PositionProfit - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.CloseProfit - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.CloseProfitByDate - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.CloseProfitByTrade - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.PreSettlementPrice - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.SettlementPrice - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.CombPosition - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.CombLongFrozen - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.CombShortFrozen - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.PositionProfitByTrade - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.TotalPositionProfitByDate - (int)&lInfo);

	PushColumnDataToVector(ltest,OCCIINT,sizeof(int),(int)&lInfo.FTID - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIINT,sizeof(int),(int)&lInfo.UpdateSeq - (int)&lInfo);


	std::string lsDeleteSql;
	if(nbDelete)
	{
		if(nsBrokerID.empty() && nsAccountID.empty())
			lsDeleteSql = "delete from "+nsTableName+" t where t.ValidateDate = \'" 
			+ strTime + "\'";
		else
			lsDeleteSql = "delete from "+nsTableName+" t where t.ValidateDate = \'" 
			+ strTime
			+ "\' AND BrokerID = \'" + nsBrokerID 
			+ "\' AND InvestorID = \'" + nsAccountID + "\'";
	}

	return BatchInsert(	nsTableName,
		lsDeleteSql,
		strTime,
		ltest,
		nPositions,
		nErrorCode);
	return false;
}

//持仓明细
bool CSvrDBOprImpl::SaveAccountPositionDetailInfos( const std::string& nsTableName,
												bool nbDelete,
												const std::string& strTime,
												const std::string& nsBrokerID,
												const std::string& nsAccountID,
												const std::vector<PlatformStru_PositionDetail>& nPositionDetails ,
												int& nErrorCode )
{
	PlatformStru_PositionDetail lInfo;
	std::vector<ColumeData> ltest;
	PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcInstrumentIDType),(int)&lInfo.InstrumentID - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcBrokerIDType),(int)&lInfo.BrokerID - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcInvestorIDType),(int)&lInfo.InvestorID - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCI_SQLT_CHR,sizeof(TThostFtdcHedgeFlagType),(int)&lInfo.HedgeFlag - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCI_SQLT_CHR,sizeof(TThostFtdcDirectionType),(int)&lInfo.Direction - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcDateType),(int)&lInfo.OpenDate - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcTradeIDType),(int)&lInfo.TradeID - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.Volume - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.OpenPrice - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcDateType),(int)&lInfo.TradingDay - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcSettlementIDType),(int)&lInfo.SettlementID - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCI_SQLT_CHR,sizeof(TThostFtdcTradeTypeType),(int)&lInfo.TradeType - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcInstrumentIDType),(int)&lInfo.CombInstrumentID - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcExchangeIDType),(int)&lInfo.ExchangeID - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.CloseProfitByDate - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.CloseProfitByTrade - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.PositionProfitByDate - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.PositionProfitByTrade - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.Margin- (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.ExchMargin - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcRatioType),(int)&lInfo.MarginRateByMoney - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcRatioType),(int)&lInfo.MarginRateByVolume - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.LastSettlementPrice - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.SettlementPrice - (int)&lInfo);	
	PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.CloseVolume - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.CloseAmount - (int)&lInfo);


	PushColumnDataToVector(ltest,OCCIINT,sizeof(int),(int)&lInfo.FTID - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIINT,sizeof(int),(int)&lInfo.UpdateSeq - (int)&lInfo);


	std::string lsDeleteSql;
	if(nbDelete)
	{
		if(nsBrokerID.empty() && nsAccountID.empty())
			lsDeleteSql = "delete from "+nsTableName+" t where t.ValidateDate = \'" 
			+ strTime + "\'";
		else
			lsDeleteSql = "delete from "+nsTableName+" t where t.ValidateDate = \'" 
			+ strTime
			+ "\' AND BrokerID = \'" + nsBrokerID 
			+ "\' AND InvestorID = \'" + nsAccountID + "\'";
	}

	return BatchInsert(	nsTableName,
		lsDeleteSql,
		strTime,
		ltest,
		nPositionDetails,
		nErrorCode);
	return false;
}

//资金
bool CSvrDBOprImpl::SaveAccountFundInfos(const std::string& nsTableName,
									  const std::string& strTime,
									  const std::string& nsBrokerID,
									  const std::string& nsAccountID,
									  const PlatformStru_TradingAccountInfo& nFundInfos,
									  int& nErrorCode )
{
	std::vector<PlatformStru_TradingAccountInfo> nVecFundInfos;
	nVecFundInfos.push_back(nFundInfos);

	PlatformStru_TradingAccountInfo lInfo;
	std::vector<ColumeData> ltest;
	PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcBrokerIDType),(int)&lInfo.BrokerID - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcInvestorIDType),(int)&lInfo.AccountID - (int)&lInfo);	
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.PreMortgage - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.PreCredit - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.PreDeposit - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.PreBalance - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.PreMargin- (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.InterestBase - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.Interest - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.Deposit - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.Withdraw - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.FrozenMargin - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.FrozenCash- (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.FrozenCommission- (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.CurrMargin - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.CashIn - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.Commission - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.CloseProfit - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.PositionProfit - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.Balance - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.Available - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.WithdrawQuota - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.Reserve - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcDateType),(int)&lInfo.TradingDay - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcSettlementIDType),(int)&lInfo.SettlementID - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.Credit - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.Mortgage - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.ExchangeMargin - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.DeliveryMargin - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.ExchangeDeliveryMargin - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.StaticProfit - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.DynamicProfit - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.RiskDegree - (int)&lInfo);


	std::string lsDeleteSql = "delete from " +nsTableName +" t where t.ValidateDate = \'" 
		+ strTime
		+ "\' AND ACCOUNTID = \'"	+ nsAccountID  
		+ "\' AND BrokerID = \'" + nsBrokerID + "\'";

	return BatchInsert(	nsTableName,
		lsDeleteSql,
		strTime,
		ltest,
		nVecFundInfos,
		nErrorCode);
	return false;
}/*
bool CSvrDBOprImpl::IsStrategyExist(std::string strName)
{
	bool bExist =false;
	CGuard guard(&g_mutex);

	if ( !IsConnected() && !Conncect())
	{
		return false;
	}

	char szBuffer[MAX_SQL_LENGTH];
	memset(szBuffer, 0, sizeof(szBuffer));
	try
	{
		sprintf(szBuffer, "select NAME from STRATEGY_STRATEGY t where NAME = '%s'", strName.c_str());
		m_pStmt = m_pCon->createStatement( szBuffer );
		m_pRes = m_pStmt->executeQuery();
		if( m_pRes->next())
		{
			bExist =true;
		}

		m_pStmt->closeResultSet(m_pRes);
		m_pCon->terminateStatement(m_pStmt);
	}catch(oracle::occi::SQLException &e){
		std::cout<<e.what()<<endl;
		std::cout<<szBuffer<<endl;

		WriteLog(szBuffer, e.what());
		return bExist;
	}

	return bExist;
}*/
void CSvrDBOprImpl::string_replace( std::string &strBig, const std::string &strsrc, const std::string &strdst )
{	
	std::string::size_type pos = 0;
	std::string::size_type srclen = strsrc.size();
	std::string::size_type dstlen = strdst.size();

	while( (pos=strBig.find(strsrc, pos)) != std::string::npos )
	{
		strBig.replace( pos, srclen, strdst );
		pos += dstlen;
	}
} 
bool CSvrDBOprImpl::DB_AddStrategy(const SStrategy& strategy, int& nErrorCode )

{		
	CGuard guard(&g_mutex);
	nErrorCode = CF_ERROR_SUCCESS;
	if ( !IsConnected() && !Conncect())
	{
		nErrorCode = CF_ERROR_DATABASE_OTHER_ERROR;
		return false;
	}

	char szBuffer[MAX_SQL_LENGTH];
	memset(szBuffer, 0, sizeof(szBuffer));
	try
	{
		sprintf(szBuffer, "delete STRATEGY_STRATEGY t where t.STRATEGYNAME = '%s'", strategy.strategyName);
		m_pStmt = m_pCon->createStatement();
		m_pStmt->execute(szBuffer);	
		
		std::string strPara = strategy.strPara;
		string_replace(strPara, "'", "''");
		std::string strComment = strategy.strComment;
		string_replace(strComment, "'", "''");
		sprintf(szBuffer, "insert into STRATEGY_STRATEGY values('%s','%s', '%s', '%s', %d, '%s','%s', %d,EMPTY_BLOB(), EMPTY_BLOB(), SYSDATE)",
			strategy.strategyName,strategy.strategyNickName,strategy.Version, strategy.strTraderName, strategy.nPrivate, strPara.c_str(),
			strComment.c_str(),strategy.nUse);
		m_pStmt->execute( szBuffer );

		m_pCon->commit();
		m_pCon->terminateStatement(m_pStmt);
	}catch(oracle::occi::SQLException &e)
	{
		RollBack();
		std::cout<<e.what()<<endl;
		std::cout<<szBuffer<<endl;
		nErrorCode = GetErrorCode(e.what());
		WriteLog(szBuffer, e.what());
		return false;
	}

	return true;
}
bool CSvrDBOprImpl::DB_DelStrategy(std::string strName, int& nErrorCode )
{
	CGuard guard(&g_mutex);
	nErrorCode = CF_ERROR_SUCCESS;
	if ( !IsConnected() && !Conncect())
	{
		nErrorCode = CF_ERROR_DATABASE_OTHER_ERROR;
		return false;
	}

	char szBuffer[MAX_SQL_LENGTH];
	memset(szBuffer, 0, sizeof(szBuffer));
	try
	{		
		sprintf(szBuffer, "DELETE  from STRATEGY_STRATEGY  where STRATEGYNAME = '%s'",strName.c_str());
		m_pStmt = m_pCon->createStatement();
		m_pStmt->execute(szBuffer);
		m_pCon->commit();
		m_pCon->terminateStatement(m_pStmt);
	}catch(oracle::occi::SQLException &e)
	{
		RollBack();
		std::cout<<e.what()<<endl;
		std::cout<<szBuffer<<endl;
		nErrorCode = GetErrorCode(e.what());
		WriteLog(szBuffer, e.what());
		return false;
	}

	return true;
}
bool CSvrDBOprImpl::DB_SetStragegyStatus(std::string strName, int nUse, int& nErrorCode)
{
	CGuard guard(&g_mutex);
	nErrorCode = CF_ERROR_SUCCESS;
	if ( !IsConnected() && !Conncect())
	{
		nErrorCode = CF_ERROR_DATABASE_OTHER_ERROR;
		return false;
	}

	char szBuffer[MAX_SQL_LENGTH];
	memset(szBuffer, 0, sizeof(szBuffer));
	try
	{		
		sprintf(szBuffer, "update  STRATEGY_STRATEGY t set t.use = %d WHERE STRATEGYNAME ='%s'",nUse, strName.c_str());
		m_pStmt = m_pCon->createStatement();
		m_pStmt->executeUpdate( szBuffer );
		m_pCon->commit();

		m_pCon->terminateStatement(m_pStmt);
	}catch(oracle::occi::SQLException &e)
	{
		RollBack();
		std::cout<<e.what()<<endl;
		std::cout<<szBuffer<<endl;
		nErrorCode = GetErrorCode(e.what());
		WriteLog(szBuffer, e.what());
		return false;
	}

	return true;
}
bool CSvrDBOprImpl::DB_AddInstance(const SStrategyInstance& Instance, int& nErrorCode)
{
	CGuard guard(&g_mutex);
	nErrorCode = CF_ERROR_SUCCESS;
	if ( !IsConnected() && !Conncect())
	{
		nErrorCode = CF_ERROR_DATABASE_OTHER_ERROR;
		return false;
	}

	char szBuffer[MAX_SQL_LENGTH];

	try
	{
		sprintf(szBuffer, "delete STRATEGY_INSTANCE t where t.STRATEGYNAME = '%s' and  t.TRADERNAME = '%s'", Instance.strategyName, Instance.strTraderName);
		m_pStmt = m_pCon->createStatement();
		m_pStmt->execute(szBuffer);	

		std::string strPara = Instance.strPara;
		string_replace(strPara, "'", "''");
		std::string strComment = Instance.strComment;
		string_replace(strComment, "'", "''");
		sprintf(szBuffer, "insert into STRATEGY_INSTANCE values('%s','%s', '%s', %d, %d, %d, '%s','%s', %d, SYSDATE, %d, %d)",
			Instance.strategyName, Instance.strTraderName, Instance.Instruments, Instance.nRunAfterLoad, Instance.nRunAfterOffline,
			Instance.nOrderActionBeforeStop, strPara.c_str(), strComment.c_str(), Instance.nStart, Instance.bStartTimer, Instance.nTimerSpan);
		m_pStmt->execute( szBuffer );

		m_pCon->commit();
		m_pCon->terminateStatement(m_pStmt);
	}catch(oracle::occi::SQLException &e)
	{
		RollBack();
		std::cout<<e.what()<<endl;
		std::cout<<szBuffer<<endl;
		nErrorCode = GetErrorCode(e.what());
		WriteLog(szBuffer, e.what());
		return false;
	}

	return true;


}
bool CSvrDBOprImpl::DB_DelInstance(std::string strName, int& nErrorCode)
{
	CGuard guard(&g_mutex);
	nErrorCode = CF_ERROR_SUCCESS;
	if ( !IsConnected() && !Conncect())
	{
		nErrorCode = CF_ERROR_DATABASE_OTHER_ERROR;
		return false;
	}

	char szBuffer[MAX_SQL_LENGTH];
	memset(szBuffer, 0, sizeof(szBuffer));
	try
	{		
		sprintf(szBuffer, "DELETE  from STRATEGY_INSTANCE  where STRATEGYNAME = '%s'",strName.c_str());
		m_pStmt = m_pCon->createStatement();
		m_pStmt->execute(szBuffer);
		m_pCon->commit();
		m_pCon->terminateStatement(m_pStmt);
	}catch(oracle::occi::SQLException &e)
	{
		RollBack();
		std::cout<<e.what()<<endl;
		std::cout<<szBuffer<<endl;
		nErrorCode = GetErrorCode(e.what());
		WriteLog(szBuffer, e.what());
		return false;
	}

	return true;
}
bool CSvrDBOprImpl::DB_DelInstance_ByTrader(std::string strName, std::string strUploader, int& nErrorCode)
{
	CGuard guard(&g_mutex);
	nErrorCode = CF_ERROR_SUCCESS;
	if ( !IsConnected() && !Conncect())
	{
		nErrorCode = CF_ERROR_DATABASE_OTHER_ERROR;
		return false;
	}

	char szBuffer[MAX_SQL_LENGTH];
	memset(szBuffer, 0, sizeof(szBuffer));
	try
	{		
		sprintf(szBuffer, "DELETE  from STRATEGY_INSTANCE  where STRATEGYNAME = '%s'and TRADERNAME='%s'",strName.c_str(), strUploader.c_str());
		m_pStmt = m_pCon->createStatement();
		m_pStmt->execute(szBuffer);
		m_pCon->commit();
		m_pCon->terminateStatement(m_pStmt);
	}catch(oracle::occi::SQLException &e)
	{
		RollBack();
		std::cout<<e.what()<<endl;
		std::cout<<szBuffer<<endl;
		nErrorCode = GetErrorCode(e.what());
		WriteLog(szBuffer, e.what());
		return false;
	}

	return true;
}
bool CSvrDBOprImpl::DB_SetInstanceStatus(std::string strName, std::string strUploader, int nUse, int& nErrorCode)
{
	CGuard guard(&g_mutex);
	nErrorCode = CF_ERROR_SUCCESS;
	if ( !IsConnected() && !Conncect())
	{
		nErrorCode = CF_ERROR_DATABASE_OTHER_ERROR;
		return false;
	}

	char szBuffer[MAX_SQL_LENGTH];
	memset(szBuffer, 0, sizeof(szBuffer));
	try
	{		
		sprintf(szBuffer, "update  STRATEGY_INSTANCE t set t.STATUS = %d WHERE STRATEGYNAME ='%s' and TRADERNAME = '%s'",nUse, strName.c_str(), strUploader.c_str());
		m_pStmt = m_pCon->createStatement();
		m_pStmt->executeUpdate( szBuffer );
		m_pCon->commit();

		m_pCon->terminateStatement(m_pStmt);
	}catch(oracle::occi::SQLException &e)
	{
		RollBack();
		std::cout<<e.what()<<endl;
		std::cout<<szBuffer<<endl;
		nErrorCode = GetErrorCode(e.what());
		WriteLog(szBuffer, e.what());
		return false;
	}

	return true;
}
bool CSvrDBOprImpl::DB_AddIndex(const SIndex& strategy, int& nErrorCode )
{
	CGuard guard(&g_mutex);
	nErrorCode = CF_ERROR_SUCCESS;
	if ( !IsConnected() && !Conncect())
	{
		nErrorCode = CF_ERROR_DATABASE_OTHER_ERROR;
		return false;
	}

	char szBuffer[MAX_SQL_LENGTH];

	try
	{
		sprintf(szBuffer, "delete STRATEGY_INDEX t where t.INDEXNAME = '%s'", strategy.IndexName);
		m_pStmt = m_pCon->createStatement();
		m_pStmt->execute(szBuffer);	

		std::string strPara = strategy.strPara;
		string_replace(strPara, "'", "''");
		std::string strComment = strategy.strComment;
		string_replace(strComment, "'", "''");
		sprintf(szBuffer, "insert into STRATEGY_INDEX values('%s',%d, '%s',%d, '%s', '%s', EMPTY_BLOB(), EMPTY_BLOB(), SYSDATE)",
			strategy.IndexName, strategy.nType, strategy.strTraderName, strategy.nPrivate, strPara.c_str(), strComment.c_str());
		m_pStmt->execute( szBuffer );

		m_pCon->commit();
		m_pCon->terminateStatement(m_pStmt);
	}catch(oracle::occi::SQLException &e)
	{
		RollBack();
		std::cout<<e.what()<<endl;
		std::cout<<szBuffer<<endl;
		nErrorCode = GetErrorCode(e.what());
		WriteLog(szBuffer, e.what());
		return false;
	}

	return true;
}
bool CSvrDBOprImpl::DB_DelIndex(std::string strName, int& nErrorCode)
{
	CGuard guard(&g_mutex);
	nErrorCode = CF_ERROR_SUCCESS;
	if ( !IsConnected() && !Conncect())
	{
		nErrorCode = CF_ERROR_DATABASE_OTHER_ERROR;
		return false;
	}

	char szBuffer[MAX_SQL_LENGTH];
	memset(szBuffer, 0, sizeof(szBuffer));
	try
	{		
		sprintf(szBuffer, "DELETE * from STRATEGY_INDEX  where INDEXNAME = '%s')",strName.c_str());
		m_pStmt = m_pCon->createStatement();
		m_pStmt->execute(szBuffer);
		m_pCon->commit();
		m_pCon->terminateStatement(m_pStmt);
	}catch(oracle::occi::SQLException &e)
	{
		RollBack();
		std::cout<<e.what()<<endl;
		std::cout<<szBuffer<<endl;
		nErrorCode = GetErrorCode(e.what());
		WriteLog(szBuffer, e.what());
		return false;
	}

	return true;
}
bool CSvrDBOprImpl::DB_GetUserStrategys(std::vector<SStrategy>& vecStrategy, std::string strUser)
{
	CGuard guard(&g_mutex);
	int nErrorCode = CF_ERROR_SUCCESS;
	if ( !IsConnected() && !Conncect())
	{
		nErrorCode = CF_ERROR_DATABASE_OTHER_ERROR;
		return false;
	}

	char szBuffer[MAX_SQL_LENGTH];

	try
	{		
		if(strUser == "")
			sprintf(szBuffer, "select STRATEGYNAME,STRATEGYNICKNAME,VERSION,TRADERNAME,PRIVILEDGE,PARAMETER, COMMENTS,USE, DLLFILE, SOURCEFILE from STRATEGY_STRATEGY t");
		else
			sprintf(szBuffer, "select STRATEGYNAME,STRATEGYNICKNAME,VERSION,TRADERNAME,PRIVILEDGE,PARAMETER, COMMENTS,USE, DLLFILE, SOURCEFILE from STRATEGY_STRATEGY t where STRATEGYNAME = '%s'", strUser.c_str());

		m_pStmt = m_pCon->createStatement( szBuffer );		
		m_pRes = m_pStmt->executeQuery();
		while( m_pRes->next())
		{
			SStrategy strategy;
			std::string strValue = m_pRes->getString(1);
			strcpy(strategy.strategyName, strValue.c_str());

			std::string STRATEGYNICKNAME = m_pRes->getString(2);
			strcpy(strategy.strategyNickName, STRATEGYNICKNAME.c_str());

			std::string Version = m_pRes->getString(3);
			strcpy(strategy.Version, Version.c_str());

			std::string TRADERNAME = m_pRes->getString(4);
			strcpy(strategy.strTraderName, TRADERNAME.c_str());
		

			int PRIVILEDGE = m_pRes->getInt(5);
			strategy.nPrivate  = PRIVILEDGE;

			std::string PARAMETER = m_pRes->getString(6);
			strcpy(strategy.strPara, PARAMETER.c_str());

			std::string COMMENTS = m_pRes->getString(7);
			strcpy(strategy.strComment, COMMENTS.c_str());

			int USE = m_pRes->getInt(8);
			strategy.nUse  = USE;

			Blob blob = m_pRes->getBlob(9);
			blob.open(OCCI_LOB_READONLY);

			std::string strFileFolder = "Strategy";
			std::string strFileName   = strValue +".dll";
			GetFolderFileName(strFileName, strFileFolder);
			ReadBlobToFile(blob, strFileName);
			blob.close();
			
			Blob blobZip = m_pRes->getBlob(10);
			blobZip.open(OCCI_LOB_READONLY);
			strFileFolder = "Strategy";
			strFileName   = strValue +".zip";
			GetFolderFileName(strFileName, strFileFolder);
			ReadBlobToFile(blobZip, strFileName);
			blobZip.close();

			vecStrategy.push_back(strategy);
		}
		m_pStmt->closeResultSet(m_pRes);
		m_pCon->commit();
		m_pCon->terminateStatement(m_pStmt);
	}
	catch(oracle::occi::SQLException &e)
	{
		RollBack();
		std::cout<<e.what()<<endl;
		std::cout<<szBuffer<<endl;
		nErrorCode = GetErrorCode(e.what());
		WriteLog(szBuffer, e.what());
		return false;
	}
	return true;
}
bool CSvrDBOprImpl::DB_GetUserIndex(std::vector<SIndex>& vecIndex, std::string strUser)
{
	CGuard guard(&g_mutex);
	int nErrorCode = CF_ERROR_SUCCESS;
	if ( !IsConnected() && !Conncect())
	{
		nErrorCode = CF_ERROR_DATABASE_OTHER_ERROR;
		return false;
	}

	char szBuffer[MAX_SQL_LENGTH];

	try
	{		
		if(strUser == "")
			sprintf(szBuffer, "select INDEXNAME,NTYPE,TRADERNAME,NPRIVATE,parameter, COMMENTS,DLLFILE,SOURCEFILE  from STRATEGY_INDEX t");
		else
			sprintf(szBuffer, "select INDEXNAME,NTYPE,TRADERNAME,NPRIVATE,parameter, COMMENTS,DLLFILE, SOURCEFILE from STRATEGY_INDEX t where STRATEGYNAME = '%s'", strUser.c_str());

		m_pStmt = m_pCon->createStatement( szBuffer );		
		m_pRes = m_pStmt->executeQuery();
		while( m_pRes->next())
		{
			SIndex strategy;
			std::string INDEXNAME = m_pRes->getString(1);
			strcpy(strategy.IndexName, INDEXNAME.c_str());

			int NTYPE = m_pRes->getInt(2);
			strategy.nType = NTYPE;

			std::string TRADERNAME = m_pRes->getString(3);
			strcpy(strategy.strTraderName, TRADERNAME.c_str());

			int PRIVILEDGE = m_pRes->getInt(4);
			strategy.nPrivate  = PRIVILEDGE;

			std::string PARAMETER = m_pRes->getString(5);
			strcpy(strategy.strPara, PARAMETER.c_str());

			std::string COMMENTS = m_pRes->getString(6);
			strcpy(strategy.strComment, COMMENTS.c_str());

			
			Blob blob = m_pRes->getBlob(7);
			blob.open(OCCI_LOB_READONLY);

			std::string strFileFolder = "Strategy";
			std::string strFileName   = INDEXNAME +".dll";
			GetFolderFileName(strFileName, strFileFolder);
			ReadBlobToFile(blob, strFileName);
			blob.close();
			
			Blob blob2 = m_pRes->getBlob(8);
			blob2.open(OCCI_LOB_READONLY);
			strFileFolder = "Strategy";
			strFileName   = INDEXNAME +".zip";
			GetFolderFileName(strFileName, strFileFolder);
			ReadBlobToFile(blob2, strFileName);
			blob2.close();

			vecIndex.push_back(strategy);
		}
		m_pStmt->closeResultSet(m_pRes);

		m_pCon->commit();
		m_pCon->terminateStatement(m_pStmt);
	}catch(oracle::occi::SQLException &e)
	{
		RollBack();
		std::cout<<e.what()<<endl;
		std::cout<<szBuffer<<endl;
		nErrorCode = GetErrorCode(e.what());
		WriteLog(szBuffer, e.what());
		return false;
	}
	return true;
}
bool CSvrDBOprImpl::DB_GetUserInstance(std::vector<SStrategyInstance>& vecInstance, std::string strUser)
{
	CGuard guard(&g_mutex);
	int nErrorCode = CF_ERROR_SUCCESS;
	if ( !IsConnected() && !Conncect())
	{
		nErrorCode = CF_ERROR_DATABASE_OTHER_ERROR;
		return false;
	}

	char szBuffer[MAX_SQL_LENGTH];

	try
	{		
		if(strUser == "")
			sprintf(szBuffer, "select STRATEGYNAME,TRADERNAME,INSTRUMENTS,RUNAFTERLOAD,RUNAFTEROFFLINE,ORDERACTIONBEFORESTOP, PARAMETER,COMMENTS, STATUS, STARTTIME, TIMERSPAN from STRATEGY_INSTANCE t");
		else
			sprintf(szBuffer, "select STRATEGYNAME,TRADERNAME,INSTRUMENTS,RUNAFTERLOAD,RUNAFTEROFFLINE,ORDERACTIONBEFORESTOP, PARAMETER,COMMENTS, STATUS, STARTTIME, TIMERSPAN from STRATEGY_INSTANCE t where STRATEGYNAME = '%s'", strUser.c_str());

		m_pStmt = m_pCon->createStatement( szBuffer );		
		m_pRes = m_pStmt->executeQuery();
		while( m_pRes->next())
		{
			SStrategyInstance strategy;
			std::string STRATEGYNAME = m_pRes->getString(1);
			strcpy(strategy.strategyName, STRATEGYNAME.c_str());

			std::string TRADERNAME = m_pRes->getString(2);
			strcpy(strategy.strTraderName, TRADERNAME.c_str());

			std::string INSTRUMENTS = m_pRes->getString(3);
			strcpy(strategy.Instruments, INSTRUMENTS.c_str());

			int RUNAFTERLOAD = m_pRes->getInt(4);
			strategy.nRunAfterLoad  = RUNAFTERLOAD;

			int RUNAFTEROFFLINE = m_pRes->getInt(5);
			strategy.nRunAfterOffline  = RUNAFTEROFFLINE;

			int ORDERACTIONBEFORESTOP = m_pRes->getInt(6);
			strategy.nOrderActionBeforeStop  = ORDERACTIONBEFORESTOP;

			std::string PARAMETER = m_pRes->getString(7);
			strcpy(strategy.strPara, PARAMETER.c_str());

			std::string COMMENTS = m_pRes->getString(8);
			strcpy(strategy.strComment, COMMENTS.c_str());

			int STATUS = m_pRes->getInt(9);
			strategy.nStart  = STATUS;

			int STARTTIME = m_pRes->getInt(10);
			strategy.bStartTimer  = STARTTIME==0?false:true;

			int TIMERSPAN = m_pRes->getInt(11);
			strategy.nTimerSpan  = TIMERSPAN;

			vecInstance.push_back(strategy);
		}
		m_pStmt->closeResultSet(m_pRes);
		m_pCon->commit();
		m_pCon->terminateStatement(m_pStmt);
	}
	catch(oracle::occi::SQLException &e)
	{
		RollBack();
		std::cout<<e.what()<<endl;
		std::cout<<szBuffer<<endl;
		nErrorCode = GetErrorCode(e.what());
		WriteLog(szBuffer, e.what());
		return false;
	}
	return true;
}
void CSvrDBOprImpl::ReadBlobToFile(oracle::occi::Blob& blob, std::string strFileName)
{
	FILE* pFile = fopen(strFileName.c_str(), "wb" );
	if(pFile == NULL)
		return;

	Stream *instream = blob.getStream();
	char bVal[CHUNKSIZE+1];
	memset(bVal, 0 , CHUNKSIZE+1);
	long llswrite = 0;
	long size = blob.length();
	int n;
	n = (size%CHUNKSIZE) ==0?(size/CHUNKSIZE):(size/CHUNKSIZE+1);
	for(int i =0; i<n; i++)
	{
		llswrite = instream->readBuffer(bVal, CHUNKSIZE);
		if(llswrite)
			fwrite(bVal, llswrite, 1, pFile);
	}
	fclose(pFile);
	
}
void CSvrDBOprImpl::GetFolderFileName(std::string& strFilePath,const std::string& folder)
{
	char szLocalPath[256];
	memset(szLocalPath, 0, 256);
	GetModuleFileNameA( NULL, szLocalPath, 256 );
	std::string strSystemPath( szLocalPath );
	int nPos = strSystemPath.rfind( '\\' );
	if ( -1 != nPos )
	{
		strSystemPath = strSystemPath.substr( 0, nPos + 1 );
		if(!folder.empty())
		{
			if(std::string::npos==folder.rfind("\\"))
				strSystemPath+=folder+"\\";
			else
				strSystemPath+=folder;
			CreateDirectoryA(strSystemPath.c_str(),NULL);
		}
		strFilePath = strSystemPath + strFilePath;
	}	
}
void CSvrDBOprImpl::WriteFileToBlob(oracle::occi::Blob& blob, char *pData, int nLength)
{
	if(pData == NULL)
		return;
	Stream *strm = blob.getStream();
	strm->writeBuffer(pData, nLength);
	char*c = (char*)"";
	strm->writeLastBuffer(c, 0);
	blob.closeStream(strm);
}
bool CSvrDBOprImpl::DB_WriteStragegyFile(std::string strName, int nType, char *pData, int nLength, int& nErrorCode)
{
	bool bSuccess = false;
	CGuard guard(&g_mutex);
	nErrorCode = CF_ERROR_SUCCESS;
	if ( !IsConnected() && !Conncect())
	{
		nErrorCode = CF_ERROR_DATABASE_OTHER_ERROR;
		return false;
	}

	char szBuffer[MAX_SQL_LENGTH];

	try
	{
		if(nType == UPLOAD_STRATEGY_DLL)
			sprintf(szBuffer, "select DLLFILE from STRATEGY_STRATEGY  where STRATEGYNAME = '%s' FOR UPDATE",strName.c_str() );
		else if(nType == UPLOAD_STRATEGY_ZIP)
			sprintf(szBuffer, "select SOURCEFILE from STRATEGY_STRATEGY   where STRATEGYNAME = '%s' FOR UPDATE", strName.c_str() );

		m_pStmt = m_pCon->createStatement( szBuffer );		
		m_pRes = m_pStmt->executeQuery();	
		if( m_pRes->next())
		{
			oracle::occi::Blob blob = m_pRes->getBlob(1);		
			WriteFileToBlob(blob, pData, nLength);		
			int i = blob.length();
			bSuccess  = true;
		}
		m_pStmt->closeResultSet(m_pRes);

		m_pCon->commit();
		m_pCon->terminateStatement(m_pStmt);
	}catch(oracle::occi::SQLException &e)
	{
		RollBack();
	
		std::cout<<e.what()<<endl;
		std::cout<<szBuffer<<endl;
		nErrorCode = GetErrorCode(e.what());
		WriteLog(szBuffer, e.what());
		return false;
	}

	return bSuccess;
}
bool CSvrDBOprImpl::DB_WriteIndexFile(std::string strName, int nType, char *pData, int nLength, int& nErrorCode)
{
	bool bSuccess = false;
	CGuard guard(&g_mutex);
	nErrorCode = CF_ERROR_SUCCESS;
	if ( !IsConnected() && !Conncect())
	{
		nErrorCode = CF_ERROR_DATABASE_OTHER_ERROR;
		return false;
	}

	char szBuffer[MAX_SQL_LENGTH];

	try
	{
		if(nType == UPLOAD_INDEX_DLL)
			sprintf(szBuffer, "select DLLFILE from STRATEGY_INDEX  where INDEXNAME = '%s' FOR UPDATE",strName.c_str() );
		else if(nType == UPLOAD_INDEX_ZIP)
			sprintf(szBuffer, "select SOURCEFILE from STRATEGY_INDEX  where INDEXNAME = '%s' FOR UPDATE",strName.c_str() );
	

		m_pStmt = m_pCon->createStatement( szBuffer );		
		m_pRes = m_pStmt->executeQuery();
		if( m_pRes->next())
		{
			oracle::occi::Blob blob = m_pRes->getBlob(1);
			//blob.open(OCCI_LOB_READWRITE);
			WriteFileToBlob(blob, pData, nLength);				
			//blob.close();
			bSuccess  = true;
		}
		m_pStmt->closeResultSet(m_pRes);

		m_pCon->commit();
		m_pCon->terminateStatement(m_pStmt);
	}catch(oracle::occi::SQLException &e)
	{
		RollBack();
		std::cout<<e.what()<<endl;
		std::cout<<szBuffer<<endl;
		nErrorCode = GetErrorCode(e.what());
		WriteLog(szBuffer, e.what());
		return false;
	}

	return bSuccess;
}
bool CSvrDBOprImpl::DB_AddStrategy2IndexRelation(std::string strName, std::string strIndex, int& nErrorCode)
{
	CGuard guard(&g_mutex);
	nErrorCode = CF_ERROR_SUCCESS;
	if ( !IsConnected() && !Conncect())
	{
		nErrorCode = CF_ERROR_DATABASE_OTHER_ERROR;
		return false;
	}

	char szBuffer[MAX_SQL_LENGTH];

	try
	{
		sprintf(szBuffer, "select STRATEGYNAME,STRATEGYINDEX from STRATEGY_INDEX_RELATION t where STRATEGYNAME = '%s' and STRATEGYINDEX = '%s'", strName.c_str(), strIndex.c_str());

		m_pStmt = m_pCon->createStatement( szBuffer );		
		m_pRes = m_pStmt->executeQuery();
		if( m_pRes->next())
		{//如果数据库里面已经有这个结果了，则不再重复插入
			return true;
		}

	

		sprintf(szBuffer, "insert into STRATEGY_INDEX_RELATION values('%s','%s')", strName.c_str(), strIndex.c_str());
		m_pStmt->execute( szBuffer );

		m_pCon->commit();
		m_pCon->terminateStatement(m_pStmt);
	}catch(oracle::occi::SQLException &e)
	{
		RollBack();
		std::cout<<e.what()<<endl;
		std::cout<<szBuffer<<endl;
		nErrorCode = GetErrorCode(e.what());
		WriteLog(szBuffer, e.what());
		return false;
	}

	return true;
}
bool CSvrDBOprImpl::DB_DelStrategy2IndexRelation(std::string strName, int& nErrorCode)
{
	CGuard guard(&g_mutex);
	nErrorCode = CF_ERROR_SUCCESS;
	if ( !IsConnected() && !Conncect())
	{
		nErrorCode = CF_ERROR_DATABASE_OTHER_ERROR;
		return false;
	}

	char szBuffer[MAX_SQL_LENGTH];
	memset(szBuffer, 0, sizeof(szBuffer));
	try
	{		
		sprintf(szBuffer, "DELETE  from STRATEGY_INDEX_RELATION  where STRATEGYNAME = '%s'",strName.c_str());
		m_pStmt = m_pCon->createStatement();
		m_pStmt->execute(szBuffer);
		m_pCon->commit();
		m_pCon->terminateStatement(m_pStmt);
	}catch(oracle::occi::SQLException &e)
	{
		RollBack();
		std::cout<<e.what()<<endl;
		std::cout<<szBuffer<<endl;
		nErrorCode = GetErrorCode(e.what());
		WriteLog(szBuffer, e.what());
		return false;
	}

	return true;
}
bool CSvrDBOprImpl::DB_GetStrategy2IndexRelation(std::vector<SStrategy2Index>& vec)
{
	CGuard guard(&g_mutex);
	int nErrorCode = CF_ERROR_SUCCESS;
	if ( !IsConnected() && !Conncect())
	{
		nErrorCode = CF_ERROR_DATABASE_OTHER_ERROR;
		return false;
	}

	char szBuffer[MAX_SQL_LENGTH];

	try
	{		
		
		sprintf(szBuffer, "select STRATEGYNAME,STRATEGYINDEX from STRATEGY_INDEX_RELATION t");
	
		m_pStmt = m_pCon->createStatement( szBuffer );		
		m_pRes = m_pStmt->executeQuery();
		while( m_pRes->next())
		{
			SStrategy2Index strategy;
			std::string STRATEGYNAME = m_pRes->getString(1);
			strcpy(strategy.strategyName, STRATEGYNAME.c_str());

		

			std::string STRATEGYINDEX = m_pRes->getString(2);
			strcpy(strategy.IndexName, STRATEGYINDEX.c_str());

			vec.push_back(strategy);
		}
		m_pStmt->closeResultSet(m_pRes);

		m_pCon->commit();
		m_pCon->terminateStatement(m_pStmt);
	}catch(oracle::occi::SQLException &e)
	{
		RollBack();
		std::cout<<e.what()<<endl;
		std::cout<<szBuffer<<endl;
		nErrorCode = GetErrorCode(e.what());
		WriteLog(szBuffer, e.what());
		return false;
	}
	return true;

}
//策略操作入库
bool CSvrDBOprImpl::DB_WriteLogToDB( const LogStrategy& logData, int& nErrorCode )
{
	CGuard guard(&g_mutex);
	nErrorCode = CF_ERROR_SUCCESS;
	if ( !IsConnected() && !Conncect())
	{
		nErrorCode = CF_ERROR_DATABASE_OTHER_ERROR;
		return false;
	}

	char szBuffer[MAX_SQL_LENGTH];

	try
	{
		m_pStmt = m_pCon->createStatement();

		sprintf(szBuffer, "insert into %s values('%s', '%s', '%s', %d, '%s', %d)",
			logData.logTable,logData.logDateTime,logData.logOperatorID,logData.logTargetorID,logData.logAction,logData.logContent,logData.logResult);
		m_pStmt->execute( szBuffer );

		m_pCon->commit();
		m_pCon->terminateStatement(m_pStmt);
	}catch(oracle::occi::SQLException &e)
	{
		RollBack();
		std::cout<<e.what()<<endl;
		std::cout<<szBuffer<<endl;
		nErrorCode = GetErrorCode(e.what());
		WriteLog(szBuffer, e.what());
		return false;
	}

	return true;
}
std::string CSvrDBOprImpl::Int2String(int field)
{

		char strVolume[128];
		memset(strVolume,0,sizeof(strVolume));
		sprintf(strVolume, "%d",field);
		return strVolume;

}
std::string CSvrDBOprImpl::Double2String(double field)
{
	double InvalideValue=util::GetDoubleInvalidValue();
	if (field != InvalideValue)
	{
		char strVolume[128];
		memset(strVolume,0,sizeof(strVolume));
		if (field > pow(10.0, 16))
		{
			sprintf(strVolume, "%.2f",field);
		}
		else
		{
			sprintf(strVolume, "%16.2f",field);
		}
		return strVolume;
	}
	else
		return "";
}
//风险事件
void CSvrDBOprImpl::SaveRiskEvent2File( std::vector<RiskEvent>& vRiskEvent)
{
	std::string strData="";
	std::string strnewline = "\r\n";
	if (vRiskEvent.size()>0)
	{
		strData+="风险事件";
		strData+=strnewline;
		std::string strSeparator="|";

		for (UINT i = 0; i < vRiskEvent.size(); i++)
		{
			strData+=Int2String(vRiskEvent[i].nRiskEventID);
			strData+=strSeparator;
			strData+=Int2String(vRiskEvent[i].nRiskEventSubID);
			strData+=strSeparator;			
			strData+= vRiskEvent[i].cTimeBegin;
			strData+=strSeparator;			
			strData+= vRiskEvent[i].cTimeEnd;
			strData+=strSeparator;
			strData+=Int2String((int)(vRiskEvent[i].lEventTime));
			strData+=strSeparator;
			strData+=vRiskEvent[i].BrokerID;
			strData+=strSeparator;
			strData+=Int2String(vRiskEvent[i].nTradeInvestorID);
			strData+=strSeparator;
			strData+=Int2String(vRiskEvent[i].nRiskIndicatorID);
			strData+=strSeparator;
			strData+=vRiskEvent[i].InstrumentID;
			strData+=strSeparator;
			strData+=Int2String(vRiskEvent[i].nRiskLevelID);
			strData+=strSeparator;
			strData+=Double2String(vRiskEvent[i].dblIndicatorValue);
			strData+=strSeparator;
			strData+=Double2String(vRiskEvent[i].dblIndicatorCurrentValue);
			strData+=strSeparator;
			strData+=Int2String(vRiskEvent[i].nMsgSendStatus);
			strData+=strSeparator;
			strData+=Int2String(vRiskEvent[i].nIsValid);
			strData+=strSeparator;
		    strData+=Int2String(vRiskEvent[i].nResponseType);
			strData+=strSeparator;
			strData+=Double2String(vRiskEvent[i].dblIndicatorValue2);
			strData+=strSeparator;
			strData+=Double2String(vRiskEvent[i].dblIndicatorCurrentValue2);
			strData+=strnewline;

			


		}
		RISKDATA_LOG("%s",strData.c_str());
	}
}
//基金净值
void CSvrDBOprImpl::SaveNetFundCalcResult2File( std::vector<NetFundCalcResult>& vResult)
{
	std::string strData="";
	std::string strnewline = "\r\n";
	if (vResult.size()>0)
	{



		strData+="基金净值";
		strData+=strnewline;
		std::string strSeparator="|";

		for (UINT i = 0; i < vResult.size(); i++)
		{
			strData+=Int2String(vResult[i].nTradeAccountID);
			strData+=strSeparator;
			strData+=Double2String(vResult[i].dInnerVolumn);
			strData+=strSeparator;
			strData+=Double2String(vResult[i].dOuterVolumn);
			strData+=strSeparator;
			strData+=Double2String(vResult[i].dInnerNetAsset);
			strData+=strSeparator;
			strData+=Double2String(vResult[i].dOuterNetAsset);
			strData+=strSeparator;
			strData+=Double2String(vResult[i].dInnerPerNet);
			strData+=strSeparator;
			strData+=Double2String(vResult[i].dOuterPerNet);
			strData+=strSeparator;
			strData+=Double2String(vResult[i].dTotalNetAsset);
			strData+=strSeparator;
			strData+=vResult[i].szUpdateDate;
			
			strData+=strnewline;

		}
		RISKDATA_LOG("%s",strData.c_str());
	}
}
//风控强平下单记录
void CSvrDBOprImpl::SaveForceCloseOrder2File(PlatformStru_InputOrder& order, InputOrderKey& lKey, bool bForceCloseType, std::string strRiskName)
{
	std::string strData="";
	std::string strnewline = "\r\n";
	strData+="风控强平下单记录";
	strData+=strnewline;
	std::string strSeparator="|";

	strData+=order.BrokerID;
	strData+=strSeparator;
	strData+=order.InvestorID;
	strData+=strSeparator;
	strData+=order.InstrumentID;
	strData+=strSeparator;
	strData+=order.OrderRef;
	strData+=strSeparator;
	strData+=order.UserID;
	strData+=strSeparator;
	strData+=order.OrderPriceType;
	strData+=strSeparator;
	strData+=order.Direction;
	strData+=strSeparator;
	strData+=order.CombOffsetFlag;
	strData+=strSeparator;
	strData+=order.CombHedgeFlag;
	strData+=strSeparator;
	strData+=Double2String(order.LimitPrice);
	strData+=strSeparator;
	strData+=Int2String(order.VolumeTotalOriginal);
	strData+=strSeparator;
	strData+=order.TimeCondition;
	strData+=strSeparator;
	strData+=order.GTDDate;
	strData+=strSeparator;
	strData+=order.VolumeCondition;
	strData+=strSeparator;
	
	strData+=Int2String(order.MinVolume);
	strData+=strSeparator;
	strData+=order.ContingentCondition;
	strData+=strSeparator;
	strData+=Double2String(order.StopPrice);
	strData+=strSeparator;
	strData+=order.ForceCloseReason;
	strData+=strSeparator;
	strData+=Int2String(order.IsAutoSuspend);
	strData+=strSeparator;
	strData+=order.BusinessUnit;
	strData+=strSeparator;
	strData+=order.RequestID;
	strData+=strSeparator;
	strData+=Int2String(order.UserForceClose);
	strData+=strSeparator;
	strData+=order.OrderClass;
	strData+=strSeparator;
	strData+=order.strAccount;
	strData+=strSeparator;
	strData+=order.strLocalRequestID;
	strData+=strSeparator;
	strData+=order.strLocalRefID;
	strData+=strSeparator;
	strData+=order.strExchange;
	strData+=strSeparator;
	strData+=Int2String(lKey.nFrontID);
	strData+=strSeparator;
	strData+=Int2String(lKey.nSessionID);
	strData+=strSeparator;
	strData+=lKey.szOrderRef;
	strData+=strSeparator;
	strData+=Int2String(bForceCloseType);
	strData+=strSeparator;
	strData+=strRiskName;

	strData+=strnewline;


	RISKDATA_LOG("%s",strData.c_str());
}
//风控强平撤单
void CSvrDBOprImpl::SaveForceCloseOrderAction2File(PlatformStru_InputOrderAction& order, bool bForceCloseType, std::string strRiskName)
{
	std::string strData="";
	std::string strnewline = "\r\n";
	strData+="风控强平撤单记录";
	strData+=strnewline;
	std::string strSeparator="|";

	strData+=order.Thost.BrokerID;
	strData+=strSeparator;
	strData+=order.Thost.InvestorID;
	strData+=strSeparator;
	strData+=order.Thost.OrderActionRef;
	strData+=strSeparator;
	strData+=order.Thost.OrderRef;
	strData+=strSeparator;
	strData+=Int2String(order.Thost.RequestID);
	strData+=strSeparator;
	strData+=Int2String(order.Thost.FrontID);
	strData+=strSeparator;
	strData+=Int2String(order.Thost.SessionID);
	strData+=strSeparator;
	strData+=order.Thost.ExchangeID;
	strData+=strSeparator;
	strData+=order.Thost.OrderSysID;
	strData+=strSeparator;
	strData+=order.Thost.ActionFlag;
	strData+=strSeparator;
	strData+=Double2String(order.Thost.LimitPrice);
	strData+=strSeparator;
	strData+=Int2String(order.Thost.VolumeChange);
	strData+=strSeparator;
	strData+=order.Thost.UserID;
	strData+=strSeparator;
	strData+=order.Thost.InstrumentID;
	strData+=strSeparator;
	strData+=strRiskName;
	strData+=strSeparator;
	strData+=Int2String(bForceCloseType);
	strData+=strSeparator;


	strData+=strnewline;


	RISKDATA_LOG("%s",strData.c_str());
}
//报单审核
void CSvrDBOprImpl::SaveVertifyOrder2File(SVerisyOrder& order)
{
	std::string strData="";
	std::string strnewline = "\r\n";
	strData+="报单审核";
	strData+=strnewline;
	std::string strSeparator="|";
	strData+=Int2String(order.nVerifyUser);
	strData+=strSeparator;
	strData+=Int2String(order.nResult);
	strData+=strSeparator;
	strData+=order.orderKey.Account;
	strData+=strSeparator;
	strData+=order.orderKey.InstrumentID;
	strData+=strSeparator;
	strData+=Int2String(order.orderKey.FrontID);
	strData+=strSeparator;
	strData+=Int2String(order.orderKey.SessionID);
	strData+=strSeparator;
	strData+=order.orderKey.OrderRef;


	strData+=strnewline;


	RISKDATA_LOG("%s",strData.c_str());
}



/*
bool CSvrDBOprImpl::UpdateStrategy(SStrategy& strategy, int nType)
{
	CGuard guard(&g_mutex);
	int nErrorCode = CF_ERROR_SUCCESS;
	if ( !IsConnected() && !Conncect())
	{
		nErrorCode = CF_ERROR_DATABASE_OTHER_ERROR;
		return false;
	}

	char szBuffer[MAX_SQL_LENGTH];

	try
	{
		if(nType == 0)
			sprintf(szBuffer, "select FILECONTENT from STRATEGY_STRATEGY  where NAME = '%s')",strategy.szName );
		else 
			sprintf(szBuffer, "select SOURCE from STRATEGY_STRATEGY  where NAME = '%s' )", strategy.szName );

		m_pStmt = m_pCon->createStatement( szBuffer );		
		m_pRes = m_pStmt->executeQuery();
		if( m_pRes->next())
		{
			oracle::occi::Blob blob = m_pRes->getBlob(1);
			blob.open(OCCI_LOB_READWRITE);
			if(nType == 0)
			{
				WriteFileToBlob(blob, strategy.pFileContent, strategy.nFileContentLength);
			}
			else
			{
				WriteFileToBlob(blob, strategy.pSource, strategy.nSourceLength);
			}
			blob.close();
		}
		m_pStmt->closeResultSet(m_pRes);

		m_pCon->commit();
		m_pCon->terminateStatement(m_pStmt);
	}catch(oracle::occi::SQLException &e)
	{
		RollBack();
		std::cout<<e.what()<<endl;
		std::cout<<szBuffer<<endl;
		nErrorCode = GetErrorCode(e.what());
		WriteLog(szBuffer, e.what());
		return false;
	}

	return true;
}
bool CSvrDBOprImpl::SelectStrategy(std::vector<SStrategy>& vecStrategy)
{
	CGuard guard(&g_mutex);
	int nErrorCode = CF_ERROR_SUCCESS;
	if ( !IsConnected() && !Conncect())
	{
		nErrorCode = CF_ERROR_DATABASE_OTHER_ERROR;
		return false;
	}

	char szBuffer[MAX_SQL_LENGTH];

	try
	{		
		sprintf(szBuffer, "select NAME,UPLOADER,FILECONTENT,SOURCENAME,SOURCE,NOTE from STRATEGY_STRATEGY t");
		m_pStmt = m_pCon->createStatement( szBuffer );		
		m_pRes = m_pStmt->executeQuery();
		while( m_pRes->next())
		{
			SStrategy strategy;
			std::string strValue = m_pRes->getString(1);
			strcpy(strategy.szName, strValue.c_str());

			std::string UPLOADER = m_pRes->getString(2);
			strcpy(strategy.szUploader, UPLOADER.c_str());

			Blob blob = m_pRes->getBlob(3);
			blob.open(OCCI_LOB_READONLY);

			std::string strFileFolder = "Strategy";
			std::string strFileName   = strValue;
			GetFolderFileName(strFileName, strFileFolder);
			ReadBlobToFile(blob, strFileName);

			std::string SOURCENAME = m_pRes->getString(4);
			strcpy(strategy.szSourceName, SOURCENAME.c_str());

			strFileFolder = "Strategy";
			strFileName   = SOURCENAME;
			GetFolderFileName(strFileName, strFileFolder);
			ReadBlobToFile(blob, strFileName);

			std::string NOTE = m_pRes->getString(4);
			strcpy(strategy.szNote, NOTE.c_str());

			vecStrategy.push_back(strategy);
		}
		m_pStmt->closeResultSet(m_pRes);

		m_pCon->commit();
		m_pCon->terminateStatement(m_pStmt);
	}catch(oracle::occi::SQLException &e)
	{
		RollBack();
		std::cout<<e.what()<<endl;
		std::cout<<szBuffer<<endl;
		nErrorCode = GetErrorCode(e.what());
		WriteLog(szBuffer, e.what());
		return false;
	}

	return true;

}
void CSvrDBOprImpl::WriteFileToBlob(oracle::occi::Blob& blob, char *pData, int nLength)
{
	if(pData == NULL)
		return;
	Stream *strm = blob.getStream();
	strm->writeBuffer(pData, nLength);
	char*c = (char*)"";
	strm->writeLastBuffer(c, 0);
	blob.closeStream();
}
void CSvrDBOprImpl::ReadBlobToFile(oracle::occi::Blob& blob, std::string strFileName)
{
	FILE* pFile = fopen(strFileName.c_str(), "ab+" );
	//
	Stream *instream = blob.getStream();

	char bVal[CHUNKSIZE+1];
	memset(bVal, 0 , CHUNKSIZE+1);
	long llswrite = 0;
	long size = blob.length();
	int n;
	n = (size%CHUNKSIZE) ==0?(size/CHUNKSIZE):(size/CHUNKSIZE+1);
	for(int i =0; i<n; i++)
	{
		llswrite = instream->readBuffer(bVal, CHUNKSIZE);
		fwrite(bVal, llswrite, 1, pFile);
	}
	fclose(pFile);
	blob.close();
}
void CSvrDBOprImpl::GetFolderFileName(std::string& strFilePath,const std::string& folder)
{
	char szLocalPath[256];
	memset(szLocalPath, 0, 256);
	GetModuleFileNameA( NULL, szLocalPath, 256 );
	std::string strSystemPath( szLocalPath );
	int nPos = strSystemPath.rfind( '\\' );
	if ( -1 != nPos )
	{
		strSystemPath = strSystemPath.substr( 0, nPos + 1 );
		if(!folder.empty())
		{
			if(std::string::npos==folder.rfind("\\"))
				strSystemPath+=folder+"\\";
			else
				strSystemPath+=folder;
			CreateDirectoryA(strSystemPath.c_str(),NULL);
		}
		strFilePath = strSystemPath + strFilePath;
	}	
}
bool CSvrDBOprImpl::IsIndexExist(std::string strName)
{
	bool bExist =false;
	CGuard guard(&g_mutex);

	if ( !IsConnected() && !Conncect())
	{
		return false;
	}

	char szBuffer[MAX_SQL_LENGTH];
	memset(szBuffer, 0, sizeof(szBuffer));
	try
	{
		sprintf(szBuffer, "select NAME from STRATEGY_STRATEGY t where NAME = '%s'", strName.c_str());
		m_pStmt = m_pCon->createStatement( szBuffer );
		m_pRes = m_pStmt->executeQuery();
		if( m_pRes->next())
		{
			bExist =true;
		}
		m_pStmt->closeResultSet(m_pRes);
		m_pCon->terminateStatement(m_pStmt);
	}
	catch(oracle::occi::SQLException &e)
	{
		std::cout<<e.what()<<endl;
		std::cout<<szBuffer<<endl;

		WriteLog(szBuffer, e.what());
		return bExist;
	}

	return bExist;
}
bool CSvrDBOprImpl::SaveIndex(SStrategyInstance& strategyIndex, int& nErrorCode )
{
	CGuard guard(&g_mutex);
	nErrorCode = CF_ERROR_SUCCESS;
	if ( !IsConnected() && !Conncect())
	{
		nErrorCode = CF_ERROR_DATABASE_OTHER_ERROR;
		return false;
	}

	char szBuffer[MAX_SQL_LENGTH];

	try
	{
		m_pStmt = m_pCon->createStatement();

		sprintf(szBuffer, "insert into STRATEGY_INDEX values('%s','%s', EMPTY_BLOB(), '%s', SYSDATE)",
			strategyIndex.szIndexName,strategyIndex.szUploader, strategyIndex.szNote);
		m_pStmt->execute( szBuffer );

		m_pCon->commit();
		m_pCon->terminateStatement(m_pStmt);
	}catch(oracle::occi::SQLException &e)
	{
		RollBack();
		std::cout<<e.what()<<endl;
		std::cout<<szBuffer<<endl;
		nErrorCode = GetErrorCode(e.what());
		WriteLog(szBuffer, e.what());
		return false;
	}

	return true;
}
bool CSvrDBOprImpl::UpdateIndex(SStrategyInstance& Index)
{
	CGuard guard(&g_mutex);
	int nErrorCode = CF_ERROR_SUCCESS;
	if ( !IsConnected() && !Conncect())
	{
		nErrorCode = CF_ERROR_DATABASE_OTHER_ERROR;
		return false;
	}

	char szBuffer[MAX_SQL_LENGTH];

	try
	{		
		sprintf(szBuffer, "select FILECONTENT from STRATEGY_INDEX  where NAME = '%s')",Index.szName );
		m_pStmt = m_pCon->createStatement( szBuffer );		
		m_pRes = m_pStmt->executeQuery();
		if( m_pRes->next())
		{
			oracle::occi::Blob blob = m_pRes->getBlob(1);
			blob.open(OCCI_LOB_READWRITE);
			WriteFileToBlob(blob, Index.pFileContent, Index.nFileContentLength);	
			blob.close();
		}
		m_pStmt->closeResultSet(m_pRes);

		m_pCon->commit();
		m_pCon->terminateStatement(m_pStmt);
	}catch(oracle::occi::SQLException &e)
	{
		RollBack();
		std::cout<<e.what()<<endl;
		std::cout<<szBuffer<<endl;
		nErrorCode = GetErrorCode(e.what());
		WriteLog(szBuffer, e.what());
		return false;
	}

	return true;
}*/


bool CSvrDBOprImpl::SaveStockQuot(const int nType,
                                  const std::string& nsTableName,
                                  const std::string& strTime,
                                  const vector< PlatformStru_DepthMarketData >& vData,
                                  int& nErrorCode )
{

	CGuard guard(&g_mutex);
	//一般方法
	//char szBuffer[MAX_SQL_LENGTH];
	//for(vector< PlatformStru_DepthMarketData >::const_iterator it = vData.begin(); it!= vData.end(); it++)
	//{
	//	memset(szBuffer, 0, sizeof(szBuffer));
	//	sprintf(szBuffer, 
	//		"update %s t set t.OPENPRICE = %f , t.LASTPRICE = %f , t.HIGHESTPRICE = %f , t.LOWESTPRICE = %f where t.NEWVOLUME = %d and t.INSTRUMENTID = '%s'",
	//		nsTableName.c_str(),it->OpenPrice,it->LastPrice, it->HighestPrice,it->LowestPrice,it->NewVolume,it->InstrumentID);
	//	int nNum = 0;
	//	if ( !ExcuteUpdate(szBuffer, nNum, nErrorCode))
	//	{
	//		printf("ExcuteUpdate %s Error\n",szBuffer);
	//	}
	//}
	//
	//return true;

	////批量全部删除再插入方法
	//PlatformStru_DepthMarketData lInfo;
	//std::vector<ColumeData> ltest;
	//PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcDateType),(int)&lInfo.TradingDay - (int)&lInfo);
	//PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(InstrumentIDType),(int)&lInfo.InstrumentID - (int)&lInfo);
	//PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcExchangeIDType),(int)&lInfo.ExchangeID - (int)&lInfo);
	//PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcExchangeInstIDType),(int)&lInfo.ExchangeInstID - (int)&lInfo);
	//PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.LastPrice - (int)&lInfo);
	//PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.PreSettlementPrice - (int)&lInfo);
	//PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.PreClosePrice - (int)&lInfo);
	//PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcLargeVolumeType),(int)&lInfo.PreOpenInterest - (int)&lInfo);
	//PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.OpenPrice- (int)&lInfo);
	//PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.HighestPrice - (int)&lInfo);
	//PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.LowestPrice - (int)&lInfo);
	//PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.Volume - (int)&lInfo);
	//PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.Turnover - (int)&lInfo);
	//PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcLargeVolumeType),(int)&lInfo.OpenInterest - (int)&lInfo);
	//PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.ClosePrice - (int)&lInfo);
	//PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.SettlementPrice- (int)&lInfo);
	//PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.UpperLimitPrice- (int)&lInfo);
	//PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.LowerLimitPrice - (int)&lInfo);
	//PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcRatioType),(int)&lInfo.PreDelta - (int)&lInfo);
	//PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcRatioType),(int)&lInfo.CurrDelta - (int)&lInfo);
	//PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcTimeType),(int)&lInfo.UpdateTime - (int)&lInfo);
	//PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcMillisecType),(int)&lInfo.UpdateMillisec - (int)&lInfo);
	//PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.BidPrice1 - (int)&lInfo);
	//PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.BidVolume1 - (int)&lInfo);
	//PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.AskPrice1 - (int)&lInfo);
	//PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcSettlementIDType),(int)&lInfo.AskVolume1 - (int)&lInfo);
	//PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.BidPrice2 - (int)&lInfo);
	//PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.BidVolume2 - (int)&lInfo);
	//PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.AskPrice2 - (int)&lInfo);
	//PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.AskVolume2 - (int)&lInfo);
	//PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.BidPrice3 - (int)&lInfo);
	//PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.BidVolume3 - (int)&lInfo);
	//PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.AskPrice3 - (int)&lInfo);
	//PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.AskVolume3 - (int)&lInfo);
	//PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.BidPrice4 - (int)&lInfo);
	//PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.BidVolume4 - (int)&lInfo);
	//PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.AskPrice4 - (int)&lInfo);
	//PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.AskVolume4 - (int)&lInfo);
	//PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.BidPrice5 - (int)&lInfo);
	//PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.BidVolume5 - (int)&lInfo);
	//PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.AskPrice5 - (int)&lInfo);
	//PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.AskVolume5 - (int)&lInfo);
	//PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.AveragePrice - (int)&lInfo);
	//PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.BidPrice6 - (int)&lInfo);
	//PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.BidVolume6 - (int)&lInfo);
	//PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.AskPrice6 - (int)&lInfo);
	//PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.AskVolume6 - (int)&lInfo);
	//PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.BidPrice7 - (int)&lInfo);
	//PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.BidVolume7 - (int)&lInfo);
	//PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.AskPrice7 - (int)&lInfo);
	//PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.AskVolume7 - (int)&lInfo);
	//PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.BidPrice8 - (int)&lInfo);
	//PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.BidVolume8 - (int)&lInfo);
	//PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.AskPrice8 - (int)&lInfo);
	//PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.AskVolume8 - (int)&lInfo);
	//PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.BidPrice9 - (int)&lInfo);
	//PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.BidVolume9 - (int)&lInfo);
	//PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.AskPrice9 - (int)&lInfo);
	//PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.AskVolume9 - (int)&lInfo);
	//PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.BidPrice10 - (int)&lInfo);
	//PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.BidVolume10 - (int)&lInfo);
	//PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.AskPrice10 - (int)&lInfo);
	//PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.AskVolume10 - (int)&lInfo);
	//PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.NewVolume - (int)&lInfo);
 //   
	//std::string lsDeleteSql = "truncate table " + nsTableName; //"delete from " + nsTableName + " t where t.VALIDATEDATE < " + "\'" + strTime + "\'" ;
	//
 //   return BatchInsert(	nsTableName,
	//					lsDeleteSql,
	//					strTime,
	//					ltest,
	//					vData,
	//					nErrorCode);	
	//return false;

	
	//批量全部更新再插入方法
	//先批量删除
	DeleteStruct ldInfo;
	std::vector<ColumeDataEx> ldtest;
	PushColumnDataToVectorEx(ldtest,OCCI_SQLT_STR,sizeof(InstrumentIDType),(int)&ldInfo.InstrumentID - (int)&ldInfo,"INSTRUMENTID");
	PushColumnDataToVectorEx(ldtest,OCCI_SQLT_STR,sizeof(TThostFtdcExchangeIDType),(int)&ldInfo.ExchangeID - (int)&ldInfo,"EXCHANGEID");

	std::string lsdConditionSql;
	lsdConditionSql = " T where T.InstrumentID = :v1 AND T.ExchangeID = :v2";

	vector< DeleteStruct > vDataDel;
	for (vector< PlatformStru_DepthMarketData >::const_iterator it = vData.begin(); it != vData.end(); it++)
	{
		memset(&ldInfo,0,sizeof(DeleteStruct));
		strcpy(ldInfo.InstrumentID,it->InstrumentID);
		strcpy(ldInfo.ExchangeID,it->ExchangeID);
		vDataDel.push_back(ldInfo);
	}

	if(! BatchDelete(	nsTableName,
						lsdConditionSql,
						ldtest,
						vDataDel,
						nErrorCode))
		return false;

	//再批量插入
	PlatformStru_DepthMarketData lInfo;
	std::vector<ColumeData> ltest;
	PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcDateType),(int)&lInfo.TradingDay - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(InstrumentIDType),(int)&lInfo.InstrumentID - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcExchangeIDType),(int)&lInfo.ExchangeID - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcExchangeInstIDType),(int)&lInfo.ExchangeInstID - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.LastPrice - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.PreSettlementPrice - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.PreClosePrice - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcLargeVolumeType),(int)&lInfo.PreOpenInterest - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.OpenPrice- (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.HighestPrice - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.LowestPrice - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.Volume - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcMoneyType),(int)&lInfo.Turnover - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcLargeVolumeType),(int)&lInfo.OpenInterest - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.ClosePrice - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.SettlementPrice- (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.UpperLimitPrice- (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.LowerLimitPrice - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcRatioType),(int)&lInfo.PreDelta - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcRatioType),(int)&lInfo.CurrDelta - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCI_SQLT_STR,sizeof(TThostFtdcTimeType),(int)&lInfo.UpdateTime - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcMillisecType),(int)&lInfo.UpdateMillisec - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.BidPrice1 - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.BidVolume1 - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.AskPrice1 - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcSettlementIDType),(int)&lInfo.AskVolume1 - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.BidPrice2 - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.BidVolume2 - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.AskPrice2 - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.AskVolume2 - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.BidPrice3 - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.BidVolume3 - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.AskPrice3 - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.AskVolume3 - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.BidPrice4 - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.BidVolume4 - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.AskPrice4 - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.AskVolume4 - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.BidPrice5 - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.BidVolume5 - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.AskPrice5 - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.AskVolume5 - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.AveragePrice - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.BidPrice6 - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.BidVolume6 - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.AskPrice6 - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.AskVolume6 - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.BidPrice7 - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.BidVolume7 - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.AskPrice7 - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.AskVolume7 - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.BidPrice8 - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.BidVolume8 - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.AskPrice8 - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.AskVolume8 - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.BidPrice9 - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.BidVolume9 - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.AskPrice9 - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.AskVolume9 - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.BidPrice10 - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.BidVolume10 - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIBDOUBLE,sizeof(TThostFtdcPriceType),(int)&lInfo.AskPrice10 - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.AskVolume10 - (int)&lInfo);
	PushColumnDataToVector(ltest,OCCIINT,sizeof(TThostFtdcVolumeType),(int)&lInfo.NewVolume - (int)&lInfo);

	std::string nDeleteSql;
	nDeleteSql = "";

	return BatchInsert(	nsTableName,
						nDeleteSql,
						strTime,
						ltest,
						vData,
						nErrorCode);

	return false;
}