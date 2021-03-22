// StopWait.cpp : �������̨Ӧ�ó������ڵ㡣
//
#include "stdafx.h"
#include "Global.h"
#include "RdtSender.h"
#include "RdtReceiver.h"
#include "gnbRdtSender.h"
#include "gnbRdtReceiver.h"


int main(int argc, char* argv[])
{
	RdtSender *ps = new gnbRdtSender();
	RdtReceiver * pr = new gnbRdtReceiver();
//	pns->setRunMode(0);  //VERBOSģʽ
	pns->setRunMode(1);  //����ģʽ
	pns->init();
	pns->setRtdSender(ps);
	pns->setRtdReceiver(pr);
	pns->setInputFile("F:\\vs\\StopWait\\GBN\\Debug\\input.txt");
	pns->setOutputFile("F:\\vs\\StopWait\\GBN\\Debug\\OUTPUT.txt");

	pns->start();

	delete ps;
	delete pr;
	delete pUtils;									//ָ��Ψһ�Ĺ�����ʵ����ֻ��main��������ǰdelete
	delete pns;										//ָ��Ψһ��ģ�����绷����ʵ����ֻ��main��������ǰdelete
	
	return 0;
}

