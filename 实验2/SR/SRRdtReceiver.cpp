#include "stdafx.h"
#include "Global.h"
#include "SRRdtReceiver.h"

FILE* rcvWindow = fopen("F:\\vs\\StopWait\\SR\\Debug\\rcvwindow.txt", "w");
SRRdtReceiver::SRRdtReceiver():expectSequenceNumberRcvd(0),winlen(4),base(0), SequenceNumber(8)
{
	window = new deque<rcvPkt>;//初始化窗口
	lastAckPkt.acknum = (expectSequenceNumberRcvd + 7) % 8; //初始状态下，上次发送的确认包的确认序号为-1，使得当第一个接受的数据包出错时该确认报文的确认号为-1
	lastAckPkt.checksum = 0;
	lastAckPkt.seqnum = -1;	//忽略该字段
	for(int i = 0; i < Configuration::PAYLOAD_SIZE;i++){
		lastAckPkt.payload[i] = '.';
	}
	lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
	for (int i = 0; i < winlen; i++) {
		rcvPkt blankPkt;//生成未标记的数据包占满窗口
		blankPkt.mark = false;
		window->push_back(blankPkt);
	}
}


SRRdtReceiver::~SRRdtReceiver()
{
}

void SRRdtReceiver::receive(const Packet &packet) {
	//检查校验和是否正确
	int checkSum = pUtils->calculateCheckSum(packet);
	int offset = (packet.seqnum - this->base + 8) % 8;
	//如果校验和正确，并且报文在要收取的范围内且该报文未被收到过
	if (checkSum == packet.checksum && offset<window->size() && window->at(offset).mark!=true){
		fprintf(rcvWindow,"接收方base=%d,offset=%d\n", this->base,offset);
		window->at(offset).Pkt= packet;
		window->at(offset).mark = true;
		pUtils->printPacket("接收方正确收到发送方的报文", packet);
		lastAckPkt.acknum = packet.seqnum; //确认序号等于收到的报文序号
		lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
		pUtils->printPacket("接收方发送确认报文", lastAckPkt);
		pns->sendToNetworkLayer(SENDER, lastAckPkt);	//调用模拟网络环境的sendToNetworkLayer，通过网络层发送确认报文到对方
		
		fprintf(rcvWindow, "接收方正确收到发送方的报文sequm=%d\n", packet.seqnum);//定向到文件
		fprintf(rcvWindow, "接收方发送确认报文acknum=%d\n",lastAckPkt.acknum);
		fprintf(rcvWindow, "滑动窗口之前base=%d,size=%d,\n", this->base, window->size());
		for (int i = 0; i < window->size(); i++) {
			fprintf(rcvWindow,"sequm=%d,mark=%d\n",window->at(i).Pkt.seqnum, window->at(i).mark);
		}

		while(window->front().mark == true ) {//滑动窗口
			Message msg;
			memcpy(msg.data, window->front().Pkt.payload, sizeof(window->front().Pkt.payload));
			pns->delivertoAppLayer(RECEIVER, msg);//模拟网络环境发送给应用层
			this->base = (this->base + 1) % 8;
			this->expectSequenceNumberRcvd = (this->expectSequenceNumberRcvd + 1) % 8;//接收序号在0-7之间切换
			window->pop_front();
			rcvPkt blankPkt;//生成未标记的数据包占满窗口
			blankPkt.mark = false;
			window->push_back(blankPkt);
		}

		fprintf(rcvWindow, "滑动窗口之后base=%d,size=%d,\n", this->base, window->size());//定向到文件
		for (int i = 0; i < window->size(); i++) {
			fprintf(rcvWindow, "sequm=%d,mark=%d\n", window->at(i).Pkt.seqnum, window->at(i).mark);
		}
	}
	else {
		if (checkSum != packet.checksum) {
			pUtils->printPacket("接收方没有正确收到发送方的报文,数据校验错误", packet);
			fprintf(rcvWindow, "接收方没有正确收到发送方的报文,数据校验错误seqnum=%d\n", packet.seqnum);
		}
		else {
			pUtils->printPacket("接收方收到已确认的报文", packet);
			lastAckPkt.acknum = packet.seqnum; //确认序号等于收到的报文序号
			lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
			pUtils->printPacket("接收方重新发送确认报文", lastAckPkt);
			pns->sendToNetworkLayer(SENDER, lastAckPkt);	//调用模拟网络环境的sendToNetworkLayer，通过网络层发送确认报文到对方

			fprintf(rcvWindow, "接收方收到已确认的报文seqnum=%d\n", packet.seqnum);
			fprintf(rcvWindow, "接收方重新发送确认报文acknum=%d\n", lastAckPkt.acknum);
		}
	}
}