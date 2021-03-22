#include "stdafx.h"
#include "Global.h"
#include "SRRdtSender.h"

FILE* Window = fopen("F:\\vs\\StopWait\\SR\\Debug\\sndwindow.txt", "w");
SRRdtSender::SRRdtSender() :SequenceNumber(8), winlen(4), base(0), expectSequenceNumberSend(0), waitingState(false)
{
	window = new deque<sndPkt>;//��ʼ������
}


SRRdtSender::~SRRdtSender()
{
}



bool SRRdtSender::getWaitingState() {
	return (window->size() == winlen);//�޳��ȴ�
}




bool SRRdtSender::send(const Message& message) {
	if (window->size() == winlen) { //���ͷ����ڵȴ�ȷ��״̬
		return false;
	}

	this->packetWaitingAck.Pkt.acknum = -1; //���Ը��ֶ�
	this->packetWaitingAck.Pkt.seqnum = this->expectSequenceNumberSend;
	this->packetWaitingAck.Pkt.checksum = 0;
	this->packetWaitingAck.mark= false;
	memcpy(this->packetWaitingAck.Pkt.payload, message.data, sizeof(message.data));
	this->packetWaitingAck.Pkt.checksum = pUtils->calculateCheckSum(this->packetWaitingAck.Pkt);
	window->push_back(packetWaitingAck);//�������͵İ����봰�ڶ���

	pUtils->printPacket("���ͷ����ͱ���", this->packetWaitingAck.Pkt);
	fprintf(Window,"���ͷ�����seqnum=%d\n", this->packetWaitingAck.Pkt.seqnum);
	pns->startTimer(SENDER, Configuration::TIME_OUT, this->packetWaitingAck.Pkt.seqnum);			//�������ͷ���ʱ��
	pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck.Pkt);								//����ģ�����绷����sendToNetworkLayer��ͨ������㷢�͵��Է�
	
	this->expectSequenceNumberSend = (this->expectSequenceNumberSend+1) % this->SequenceNumber;
	
	return true;
}

void SRRdtSender::receive(const Packet& ackPkt) {
	
	//���У����Ƿ���ȷ
	int checkSum = pUtils->calculateCheckSum(ackPkt);
	int offset = (ackPkt.acknum - this->base + 8) % 8;//���㱨���ڴ��ڵ�ƫ����
	//���У�����ȷ������ȷ�������Ҫ��ȡ�ķ�Χ���Ҹ�ȷ�ϱ���δ���յ���
	if (checkSum == ackPkt.checksum && offset<window->size() && window->at(offset).mark != true) {
		window->at(offset).mark = true;//����յ�ack
		pns->stopTimer(SENDER, ackPkt.acknum);//ֹͣ�ñ��ĵĶ�ʱ��
		pUtils->printPacket("���ͷ���ȷ�յ�ȷ��", ackPkt);

		fprintf(Window, "���ͷ���ȷ�յ�ȷ��acknum=%d\n", ackPkt.acknum);//�����ļ�
		fprintf(Window, "���ͷ���������֮ǰbase=%d,size=%d,\n", this->base, window->size());
		for (int i = 0; i < window->size(); i++) {
			fprintf(Window, "seqnum=%d,mark=%d\n", window->at(i).Pkt.seqnum, window->at(i).mark);
		}

		while ( window->size() != 0 && window->front().mark == true ) {//��������
			window->pop_front();
			this->base = (this->base + 1) % this->SequenceNumber;
		}

		fprintf(Window,"���ͷ���������֮��base=%d,size=%d,\n", this->base, window->size());//�����ļ�
		for (int i = 0; i < window->size(); i++) {
			fprintf(Window, "seqnum=%d,mark=%d\n", window->at(i).Pkt.seqnum, window->at(i).mark);
		}
	}
}

void SRRdtSender::timeoutHandler(int seqNum) {
	pns->stopTimer(SENDER, seqNum);
	int offset = (seqNum - this->base + 8) % this->SequenceNumber;
	//���·������ݰ�
	if (offset < window->size()) {
		pUtils->printPacket("���ͷ���ʱ��ʱ�䵽���ط�����", window->at(offset).Pkt);
		fprintf(Window, "���ͷ���ʱ��ʱ�䵽���ط�����seqnum=%d��base=%d\n", seqNum, this->base);
		pns->startTimer(SENDER, Configuration::TIME_OUT, seqNum);			//�����������ͷ���ʱ��
		pns->sendToNetworkLayer(RECEIVER, window->at(offset).Pkt);
	}
	else {
		pUtils->printPacket("���ͷ���ʱ��ʱ�䵽���ñ����ѵõ�ȷ��", window->at(offset).Pkt);
		fprintf(Window, "���ͷ���ʱ��ʱ�䵽�����ͷ���ʱ��ʱ�䵽���ñ����ѵõ�ȷ��seqnum=%d��base=%d\n", seqNum, this->base);
	}

}


