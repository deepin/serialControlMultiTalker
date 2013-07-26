//////////////////////////////////////////////////////////////////////////  
/// COPYRIGHT NOTICE  
/// Copyright (c) 2009, 华中科技大学tickTick Group  （版权声明）  
/// All rights reserved.  
///   
/// @file    SerialPort.cpp    
/// @brief   串口通信类的实现文件  
///  
/// 本文件为串口通信类的实现代码  
///  
/// @version 1.0     
/// @author  卢俊    
/// @E-mail：lujun.hust@gmail.com  
/// @date    2010/03/19  
///   
///  
///  修订说明：  
//////////////////////////////////////////////////////////////////////////  
 
//#include "StdAfx.h"  
#include "SerialPort.h"  
#include <process.h>  
#include <iostream>  
#include <fstream>
#include <string.h>
/** 线程退出标志 */   
bool CSerialPort::s_bExit = false;  
/** 当串口无数据时,sleep至下次查询间隔的时间,单位:秒 */   
const UINT SLEEP_TIME_INTERVAL = 5; 

 HANDLE hsemTemCome, hsemDataReady, hsemMeasDone, hsemNewTem;
 const unsigned char NUMOFSLAVES = 2;
 const unsigned int TemCnt = 3, RoundPerTem = 2;
 unsigned int ExitFlag1 = 0, ExitFlag2 = 0;

 CRITICAL_SECTION crtc;
 CRITICAL_SECTION crtcCout;

 const unsigned int WAIT_BEFORE_COLLECT = 1 * 1000;
 const unsigned int WAIT_WHILE_COLLECT = 1 * 1000;
 const unsigned int WIDTH_OF_VALLEY = 1 * 1000;
 const unsigned int TEM_FRAME_MAXLEN = 10;
 const unsigned int IN_BUF_SZ = 100;
 const unsigned int OUT_BUF_SZ = 100;

 const unsigned int WAIT_AFTER_RXCHAR = 1000;
 
CSerialPort::CSerialPort(void)  
: m_hListenThread(INVALID_HANDLE_VALUE)  
{  
    m_hComm = INVALID_HANDLE_VALUE;  
    m_hListenThread = INVALID_HANDLE_VALUE;  
 
    InitializeCriticalSection(&m_csCommunicationSync);  
	InitializeCriticalSection(&crtc);
	InitializeCriticalSection(&crtcCout);
}  
 
CSerialPort::~CSerialPort(void)  
{  
    CloseListenTread();  
    ClosePort();  
    DeleteCriticalSection(&m_csCommunicationSync);  
}  
	
bool CSerialPort::openPort( UINT portNo )  
{  
    /** 进入临界段 */   
    EnterCriticalSection(&m_csCommunicationSync);  
 
    /** 把串口的编号转换为设备名 */   
    char szPort[50];  
    sprintf_s(szPort, "COM%d", portNo);  

    /** 打开指定的串口 */   
    m_hComm = CreateFileA(szPort, /** 设备名,COM1,COM2等 */   
              GENERIC_READ | GENERIC_WRITE, /** 访问模式,可同时读写 */     
              0,                           /** 共享模式,0表示不共享 */   
              NULL,                         /** 安全性设置,一般使用NULL */   
              OPEN_EXISTING,                /** 该参数表示设备必须存在,否则创建失败 */   
              0,      
              0);      
 
    /** 如果打开失败，释放资源并返回 */   
    if (m_hComm == INVALID_HANDLE_VALUE)  
    {  
        LeaveCriticalSection(&m_csCommunicationSync);  
        return false;  
    }  
 
    /** 退出临界区 */   
    LeaveCriticalSection(&m_csCommunicationSync);   
 
    return true;  
} 
using std::cout;
using std::endl;
bool CSerialPort::InitPort( UINT portNo /*= 1*/,UINT baud /*= CBR_9600*/,char parity /*= 'N'*/,  
                            UINT databits /*= 8*/, UINT stopsbits /*= 1*/,DWORD dwCommEvents /*= EV_RXCHAR*/ )  
{  
 
    /** 临时变量,将制定参数转化为字符串形式,以构造DCB结构 */   
    char szDCBparam[50];  
    sprintf_s(szDCBparam, "baud=%d parity=%c data=%d stop=%d", baud, parity, databits, stopsbits);  // fDtrControl=%

    /** 打开指定串口,该函数内部已经有临界区保护,上面请不要加保护 */   
    if (!openPort(portNo))  
    {  
        return false;  
    }  
 
    /** 进入临界段 */   
    EnterCriticalSection(&m_csCommunicationSync);  
 
    /** 是否有错误发生 */   
    BOOL bIsSuccess = TRUE;  
 
    /** 在此可以设置输入输出的缓冲区大小,如果不设置,则系统会设置默认值.  
     *  自己设置缓冲区大小时,要注意设置稍大一些,避免缓冲区溢出  
     */ 
    if (bIsSuccess )  
    {  
        bIsSuccess = SetupComm(m_hComm,IN_BUF_SZ,OUT_BUF_SZ);  
    }
 
    /** 设置串口的超时时间,均设为0,表示不使用超时限制 */ 
    COMMTIMEOUTS  CommTimeouts;  
    CommTimeouts.ReadIntervalTimeout         = 0;  
    CommTimeouts.ReadTotalTimeoutMultiplier  = 0;  
    CommTimeouts.ReadTotalTimeoutConstant    = 0;  
    CommTimeouts.WriteTotalTimeoutMultiplier = 0;  
    CommTimeouts.WriteTotalTimeoutConstant   = 0;   
    if ( bIsSuccess)  
    {  
        bIsSuccess = SetCommTimeouts(m_hComm, &CommTimeouts);  
    }  
 
    DCB  dcb;  
    if ( bIsSuccess )  
    {  
        // 将ANSI字符串转换为UNICODE字符串  
        DWORD dwNum = MultiByteToWideChar (CP_ACP, 0, szDCBparam, -1, NULL, 0);  
        wchar_t *pwText = new wchar_t[dwNum] ;  
        if (!MultiByteToWideChar (CP_ACP, 0, szDCBparam, -1, pwText, dwNum))  
        {  
            bIsSuccess = TRUE;  
        }  
 
        /** 获取当前串口配置参数,并且构造串口DCB参数 */   
        bIsSuccess = GetCommState(m_hComm, &dcb) && BuildCommDCB(pwText, &dcb) ;  
        /** 开启RTS flow控制 */   
        dcb.fRtsControl = RTS_CONTROL_ENABLE;   
 
        /** 释放内存空间 */   
        delete [] pwText;  
    }  
 
    if ( bIsSuccess )  
    {  
        /** 使用DCB参数配置串口状态 */   
        bIsSuccess = SetCommState(m_hComm, &dcb);  
    }  
          
    /**  清空串口缓冲区 */ 
    PurgeComm(m_hComm, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);  
 
    /** 离开临界段 */   
    LeaveCriticalSection(&m_csCommunicationSync);  
 
    return bIsSuccess==TRUE;  
}  
 
bool CSerialPort::InitPort( UINT portNo ,const LPDCB& plDCB )  
{  
    /** 打开指定串口,该函数内部已经有临界区保护,上面请不要加保护 */   
    if (!openPort(portNo))  
    {  
        return false;  
    }  
      
    /** 进入临界段 */   
    EnterCriticalSection(&m_csCommunicationSync);  
 
    /** 配置串口参数 */   
    if (!SetCommState(m_hComm, plDCB))  
    {  
        return false;
    }  
 
    /**  清空串口缓冲区 */ 
    PurgeComm(m_hComm, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);  
 
    /** 离开临界段 */   
    LeaveCriticalSection(&m_csCommunicationSync);  
 
    return true;  
}  
 
void CSerialPort::ClosePort()  
{  
    /** 如果有串口被打开，关闭它 */ 
    if( m_hComm != INVALID_HANDLE_VALUE )  
    {  
        CloseHandle( m_hComm );  
        m_hComm = INVALID_HANDLE_VALUE;  
    }  
}  

 HANDLE CSerialPort::OpenThread(unsigned int (WINAPI *pFun)(void *))  
{  
    /** 检测线程是否已经开启了 */   
    if (m_hListenThread != INVALID_HANDLE_VALUE)  
    {  
        /** 线程已经开启 */   
        //return false;  
    }  
	HANDLE res = 0;
    s_bExit = false;  
    /** 线程ID */   
    UINT threadId;  
    /** 开启串口数据监听线程 */   
    res = (HANDLE)_beginthreadex(NULL, 0, pFun, this, 0, &threadId);  
    if (!res)  
    {  
        return 0;  
    }  
    /** 设置线程的优先级,高于普通线程 */   
    //if (!SetThreadPriority(res, THREAD_PRIORITY_ABOVE_NORMAL))  
    //{  
        //return 0;  
   // }  
 
    return res;  
}  

bool CSerialPort::CloseListenTread()  
{     
    if (m_hListenThread != INVALID_HANDLE_VALUE)  
    {  
        /** 通知线程退出 */   
        s_bExit = true;  
 
        /** 等待线程退出 */   
        Sleep(10);  
 
        /** 置线程句柄无效 */   
        CloseHandle( m_hListenThread );  
        m_hListenThread = INVALID_HANDLE_VALUE;  
    }  
    return true;  
}  
 
UINT CSerialPort::GetBytesInCOM()  
{  
    DWORD dwError = 0;  /** 错误码 */   
    COMSTAT  comstat;   /** COMSTAT结构体,记录通信设备的状态信息 */   
    memset(&comstat, 0, sizeof(COMSTAT));  
 
    UINT BytesInQue = 0;  
    /** 在调用ReadFile和WriteFile之前,通过本函数清除以前遗留的错误标志 */   
    if ( ClearCommError(m_hComm, &dwError, &comstat) )  
    {  
        BytesInQue = comstat.cbInQue; /** 获取在输入缓冲区中的字节数 */   
    }  
 
    return BytesInQue;  
}  
 UINT CSerialPort::GetOutgoingBytesInCOM()  
{  
    DWORD dwError = 0;  /** 错误码 */   
    COMSTAT  comstat;   /** COMSTAT结构体,记录通信设备的状态信息 */   
    memset(&comstat, 0, sizeof(COMSTAT));  
 
    UINT BytesInQue = 0;  
    /** 在调用ReadFile和WriteFile之前,通过本函数清除以前遗留的错误标志 */   
    if ( ClearCommError(m_hComm, &dwError, &comstat) )  
    {  
        BytesInQue = comstat.cbOutQue; /** 获取在输入缓冲区中的字节数 */   
    }  
 
    return BytesInQue;  
}


bool CSerialPort::ReadChar( char &cRecved )  
{  
    BOOL  bResult     = TRUE;  
    DWORD BytesRead   = 0;  
    if(m_hComm == INVALID_HANDLE_VALUE)  
    {  
        return false;  
    }  
 
    /** 临界区保护 */   
    EnterCriticalSection(&m_csCommunicationSync);  
 
    /** 从缓冲区读取一个字节的数据 */   
    bResult = ReadFile(m_hComm, &cRecved, 1, &BytesRead, NULL);  
    if ((!bResult))  
    {   
        /** 获取错误码,可以根据该错误码查出错误原因 */   
        DWORD dwError = GetLastError();  
 
        /** 清空串口缓冲区 */   
        PurgeComm(m_hComm, PURGE_RXCLEAR | PURGE_RXABORT);  
        LeaveCriticalSection(&m_csCommunicationSync);  
 
        return false;  
    }  
 
    /** 离开临界区 */   
    LeaveCriticalSection(&m_csCommunicationSync);  
 
    return (BytesRead == 1);  
 
}  
 
bool CSerialPort::WriteData( unsigned char* pData, unsigned int length )  
{  
    BOOL   bResult     = TRUE;  
    DWORD  BytesToSend = 0;  
    if(m_hComm == INVALID_HANDLE_VALUE)  
    {  
        return false;  
    }  
 
    /** 临界区保护 */   
    EnterCriticalSection(&m_csCommunicationSync);  
 
    /** 向缓冲区写入指定量的数据 */   
    bResult = WriteFile(m_hComm, pData, length, &BytesToSend, NULL);  
    if (!bResult)    
    {  
        DWORD dwError = GetLastError();  
        /** 清空串口缓冲区 */   
        PurgeComm(m_hComm, PURGE_RXCLEAR | PURGE_RXABORT);  
        LeaveCriticalSection(&m_csCommunicationSync);  
 
        return false;  
    }  
 
    /** 离开临界区 */   
    LeaveCriticalSection(&m_csCommunicationSync);
 
    return true;  
} 
unsigned int WINAPI TemOven(void *pParam)
{
	//Sleep(WAIT_TILL_ALL_READY);
	CSerialPort *hcomm = reinterpret_cast<CSerialPort *>(pParam);
	DWORD oddeven = 0, cnt = TemCnt, oldmask;
	//GetCommMask(hComm, &oldmask);
	//SetCommMask(hComm, oldmask | EV_DSR);	 
	//Sleep(300);
	while(cnt){
		cout << "Temoven: sleeping for 6 seconds" << endl;
		Sleep(3000);Sleep(3000);
		cout << "TemOven: Attention, new tem!" << endl;

		if(oddeven){
			EscapeCommFunction(hcomm ->m_hComm, SETDTR);
		}
		else{
			EscapeCommFunction(hcomm ->m_hComm, CLRDTR);
		}		

		oddeven = 1 - oddeven;
		--cnt;
		WaitForSingleObject(hsemNewTem, INFINITE);
	}
	EnterCriticalSection(&crtcCout);
	ExitFlag1 = 1;
	LeaveCriticalSection(&crtcCout);

	cout << "\nTemOven: i'm done!\n" << endl;
	return 0;
}

 unsigned int WINAPI TemOvenListen(void *pParam)
 {
	//Sleep(WAIT_TILL_ALL_READY);
    CSerialPort *pSerialPort = reinterpret_cast<CSerialPort*>(pParam);  

    //线程循环,轮询方式读取串口数据
	DWORD instanceflag = 0;
	int temcnt = TemCnt;

	SetCommMask(pSerialPort ->m_hComm, EV_DSR);


	//EscapeCommFunction(pSerialPort ->m_hComm, SETRTS|SETDTR);
    while (1) 
    {  

		WaitCommEvent(pSerialPort ->m_hComm, &instanceflag, NULL);//该语句导致CardInformer当中的EscapeCommFunction函数无法执行，可能因为属于同一个端口的缘故
	
		cout <<"TemOvenListen: " <<  temcnt << " tems to go!" << endl;
		ReleaseSemaphore(hsemTemCome, 1, 0);		

		--temcnt;
		EnterCriticalSection(&crtc);
		if(temcnt == 0){
			ExitFlag1 = 1;
			cout << "\nTemOvenListen: i'm done!\n" << endl;
			break;
		}
		LeaveCriticalSection(&crtc);
    }

	return 0;
 }

 unsigned int WINAPI Card(void *pParam)
 {
	 //Sleep(WAIT_TILL_ALL_READY);

	 CSerialPort *hcomm = reinterpret_cast<CSerialPort *>(pParam);
	 DWORD tmp, oldmask;
	 int  cnt = TemCnt * RoundPerTem;
	 //GetCommMask(hcomm ->m_hComm, &oldmask);
	 SetCommMask(hcomm ->m_hComm, EV_CTS);	 

	 while(1){
		 WaitCommEvent(hcomm ->m_hComm, &tmp, 0);

		 cout << "Card: okay, i'm gonna collect data" << endl;
		 --cnt;
		 //结束
		 if(!cnt){
			 cout << "\nCard: i'm done!\n" << endl;
			 return 0;
		 }
	 }

 }


 unsigned int WINAPI CardInformer(void *pParam)
 {

    CSerialPort *pSerialPort = reinterpret_cast<CSerialPort*>(pParam);  

	unsigned int cnt = TemCnt * RoundPerTem;
	unsigned int oddeven = 1;

	//EscapeCommFunction(pSerialPort ->m_hComm, SETRTS);//外部触发信号初始化为高电平（负脉冲有效）
    while (1)
    { 
		WaitForSingleObject(hsemDataReady, INFINITE);
		cout << "CardInformer: yeah, got it!" << endl;
		cout << "CardInformer: sleeping for " << WAIT_BEFORE_COLLECT / 1000 << " seconds" << endl;
		Sleep(WAIT_BEFORE_COLLECT);


		cout << "CardInformer: negtive edge is generated!" << endl;
		if(oddeven)
			EscapeCommFunction(pSerialPort ->m_hComm, CLRRTS);//发送一个下降沿
		else
			EscapeCommFunction(pSerialPort ->m_hComm, SETRTS);///////////////////////////////////与正式工作时消息方式不同
		
		oddeven = 1 - oddeven;		

		//Sleep(WIDTH_OF_VALLEY);
		//EscapeCommFunction(pSerialPort ->m_hComm, SETRTS);////////////////////////正式工作取消注释
		cout << "CardInformer: let's wait for card collecting!" << endl;
		//Sleep(WAIT_WHILE_COLLECT);
		cout << "CardInformer: really long time meas is done!" << endl;
		ReleaseSemaphore(hsemMeasDone, 1, 0);

		if(ExitFlag2){
			break;
		}
    } 
	cout << "\nCardInformer: i'm done!\n" << endl;
	return 0;
 } 
 using std::ofstream;

unsigned int WINAPI McuComm(void *pParam)
 {

    CSerialPort *pSerialPort = reinterpret_cast<CSerialPort*>(pParam);
	unsigned char addr[10];
	unsigned char *tem = reinterpret_cast<unsigned char *>("tem");
	unsigned char *beginwrite = reinterpret_cast<unsigned char *>("nextdata");

	DWORD instanceflag = 0;

	ofstream temfile("temperatures.txt");
	char tembuf[IN_BUF_SZ];
	int nbytegot, counter = 0, i = 0;

	//EscapeCommFunction(pSerialPort ->m_hComm, SETDTR);//外部触发信号初始化为高电平（负脉冲有效）

	SetCommMask(pSerialPort ->m_hComm, EV_RXCHAR);
    while (1)
    { 

		WaitForSingleObject(hsemTemCome, INFINITE);

		cout << "McuComm: tem come copied" << endl;

		temfile << ++counter << ": ";

		for(i = 1; i <= NUMOFSLAVES; ++i){
			itoa(i, (char*)addr, 10);
			sendWithAck(pSerialPort, addr, strlen((const char *)addr));
			//////switch to data mode, sending data frame, i.e. 9th bit is 0

			sendWithAck(pSerialPort, tem, strlen((char*)tem));//发送读取温度指令

			
			WaitCommEvent(pSerialPort ->m_hComm, &instanceflag, NULL);//read and store
			Sleep(WAIT_AFTER_RXCHAR / 10);

			receiveTem(pSerialPort, tembuf, nbytegot);//how does it receive in 9-bit mode; is there a problem?
			
			temfile << tembuf << "; ";

			//switch back to address mode
			
		}
		temfile << "\n";



		for(int i = 0; i < RoundPerTem; ++i){

			for(int j = 1; j <= NUMOFSLAVES; ++j){
				itoa(j, (char*)addr, 10);
				sendWithAck(pSerialPort, addr, strlen((const char *)addr));

				///switch to data mode
				cout << "McuComm: \"beginwrite\" is sent" << endl;
				pSerialPort ->WriteData(beginwrite, strlen((char*)beginwrite));			

				WaitCommEvent(pSerialPort ->m_hComm, &instanceflag, NULL);//wait for data to be written

				Sleep(WAIT_AFTER_RXCHAR / 10);

				receive(pSerialPort, tembuf, nbytegot);				//useless data must be cleared
				cout << "McuComm: hearing that data has been written into #" << j << " mcu" << endl;

				//switch back to address mode

			}

			cout << "McuComm: hearing that data has been written already" << endl;
			cout << "MucComm: Cardinformer?it's your turn now! wait for good news" << endl;
			ReleaseSemaphore(hsemDataReady, 1, 0);					//inform cardinformer thread to go

			if(ExitFlag1 && i == RoundPerTem - 1){
				ExitFlag2 = 1;
			}
			WaitForSingleObject(hsemMeasDone, INFINITE);//which is able to give this signal? maybe have to use timer
			cout << "MucComm: one round is done!" << endl;

		}

#ifdef _test
		ReleaseSemaphore(hsemNewTem, 1, 0);
#endif
		if(ExitFlag1 == 1){
			cout << "\nMcuComm: i'm done!\n" << endl;
			break;
		}

    }  
	temfile.close();
	return 0;
 }
#define DATA_WRITING_TIME 1000
#include <stdlib.h>
#include <time.h>
 unsigned int WINAPI Mcu(void *pParam)
 {
	 //Sleep(WAIT_TILL_ALL_READY);
	 CSerialPort *hcomm = reinterpret_cast<CSerialPort *>(pParam);
	 int temcnt = TemCnt, cnt, nrcvd;
	 DWORD temp;
	 char buf[IN_BUF_SZ];
	 bool ext = false;
	 char ack[IN_BUF_SZ];
	 char randomTem[10];
	 
	 srand(time((unsigned)0));
	 SetCommMask(hcomm ->m_hComm, EV_RXCHAR);
	 while(1){
		cnt = RoundPerTem;
		
		//等待温度数据请求指令“temz”

		WaitCommEvent(hcomm ->m_hComm, &temp, 0);
		Sleep(WAIT_AFTER_RXCHAR / 10);

		receive(hcomm, buf, nrcvd);
		cout << "Mcu: tem cmd copied!" << endl;

		//模拟单片机发送温度数据
		int x = rand() % 100 + 1;
		randomTem[0] = char(x / 100 + '0');
		randomTem[1] = char((x / 10) % 10 + '0');
		randomTem[2] = char(x % 10 + '0');
		randomTem[3] = '\0';
		cout << "Mcu: sending tem data done!" << endl;
		sendWithAck(hcomm,(unsigned char *) randomTem, strlen(randomTem));
		
		//cnt次数据写入
		while(cnt){
			//等待数据写入指令

			WaitCommEvent(hcomm ->m_hComm, &temp, 0);
			Sleep(WAIT_AFTER_RXCHAR / 10);

			receive(hcomm, buf, nrcvd);

			cout << "Mcu: data writing cmd copied!" << endl;

	
			//模拟写入数据
			cout << "Mcu: sleeping for " << DATA_WRITING_TIME / 1000 << " seconds" << endl;
			Sleep(DATA_WRITING_TIME);
		
			cout << "Mcu: writing data to asic done!" << endl;

			sendWithAck(hcomm,(unsigned char *) "datawritten", strlen("datawritten"));
			--cnt;
			if(!cnt)
				break;			
		}

		--temcnt;
		if(!temcnt){
			cout << "\nMcu: i'm done!\n" << endl;
			return 0;
		}
	 }
 }
 
  
 
//bool receiveTem(char *rcv, int nDesire, int &nActual, bool hashead = false);
bool receiveTem(CSerialPort *pSerialPort, char *rcv, int &numWritten)
{  
	char byt;
	int i;
	bool lp = true;

	numWritten = 0;

    while (lp){  
		UINT BytesInQue = pSerialPort->GetBytesInCOM(); 
		if(BytesInQue == 0){
			Sleep(100);
			continue;
		}

		for(i = 0; i < BytesInQue; ++i){
			pSerialPort->ReadChar(byt);
			if(byt < '0' || byt > '9'){
				cout << "receiveTem: each byte supposed to be 0 ~ 9, and is not" << endl;
				return false;
			}
			rcv[numWritten++] = byt;
		}
		rcv[numWritten] = '\0';
		break;
		 
    }  
	cout << "\"" << rcv << "\"" << "received!" << endl;
    return true;  
}
bool send(CSerialPort *pSerialPort, unsigned char *tosend, const int n)
{
	//DWORD evtmasksave, temp;
	//GetCommMask(pSerialPort ->m_hComm, &evtmasksave);//备份mask
	//SetCommMask(pSerialPort ->m_hComm, EV_RXCHAR);

	//EnterCriticalSection(&pSerialPort ->m_csCommunicationSync);
	pSerialPort ->WriteData(tosend, n);//发送
	//LeaveCriticalSection(&pSerialPort ->m_csCommunicationSync);

	return true;
	
}
bool sendWithAck(CSerialPort *pSerialPort, unsigned char *tosend, const int n)
{
	//DWORD evtmasksave, temp;
	//GetCommMask(pSerialPort ->m_hComm, &evtmasksave);//备份mask
	//SetCommMask(pSerialPort ->m_hComm, EV_RXCHAR);

	pSerialPort ->WriteData(tosend, n);//发送

	cout << "\"" << tosend << "\"" << " is sent" << endl;

	//SetCommMask(pSerialPort ->m_hComm, evtmasksave);//还原mask
	return true;
	
}
bool receive(CSerialPort *pSerialPort, char *rcv, int &numWritten)
{  
	char byte;
	int i;
	numWritten = 0;

    while (true){
		UINT BytesInQue = pSerialPort->GetBytesInCOM(); 
		if(BytesInQue == 0){
			Sleep(100);
			continue;
		}

		for(i = 0; i < BytesInQue; ++i){
			pSerialPort->ReadChar(byte);
			rcv[numWritten++] = byte;
		}
		rcv[numWritten] = '\0';
		break;
		 
    }  
	cout << "\"" << rcv << "\"" << "received!" << endl;
    return true;  
}
