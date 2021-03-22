#include "stdafx.h"
#include "Global.h"
#include "gnbRdtSender.h"

FILE* Window = fopen("window.txt", "w");
gnbRdtSender::gnbRdtSender() :SequenceNumber(8), winlen(4), base(0), expectSequenceNumberSend(0), waitingState(false)
{
	window = new deque<Packet>;//初始化窗口
}


gnbRdtSender::~gnbRdtSender()
{
}



bool gnbRdtSender::getWaitingState() {
	return (window->size() == winlen);//限长等待
}




bool gnbRdtSender::send(const Message& message) {
	if (this->getWaitingState()) { //发送方处于等待确认状态
		return false;
	}

	this->packetWaitingAck.acknum = -1; //忽略该字段
	this->packetWaitingAck.seqnum = this->expectSequenceNumberSend;
	this->packetWaitingAck.checksum = 0;
	memcpy(this->packetWaitingAck.payload, message.data, sizeof(message.data));
	this->packetWaitingAck.checksum = pUtils->calculateCheckSum(this->packetWaitingAck);
	window->push_back(packetWaitingAck);//将待发送的包加入窗口队列

	pUtils->printPacket("发送方发送报文", this->packetWaitingAck);
	fprintf(Window,"发送方发送seqnum=%d\n", this->packetWaitingAck.seqnum);
	if (this->base == this->expectSequenceNumberSend) {
		pns->startTimer(SENDER, Configuration::TIME_OUT, this->packetWaitingAck.seqnum);			//启动发送方定时器
	}
	pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck);								//调用模拟网络环境的sendToNetworkLayer，通过网络层发送到对方
	
	this->expectSequenceNumberSend = (this->expectSequenceNumberSend+1) % this->SequenceNumber;
	
	return true;
}

void gnbRdtSender::receive(const Packet& ackPkt) {
	
	//检查校验和是否正确
	int checkSum = pUtils->calculateCheckSum(ackPkt);
	//如果校验和正确，并且确认序号不是发送方已确认的数据包序号
	if (checkSum == ackPkt.checksum && ackPkt.acknum != (this->base + 7) % 8) {
		pUtils->printPacket("发送方正确收到确认", ackPkt);
		fprintf(Window, "发送方正确收到确认acknum=%d\n", ackPkt.acknum);
		fprintf(Window, "滑动窗口之前base=%d,size=%d,\n", this->base, window->size());
		for (int i = 0; i < window->size(); i++) {
			fprintf(Window, "发送方窗口:seqnum=%d\n", window->at(i).seqnum, window->at(i));
		}
		while (this->base != (ackPkt.acknum + 1) % this->SequenceNumber) {//滑动窗口
			pns->stopTimer(SENDER, this->base);
			window->pop_front();
			this->base = (this->base + 1) % this->SequenceNumber;
		}
		printf("滑动窗口之后base=%d,size=%d\n", this->base,window->size());
		fprintf(Window,"滑动窗口之后base=%d,size=%d\n", this->base, window->size());
		for (int i = 0; i < window->size(); i++) {
			fprintf(Window, "发送方窗口:seqnum=%d\n", window->at(i).seqnum);
		}
		if (window->size()!=0) {
			pns->startTimer(SENDER, Configuration::TIME_OUT, window->front().seqnum);//以第一个包的序号开启计时器
		}
	}
	else {
		if (checkSum != ackPkt.checksum) {
			pUtils->printPacket("发送方没有正确收到该报文确认,数据校验错误", ackPkt);
			fprintf(Window, "发送方没有正确收到该报文确认，数据校验错误\n");
		}
		else if(ackPkt.acknum == (this->base + 7) % 8){
			pUtils->printPacket("发送方已正确收到过该报文确认", ackPkt);
			fprintf(Window, "发送方已正确收到过该报文确认,acknum=%d\n", ackPkt.acknum);
		}
	}
}

void gnbRdtSender::timeoutHandler(int seqNum) {
	//唯一一个定时器,无需考虑seqNum
	pUtils->printPacket("发送方定时器时间到，重发上次发送的报文", this->packetWaitingAck);
	fprintf(Window, "发送方定时器时间到，base=%d,size=%d,\n", this->base, window->size());
	//重新发送数据包
	pns->stopTimer(SENDER, window->front().seqnum);										//首先关闭定时器
	pns->startTimer(SENDER, Configuration::TIME_OUT, window->front().seqnum);			//重新启动发送方定时器
	//printf("启动seqnum=%d的计时器\n", window->front().seqnum);
	for (deque<Packet>::iterator itor = window->begin(); itor != window->end(); ++itor) {
		pns->sendToNetworkLayer(RECEIVER, *itor);
	}
}


