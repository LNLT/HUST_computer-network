#include "stdafx.h"
#include "Global.h"
#include "gnbRdtSender.h"

FILE* Window = fopen("window.txt", "w");
gnbRdtSender::gnbRdtSender() :SequenceNumber(8), winlen(4), base(0), expectSequenceNumberSend(0), waitingState(false)
{
	window = new deque<Packet>;//��ʼ������
}


gnbRdtSender::~gnbRdtSender()
{
}



bool gnbRdtSender::getWaitingState() {
	return (window->size() == winlen);//�޳��ȴ�
}




bool gnbRdtSender::send(const Message& message) {
	if (this->getWaitingState()) { //���ͷ����ڵȴ�ȷ��״̬
		return false;
	}

	this->packetWaitingAck.acknum = -1; //���Ը��ֶ�
	this->packetWaitingAck.seqnum = this->expectSequenceNumberSend;
	this->packetWaitingAck.checksum = 0;
	memcpy(this->packetWaitingAck.payload, message.data, sizeof(message.data));
	this->packetWaitingAck.checksum = pUtils->calculateCheckSum(this->packetWaitingAck);
	window->push_back(packetWaitingAck);//�������͵İ����봰�ڶ���

	pUtils->printPacket("���ͷ����ͱ���", this->packetWaitingAck);
	fprintf(Window,"���ͷ�����seqnum=%d\n", this->packetWaitingAck.seqnum);
	if (this->base == this->expectSequenceNumberSend) {
		pns->startTimer(SENDER, Configuration::TIME_OUT, this->packetWaitingAck.seqnum);			//�������ͷ���ʱ��
	}
	pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck);								//����ģ�����绷����sendToNetworkLayer��ͨ������㷢�͵��Է�
	
	this->expectSequenceNumberSend = (this->expectSequenceNumberSend+1) % this->SequenceNumber;
	
	return true;
}

void gnbRdtSender::receive(const Packet& ackPkt) {
	
	//���У����Ƿ���ȷ
	int checkSum = pUtils->calculateCheckSum(ackPkt);
	//���У�����ȷ������ȷ����Ų��Ƿ��ͷ���ȷ�ϵ����ݰ����
	if (checkSum == ackPkt.checksum && ackPkt.acknum != (this->base + 7) % 8) {
		pUtils->printPacket("���ͷ���ȷ�յ�ȷ��", ackPkt);
		fprintf(Window, "���ͷ���ȷ�յ�ȷ��acknum=%d\n", ackPkt.acknum);
		fprintf(Window, "��������֮ǰbase=%d,size=%d,\n", this->base, window->size());
		for (int i = 0; i < window->size(); i++) {
			fprintf(Window, "���ͷ�����:seqnum=%d\n", window->at(i).seqnum, window->at(i));
		}
		while (this->base != (ackPkt.acknum + 1) % this->SequenceNumber) {//��������
			pns->stopTimer(SENDER, this->base);
			window->pop_front();
			this->base = (this->base + 1) % this->SequenceNumber;
		}
		printf("��������֮��base=%d,size=%d\n", this->base,window->size());
		fprintf(Window,"��������֮��base=%d,size=%d\n", this->base, window->size());
		for (int i = 0; i < window->size(); i++) {
			fprintf(Window, "���ͷ�����:seqnum=%d\n", window->at(i).seqnum);
		}
		if (window->size()!=0) {
			pns->startTimer(SENDER, Configuration::TIME_OUT, window->front().seqnum);//�Ե�һ��������ſ�����ʱ��
		}
	}
	else {
		if (checkSum != ackPkt.checksum) {
			pUtils->printPacket("���ͷ�û����ȷ�յ��ñ���ȷ��,����У�����", ackPkt);
			fprintf(Window, "���ͷ�û����ȷ�յ��ñ���ȷ�ϣ�����У�����\n");
		}
		else if(ackPkt.acknum == (this->base + 7) % 8){
			pUtils->printPacket("���ͷ�����ȷ�յ����ñ���ȷ��", ackPkt);
			fprintf(Window, "���ͷ�����ȷ�յ����ñ���ȷ��,acknum=%d\n", ackPkt.acknum);
		}
	}
}

void gnbRdtSender::timeoutHandler(int seqNum) {
	//Ψһһ����ʱ��,���迼��seqNum
	pUtils->printPacket("���ͷ���ʱ��ʱ�䵽���ط��ϴη��͵ı���", this->packetWaitingAck);
	fprintf(Window, "���ͷ���ʱ��ʱ�䵽��base=%d,size=%d,\n", this->base, window->size());
	//���·������ݰ�
	pns->stopTimer(SENDER, window->front().seqnum);										//���ȹرն�ʱ��
	pns->startTimer(SENDER, Configuration::TIME_OUT, window->front().seqnum);			//�����������ͷ���ʱ��
	//printf("����seqnum=%d�ļ�ʱ��\n", window->front().seqnum);
	for (deque<Packet>::iterator itor = window->begin(); itor != window->end(); ++itor) {
		pns->sendToNetworkLayer(RECEIVER, *itor);
	}
}


