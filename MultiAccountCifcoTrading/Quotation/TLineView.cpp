#include "StdAfx.h"

#include "../inc/Quotation/TLineView.h"

#include "wx/dcmirror.h"
#define  ID_PHRASE_1MIN				23114
#define  ID_PHRASE_5MIN				23115
#define  ID_PHRASE_15MIN			23116
#define  ID_PHRASE_30MIN			23117
#define  ID_PHRASE_60MIN			23118
#define  ID_PHRASE_DAY				23119
#define  ID_PHRASE_WEEK				23120
#define  ID_PHRASE_MONTH			23121

#define  ID_CROSS_SHOW				33134
#define  ID_RETURN_KLINE			33135
#define  ID_RETURN_BAOJIABIAO2		33136

DEFINE_EVENT_TYPE(wxEVT_TLINEVIEW_MOUSEMOVEHOOK)
HHOOK g_hMouseTLine=NULL; //鼠标的钩子过程
wxPanel *g_pWndTLine = NULL;

DEFINE_EVENT_TYPE(wxEVT_RETURN_KLINE)
DEFINE_EVENT_TYPE(wxEVT_RETURN_BAOJIABIAO2)
IMPLEMENT_CLASS(CTLineView, wxPanel)
CTLineView::CTLineView(void)
{
	m_pTLineCtrl	= NULL;
	m_pTVolumeCtrl	= NULL;
	m_pTTechIndex	= NULL;
	m_pTScalesCtrl  = NULL;
	m_splitter		= NULL;
	m_splitterTop   = NULL;
	m_splitterBottom= NULL;


	m_pFData = NULL;
}

CTLineView::~CTLineView(void)
{
	UnhookWindowsHookEx(g_hMouseTLine);
	ClearMemory();
	m_pFData = NULL;
}

BEGIN_EVENT_TABLE(CTLineView, wxPanel)

EVT_PAINT(CTLineView::OnPaint)
EVT_SIZE(CTLineView::OnSize)
EVT_MENU(ID_CROSS_SHOW,	CrossShow)
EVT_MENU(ID_RETURN_KLINE,	OnReturnKLine)
EVT_MENU(ID_RETURN_BAOJIABIAO2,	OnReturnBaoJiaBiao)

EVT_COMMAND(wxID_ANY, wxEVT_TCtrl_KeyDown, CTLineView::OnFuncKeyDown)
EVT_COMMAND(wxID_ANY, wxEVT_TCtrl_LBUTTONDOWN, CTLineView::OnFuncLButtonDown)
EVT_COMMAND(wxID_ANY, wxEVT_TCtrl_RBUTTONUP, CTLineView::OnFuncRButtonUp)
EVT_COMMAND(wxID_ANY, wxEVT_TCtrl_MOUSELEAVE, CTLineView::OnFuncMouseLeave)
EVT_COMMAND(wxID_ANY, wxEVT_TCtrl_MOUSEENTER, CTLineView::OnFuncMouseEnter)
EVT_COMMAND(wxID_ANY, wxEVT_TCtrl_LEFTDBCLICK, CTLineView::OnFuncLDBClick)

EVT_COMMAND(wxID_ANY, wxEVT_TLINEVIEW_MOUSEMOVEHOOK, CTLineView::OnFuncMouseMoveHook)
END_EVENT_TABLE()


void CTLineView::ClearMemory()
{


}
void CTLineView::SetFData(CFData *pFData)
{
	m_pFData = pFData;
	if(m_pTLineCtrl)
	{
		m_pTLineCtrl->SetFData(pFData);
		m_pTLineCtrl->SetVolumeMultiple(pFData->m_VolumeMultiple);
		m_pTLineCtrl->SetLastClosePrice(pFData->m_PreSettlementPrice);
		m_pTLineCtrl->SetScales(pFData->m_vecTimeScales);//刻度必须先调用
		m_pTLineCtrl->SetFuture(pFData->m_ExchangeName, pFData->m_ContractName, pFData->m_ContractDate);	
		m_pTLineCtrl->SetVectorData(pFData->m_vecTLine);
		m_pTLineCtrl->SetMaxMinValue(pFData->m_fMaxValue, pFData->m_fMinValue);
	}	
	if(m_pTVolumeCtrl)
	{	
		m_pTVolumeCtrl->SetFData(pFData);
		m_pTVolumeCtrl->SetScales(pFData->m_vecTimeScales);
		m_pTVolumeCtrl->SetFuture(pFData->m_ExchangeName, pFData->m_ContractName, pFData->m_ContractDate);			
		m_pTVolumeCtrl->SetVectorData(pFData->m_vecTLine);
	}
	if(m_pTScalesCtrl)
	{	
		m_pTScalesCtrl->SetScales(pFData->m_vecTimeScales);
		m_pTScalesCtrl->SetFuture(pFData->m_ExchangeName, pFData->m_ContractName, pFData->m_ContractDate);
		m_pTScalesCtrl->SetVectorData(pFData->m_vecTLine);
	}	
}
LRESULT CALLBACK MouseProcTLine( int nCode,WPARAM wParam, LPARAM lParam)
{
	if(wParam == WM_MOUSEMOVE)
	{
		MOUSEHOOKSTRUCT *mhookstruct; //鼠标HOOK结构体 
		mhookstruct = (MOUSEHOOKSTRUCT*)lParam; 
		POINT pt = mhookstruct->pt; 
		if(g_pWndTLine != NULL)
		{		
			wxCommandEvent myEvent(wxEVT_TLINEVIEW_MOUSEMOVEHOOK);
			myEvent.SetClientData((void*)&pt);
			g_pWndTLine->ProcessEvent(myEvent);
		}	
	}
	return CallNextHookEx(g_hMouseTLine,nCode,wParam,lParam);
}



wxPanel* CTLineView::GetControl(TTYPE TTYPE)
{
	wxPanel* pWnd = NULL;	
	switch(TTYPE)
	{
	case TTYPE_TLINE:
		pWnd = m_pTLineCtrl;
		break;
	case TTYPE_VOLUME:
		pWnd = m_pTVolumeCtrl;
		break;
	case TTYPE_TECHINDEX:

		break;
	case TTYPE_SCALES:
		pWnd = m_pTScalesCtrl;
		break;

	}
	return pWnd;
}
void CTLineView::CfgShow(long lKLineType)
{
	m_lKLineType = lKLineType;

	m_splitter = new KSplitterWindow(this,wxID_ANY,wxDefaultPosition,wxDefaultSize,wxSP_3D|wxNO_BORDER|wxSP_NOSASH);//wxSP_NOSASH这里主要为了刻度栏不可以拉大小，以后窗口不同的分隔形式的话，就需要改
	m_splitter->SetSashGravity(1.0);	
	m_splitter->SetSashSize(1);

	m_splitterTop = new KSplitterWindow(m_splitter,wxID_ANY,wxDefaultPosition,wxDefaultSize,wxSP_3D|wxNO_BORDER);
	m_splitterTop->SetSashGravity(1.0);
	m_splitterTop->SetSashSize(1);
	
	m_splitterBottom = new KSplitterWindow(m_splitter,wxID_ANY,wxDefaultPosition,wxDefaultSize,wxSP_3D|wxNO_BORDER);
	m_splitterBottom->SetSashGravity(1.0);
	m_splitterBottom->SetSashSize(1);
	

	wxRect rtClient = GetClientRect();
	if( (lKLineType & TTYPE_TLINE) == TTYPE_TLINE)
	{
		wxSize sz(0,0);
		m_pTLineCtrl = new CTLine;
		m_pTLineCtrl->Create(m_splitterTop, 3111, wxPoint(0,0), sz);		
	}
	if( (lKLineType & TTYPE_VOLUME) == TTYPE_VOLUME)
	{
		wxSize sz(0,0);
		m_pTVolumeCtrl = new CTVolume;
		m_pTVolumeCtrl->Create(m_splitterTop, 3112, wxPoint(0,0), sz);	
	}
	if( (lKLineType & TTYPE_TECHINDEX) ==  TTYPE_TECHINDEX)
	{

	}
	if( (lKLineType & TTYPE_SCALES) ==  TTYPE_SCALES)
	{
		wxSize sz(0,0);
		m_pTScalesCtrl = new CTScales;
		m_pTScalesCtrl->Create(m_splitterBottom, 3113,  wxPoint(0,0), sz);
	}
	m_splitter->SetSize(rtClient.width, rtClient.height);

	if(m_pTLineCtrl && m_pTVolumeCtrl)
	{
		m_splitterTop->SplitHorizontally(m_pTLineCtrl, m_pTVolumeCtrl, rtClient.GetHeight()/2);//k线占1/2
		m_splitterTop->SetSashGravity(0.5);
	}

	else if(m_pTLineCtrl)
		m_splitterTop->Initialize(m_pTLineCtrl);
	else if(m_pTVolumeCtrl)
		m_splitterTop->Initialize(m_pTVolumeCtrl);	

	if(m_pTTechIndex && m_pTScalesCtrl)
		m_splitterBottom->SplitHorizontally(m_pTTechIndex, m_pTScalesCtrl,-20);//时间轴20个像素
	else if(m_pTTechIndex)
		m_splitterBottom->Initialize(m_pTTechIndex);
	else if(m_pTScalesCtrl)
		m_splitterBottom->Initialize(m_pTScalesCtrl);



	int nBottomHeight = 0;	
	if( (m_lKLineType & TTYPE_TECHINDEX) ==  TTYPE_TECHINDEX)
	{
		nBottomHeight = rtClient.GetHeight()/4 - 20;	
	}
	if( (m_lKLineType & TTYPE_SCALES) ==  TTYPE_SCALES)
	{
		nBottomHeight += 20;	
	}

	m_splitter->SplitHorizontally(m_splitterTop, m_splitterBottom, -nBottomHeight);	
}
wxWindow* CTLineView::CreateDlg(wxWindow *parent,
									 wxWindowID winid ,
									 const wxPoint& pos ,
									 const wxSize& size,
									 long style,
									 const wxString& name )
{		
	style |= wxWANTS_CHARS|wxTE_PROCESS_ENTER|wxCLIP_CHILDREN;
	bool bReturn = wxPanel::Create(parent,winid,pos, size,	style|wxCLIP_CHILDREN|wxCLIP_SIBLINGS, name);			
	if(!bReturn)
		return NULL;

	g_pWndTLine = this;
	g_hMouseTLine = SetWindowsHookEx(WH_MOUSE,MouseProcTLine,NULL,GetCurrentThreadId());

	SetBackgroundStyle(wxBG_STYLE_CUSTOM);
	return this;
}


void CTLineView::OnSize(wxSizeEvent& event)
{
	if(m_splitter == NULL)
		return;
	wxRect rtClient = GetClientRect();
	m_splitter->SetSize(rtClient.width, rtClient.height);
}

void CTLineView::OnPaint (wxPaintEvent & PaintEvent)
{
	wxPaintDC dc(this);
	wxMemoryDC memdc;  
	memdc.SetLayoutDirection(dc.GetLayoutDirection());
	wxBitmap bmp;
	wxSize size = GetClientSize();
	bmp.Create(size.GetWidth(), size.GetHeight());
	memdc.SelectObject(bmp);

	size = GetClientSize();
	wxBrush brush(wxColour(RGB(192,0,110)));
	memdc.SetBrush(brush);
	memdc.DrawRectangle(0, 0, size.x, size.y);

	dc.Blit(0, 0, size.GetWidth(),size.GetHeight(),&memdc, 0, 0);

}

void CTLineView::OnFuncMouseMoveHook(wxCommandEvent& event)
{
	POINT pt = *(POINT *)event.GetClientData();
	wxPoint ptWX(pt.x, pt.y);

	wxPoint wxpt = ptWX;
	wxpt = ScreenToClient(wxpt);
	wxSize size = GetClientSize();
	if(wxpt.x > size.x-1 || wxpt.y >size.y)
		return;
	if(m_pTLineCtrl)
	{
		m_pTLineCtrl->DoMouseMove(ptWX);
	}
	if(m_pTVolumeCtrl)
	{
		m_pTVolumeCtrl->DoMouseMove(ptWX);

	}
	if(m_pTScalesCtrl)
	{
		m_pTScalesCtrl->DoMouseMove(ptWX);
	}

}
void CTLineView::OnFuncKeyDown(wxCommandEvent& event)
{
	int keyCode = event.GetInt();
	if(keyCode == VK_LEFT 
		|| keyCode == VK_RIGHT
		)
	{
		STItem&  sItem = *(STItem *)event.GetClientData();
		if(m_pTLineCtrl)
		{
			m_pTLineCtrl->SetCursel(sItem);
			m_pTLineCtrl->SetShowCross(true);
			m_pTLineCtrl->SetMouseIn(true);
			m_pTLineCtrl->Refresh(false);
			UpdateWindow((HWND)m_pTLineCtrl->GetHWND() );
		}
		if(m_pTVolumeCtrl)
		{
			m_pTVolumeCtrl->SetCursel(sItem);
			m_pTVolumeCtrl->SetShowCross(true);
			m_pTVolumeCtrl->SetMouseIn(true);
			m_pTVolumeCtrl->Refresh(false);
			UpdateWindow((HWND)m_pTVolumeCtrl->GetHWND() );
		}
		if(m_pTScalesCtrl)
		{
			m_pTScalesCtrl->SetCursel(sItem);
			m_pTScalesCtrl->SetShowCross(true);
			m_pTScalesCtrl->SetMouseIn(true);
			m_pTScalesCtrl->Refresh(false);
			UpdateWindow((HWND)m_pTScalesCtrl->GetHWND() );
		}

	}

	wxWindow* window= GetParent();	
	window->ProcessEvent(event);
}

void CTLineView::OnFuncLButtonDown(wxCommandEvent& event)
{	
	if(m_pTLineCtrl)
	{
		m_pTLineCtrl->SetMouseIn(true);
		//m_pTLineCtrl->SetShowCross(true);
		m_pTLineCtrl->Refresh(false);
		UpdateWindow((HWND)m_pTLineCtrl->GetHWND() );	 
	}
	if(m_pTVolumeCtrl)
	{
		m_pTVolumeCtrl->SetMouseIn(true);
		//m_pTVolumeCtrl->SetShowCross(true);
		m_pTVolumeCtrl->Refresh(false);
		UpdateWindow((HWND)m_pTVolumeCtrl->GetHWND() );	 
	}
	if(m_pTScalesCtrl)
	{
		m_pTScalesCtrl->SetMouseIn(true);
		//m_pTScalesCtrl->SetShowCross(true);
		m_pTScalesCtrl->Refresh(false);
		UpdateWindow((HWND)m_pTScalesCtrl->GetHWND() );	 
	}
	

	wxWindow* window= GetParent();	
	window->ProcessEvent(event);
}
void CTLineView::OnFuncMouseLeave(wxCommandEvent& event)
{
	void *pCtrl = event.GetClientData();
	if(m_pTLineCtrl && pCtrl != m_pTLineCtrl)
		m_pTLineCtrl->DoMouseLeave();
	if(m_pTVolumeCtrl && pCtrl != m_pTVolumeCtrl)
		m_pTVolumeCtrl->DoMouseLeave();
	if(m_pTScalesCtrl && pCtrl != m_pTScalesCtrl)
		m_pTScalesCtrl->DoMouseLeave();

	wxRect rt = GetClientRect();
	wxPoint pt = wxGetMousePosition();
	pt =  ScreenToClient(pt);
	if(pt.x < rt.GetWidth() && pt.y <rt.GetHeight() && pt.x >=0 && pt.y >=0)
		return;
	else
	{
		if(m_pTLineCtrl)
		{
			m_pTLineCtrl->SetMouseIn(false);
			m_pTLineCtrl->Refresh(false);
			UpdateWindow((HWND)m_pTLineCtrl->GetHWND() );	 
		}
		if(m_pTVolumeCtrl)
		{
			m_pTVolumeCtrl->SetMouseIn(false);
			m_pTVolumeCtrl->Refresh(false);
			UpdateWindow((HWND)m_pTVolumeCtrl->GetHWND() );	 
		}
		if(m_pTScalesCtrl)
		{		
			m_pTScalesCtrl->SetMouseIn(false);
			m_pTScalesCtrl->Refresh(false);
			UpdateWindow((HWND)m_pTScalesCtrl->GetHWND() );	 
		}	
	}
}
void CTLineView::OnFuncMouseEnter(wxCommandEvent& event)
{
	wxRect rt = GetClientRect();
	wxPoint pt = wxGetMousePosition();
	pt = ScreenToClient(pt);
	if(pt.x < rt.GetWidth() && pt.y <rt.GetHeight() && pt.x >=0 && pt.y >=0)
	{
		if(m_pTLineCtrl)
		{
			m_pTLineCtrl->SetMouseIn(true);
			m_pTLineCtrl->Refresh(false);
			UpdateWindow((HWND)m_pTLineCtrl->GetHWND() );	 
		}
		if(m_pTVolumeCtrl)
		{
			m_pTVolumeCtrl->SetMouseIn(true);
			m_pTVolumeCtrl->Refresh(false);
			UpdateWindow((HWND)m_pTVolumeCtrl->GetHWND() );	 
		}
		if(m_pTScalesCtrl)
		{		
			m_pTScalesCtrl->SetMouseIn(true);
			m_pTScalesCtrl->Refresh(false);
			UpdateWindow((HWND)m_pTScalesCtrl->GetHWND() );	 
		}	
	}
	else
	{
		
	}
}
void CTLineView::OnFuncRButtonUp(wxCommandEvent& event)
{
	wxSize sz(200,200);
	wxPoint pos =  wxGetMousePosition();
	pos = ScreenToClient(pos);


	wxMenu *menuPopUp = new wxMenu(wxEmptyString, wxMENU_TEAROFF);
	menuPopUp->AppendCheckItem(ID_CROSS_SHOW, _T("十字光标"), _T(""));

	menuPopUp->AppendSeparator();
	menuPopUp->AppendCheckItem(ID_RETURN_KLINE,	_T("K线图"), _T(""));
	menuPopUp->AppendCheckItem(ID_RETURN_BAOJIABIAO2,	_T("报价表"), _T(""));

	bool bShowCross = false;
	if(m_pTLineCtrl)
		bShowCross = m_pTLineCtrl->GetShowCross();
	menuPopUp->Check(ID_CROSS_SHOW, bShowCross);
	PopupMenu( menuPopUp, pos );
	delete menuPopUp;
}
void CTLineView::SetLastClosePrice(double dbPrice)
{
	//CAutoLock l(&m_CritSecVector);
	if(m_pTLineCtrl)
		m_pTLineCtrl->SetLastClosePrice(dbPrice);

	//m_dbLastdbPrice = dbPrice;
}
BOOL CTLineView::SetInTimeData()
{
	if(m_pTLineCtrl)
	{		
		m_pTLineCtrl->SetVectorData_InTime(m_pFData->m_vecTLine);	
		m_pTLineCtrl->SetMaxMinValue(m_pFData->m_fMaxValue, m_pFData->m_fMinValue);
	}
	if(m_pTVolumeCtrl)
	{		
		m_pTVolumeCtrl->SetVectorData_InTime(m_pFData->m_vecTLine);				
	}
	if(m_pTScalesCtrl)
	{	
		m_pTScalesCtrl->SetVectorData_InTime(m_pFData->m_vecTLine);				
	}

	return TRUE;
}

void CTLineView::OnFuncLDBClick(wxCommandEvent& event)
{
	wxWindow* window= GetParent();
	window->ProcessEvent(event);
}
void CTLineView::SetLastTransTime(struct tm tmTime)
{
	if(m_pFData == NULL)
		return;
	if(m_pTLineCtrl)
	{		
		m_pTLineCtrl->SetVectorData_InTime(m_pFData->m_vecTLine);			
	}
	if(m_pTVolumeCtrl)
	{		
		m_pTVolumeCtrl->SetVectorData_InTime(m_pFData->m_vecTLine);				
	}
	if(m_pTScalesCtrl)
	{	
		m_pTScalesCtrl->SetVectorData_InTime(m_pFData->m_vecTLine);				
	}

}
bool CTLineView::InitCfg(TiXmlElement *root)
{
	if(m_pTLineCtrl)
	{
		if(!m_pTLineCtrl->InitCfg(root))
		{
			return false;
		}
	}

	if(m_pTVolumeCtrl)
	{
		if(!m_pTVolumeCtrl->InitCfg(root))
		{
			return false;
		}
	}

	if(m_pTScalesCtrl)
	{
		if(!m_pTScalesCtrl->InitCfg(root))
		{
			return false;
		}
	}

	return true;
}
void CTLineView::CrossShow(wxCommandEvent& event)
{
	bool bShowCross = false;
	if(m_pTLineCtrl)
		bShowCross = m_pTLineCtrl->GetShowCross();
	 
	 if(m_pTLineCtrl)
	 {	
		  m_pTLineCtrl->SetShowCross(!bShowCross);
		  m_pTLineCtrl->Refresh(false);
		

		 UpdateWindow((HWND)m_pTLineCtrl->GetHWND() );	 
	 }
	 if(m_pTVolumeCtrl)
	 {		
		m_pTVolumeCtrl->SetShowCross(!bShowCross);
		m_pTVolumeCtrl->Refresh(false);
		UpdateWindow((HWND)m_pTVolumeCtrl->GetHWND() );	 
	 }
	 if(m_pTScalesCtrl)
	 {	
		 m_pTScalesCtrl->SetShowCross(!bShowCross);
		 m_pTScalesCtrl->Refresh(false);
		 UpdateWindow((HWND)m_pTScalesCtrl->GetHWND() );	 
	 }
	
}
void CTLineView::OnReturnKLine(wxCommandEvent& event)
{
	wxWindow* window= GetParent();
	wxCommandEvent myEvent(wxEVT_RETURN_KLINE);
	window->ProcessEvent(myEvent);
}
void CTLineView::OnReturnBaoJiaBiao(wxCommandEvent& event)
{
	wxWindow* window= GetParent();
	wxCommandEvent myEvent(wxEVT_RETURN_BAOJIABIAO2);
	window->ProcessEvent(myEvent);
}