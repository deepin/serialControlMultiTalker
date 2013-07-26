// main.cpp : Defines the entry point for the console application.  
//  
 
//#include "stdafx.h"  
#include "SerialPort.h"  
#include <Windows.h>
#include <iostream>  
 
typedef char * _TCHAR;
#define _tmain main


//unsigned int WINAPI processFun(void *pParam);
int _tmain(int argc, _TCHAR* argv[])  
{  
 
	hsemTemCome = CreateSemaphore(NULL, 0, 1, NULL);//
	hsemDataReady = CreateSemaphore(NULL, 0, 1, NULL);//
	hsemMeasDone = CreateSemaphore(NULL, 0, 1, NULL);//
	hsemNewTem = CreateSemaphore(NULL, 0, 1, NULL);

    CSerialPort mySerialPort1;  
	CSerialPort mySerialPort2; 
    CSerialPort mySerialPort3;  
	CSerialPort mySerialPort4;
    CSerialPort mySerialPort5;  
	CSerialPort mySerialPort6;

	HANDLE  thtable[6] = {0};
	//��ʼ��COM1
    if (!mySerialPort1.InitPort(1))  
    {  
        std::cout << "initPort fail !" << std::endl;  
    }  
    else 
    {  
        std::cout << "initPort success !" << std::endl;  
    }  
	//��ʼ��COM2
	if (!mySerialPort2.InitPort(2))  
    {  
        std::cout << "initPort fail !" << std::endl;  
    }  
    else 
    {  
        std::cout << "initPort success !" << std::endl;  
    }
	//��ʼ��COM3
    if (!mySerialPort3.InitPort(3))  
    {  
        std::cout << "initPort fail !" << std::endl;  
    }  
    else 
    {  
        std::cout << "initPort success !" << std::endl;  
    }  
	//��ʼ��COM4

	if (!mySerialPort4.InitPort(4))  
    {  
        std::cout << "initPort fail !" << std::endl;  
    }  
    else 
    {  
        std::cout << "initPort success !" << std::endl;  
    }

	if (!mySerialPort5.InitPort(5))  
    {  
        std::cout << "initPort fail !" << std::endl;  
    }  
    else 
    {  
        std::cout << "initPort success !" << std::endl;  
    }
	if (!mySerialPort6.InitPort(6))  
    {  
        std::cout << "initPort fail !" << std::endl;  
    }  
    else 
    {  
        std::cout << "initPort success !" << std::endl;  
    }


	//�߳�1--�����¶ȼ���
    if (!(thtable[0] = mySerialPort1.OpenThread(TemOvenListen)))  
    {  
        std::cout << "Open TemOvenListen Thread fail !" << std::endl;  
    }  
    else 
    {  
        std::cout << "Open TemOvenListen Thread success !" << std::endl;  
    }
	
	//�߳�2--֪ͨ�ɼ����ɼ�����
    if (!(thtable[1] = mySerialPort2.OpenThread(CardInformer))) 
    { 
        std::cout << "Open CardInformer Thread fail !" << std::endl;  
    }  
    else 
    {  
        std::cout << "Open CardInformer Thread success !" << std::endl;  
    }
	//�߳�3--��MCU���������ڶ�ȡtem1��ѹ��֪ͨMCU��ASIC������
    if (!(thtable[2] = mySerialPort3.OpenThread(McuComm)))  
    {  
        std::cout << "Open McuComm Thread fail !" << std::endl;  
    }  
    else 
    {  
        std::cout << "Open McuComm Thread success !" << std::endl;  
    }
	//�߳�4--ģ�ⵥƬ��

    if (!(thtable[3] = mySerialPort4.OpenThread(Mcu)))  
    {  
        std::cout << "Open Mcu Thread fail !" << std::endl;  
    }  
    else 
    {  
        std::cout << "Open Mcu Thread success !" << std::endl;  
    }

	//�߳�5--ģ������
    if (!(thtable[4] = mySerialPort5.OpenThread(TemOven)))
    {  
        std::cout << "Open TemOven Thread fail !" << std::endl;  
    }  
    else 
    {  
        std::cout << "Open TemOven Thread success !" << std::endl;  
    }
	//�߳�6--ģ��ɼ���
    if (!(thtable[5] = mySerialPort6.OpenThread(Card)))
    {  
        std::cout << "Open Card Thread fail !" << std::endl;
    }  
    else 
    {  
        std::cout << "Open Card Thread success !" << std::endl;  
    }

	//�ȴ��߳̽���,Ȼ�������������
	WaitForMultipleObjects(6, thtable, true, INFINITE);

	CloseHandle(hsemTemCome);
	CloseHandle(hsemDataReady);
	CloseHandle(hsemMeasDone);
	CloseHandle(hsemNewTem);
	system("notify.wav");
	system("temperatures.txt");
    return 0;  
} 