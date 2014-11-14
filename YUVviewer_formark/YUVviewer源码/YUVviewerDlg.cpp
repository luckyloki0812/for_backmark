
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
定义一些全局函数(在类的成员函数外定义的)
*/

BOOL g_bPlay;
int g_nFrameNumber = 0;
int g_nOldFrameNumber = -1000; 
BOOL g_Play = true;

int g_nStartFrame = 0;		//开始帧(可以是第一帧之后的，由窗口输入)
int g_nEndFrame = 10000;	//结束帧(可以小于总帧数，由窗口输入的值确定)
int g_nCurrentFrame = 0;	//当前帧
BOOL g_bReversePlay = FALSE;	//反向播放

void getSeqName(char *inseqpath, char *seqname);

//int nImgWidth, nImgHeight;

/*
线程入口函数
*/
UINT PlayVideo( LPVOID pParam )
{
	int i;//,j;

	BOOL bPlay = g_bPlay;
	BOOL bEof = FALSE;

	CYUVviewerDlg *pWin = (CYUVviewerDlg *)pParam;	//从线程参数(结构体)取出对话框的指针
	UINT picsize = pWin->m_nWidth*pWin->m_nHeight;	//通过对话框的指针访问对话框的成员函数
	int timespan = 1000/atoi(pWin->m_sFrameRate);	//
	
	if(g_nCurrentFrame < g_nStartFrame) g_nCurrentFrame = g_nStartFrame;
	if(g_nCurrentFrame > g_nEndFrame) g_nCurrentFrame = g_nEndFrame;

	for(i=0; i<pWin->m_iCount; i++)
	{
		pWin->m_pFile[i]->Seek(g_nCurrentFrame*picsize*3/2, SEEK_SET);
		pWin->m_pWnd[i]->nPicShowOrder = g_nCurrentFrame;
	}
	
	HANDLE hPlayTemp1 = OpenMutex(MUTEX_ALL_ACCESS,FALSE,"Play");//函数功能：为现有的一个已命名互斥体对象创建一个新句柄，一旦不再需要，注意一定要用 CloseHandle 关闭互斥体句柄。如对象的所有句柄都已关闭，那么对象也会删除
	
	while(g_nCurrentFrame >= g_nStartFrame && g_nCurrentFrame <= g_nEndFrame && !bEof)
	{
		DWORD t2=GetTickCount();	//从操作系统启动到现在所经过（elapsed）的毫秒数，它的返回值是DWORD。
		g_nFrameNumber = g_nCurrentFrame;//j;
 
		if ( WAIT_OBJECT_0 == WaitForSingleObject(hPlayTemp1,INFINITE) )
			ReleaseMutex( hPlayTemp1 );
		
		for(i=0; i<pWin->m_iCount; i++)
		{
			pWin->m_pFile[i]->Seek(g_nCurrentFrame*picsize*3/2, SEEK_SET);//Seek 函数 返回一个 Long，在 Open 语句打开的文件中指定当前的读/写位置。

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
		m_buttonOrder.SetWindowText("普通播放");
		g_bReversePlay = TRUE;		//Reverse:反向
	}
	else
	{
		m_buttonOrder.SetWindowText("倒播");
		g_bReversePlay = FALSE;
	}
}

/*
播放/暂停按钮
*/
void CYUVviewerDlg::OnPauseplay() 
{

	UpdateData(TRUE);

	g_nStartFrame = m_nFrameFrom;
	if(m_nFrameTo != 0) g_nEndFrame = m_nFrameTo;
	else g_nEndFrame = 10000;

	// create a new thread
	// 创建 一个 新 线程
	if (m_bPlay)	//m_bPlay:真?
	{
		m_buttonPausePlay.SetWindowText("暂停");	//设置按钮上的文本
		m_bPlay = false;	//设置标识
		g_Play = true;
	}
	else			//m_bPlay:假?
	{
		m_buttonPausePlay.SetWindowText("播放");	//设置按钮上的文本
		m_bPlay = true;
	}

	char chTitle[10];								//char数组，就是常说的C风格的字符串
	m_buttonPausePlay.GetWindowText(chTitle,10);	//取得按钮上的文本
	hPlayTemp = NULL;
	hPlayTemp=OpenMutex(MUTEX_ALL_ACCESS,FALSE,"播放");
	if ( strcmp( chTitle,"播放" ) == 0 )			//字符串比较
	{
		WaitForSingleObject( hPlayTemp,0);//WaitForSingleObject函数用来检测hHandle事件的信号状态，在某一线程中调用该函数时，线程暂时挂起，如果在挂起的dwMilliseconds毫秒内，线程所等待的对象变为有信号状态，则该函数立即返回；如果超时时间已经到达dwMilliseconds毫秒，但hHandle所指向的对象还没有变成有信号状态，函数照样返回。
		
	}
	else
		ReleaseMutex(hPlayTemp);	//释放由线程拥有的一个互斥体

	if ( m_pWinThread == NULL)
		m_pWinThread = AfxBeginThread( (AFX_THREADPROC)PlayVideo , (void*)this);//用于创建工作者线程，参数1为线程入口函数，参数2为参数，一般为结构体的指针，可以传递多个信息到线程内部

}

void CYUVviewerDlg::OnCloseall() 
{
	int i;

	for(i=0; i<m_iCount; i++)		
	{
		if(m_pFile[i])
			m_pFile[i]->Close();		//关闭所有文件
		if(m_pWnd[i])
			m_pWnd[i]->DestroyWindow();	//销毁播放窗口
	}
	m_iCount = 0;						//打开的文件数=0

	g_nFrameNumber = 0;					//
	g_nOldFrameNumber = -1000;			//
	g_Play = true;						//

	g_nStartFrame = 0;					//开始帧
	g_nEndFrame = 10000;				//最后帧数
	g_nCurrentFrame = 0;				//当前播放的帧序号(0、1、2、3...)
	g_bReversePlay = FALSE;				//反向播放标记
}

void CYUVviewerDlg::OnCancel() //通过右上角的叉号退出，也是这个响应函数
{
	int i;

	for(i=0; i<m_iCount; i++)		//m_iCount为打开的文件总数
	{
		if(m_pFile[i])
			m_pFile[i]->Close();	//一个个的关闭文件
		if(m_pWnd[i])
			m_pWnd[i]->DestroyWindow();	//销毁播放窗口
	}
	
	CDialog::OnCancel();
}

void CYUVviewerDlg::OnOpenfile() 
{
	UpdateData(TRUE);

	UINT picsize = m_nWidth*m_nHeight;	//帧的像素总数

	m_pFile[m_iCount] = new CFile();	//new一个CFILE类对象

	char BASED_CODE szFilter[] = "All Files (*.*)|*.*||";	//文件打开对话框用的过滤
	CFileDialog dlg( TRUE, "yuv", NULL, OFN_HIDEREADONLY,szFilter);	//CFileDialog类对象的构造，文件打开对话框
	if(dlg.DoModal()!=IDOK) //显示文件打开对话框
//		return 0; 

	sprintf( inSeqence[m_iCount], "%s", dlg.GetPathName() );
	//getSeqName(inSeqence[m_iCount], inSeqName[m_iCount]);
	//change by wyn
	char * pszFileName2="b.yuv";
	if(!m_pFile[m_iCount]->Open(pszFileName2, CFile::modeRead )) //读方式打开文件
	{
		AfxMessageBox("不能打开输入文件");
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

	AfxMessageBox("准备 new CChildWindow");
	//change by wyn 因为是黑白图像，所以将1改为0
	m_pWnd[m_iCount]=new CChildWindow((CFrameWnd*)this, m_nWidth, m_nHeight,0);//播放窗口

	if(picsize != m_pFile[m_iCount]->Read(m_pWnd[m_iCount]->Y,picsize))
	{
		MessageBox("到达文件尾");
	//	return 0;
	}
	//if(1)//bColorImage) 
	//{
	//	if(picsize/4 != m_pFile[m_iCount]->Read(m_pWnd[m_iCount]->Cb,picsize/4))
	//	{
	//		MessageBox("到达文件尾");
	//	//	return 0;
	//	}
	//	if(picsize/4 != m_pFile[m_iCount]->Read(m_pWnd[m_iCount]->Cr,picsize/4))
	//	{
	//		MessageBox("到达文件尾");
	//	//	return 0;
	//	}
	//}

	//m_pWnd[m_iCount]->ShowWindow(SW_SHOW);

	if(m_nZoom == -1)
		m_pWnd[m_iCount]->CenterWindow(m_nWidth,m_nHeight);
	else if(m_nZoom == 0) 
		m_pWnd[m_iCount]->CenterWindow(m_nWidth*2,m_nHeight*2);

	m_pWnd[m_iCount]->ShowWindow(SW_SHOW);	//显示播放窗口(把这句往后挪了一下，先移动到位置，然后再显示出来)

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

void CYUVviewerDlg::OnNext() //“下1帧”按钮
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

void CYUVviewerDlg::OnNext5() //“下5帧”按钮
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

void CYUVviewerDlg::OnPrevious() //向前进1帧(倒1帧)
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

void CYUVviewerDlg::OnPrevious5() //向前进5帧(相当于倒带)
{
	int i;
	int picsize = m_nWidth*m_nHeight;	//宽*高(图像的像素总数)

	UpdateData(TRUE);

	g_nStartFrame = m_nFrameFrom;	//从m_nFrameFrom(窗口输入)开始
	if(m_nFrameTo != 0)				//如果在窗口输入了一个结束帧
		g_nEndFrame = m_nFrameTo;	//赋值
	else 
		g_nEndFrame = 10000;	//没有输入的话，设一个默认值10000

	g_nCurrentFrame -= 6;	////g_nCurrentFrame = g_nCurrentFrame - 6
	if(g_nCurrentFrame<0) 
		g_nCurrentFrame = 0;	//如果g_nCurrentFrame<0 ，让它=0

	for(i=0; i<m_iCount; i++)
	{
		m_pFile[i]->Seek(g_nCurrentFrame*picsize*3/2, SEEK_SET);//定位到文件的某个位置(应该是帧的开始)//#define SEEK_SET    0
																//
		m_pWnd[i]->nPicShowOrder = g_nCurrentFrame;
	}
	
	if(g_nCurrentFrame < g_nEndFrame) // && !bEof)	//当前帧数<最后的帧数，即未到尾
	{
		g_nFrameNumber = g_nCurrentFrame;//j;		//帧序号
 
		for(i=0; i<m_iCount; i++)
		{
			m_pFile[i]->Read(m_pWnd[i]->Y,picsize);//读亮度Y	从帧开始位置读入像素总数个字节
			if(1)//这儿肯定会执行啊，加这个if貌似多余//bColorImage) 
			{
				//Y、Cb、Cr所指的内存在ChildWindow.cpp中用malloc动态内存分配
				m_pFile[i]->Read(m_pWnd[i]->Cb,picsize/4);//读色度Cb	//CFile对象的当前位置正好处在Y亮度区的后面，即Cb处
				m_pFile[i]->Read(m_pWnd[i]->Cr,picsize/4);//读色度Cr	//CFile对象的当前位置正好处在Cb色度区的后面，即Cr处,Cb和Cr及Y ，是LPBYTE类型，所以某处肯定已分配了内存空间，并将分配的这块空间地址告诉了Cb和Cr及Y
			}
			m_pWnd[i]->InvalidateRect (NULL,FALSE);//CWnd::InvalidateRect//刷新播放窗口//第1个参数：指向一个CRect对象或RECT结构，其中包含了要被加入更新区域的矩形（客户区坐标）。如果lpRect为NULL，则整个客户区都被加入更新区域。第2个参数：指定更新区域内的背景是否要被擦除
			m_pWnd[i]->UpdateWindow ();		//CChildWindow *m_pWnd[36];
			m_pWnd[i]->nPicShowOrder ++;	//注意下标i从0到m_iCount-1
		}
		g_nCurrentFrame++;	//?
		//Sleep(200); // sleep time in milliseconds
		//YCbCr是YUV的4:2:0格式，即4个像素，有4个亮度Y(共4字节)，1个色度Cr，1个色度Cb
		//所以，picsize(=m_nWidth*m_nHeight)为一帧的像素总数
		//Y的字节数和它是一样的，
		//Cr和Cb都是像素总数的1/4(只说数值，当然单位是不一样的，像素总数是个,Cb和Cr是字节数)
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
对话框的构造函数，设置了一些默认参数，如长宽和帧速
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
 数据交换
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

	m_nZoom = -1;			//z加的，这样默认就不放大了
	UpdateData(FALSE);		//z加的，更新到控件显示
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

	/*禁用了一些控件*/
	Disable(IDC_SIZE_WIDTH);
	Disable(IDC_SIZE_HEIGHT);
	Disable(IDC_STATIC_H);
	Disable(IDC_STATIC_W);

	HANDLE hPlay = NULL;
	if( (hPlay=OpenMutex(MUTEX_ALL_ACCESS,FALSE,"Play"))==NULL)
	{
		//如果没有其他进程创建这个互斥量，则重新创建
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
CIF，第一个单选按钮
*/
void CYUVviewerDlg::OnSizeCif() 
{
	UpdateData(TRUE);
	m_nWidth = 352;
	m_nHeight = 288;

	/*禁止输入*/
	Disable(IDC_SIZE_WIDTH);	//输入框，宽
	Disable(IDC_SIZE_HEIGHT);	//输入框，高
	Disable(IDC_STATIC_H);		//静态控件，高，这个禁不禁无所谓
	Disable(IDC_STATIC_W);		//静态控件，宽，这个禁不禁无所谓
	UpdateData(FALSE);
}

/*
Qcif，第二个单选按钮
*/
void CYUVviewerDlg::OnSizeQcif() 
{
	UpdateData(TRUE);
	m_nWidth = 176;		//默认好的尺寸
	m_nHeight = 144;
	Disable(IDC_SIZE_WIDTH);
	Disable(IDC_SIZE_HEIGHT);
	Disable(IDC_STATIC_H);
	Disable(IDC_STATIC_W);
	UpdateData(FALSE);
}

/*
第三个单选按钮
*/
void CYUVviewerDlg::OnSizeOther() 
{
	UpdateData(TRUE);	//函数说明 UpdateData() 是MFC的窗口函数，用来刷新数据的//UpdateData(TRUE) ——刷新控件的值到对应的变量。(外部输入值交给内部变量) 
	m_nWidth = 640;		//默认好的尺寸(我的摄像头是这个尺寸，呵呵)
	m_nHeight = 480;

	Enable(IDC_SIZE_WIDTH);
	Enable(IDC_SIZE_HEIGHT);
	Enable(IDC_STATIC_H);
	Enable(IDC_STATIC_W);
	UpdateData(FALSE);	//UpdateData(FALSE) —— 拷贝变量值到控件显示。(变量的最终运算结果值交给外部输出显示) 
}

void CYUVviewerDlg::Disable(int nID)//其实不用定义这个也行，直接用CWnd::EnableWindow对控件，当然这样会更有规范可能
{
	CWnd *pObject1;
	pObject1 = GetDlgItem(nID);
	pObject1->EnableWindow(FALSE);//调用BOOL CWnd::EnableWindow(BOOL bEnable = TRUE);
}
void CYUVviewerDlg::Enable(int nID)//与Disable一个道理
{
	CWnd *pObject1;
	pObject1 = GetDlgItem(nID);
	pObject1->EnableWindow(TRUE);
}
BOOL CYUVviewerDlg::Enabled(int nID)//查询一个控件的状态
{
	CWnd *pObject1;
	pObject1 = GetDlgItem(nID);
	return (pObject1->IsWindowEnabled());//	BOOL CWnd::IsWindowEnabled() const;
}

void CYUVviewerDlg::OnTransfer() //窗口显示第一帧 //Transfer:转移
{
  // the following code is to set the current displayed picture to the first frame, frame 0.
  // added Jan. 15, 2002.
	int i;
	int picsize = m_nWidth*m_nHeight;	//宽*高 = 一帧的像素总数 (从单纯的数值来说，实际也就是亮度值Y的个数，Cb的个数的4倍，Cr的个数的4倍)

	UpdateData(TRUE);

	g_nStartFrame = m_nFrameFrom = 0;	//窗口输入的开始帧，可能大于0
	if(m_nFrameTo != 0) 
		g_nEndFrame = m_nFrameTo;	//如果结束帧不为0(说明用户指定了一个结束帧)，则采用指定的结束帧
	else 
		g_nEndFrame = 10000;		//如果用户未指定，给它定一个10000

	g_nCurrentFrame = 0;			//当前帧=0


	//把文件指针都定位到开始
	for(i=0; i<m_iCount; i++)		//循环(因为可能打开多个YUV文件)
	{
		m_pFile[i]->SeekToBegin();	//函数SeekToBegin将文件指针指向文件开始处
		m_pWnd[i]->nPicShowOrder = g_nCurrentFrame;	//=0
	}
	
	if(g_nCurrentFrame < g_nEndFrame) // && !bEof //没播到最后一帧
	{
		g_nFrameNumber = g_nCurrentFrame;//j;
 
		for(i=0; i<m_iCount; i++)	//这个是循环处理每一个文件(因为软件可以同时打开并同时播放多个文件)
		{
			m_pFile[i]->Read(m_pWnd[i]->Y,picsize);			//Y;m_pWnd[i]->Y为指向一块内存区，用来存放读到的亮度值
			if(1)//bColorImage(彩色图像，和灰度图像对应) 
			{
				m_pFile[i]->Read(m_pWnd[i]->Cb,picsize/4);	//Cb;m_pWnd[i]->Cb指向一块内存，用来存放色度值，这块内存区在其它地方已经动态分配好
				m_pFile[i]->Read(m_pWnd[i]->Cr,picsize/4);	//Cr;参考Cb
			}
			m_pWnd[i]->InvalidateRect (NULL,FALSE);			//函数功能：该函数向指定的窗体添加一个矩形，然后窗口客户区域的这一部分将被重新绘制；hWnd：要更新的客户区所在的窗体的句柄。如果为NULL，则系统将在函数返回前重新绘制所有的窗口, 然后发送 WM_ERASEBKGND 和 WM_NCPAINT 给窗口过程处理函数。 参数2,lpRect：无效区域的矩形代表，它是一个结构体指针，存放着矩形的大小。如果为NULL，全部的窗口客户区域将被增加到更新区域中。
			m_pWnd[i]->UpdateWindow ();						//如果窗口更新的区域不为空，UpdateWindow函数通过发送一个WM_PAINT消息来更新指定窗口的客户区。函数绕过应用程序的消息队列，直接发送WM_PAINT消息给指定窗口的窗口过程，如果更新区域为空，则不发送消息。
			m_pWnd[i]->nPicShowOrder ++;
		}
		g_nCurrentFrame++;//g_nCurrentFrame上面设为了0，现在播放了一帧，所以加1
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
		m_nZoom = 0;	//m_nZoom是单选控件关联的int型变量//	int		m_nZoom;
	else m_nZoom = -1;
		UpdateData(FALSE);	//更新到变量		//((CButton *)GetDlgItem(IDC_RADIO1))->SetCheck(TRUE);//选上
}

/*
OpenMutex　　函数功能：为现有的一个已命名互斥体对象创建一个新句柄
 　　函数原型： HANDLE OpenMutex(
 　　DWORDdwDesiredAccess, // access
 　　BOOLbInheritHandle, // inheritance option
 　　LPCTSTRlpName // object name
 　　);
 　　参数： 　　dwDesiredAccess：
 　　MUTEX_ALL_ACCESS 请求对互斥体的完全访问
 　　MUTEX_MODIFY_STATE 允许使用 ReleaseMutex 函数
 　　SYNCHRONIZE 允许互斥体对象同步使用
 　　bInheritHandle : 如希望子进程能够继承句柄，则为TRUE
 　　lpName ：要打开对象的名字
 　　返回值：如执行成功，返回对象的句柄；零表示失败。若想获得更多错误信息，请调用GetLastError函数。
 　　备注：一旦不再需要，注意一定要用 CloseHandle 关闭互斥体句柄。如对象的所有句柄都已关闭，那么对象也会删除
 　　速查：Windows NT/2000/XP：3.1以上版本；Windows 95/98/Me：95以上版本：
 　　头文件：Windows.h ;库文件：Kernel32.lib。
 　　DLL：Kernel32.dll.
*/