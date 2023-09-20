#include "tcp_receiver.hh"
#include <iostream>
using namespace std;


void TCPReceiver::receive( TCPSenderMessage message, Reassembler& reassembler, Writer& inbound_stream )
{
  // Your code here.
  if(message.SYN){
    isn = Wrap32(message.seqno);
    //wrap32头文件重载了+号
    message.seqno = message.seqno+1;
    //避免连接关闭了还接受数据
    is_set_isn = true;
  }
  if (message.FIN)
  {
    fin = Wrap32(message.seqno+message.payload.size());
  }

  if(is_set_isn){
    reassembler.insert(message.seqno.unwrap(isn,inbound_stream.bytes_pushed())-1,message.payload,message.FIN,inbound_stream);
  }

  (void)message;
  (void)reassembler;
  (void)inbound_stream;
}

TCPReceiverMessage TCPReceiver::send( const Writer& inbound_stream ) const
{
  // Your code here.
  TCPReceiverMessage tcpReceiverMessage;
  // _first_unassembled_index先+1转成abs seqno，再wrap
  Wrap32 ackNo = Wrap32::wrap(inbound_stream.bytes_pushed()+1,isn);
  if(is_set_isn){
    //不能通过is_set_fin来判断，如果有冗余的fin先到，会导致多加1,用ackno比较则fin会被过滤，不会相等
    tcpReceiverMessage.ackno = ackNo==fin?ackNo+1:ackNo;
  }
  tcpReceiverMessage.window_size  = inbound_stream.available_capacity()>UINT16_MAX?UINT16_MAX:inbound_stream.available_capacity();
  (void)inbound_stream;
  return tcpReceiverMessage;
}
