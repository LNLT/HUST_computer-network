#include "stdafx.h"
#include "Global.h"
#include "TcpRdtSender.h"

FILE* Window = fopen("F:\\vs\\StopWait\\Tcp\\Debug\\window.txt", "w");
TcpRdtSender::TcpRdtSender() :SequenceNumber(8), winlen(4), base(0), expectSequenceNumberSend(0), waitingState(false)
{
	window = new deque<Packet>;//��ʼ������
}


TcpRdtSender::~TcpRdtSender()
{
}



bool TcpRdtSender::getWaitingState() {
	return (window->size() == winlen);//�޳��ȴ�
}




bool TcpRdtSender::send(const Message& message) {
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
		printf("�������ͷ���ʱ��seqnum=%d\n", this->packetWaitingAck.seqnum);
	}
	pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck);								//����ģ�����绷����sendToNetworkLayer��ͨ������㷢�͵��Է�
	
	this->expectSequenceNumberSend = (this->expectSequenceNumberSend+1) % this->SequenceNumber;
	
	return true;
}
int i = 0;
void TcpRdtSender::receive(const Packet& ackPkt) {
	
	//���У����Ƿ���ȷ
	int checkSum = pUtils->calculateCheckSum(ackPkt);
	//���У�����ȷ������ȷ����Ų��Ƿ��ͷ���ȷ�ϵ����ݰ����
	if (checkSum == ackPkt.checksum && ackPkt.acknum != (this->base + 7) % 8) {
		pUtils->printPacket("���ͷ���ȷ�յ�ȷ��", ackPkt);
		fprintf(Window, "���ͷ���ȷ�յ�ȷ��acknum=%d\n", ackPkt.acknum);
		fprintf(Window, "��������֮ǰbase=%d,size=%d,\n", this->base, window->size());
		for (int i = 0; i < (window->size()); i++) {
			fprintf(Window, "���ͷ�����:seqnum=%d\n", window->at(i).seqnum);
		}
		while (this->base != (ackPkt.acknum + 1) % this->SequenceNumber) {//��������
			pns->stopTimer(SENDER, this->base);
			printf("�رշ��ͷ���ʱ��seqnum=%d\n", this->base);
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
			printf("�������ͷ���ʱ��seqnum=%d\n", window->front().seqnum);
		}
	}
	else {
		if (checkSum != ackPkt.checksum) {
			pUtils->printPacket("���ͷ�û����ȷ�յ��ñ���ȷ��,����У�����", ackPkt);
			fprintf(Window, "���ͷ�û����ȷ�յ��ñ���ȷ�ϣ�����У�����\n");
		}
		else if (ackPkt.acknum == (this->base + 7) % 8) {
			i += 1;
			pUtils->printPacket("���ͷ�����ȷ�յ����ñ���ȷ��", ackPkt);
			if (i == 3 && window->size()>0) {
				pns->sendToNetworkLayer(RECEIVER, window->front());
				fprintf(Window, "���ͷ�����ȷ�յ����ñ���ȷ��,acknum=%d,num=%d\n", ackPkt.acknum, i);
				fprintf(Window, "�����ش�seqnum=%d\n", window->front().seqnum);
				printf("�����ش�seqnum=%d\n", window->front().seqnum);
				i = 0;
			}
		}

	}
}

void TcpRdtSender::timeoutHandler(int seqNum) {
	//Ψһһ����ʱ��,���迼��seqNum
	pUtils->printPacket("���ͷ���ʱ��ʱ�䵽���ط��ϴη��͵ı���", window->front());
	fprintf(Window, "���ͷ���ʱ��ʱ�䵽��base=%d,size=%d,\n", this->base, window->size());
	//���·������ݰ�
	pns->stopTimer(SENDER, window->front().seqnum);										//���ȹرն�ʱ��
	pns->startTimer(SENDER, Configuration::TIME_OUT, window->front().seqnum);			//�����������ͷ���ʱ��
	printf("�����������ͷ���ʱ��seqnum=%d\n", window->front().seqnum);
	pns->sendToNetworkLayer(RECEIVER, window->front());
}


