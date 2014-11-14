
/************************************************************************
 *
 *  Ye-Kui Wang       wyk@ieee.org
 *  Juan-Juan Jiang   juanjuan_j@hotmail.com
 *  
 *  March 14, 2002
 *
 ************************************************************************/

/*
 * Disclaimer of Warranty
 *
 * These software programs are available to the user without any
 * license fee or royalty on an "as is" basis.  The developers disclaim 
 * any and all warranties, whether express, implied, or statuary, including 
 * any implied warranties or merchantability or of fitness for a particular 
 * purpose.  In no event shall the copyright-holder be liable for any incidental,
 * punitive, or consequential damages of any kind whatsoever arising from 
 * the use of these programs.
 *
 * This disclaimer of warranty extends to the user of these programs
 * and user's customers, employees, agents, transferees, successors,
 * and assigns.
 *
 * The developers does not represent or warrant that the programs furnished 
 * hereunder are free of infringement of any third-party patents.
 *
 * */


// YUVviewerDlg.cpp : implementation file
//

#include "stdafx.h"
#include "YUVviewer.h"
#include "ChildWindow.h"
#include "YUVviewerDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/*
����һЩȫ�ֺ���(����ĳ�Ա�����ⶨ���)
*/

BOOL g_bPlay;
int g_nFrameNumber = 0;
int g_nOldFrameNumber = -1000; 
BOOL g_Play = true;

int g_nStartFrame = 0;		//��ʼ֡(�����ǵ�һ֮֡��ģ��ɴ�������)
int g_nEndFrame = 10000;	//����֡(����С����֡�����ɴ��������ֵȷ��)
int g_nCurrentFrame = 0;	//��ǰ֡
BOOL g_bReversePlay = FALSE;	//���򲥷�

void getSeqName(char *inseqpath, char *seqname);

//int nImgWidth, nImgHeight;

/*
�߳���ں���
*/
UINT PlayVideo( LPVOID pParam )
{
	int i;//,j;

	BOOL bPlay = g_bPlay;
	BOOL bEof = FALSE;

	CYUVviewerDlg *pWin = (CYUVviewerDlg *)pParam;	//���̲߳���(�ṹ��)ȡ���Ի����ָ��
	UINT picsize = pWin->m_nWidth*pWin->m_nHeight;	//ͨ���Ի����ָ����ʶԻ���ĳ�Ա����
	int timespan = 1000/atoi(pWin->m_sFrameRate);	//
	
	if(g_nCurrentFrame < g_nStartFrame) g_nCurrentFrame = g_nStartFrame;
	if(g_nCurrentFrame > g_nEndFrame) g_nCurrentFrame = g_nEndFrame;

	for(i=0; i<pWin->m_iCount; i++)
	{
		pWin->m_pFile[i]->Seek(g_nCurrentFrame*picsize*3/2, SEEK_SET);
		pWin->m_pWnd[i]->nPicShowOrder = g_nCurrentFrame;
	}
	
	HANDLE hPlayTemp1 = OpenMutex(MUTEX_ALL_ACCESS,FALSE,"Play");//�������ܣ�Ϊ���е�һ����������������󴴽�һ���¾����һ��������Ҫ��ע��һ��Ҫ�� CloseHandle �رջ������������������о�����ѹرգ���ô����Ҳ��ɾ��
	
	while(g_nCurrentFrame >= g_nStartFrame && g_nCurrentFrame <= g_nEndFrame && !bEof)
	{
		DWORD t2=GetTickCount();	//�Ӳ���ϵͳ������������������elapsed���ĺ����������ķ���ֵ��DWORD��
		g_nFrameNumber = g_nCurrentFrame;//j;
 
		if ( WAIT_OBJECT_0 == WaitForSingleObject(hPlayTemp1,INFINITE) )
			ReleaseMutex( hPlayTemp1 );
		
		for(i=0; i<pWin->m_iCount; i++)
		{
			pWin->m_pFile[i]->Seek(g_nCurrentFrame*picsize*3/2, SEEK_SET);//Seek ���� ����һ�� Long���� Open ���򿪵��ļ���ָ����ǰ�Ķ�/дλ�á�

			if(picsize != pWin->m_pFile[i]->Read(pWin->m_pWnd[i]->Y,picsize))
			{
				AfxMessageBox("Get to end of file");
				bEof = TRUE;
				break;
			}
			if(1)//bColorImage) 
			{
				if(picsize/4 != pWin->m_pFile[i]->Read(pWin->m_pWnd[i]->Cb,picsize/4))
				{
					AfxMessageBox("Get to end of file");
					bEof = TRUE;
					break;
				}
				if(picsize/4 != pWin->m_pFile[i]->Read(pWin->m_pWnd[i]->Cr,picsize/4))
				{
					AfxMessageBox("Get to end of file");
					bEof = TRUE;
					break;
				}
			}

			pWin->m_pWnd[i]->InvalidateRect (NULL,FALSE);
			pWin->m_pWnd[i]->UpdateWindow ();
			pWin->m_pWnd[i]->nPicShowOrder=g_nCurrentFrame;
		}

		if(g_bReversePlay == FALSE)
			g_nCurrentFrame++;
		else 
			g_nCurrentFrame--;

		int t1=GetTickCount()-t2;
		if(t1 < timespan) 
			Sleep(timespan - t1); // sleep time in milliseconds
	}
	
	pWin->m_pWinThread = NULL;
	AfxEndThread(0);

	return 1;

}

void CYUVviewerDlg::OnOrder() 
{
	if(g_bReversePlay == FALSE)
	{
		m_buttonOrder.SetWindowText("��ͨ����");
		g_bReversePlay = TRUE;		//Reverse:����
	}
	else
	{
		m_buttonOrder.SetWindowText("����");
		g_bReversePlay = FALSE;
	}
}

/*
����/��ͣ��ť
*/
void CYUVviewerDlg::OnPauseplay() 
{

	UpdateData(TRUE);

	g_nStartFrame = m_nFrameFrom;
	if(m_nFrameTo != 0) g_nEndFrame = m_nFrameTo;
	else g_nEndFrame = 10000;

	// create a new thread
	// ���� һ�� �� �߳�
	if (m_bPlay)	//m_bPlay:��?
	{
		m_buttonPausePlay.SetWindowText("��ͣ");	//���ð�ť�ϵ��ı�
		m_bPlay = false;	//���ñ�ʶ
		g_Play = true;
	}
	else			//m_bPlay:��?
	{
		m_buttonPausePlay.SetWindowText("����");	//���ð�ť�ϵ��ı�
		m_bPlay = true;
	}

	char chTitle[10];								//char���飬���ǳ�˵��C�����ַ���
	m_buttonPausePlay.GetWindowText(chTitle,10);	//ȡ�ð�ť�ϵ��ı�
	hPlayTemp = NULL;
	hPlayTemp=OpenMutex(MUTEX_ALL_ACCESS,FALSE,"����");
	if ( strcmp( chTitle,"����" ) == 0 )			//�ַ����Ƚ�
	{
		WaitForSingleObject( hPlayTemp,0);//WaitForSingleObject�����������hHandle�¼����ź�״̬����ĳһ�߳��е��øú���ʱ���߳���ʱ��������ڹ����dwMilliseconds�����ڣ��߳����ȴ��Ķ����Ϊ���ź�״̬����ú����������أ������ʱʱ���Ѿ�����dwMilliseconds���룬��hHandle��ָ��Ķ���û�б�����ź�״̬�������������ء�
		
	}
	else
		ReleaseMutex(hPlayTemp);	//�ͷ����߳�ӵ�е�һ��������

	if ( m_pWinThread == NULL)
		m_pWinThread = AfxBeginThread( (AFX_THREADPROC)PlayVideo , (void*)this);//���ڴ����������̣߳�����1Ϊ�߳���ں���������2Ϊ������һ��Ϊ�ṹ���ָ�룬���Դ��ݶ����Ϣ���߳��ڲ�

}

void CYUVviewerDlg::OnCloseall() 
{
	int i;

	for(i=0; i<m_iCount; i++)		
	{
		if(m_pFile[i])
			m_pFile[i]->Close();		//�ر������ļ�
		if(m_pWnd[i])
			m_pWnd[i]->DestroyWindow();	//���ٲ��Ŵ���
	}
	m_iCount = 0;						//�򿪵��ļ���=0

	g_nFrameNumber = 0;					//
	g_nOldFrameNumber = -1000;			//
	g_Play = true;						//

	g_nStartFrame = 0;					//��ʼ֡
	g_nEndFrame = 10000;				//���֡��
	g_nCurrentFrame = 0;				//��ǰ���ŵ�֡���(0��1��2��3...)
	g_bReversePlay = FALSE;				//���򲥷ű��
}

void CYUVviewerDlg::OnCancel() //ͨ�����ϽǵĲ���˳���Ҳ�������Ӧ����
{
	int i;

	for(i=0; i<m_iCount; i++)		//m_iCountΪ�򿪵��ļ�����
	{
		if(m_pFile[i])
			m_pFile[i]->Close();	//һ�����Ĺر��ļ�
		if(m_pWnd[i])
			m_pWnd[i]->DestroyWindow();	//���ٲ��Ŵ���
	}
	
	CDialog::OnCancel();
}

void CYUVviewerDlg::OnOpenfile() 
{
	UpdateData(TRUE);

	UINT picsize = m_nWidth*m_nHeight;	//֡����������

	m_pFile[m_iCount] = new CFile();	//newһ��CFILE�����

	char BASED_CODE szFilter[] = "All Files (*.*)|*.*||";	//�ļ��򿪶Ի����õĹ���
	CFileDialog dlg( TRUE, "yuv", NULL, OFN_HIDEREADONLY,szFilter);	//CFileDialog�����Ĺ��죬�ļ��򿪶Ի���
	if(dlg.DoModal()!=IDOK) //��ʾ�ļ��򿪶Ի���
//		return 0; 

	sprintf( inSeqence[m_iCount], "%s", dlg.GetPathName() );
	//getSeqName(inSeqence[m_iCount], inSeqName[m_iCount]);
	//change by wyn
	char * pszFileName2="b.yuv";
	if(!m_pFile[m_iCount]->Open(pszFileName2, CFile::modeRead )) //����ʽ���ļ�
	{
		AfxMessageBox("���ܴ������ļ�");
//		return 0;
	}
/*
char *oneframe;
CFile outf;
outf.Open("right.yuv", CFile::modeCreate | CFile::modeWrite);
//  if(m_nFrameSize == 0) // 0: CIF, 1:QCIF
  oneframe = (char*) malloc(picsize*3/2);

  m_pFile[m_iCount]->Seek( picsize*3/2, CFile::begin );
  while( picsize*3/2 == m_pFile[m_iCount]->Read(oneframe, picsize*3/2) )
  {
    outf.Write(oneframe, picsize*3/2);
  }

  outf.Close();
  m_pFile[m_iCount]->Close();
  free(oneframe);
return 0;
*/
//	CYUVviewerDlg* pWin = (CYUVviewerDlg*)pParam;

	AfxMessageBox("׼�� new CChildWindow");
	//change by wyn ��Ϊ�Ǻڰ�ͼ�����Խ�1��Ϊ0
	m_pWnd[m_iCount]=new CChildWindow((CFrameWnd*)this, m_nWidth, m_nHeight,0);//���Ŵ���

	if(picsize != m_pFile[m_iCount]->Read(m_pWnd[m_iCount]->Y,picsize))
	{
		MessageBox("�����ļ�β");
	//	return 0;
	}
	//if(1)//bColorImage) 
	//{
	//	if(picsize/4 != m_pFile[m_iCount]->Read(m_pWnd[m_iCount]->Cb,picsize/4))
	//	{
	//		MessageBox("�����ļ�β");
	//	//	return 0;
	//	}
	//	if(picsize/4 != m_pFile[m_iCount]->Read(m_pWnd[m_iCount]->Cr,picsize/4))
	//	{
	//		MessageBox("�����ļ�β");
	//	//	return 0;
	//	}
	//}

	//m_pWnd[m_iCount]->ShowWindow(SW_SHOW);

	if(m_nZoom == -1)
		m_pWnd[m_iCount]->CenterWindow(m_nWidth,m_nHeight);
	else if(m_nZoom == 0) 
		m_pWnd[m_iCount]->CenterWindow(m_nWidth*2,m_nHeight*2);

	m_pWnd[m_iCount]->ShowWindow(SW_SHOW);	//��ʾ���Ŵ���(���������Ų��һ�£����ƶ���λ�ã�Ȼ������ʾ����)

	m_iCount++;
	
	// create a new thread

//	m_pWinThread[ m_iCountThread ] = AfxBeginThread( (AFX_THREADPROC)PlayVideo , (void*)this);

	
/*	if ( pWnd != NULL )
	{
		delete pWnd;
		pWnd = NULL;
	}
	m_pFile[m_iCountThread]->Close();
*/
	//return 1;

}

void CYUVviewerDlg::OnNext() //����1֡����ť
{
	int i;
	UINT picsize = m_nWidth*m_nHeight;

	UpdateData(TRUE);

	g_nStartFrame = m_nFrameFrom;
	if(m_nFrameTo != 0) g_nEndFrame = m_nFrameTo;
	else g_nEndFrame = 10000;

	for(i=0; i<m_iCount; i++)
	{
		m_pFile[i]->Seek(g_nCurrentFrame*picsize*3/2, SEEK_SET);
		m_pWnd[i]->nPicShowOrder = g_nCurrentFrame;
	}
	
	if(g_nCurrentFrame < g_nEndFrame) // && !bEof)
	{
		g_nFrameNumber = g_nCurrentFrame;//j;
 
		for(i=0; i<m_iCount; i++)
		{
			if(picsize != m_pFile[i]->Read(m_pWnd[i]->Y,picsize))
			{
				MessageBox("Get to end of file");
				return;
			}
			if(1)//bColorImage) 
			{
				if(picsize/4 != m_pFile[i]->Read(m_pWnd[i]->Cb,picsize/4))
				{
					MessageBox("Get to end of file");
					return;
				}
				if(picsize/4 != m_pFile[i]->Read(m_pWnd[i]->Cr,picsize/4))
				{
					MessageBox("Get to end of file");
					return;
				}
			}
			m_pWnd[i]->InvalidateRect (NULL,FALSE);
			m_pWnd[i]->UpdateWindow ();
			m_pWnd[i]->nPicShowOrder ++;
		}
		g_nCurrentFrame++;
		//Sleep(200); // sleep time in milliseconds
	}


/*	if ( m_bPlay && hPlayTemp != NULL)
	{
		g_nOldFrameNumber = g_nFrameNumber;
		ReleaseMutex(hPlayTemp);
		g_Play = true;
	}
	WaitForSingleObject( hPlayTemp,INFINITE );
*/
/*	if ( g_nFrameNumber == g_nOldFrameNumber+1)
	{
	//	g_Play = false;
		OpenMutex(MUTEX_ALL_ACCESS,FALSE,"Play");
	}
*/
/*	if ( !g_bPlay )
		OpenMutex(MUTEX_ALL_ACCESS,FALSE,"Play");
*/
}

void CYUVviewerDlg::OnNext5() //����5֡����ť
{
	int i;
	UINT picsize = m_nWidth*m_nHeight;

	UpdateData(TRUE);

	g_nStartFrame = m_nFrameFrom;
	if(m_nFrameTo != 0) g_nEndFrame = m_nFrameTo;
	else g_nEndFrame = 10000;

	g_nCurrentFrame += 4;
	if(g_nCurrentFrame > g_nEndFrame) g_nCurrentFrame = g_nEndFrame;

	for(i=0; i<m_iCount; i++)
	{
		m_pFile[i]->Seek(g_nCurrentFrame*picsize*3/2, SEEK_SET);
		m_pWnd[i]->nPicShowOrder = g_nCurrentFrame;
	}
	
	if(g_nCurrentFrame < g_nEndFrame) // && !bEof)
	{
		g_nFrameNumber = g_nCurrentFrame;//j;
 
		for(i=0; i<m_iCount; i++)
		{
			if(picsize != m_pFile[i]->Read(m_pWnd[i]->Y,picsize))
			{
				MessageBox("Get to end of file");
				return;
			}
			if(1)//bColorImage) 
			{
				if(picsize/4 != m_pFile[i]->Read(m_pWnd[i]->Cb,picsize/4))
				{
					MessageBox("Get to end of file");
					return;
				}
				if(picsize/4 != m_pFile[i]->Read(m_pWnd[i]->Cr,picsize/4))
				{
					MessageBox("Get to end of file");
					return;
				}
			}
			m_pWnd[i]->InvalidateRect (NULL,FALSE);
			m_pWnd[i]->UpdateWindow ();
			m_pWnd[i]->nPicShowOrder ++;
		}
		g_nCurrentFrame++;
		//Sleep(200); // sleep time in milliseconds
	}

/*	if ( m_bPlay && hPlayTemp != NULL)
	{
		g_nOldFrameNumber = g_nFrameNumber;
		ReleaseMutex(hPlayTemp);
//		g_Play = true;
	}
	WaitForSingleObject( hPlayTemp,INFINITE );
	for ( int i=0;i<5;i++)
	{
	//	g_Play = false;
		ReleaseMutex(hPlayTemp);
		WaitForSingleObject( hPlayTemp,INFINITE );
		
	}
	if ( g_nFrameNumber == g_nOldFrameNumber+5 )
	{
		OpenMutex(MUTEX_ALL_ACCESS,FALSE,"Play");
	}
*/	
}

void CYUVviewerDlg::OnPrevious() //��ǰ��1֡(��1֡)
{
	int i;
	int picsize = m_nWidth*m_nHeight;

	UpdateData(TRUE);

	g_nStartFrame = m_nFrameFrom;
	if(m_nFrameTo != 0) g_nEndFrame = m_nFrameTo;
	else g_nEndFrame = 10000;

	g_nCurrentFrame -= 2;
	if(g_nCurrentFrame<0) g_nCurrentFrame = 0;

	for(i=0; i<m_iCount; i++)
	{
		m_pFile[i]->Seek(g_nCurrentFrame*picsize*3/2, SEEK_SET);
		m_pWnd[i]->nPicShowOrder = g_nCurrentFrame;
	}
	
	if(g_nCurrentFrame < g_nEndFrame) // && !bEof)
	{
		g_nFrameNumber = g_nCurrentFrame;//j;
 
		for(i=0; i<m_iCount; i++)
		{
			m_pFile[i]->Read(m_pWnd[i]->Y,picsize);
			if(1)//bColorImage) 
			{
				m_pFile[i]->Read(m_pWnd[i]->Cb,picsize/4);
				m_pFile[i]->Read(m_pWnd[i]->Cr,picsize/4);
			}
			m_pWnd[i]->InvalidateRect (NULL,FALSE);
			m_pWnd[i]->UpdateWindow ();
			m_pWnd[i]->nPicShowOrder ++;
		}
		g_nCurrentFrame++;
		//Sleep(200); // sleep time in milliseconds
	}
}

void CYUVviewerDlg::OnPrevious5() //��ǰ��5֡(�൱�ڵ���)
{
	int i;
	int picsize = m_nWidth*m_nHeight;	//��*��(ͼ�����������)

	UpdateData(TRUE);

	g_nStartFrame = m_nFrameFrom;	//��m_nFrameFrom(��������)��ʼ
	if(m_nFrameTo != 0)				//����ڴ���������һ������֡
		g_nEndFrame = m_nFrameTo;	//��ֵ
	else 
		g_nEndFrame = 10000;	//û������Ļ�����һ��Ĭ��ֵ10000

	g_nCurrentFrame -= 6;	////g_nCurrentFrame = g_nCurrentFrame - 6
	if(g_nCurrentFrame<0) 
		g_nCurrentFrame = 0;	//���g_nCurrentFrame<0 ������=0

	for(i=0; i<m_iCount; i++)
	{
		m_pFile[i]->Seek(g_nCurrentFrame*picsize*3/2, SEEK_SET);//��λ���ļ���ĳ��λ��(Ӧ����֡�Ŀ�ʼ)//#define SEEK_SET    0
																//
		m_pWnd[i]->nPicShowOrder = g_nCurrentFrame;
	}
	
	if(g_nCurrentFrame < g_nEndFrame) // && !bEof)	//��ǰ֡��<����֡������δ��β
	{
		g_nFrameNumber = g_nCurrentFrame;//j;		//֡���
 
		for(i=0; i<m_iCount; i++)
		{
			m_pFile[i]->Read(m_pWnd[i]->Y,picsize);//������Y	��֡��ʼλ�ö��������������ֽ�
			if(1)//����϶���ִ�а��������ifò�ƶ���//bColorImage) 
			{
				//Y��Cb��Cr��ָ���ڴ���ChildWindow.cpp����malloc��̬�ڴ����
				m_pFile[i]->Read(m_pWnd[i]->Cb,picsize/4);//��ɫ��Cb	//CFile����ĵ�ǰλ�����ô���Y�������ĺ��棬��Cb��
				m_pFile[i]->Read(m_pWnd[i]->Cr,picsize/4);//��ɫ��Cr	//CFile����ĵ�ǰλ�����ô���Cbɫ�����ĺ��棬��Cr��,Cb��Cr��Y ����LPBYTE���ͣ�����ĳ���϶��ѷ������ڴ�ռ䣬������������ռ��ַ������Cb��Cr��Y
			}
			m_pWnd[i]->InvalidateRect (NULL,FALSE);//CWnd::InvalidateRect//ˢ�²��Ŵ���//��1��������ָ��һ��CRect�����RECT�ṹ�����а�����Ҫ�������������ľ��Σ��ͻ������꣩�����lpRectΪNULL���������ͻ�����������������򡣵�2��������ָ�����������ڵı����Ƿ�Ҫ������
			m_pWnd[i]->UpdateWindow ();		//CChildWindow *m_pWnd[36];
			m_pWnd[i]->nPicShowOrder ++;	//ע���±�i��0��m_iCount-1
		}
		g_nCurrentFrame++;	//?
		//Sleep(200); // sleep time in milliseconds
		//YCbCr��YUV��4:2:0��ʽ����4�����أ���4������Y(��4�ֽ�)��1��ɫ��Cr��1��ɫ��Cb
		//���ԣ�picsize(=m_nWidth*m_nHeight)Ϊһ֡����������
		//Y���ֽ���������һ���ģ�
		//Cr��Cb��������������1/4(ֻ˵��ֵ����Ȼ��λ�ǲ�һ���ģ����������Ǹ�,Cb��Cr���ֽ���)
	}
}

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CYUVviewerDlg dialog
/*
�Ի���Ĺ��캯����������һЩĬ�ϲ������糤���֡��
*/
CYUVviewerDlg::CYUVviewerDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CYUVviewerDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CYUVviewerDlg)
	m_nFrameFrom = 0;
	m_nFrameTo = 0;
	m_nFrameSize = 1;
	m_nHeight = 144;
	m_nWidth = 176;
	m_sFrameRate = _T("30");
	m_nZoom = 0;
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

}

/*
 ���ݽ���
*/
void CYUVviewerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CYUVviewerDlg)
	DDX_Control(pDX, IDC_PREVIOUS5, m_buttonPrev5);
	DDX_Control(pDX, IDC_ORDER, m_buttonOrder);
	DDX_Control(pDX, IDC_NEXT5, m_buttonNext5);
	DDX_Control(pDX, IDC_PREVIOUS, m_buttonPrev);
	DDX_Control(pDX, IDC_PAUSEPLAY, m_buttonPausePlay);
	DDX_Control(pDX, IDC_NEXT, m_buttonNext);
	DDX_Control(pDX, IDC_OPENFILE, m_buttonOpen);
	DDX_Text(pDX, IDC_FRAME_FROM, m_nFrameFrom);
	DDX_Text(pDX, IDC_FRAME_TO, m_nFrameTo);
	DDX_Radio(pDX, IDC_SIZE_CIF, m_nFrameSize);
	DDX_Text(pDX, IDC_SIZE_HEIGHT, m_nHeight);
	DDX_Text(pDX, IDC_SIZE_WIDTH, m_nWidth);
	DDX_CBString(pDX, IDC_FRAME_RATE, m_sFrameRate);
	DDX_Radio(pDX, IDC_ZOOM, m_nZoom);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CYUVviewerDlg, CDialog)
	//{{AFX_MSG_MAP(CYUVviewerDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_SIZE_CIF, OnSizeCif)
	ON_BN_CLICKED(IDC_SIZE_QCIF, OnSizeQcif)
	ON_BN_CLICKED(IDC_SIZE_OTHER, OnSizeOther)
	ON_BN_CLICKED(IDC_OPENFILE, OnOpenfile)
	ON_BN_CLICKED(IDC_NEXT, OnNext)
	ON_BN_CLICKED(IDC_PAUSEPLAY, OnPauseplay)
	ON_BN_CLICKED(IDC_PREVIOUS, OnPrevious)
	ON_BN_CLICKED(IDC_NEXT5, OnNext5)
	ON_BN_CLICKED(IDC_ORDER, OnOrder)
	ON_BN_CLICKED(IDC_PREVIOUS5, OnPrevious5)
	ON_BN_CLICKED(IDCLOSEALL, OnCloseall)
	ON_BN_CLICKED(IDC_TRANSFER, OnTransfer)
	ON_BN_CLICKED(IDC_ZOOM, OnZoom)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CYUVviewerDlg message handlers

BOOL CYUVviewerDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	m_iCount = 0;
	m_bPlay = true;
	m_pWinThread = NULL;

	m_nZoom = -1;			//z�ӵģ�����Ĭ�ϾͲ��Ŵ���
	UpdateData(FALSE);		//z�ӵģ����µ��ؼ���ʾ
	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	// TODO: Add extra initialization here

	/*������һЩ�ؼ�*/
	Disable(IDC_SIZE_WIDTH);
	Disable(IDC_SIZE_HEIGHT);
	Disable(IDC_STATIC_H);
	Disable(IDC_STATIC_W);

	HANDLE hPlay = NULL;
	if( (hPlay=OpenMutex(MUTEX_ALL_ACCESS,FALSE,"Play"))==NULL)
	{
		//���û���������̴�������������������´���
		hPlay = CreateMutex(NULL,FALSE,"Play");
	}

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CYUVviewerDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CYUVviewerDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CYUVviewerDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

/*
CIF����һ����ѡ��ť
*/
void CYUVviewerDlg::OnSizeCif() 
{
	UpdateData(TRUE);
	m_nWidth = 352;
	m_nHeight = 288;

	/*��ֹ����*/
	Disable(IDC_SIZE_WIDTH);	//����򣬿�
	Disable(IDC_SIZE_HEIGHT);	//����򣬸�
	Disable(IDC_STATIC_H);		//��̬�ؼ����ߣ��������������ν
	Disable(IDC_STATIC_W);		//��̬�ؼ������������������ν
	UpdateData(FALSE);
}

/*
Qcif���ڶ�����ѡ��ť
*/
void CYUVviewerDlg::OnSizeQcif() 
{
	UpdateData(TRUE);
	m_nWidth = 176;		//Ĭ�Ϻõĳߴ�
	m_nHeight = 144;
	Disable(IDC_SIZE_WIDTH);
	Disable(IDC_SIZE_HEIGHT);
	Disable(IDC_STATIC_H);
	Disable(IDC_STATIC_W);
	UpdateData(FALSE);
}

/*
��������ѡ��ť
*/
void CYUVviewerDlg::OnSizeOther() 
{
	UpdateData(TRUE);	//����˵�� UpdateData() ��MFC�Ĵ��ں���������ˢ�����ݵ�//UpdateData(TRUE) ����ˢ�¿ؼ���ֵ����Ӧ�ı�����(�ⲿ����ֵ�����ڲ�����) 
	m_nWidth = 640;		//Ĭ�Ϻõĳߴ�(�ҵ�����ͷ������ߴ磬�Ǻ�)
	m_nHeight = 480;

	Enable(IDC_SIZE_WIDTH);
	Enable(IDC_SIZE_HEIGHT);
	Enable(IDC_STATIC_H);
	Enable(IDC_STATIC_W);
	UpdateData(FALSE);	//UpdateData(FALSE) ���� ��������ֵ���ؼ���ʾ��(����������������ֵ�����ⲿ�����ʾ) 
}

void CYUVviewerDlg::Disable(int nID)//��ʵ���ö������Ҳ�У�ֱ����CWnd::EnableWindow�Կؼ�����Ȼ��������й淶����
{
	CWnd *pObject1;
	pObject1 = GetDlgItem(nID);
	pObject1->EnableWindow(FALSE);//����BOOL CWnd::EnableWindow(BOOL bEnable = TRUE);
}
void CYUVviewerDlg::Enable(int nID)//��Disableһ������
{
	CWnd *pObject1;
	pObject1 = GetDlgItem(nID);
	pObject1->EnableWindow(TRUE);
}
BOOL CYUVviewerDlg::Enabled(int nID)//��ѯһ���ؼ���״̬
{
	CWnd *pObject1;
	pObject1 = GetDlgItem(nID);
	return (pObject1->IsWindowEnabled());//	BOOL CWnd::IsWindowEnabled() const;
}

void CYUVviewerDlg::OnTransfer() //������ʾ��һ֡ //Transfer:ת��
{
  // the following code is to set the current displayed picture to the first frame, frame 0.
  // added Jan. 15, 2002.
	int i;
	int picsize = m_nWidth*m_nHeight;	//��*�� = һ֡���������� (�ӵ�������ֵ��˵��ʵ��Ҳ��������ֵY�ĸ�����Cb�ĸ�����4����Cr�ĸ�����4��)

	UpdateData(TRUE);

	g_nStartFrame = m_nFrameFrom = 0;	//��������Ŀ�ʼ֡�����ܴ���0
	if(m_nFrameTo != 0) 
		g_nEndFrame = m_nFrameTo;	//�������֡��Ϊ0(˵���û�ָ����һ������֡)�������ָ���Ľ���֡
	else 
		g_nEndFrame = 10000;		//����û�δָ����������һ��10000

	g_nCurrentFrame = 0;			//��ǰ֡=0


	//���ļ�ָ�붼��λ����ʼ
	for(i=0; i<m_iCount; i++)		//ѭ��(��Ϊ���ܴ򿪶��YUV�ļ�)
	{
		m_pFile[i]->SeekToBegin();	//����SeekToBegin���ļ�ָ��ָ���ļ���ʼ��
		m_pWnd[i]->nPicShowOrder = g_nCurrentFrame;	//=0
	}
	
	if(g_nCurrentFrame < g_nEndFrame) // && !bEof //û�������һ֡
	{
		g_nFrameNumber = g_nCurrentFrame;//j;
 
		for(i=0; i<m_iCount; i++)	//�����ѭ������ÿһ���ļ�(��Ϊ�������ͬʱ�򿪲�ͬʱ���Ŷ���ļ�)
		{
			m_pFile[i]->Read(m_pWnd[i]->Y,picsize);			//Y;m_pWnd[i]->YΪָ��һ���ڴ�����������Ŷ���������ֵ
			if(1)//bColorImage(��ɫͼ�񣬺ͻҶ�ͼ���Ӧ) 
			{
				m_pFile[i]->Read(m_pWnd[i]->Cb,picsize/4);	//Cb;m_pWnd[i]->Cbָ��һ���ڴ棬�������ɫ��ֵ������ڴ����������ط��Ѿ���̬�����
				m_pFile[i]->Read(m_pWnd[i]->Cr,picsize/4);	//Cr;�ο�Cb
			}
			m_pWnd[i]->InvalidateRect (NULL,FALSE);			//�������ܣ��ú�����ָ���Ĵ������һ�����Σ�Ȼ�󴰿ڿͻ��������һ���ֽ������»��ƣ�hWnd��Ҫ���µĿͻ������ڵĴ���ľ�������ΪNULL����ϵͳ���ں�������ǰ���»������еĴ���, Ȼ���� WM_ERASEBKGND �� WM_NCPAINT �����ڹ��̴������� ����2,lpRect����Ч����ľ��δ�������һ���ṹ��ָ�룬����ž��εĴ�С�����ΪNULL��ȫ���Ĵ��ڿͻ����򽫱����ӵ����������С�
			m_pWnd[i]->UpdateWindow ();						//������ڸ��µ�����Ϊ�գ�UpdateWindow����ͨ������һ��WM_PAINT��Ϣ������ָ�����ڵĿͻ����������ƹ�Ӧ�ó������Ϣ���У�ֱ�ӷ���WM_PAINT��Ϣ��ָ�����ڵĴ��ڹ��̣������������Ϊ�գ��򲻷�����Ϣ��
			m_pWnd[i]->nPicShowOrder ++;
		}
		g_nCurrentFrame++;//g_nCurrentFrame������Ϊ��0�����ڲ�����һ֡�����Լ�1
		//Sleep(200); // sleep time in milliseconds
	}

/*	int i, nframeno=750 ;
	
	OnCloseall();
	UpdateData(TRUE);

	UINT picsize = m_nWidth*m_nHeight;

	CFile fy, fu, fv, fyuv;
	if(!fy.Open("d:\\sequences\\glasgow_qcif.y", CFile::modeRead )) 
	{
		AfxMessageBox("Can't open input file");
		return;
	}
	if(!fu.Open("d:\\sequences\\glasgow_qcif.u", CFile::modeRead )) 
	{
		AfxMessageBox("Can't open input file");
		return;
	}
	if(!fv.Open("d:\\sequences\\glasgow_qcif.v", CFile::modeRead )) 
	{
		AfxMessageBox("Can't open input file");
		return;
	}
	if(!fyuv.Open("d:\\sequences\\glasgow.qcif", CFile::modeCreate | CFile::modeWrite )) 
	{
		AfxMessageBox("Can't open output file");
		return;
	}

	CChildWindow *pWnd=new CChildWindow((CFrameWnd*)this, m_nWidth, m_nHeight,1);
	pWnd->ShowWindow(SW_SHOW);
	if(m_nZoom == -1) pWnd->CenterWindow(m_nWidth,m_nHeight);
	else if(m_nZoom == 0) pWnd->CenterWindow(m_nWidth*2,m_nHeight*2);

	for(i=0; i<nframeno; i++)
	{
		//if(i >= 260) fin.Seek(0, SEEK_SET);

		if(picsize != fy.Read(pWnd->Y,picsize))
		{
			MessageBox("Get to end of file");
			goto END;
		}
		fyuv.Write(pWnd->Y,picsize);
		if(1)//bColorImage) 
		{
			if(picsize/4 != fu.Read(pWnd->Cb,picsize/4))
			{
				MessageBox("Get to end of file");
				goto END;
			}
			if(picsize/4 != fv.Read(pWnd->Cr,picsize/4))
			{
				MessageBox("Get to end of file");
				goto END;
			}
		}
		fyuv.Write(pWnd->Cb,picsize/4);
		fyuv.Write(pWnd->Cr,picsize/4);

		pWnd->nPicShowOrder=i +1;
		pWnd->InvalidateRect (NULL,FALSE);
		pWnd->UpdateWindow ();
	}
END:
	pWnd->DestroyWindow();
	fy.Close();
	fu.Close();
	fv.Close();
	fyuv.Close();
*/
}

void getSeqName(char *inseqpath, char *seqname)
{
  int lastSlashPos, lastDotPos; // the last dot is located after the last slash "\"
  int lastNonZeroPos; // last pos that tmp != 0
  int i=0;
  char tmp = '0';

  while(tmp == 0)
  {
    tmp = inseqpath[i++];
    if(tmp == '\\')
      lastSlashPos = i-1;
    if(tmp == '.')
      lastDotPos = i-1;
  }
  lastNonZeroPos = i-1;

  if(lastDotPos < lastSlashPos)
    lastDotPos = -1; // that means the file name with no extention, such as "c:\seq\forman".

  if(lastDotPos != -1)
  {
    for(i=lastSlashPos+1; i<lastDotPos; i++)
      seqname[i-lastSlashPos-1] = inseqpath[i];
    seqname[lastDotPos-lastSlashPos-1] = 0;
  }
  else
  {
    for(i=lastSlashPos+1; i<lastNonZeroPos+1; i++)
      seqname[i-lastSlashPos-1] = inseqpath[i];
    seqname[lastNonZeroPos-lastSlashPos] = 0;
  }
}

void CYUVviewerDlg::OnZoom() 
{
//	UpdateData(TRUE);
	if(m_nZoom == -1)	
		m_nZoom = 0;	//m_nZoom�ǵ�ѡ�ؼ�������int�ͱ���//	int		m_nZoom;
	else m_nZoom = -1;
		UpdateData(FALSE);	//���µ�����		//((CButton *)GetDlgItem(IDC_RADIO1))->SetCheck(TRUE);//ѡ��
}

/*
OpenMutex�����������ܣ�Ϊ���е�һ����������������󴴽�һ���¾��
 ��������ԭ�ͣ� HANDLE OpenMutex(
 ����DWORDdwDesiredAccess, // access
 ����BOOLbInheritHandle, // inheritance option
 ����LPCTSTRlpName // object name
 ����);
 ���������� ����dwDesiredAccess��
 ����MUTEX_ALL_ACCESS ����Ի��������ȫ����
 ����MUTEX_MODIFY_STATE ����ʹ�� ReleaseMutex ����
 ����SYNCHRONIZE �����������ͬ��ʹ��
 ����bInheritHandle : ��ϣ���ӽ����ܹ��̳о������ΪTRUE
 ����lpName ��Ҫ�򿪶��������
 ��������ֵ����ִ�гɹ������ض���ľ�������ʾʧ�ܡ������ø��������Ϣ�������GetLastError������
 ������ע��һ��������Ҫ��ע��һ��Ҫ�� CloseHandle �رջ������������������о�����ѹرգ���ô����Ҳ��ɾ��
 �����ٲ飺Windows NT/2000/XP��3.1���ϰ汾��Windows 95/98/Me��95���ϰ汾��
 ����ͷ�ļ���Windows.h ;���ļ���Kernel32.lib��
 ����DLL��Kernel32.dll.
*/