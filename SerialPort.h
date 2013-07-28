//////////////////////////////////////////////////////////////////////////  
/// COPYRIGHT NOTICE  
/// Copyright (c) 2009, ���пƼ���ѧtickTick Group  ����Ȩ������  
/// All rights reserved.  
///   
/// @file    SerialPort.h    
/// @brief   ����ͨ����ͷ�ļ�  
///  
/// ���ļ���ɴ���ͨ���������  
///  
/// @version 1.0     
/// @author  ¬��   
/// @E-mail��lujun.hust@gmail.com  
/// @date    2010/03/19  
///  
///  �޶�˵����  
//////////////////////////////////////////////////////////////////////////  
#define _simulation


#ifndef SERIALPORT_H_  
#define SERIALPORT_H_  

#include <Windows.h>  
extern const unsigned int IN_BUF_SZ;
/** ����ͨ����  
 *     
 *  ����ʵ���˶Դ��ڵĻ�������  
 *  �����������ָ�����ڵ����ݡ�����ָ�����ݵ�����  
 */ 
class CSerialPort  
{  
public:  
    CSerialPort(void);  
    ~CSerialPort(void);  
 
public:  
      
    /** ��ʼ�����ں���  
     *  
     *  @param:  UINT portNo ���ڱ��,Ĭ��ֵΪ1,��COM1,ע��,������Ҫ����9  
     *  @param:  UINT baud   ������,Ĭ��Ϊ9600  
     *  @param:  char parity �Ƿ������żУ��,'Y'��ʾ��Ҫ��żУ��,'N'��ʾ����Ҫ��żУ��  
     *  @param:  UINT databits ����λ�ĸ���,Ĭ��ֵΪ8������λ  
     *  @param:  UINT stopsbits ֹͣλʹ�ø�ʽ,Ĭ��ֵΪ1  
     *  @param:  DWORD dwCommEvents Ĭ��ΪEV_RXCHAR,��ֻҪ�շ�����һ���ַ�,�����һ���¼�  
     *  @return: bool  ��ʼ���Ƿ�ɹ�  
     *  @note:   ��ʹ�����������ṩ�ĺ���ǰ,���ȵ��ñ��������д��ڵĳ�ʼ��  
     *����������  \n�������ṩ��һЩ���õĴ��ڲ�������,����Ҫ����������ϸ��DCB����,��ʹ�����غ���  
     *           \n������������ʱ���Զ��رմ���,�������ִ�йرմ���  
     *  @see:      
     */ 
    bool InitPort( UINT  portNo = 1,UINT  baud = CBR_1200,char  parity = 'Y',UINT  databits = 8, UINT  stopsbits = 1,DWORD dwCommEvents = EV_RXCHAR);  
 
    /** ���ڳ�ʼ������  
     *  
     *  �������ṩֱ�Ӹ���DCB�������ô��ڲ���  
     *  @param:  UINT portNo  
     *  @param:  const LPDCB & plDCB  
     *  @return: bool  ��ʼ���Ƿ�ɹ�  
     *  @note:   �������ṩ�û��Զ���ش��ڳ�ʼ������  
     *  @see:      
     */ 
    bool InitPort( UINT  portNo ,const LPDCB& plDCB );  
 
    /** ���������߳�  
     *  
     *  �������߳���ɶԴ������ݵļ���,�������յ������ݴ�ӡ����Ļ���  
     *  @return: bool  �����Ƿ�ɹ�  
     *  @note:   ���߳��Ѿ����ڿ���״̬ʱ,����flase  
     *  @see:      
     */ 
    bool OpenListenThread();  
 
    /** �رռ����߳�  
     *  
     *    
     *  @return: bool  �����Ƿ�ɹ�  
     *  @note:   ���ñ�������,�������ڵ��߳̽��ᱻ�ر�  
     *  @see:      
     */ 
	HANDLE OpenThread(unsigned int (WINAPI * pFun)(void *));

	friend unsigned int WINAPI TemOvenListen(void *pParam);
	friend unsigned int WINAPI CardInformer(void *pParam);
	friend unsigned int WINAPI McuComm(void *pParam);
	friend unsigned int WINAPI Mcu(void *pParam);
	friend unsigned int WINAPI Card(void *pParam);
	friend unsigned int WINAPI TemOven(void *pParam);

	friend bool receiveTem(CSerialPort *pSerialPort, char *rcv, int &nActual);
	friend bool receive(CSerialPort *pSerialPort, char *rcv, int &nActual);
	friend bool sendWithAck(CSerialPort *pSerialPort, unsigned char *tosend, const int n);
	friend bool send(CSerialPort *pSerialPort, unsigned char *tosend, const int n);




    bool CloseListenTread();  
 


	bool OpenThreadTemOven(void);
    /** �򴮿�д����  
     *  
     *  ���������е�����д�뵽����  
     *  @param:  unsigned char * pData ָ����Ҫд�봮�ڵ����ݻ�����  
     *  @param:  unsigned int length ��Ҫд������ݳ���  
     *  @return: bool  �����Ƿ�ɹ�  
     *  @note:   length��Ҫ����pData��ָ�򻺳����Ĵ�С  
     *  @see:      
     */ 
    bool WriteData(unsigned char* pData, unsigned int length);  
 
    /** ��ȡ���ڻ������е��ֽ���  
     *  
     *    
     *  @return: UINT  �����Ƿ�ɹ�  
     *  @note:   �����ڻ�������������ʱ,����0  
     *  @see:      
     */ 
    UINT GetBytesInCOM();  
	UINT CSerialPort::GetOutgoingBytesInCOM();
    /** ��ȡ���ڽ��ջ�������һ���ֽڵ�����  
     *  
     *    
     *  @param:  char & cRecved ��Ŷ�ȡ���ݵ��ַ�����  
     *  @return: bool  ��ȡ�Ƿ�ɹ�  
     *  @note:     
     *  @see:      
     */ 
    bool ReadChar(char &cRecved);  
 
private:  
 
    /** �򿪴���  
     *  
     *    
     *  @param:  UINT portNo �����豸��  
     *  @return: bool  ���Ƿ�ɹ�  
     *  @note:     
     *  @see:      
     */ 
    bool openPort( UINT  portNo );  
 
    /** �رմ���  
     *  
     *    
     *  @return: void  �����Ƿ�ɹ�  
     *  @note:     
     *  @see:      
     */ 
    void ClosePort();  
      
    /** ���ڼ����߳�  
     *  
     *  �������Դ��ڵ����ݺ���Ϣ  
     *  @param:  void * pParam �̲߳���  
     *  @return: UINT WINAPI �̷߳���ֵ  
     *  @note:     
     *  @see:      
     */ 
    static UINT WINAPI ListenThread(void* pParam);  
	static UINT WINAPI ListenThreadEvt( void* pParam );
	static UINT WINAPI ListenThreadSem( void* pParam );
private:  
 
    /** ���ھ�� */   
    HANDLE  m_hComm;  
 
    /** �߳��˳���־���� */   
    static bool s_bExit;  
 
    /** �߳̾�� */   
    volatile HANDLE    m_hListenThread;  
 
    /** ͬ������,�ٽ������� */   
    CRITICAL_SECTION   m_csCommunicationSync;       //!< �����������  
 
};  
	extern HANDLE hsemTemCome, hsemDataReady, hsemMeasDone, hsemNewTem;

	extern const unsigned int TemCnt, RoundPerTem;

	unsigned int WINAPI TemOvenListen(void *pParam);
	unsigned int WINAPI CardInformer(void *pParam);
	unsigned int WINAPI Mcu(void *pParam);
	unsigned int WINAPI McuComm(void *pParam);
	unsigned int WINAPI Card(void *pParam);
	unsigned int WINAPI TemOven(void *pParam);

	bool receiveTem(CSerialPort *pSerialPort, char *rcv, int &nActual);
	bool receive(CSerialPort *pSerialPort, char *rcv, int &nActual);
	bool sendWithAck(CSerialPort *pSerialPort, unsigned char *tosend, const int n);
	bool send(CSerialPort *pSerialPort, unsigned char *tosend, const int n);
#endif //SERIALPORT_H_