#include "stdafx.h"
#include "Global.h"
#include "SRRdtSender.h"

FILE* Window = fopen("F:\\vs\\StopWait\\SR\\Debug\\sndwindow.txt", "w");
SRRdtSender::SRRdtSender() :SequenceNumber(8), winlen(4), base(0), expectSequenceNumberSend(0), waitingState(false)
{
	window = new deque<sndPkt>;//初始化窗口
}


SRRdtSender::~SRRdtSender()
{
}



bool SRRdtSender::getWaitingState() {
	return (window->size() == winlen);//限长等待
}




bool SRRdtSender::send(const Message& message) {
	if (window->size() == winlen) { //发送方处于等待确认状态
		return false;
	}

	this->packetWaitingAck.Pkt.acknum = -1; //忽略该字段
	this->packetWaitingAck.Pkt.seqnum = this->expectSequenceNumberSend;
	this->packetWaitingAck.Pkt.checksum = 0;
	this->packetWaitingAck.mark= false;
	memcpy(this->packetWaitingAck.Pkt.payload, message.data, sizeof(message.data));
	this->packetWaitingAck.Pkt.checksum = pUtils->calculateCheckSum(this->packetWaitingAck.Pkt);
	window->push_back(packetWaitingAck);//将待发送的包加入窗口队列

	pUtils->printPacket("发送方发送报文", this->packetWaitingAck.Pkt);
	fprintf(Window,"发送方发送seqnum=%d\n", this->packetWaitingAck.Pkt.seqnum);
	pns->startTimer(SENDER, Configuration::TIME_OUT, this->packetWaitingAck.Pkt.seqnum);			//启动发送方定时器
	pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck.Pkt);								//调用模拟网络环境的sendToNetworkLayer，通过网络层发送到对方
	
	this->expectSequenceNumberSend = (this->expectSequenceNumberSend+1) % this->SequenceNumber;
	
	return true;
}

void SRRdtSender::receive(const Packet& ackPkt) {
	
	//检查校验和是否正确
	int checkSum = pUtils->calculateCheckSum(ackPkt);
	int offset = (ackPkt.acknum - this->base + 8) % 8;//计算报文在窗口的偏移量
	//如果校验和正确，并且确认序号在要收取的范围内且该确认报文未被收到过
	if (checkSum == ackPkt.checksum && offset<window->size() && window->at(offset).mark != true) {
		window->at(offset).mark = true;//标记收到ack
		pns->stopTimer(SENDER, ackPkt.acknum);//停止该报文的定时器
		pUtils->printPacket("发送方正确收到确认", ackPkt);

		fprintf(Window, "发送方正确收到确认acknum=%d\n", ackPkt.acknum);//定向到文件
		fprintf(Window, "发送方滑动窗口之前base=%d,size=%d,\n", this->base, window->size());
		for (int i = 0; i < window->size(); i++) {
			fprintf(Window, "seqnum=%d,mark=%d\n", window->at(i).Pkt.seqnum, window->at(i).mark);
		}

		while ( window->size() != 0 && window->front().mark == true ) {//滑动窗口
			window->pop_front();
			this->base = (this->base + 1) % this->SequenceNumber;
		}

		fprintf(Window,"发送方滑动窗口之后base=%d,size=%d,\n", this->base, window->size());//定向到文件
		for (int i = 0; i < window->size(); i++) {
			fprintf(Window, "seqnum=%d,mark=%d\n", window->at(i).Pkt.seqnum, window->at(i).mark);
		}
	}
}

void SRRdtSender::timeoutHandler(int seqNum) {
	pns->stopTimer(SENDER, seqNum);
	int offset = (seqNum - this->base + 8) % this->SequenceNumber;
	//重新发送数据包
	if (offset < window->size()) {
		pUtils->printPacket("发送方定时器时间到，重发报文", window->at(offset).Pkt);
		fprintf(Window, "发送方定时器时间到，重发报文seqnum=%d，base=%d\n", seqNum, this->base);
		pns->startTimer(SENDER, Configuration::TIME_OUT, seqNum);			//重新启动发送方定时器
		pns->sendToNetworkLayer(RECEIVER, window->at(offset).Pkt);
	}
	else {
		pUtils->printPacket("发送方定时器时间到，该报文已得到确认", window->at(offset).Pkt);
		fprintf(Window, "发送方定时器时间到，发送方定时器时间到，该报文已得到确认seqnum=%d，base=%d\n", seqNum, this->base);
	}

}


