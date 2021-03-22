#include "stdafx.h"
#include "Global.h"
#include "SRRdtReceiver.h"

FILE* rcvWindow = fopen("F:\\vs\\StopWait\\SR\\Debug\\rcvwindow.txt", "w");
SRRdtReceiver::SRRdtReceiver():expectSequenceNumberRcvd(0),winlen(4),base(0), SequenceNumber(8)
{
	window = new deque<rcvPkt>;//��ʼ������
	lastAckPkt.acknum = (expectSequenceNumberRcvd + 7) % 8; //��ʼ״̬�£��ϴη��͵�ȷ�ϰ���ȷ�����Ϊ-1��ʹ�õ���һ�����ܵ����ݰ�����ʱ��ȷ�ϱ��ĵ�ȷ�Ϻ�Ϊ-1
	lastAckPkt.checksum = 0;
	lastAckPkt.seqnum = -1;	//���Ը��ֶ�
	for(int i = 0; i < Configuration::PAYLOAD_SIZE;i++){
		lastAckPkt.payload[i] = '.';
	}
	lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
	for (int i = 0; i < winlen; i++) {
		rcvPkt blankPkt;//����δ��ǵ����ݰ�ռ������
		blankPkt.mark = false;
		window->push_back(blankPkt);
	}
}


SRRdtReceiver::~SRRdtReceiver()
{
}

void SRRdtReceiver::receive(const Packet &packet) {
	//���У����Ƿ���ȷ
	int checkSum = pUtils->calculateCheckSum(packet);
	int offset = (packet.seqnum - this->base + 8) % 8;
	//���У�����ȷ�����ұ�����Ҫ��ȡ�ķ�Χ���Ҹñ���δ���յ���
	if (checkSum == packet.checksum && offset<window->size() && window->at(offset).mark!=true){
		fprintf(rcvWindow,"���շ�base=%d,offset=%d\n", this->base,offset);
		window->at(offset).Pkt= packet;
		window->at(offset).mark = true;
		pUtils->printPacket("���շ���ȷ�յ����ͷ��ı���", packet);
		lastAckPkt.acknum = packet.seqnum; //ȷ����ŵ����յ��ı������
		lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
		pUtils->printPacket("���շ�����ȷ�ϱ���", lastAckPkt);
		pns->sendToNetworkLayer(SENDER, lastAckPkt);	//����ģ�����绷����sendToNetworkLayer��ͨ������㷢��ȷ�ϱ��ĵ��Է�
		
		fprintf(rcvWindow, "���շ���ȷ�յ����ͷ��ı���sequm=%d\n", packet.seqnum);//�����ļ�
		fprintf(rcvWindow, "���շ�����ȷ�ϱ���acknum=%d\n",lastAckPkt.acknum);
		fprintf(rcvWindow, "��������֮ǰbase=%d,size=%d,\n", this->base, window->size());
		for (int i = 0; i < window->size(); i++) {
			fprintf(rcvWindow,"sequm=%d,mark=%d\n",window->at(i).Pkt.seqnum, window->at(i).mark);
		}

		while(window->front().mark == true ) {//��������
			Message msg;
			memcpy(msg.data, window->front().Pkt.payload, sizeof(window->front().Pkt.payload));
			pns->delivertoAppLayer(RECEIVER, msg);//ģ�����绷�����͸�Ӧ�ò�
			this->base = (this->base + 1) % 8;
			this->expectSequenceNumberRcvd = (this->expectSequenceNumberRcvd + 1) % 8;//���������0-7֮���л�
			window->pop_front();
			rcvPkt blankPkt;//����δ��ǵ����ݰ�ռ������
			blankPkt.mark = false;
			window->push_back(blankPkt);
		}

		fprintf(rcvWindow, "��������֮��base=%d,size=%d,\n", this->base, window->size());//�����ļ�
		for (int i = 0; i < window->size(); i++) {
			fprintf(rcvWindow, "sequm=%d,mark=%d\n", window->at(i).Pkt.seqnum, window->at(i).mark);
		}
	}
	else {
		if (checkSum != packet.checksum) {
			pUtils->printPacket("���շ�û����ȷ�յ����ͷ��ı���,����У�����", packet);
			fprintf(rcvWindow, "���շ�û����ȷ�յ����ͷ��ı���,����У�����seqnum=%d\n", packet.seqnum);
		}
		else {
			pUtils->printPacket("���շ��յ���ȷ�ϵı���", packet);
			lastAckPkt.acknum = packet.seqnum; //ȷ����ŵ����յ��ı������
			lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
			pUtils->printPacket("���շ����·���ȷ�ϱ���", lastAckPkt);
			pns->sendToNetworkLayer(SENDER, lastAckPkt);	//����ģ�����绷����sendToNetworkLayer��ͨ������㷢��ȷ�ϱ��ĵ��Է�

			fprintf(rcvWindow, "���շ��յ���ȷ�ϵı���seqnum=%d\n", packet.seqnum);
			fprintf(rcvWindow, "���շ����·���ȷ�ϱ���acknum=%d\n", lastAckPkt.acknum);
		}
	}
}