#include "tcp_sender.hh"
#include "tcp_config.hh"
#include <iostream>

#include <random>

using namespace std;

/* TCPSender constructor (uses a random ISN if none given) */
TCPSender::TCPSender( uint64_t initial_RTO_ms, optional<Wrap32> fixed_isn )
  : isn_( fixed_isn.value_or( Wrap32 { random_device()() } ) ), initial_RTO_ms_( initial_RTO_ms )
  ,abs_seq(0),retransmissions_times(0),senderMessageDeque(),sendedNoReceiveMessageDeque(),window_size(1),oustanding_size(0),tick_tag(false),isn_tag(true),isn_fin(false)
  ,timer_start_tag(false),cur_RTO_ms_(initial_RTO_ms),nonzero_window_size(1)
{}

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  // Your code here.
  return oustanding_size;
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  // Your code here.
  return retransmissions_times;
}

optional<TCPSenderMessage> TCPSender::maybe_send()
{
  // Your code here.
  if(tick_tag){
    std::optional<TCPSenderMessage> message_optional(sendedNoReceiveMessageDeque.front());
    tick_tag = false;
    return message_optional;
  }else{
    if(senderMessageDeque.size()== 0){
      return nullopt;
    }
    TCPSenderMessage sendMessage = senderMessageDeque.front();
    std::optional<TCPSenderMessage> message_optional(sendMessage);
    sendedNoReceiveMessageDeque.push_back(sendMessage);
    // oustanding_size += sendMessage.sequence_length();
    senderMessageDeque.pop_front();
    //启动定时器
    timer_start_tag = true;
    return message_optional;
  }
}

void TCPSender::push( Reader& outbound_stream )
{
  // Your code here.
  //如果当前发送的小于接收窗口就一直读取
  while( oustanding_size < window_size){
    TCPSenderMessage newMessage;
    //查看是否为首次
     if(isn_tag){
      newMessage.seqno = isn_;
      newMessage.SYN = true;
      isn_tag = false;
    }else{
      newMessage.seqno = Wrap32::wrap(abs_seq,isn_);
    }
    //取剩余窗口、单条消息最大值、剩余的消息中最大值
    size_t len = min(min(static_cast<size_t> (window_size-oustanding_size),TCPConfig::MAX_PAYLOAD_SIZE),
    static_cast<size_t>(outbound_stream.bytes_buffered()));
    //填充newMessage数据
    read(outbound_stream,len,newMessage.payload);
    //判断流是否结束，结束则填充fin
    if(outbound_stream.is_finished() && !isn_fin && newMessage.sequence_length() + oustanding_size < window_size){
      newMessage.FIN = true;
      isn_fin = true;
    }
    //添加数据到发送队列
    if(newMessage.sequence_length()==0){
      break;
    }else{
      senderMessageDeque.push_back(newMessage);
      oustanding_size += newMessage.sequence_length();
    }
    abs_seq += newMessage.sequence_length();
    if(newMessage.FIN){
      return;
    }
  }
  
 
  (void)outbound_stream;
}

TCPSenderMessage TCPSender::send_empty_message() const
{
  TCPSenderMessage tcpSenderMessage;
  tcpSenderMessage.seqno = Wrap32::wrap(abs_seq,isn_);
  // Your code here.
  return tcpSenderMessage;
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  // Your code here.
  // cout << "-------------------------receive"<<endl;
  //如果接收方窗口为0，设置成1，这样push可以尝试对方是否有新的空余空间
  if(msg.window_size == 0){
    window_size = 1;
    nonzero_window_size = 0;
  }else{
    window_size = msg.window_size;
    nonzero_window_size = 1;
  }
  // 过滤小于首个未接受的seqNo和大于最大值的数据
  if(msg.ackno.has_value()){
    uint64_t msgAckNo = msg.ackno.value().unwrap(isn_,abs_seq);
    if( msgAckNo > abs_seq
    ||msgAckNo < sendedNoReceiveMessageDeque.front().seqno.unwrap(isn_,abs_seq)){
      //传回来的ackNo不能是下个为啥发送的序号
      return;
    }
    // 减少发送但未完成的字节数
    // 删除小于ackNo的发送的但未收到ack的段
    // 忽略了segment重组的情况，push的时候也没兼容所以最后会相等
    // cout << "-------------------------firstUnAckMessage.seqno.unwrap(isn_,abs_seq)" << firstUnAckMessage.seqno.unwrap(isn_,abs_seq) <<endl;
    // cout << "-------------------------firstUnAckMessage.sequence_length()" <<firstUnAckMessage.sequence_length()<<endl;
    while (oustanding_size != 0 && sendedNoReceiveMessageDeque.front().seqno.unwrap(isn_,abs_seq)+sendedNoReceiveMessageDeque.front().sequence_length() <= msgAckNo )
    {
      oustanding_size -= sendedNoReceiveMessageDeque.front().sequence_length();
      sendedNoReceiveMessageDeque.pop_front();
      // cout << "-------------------------receive oustanding_size:" <<oustanding_size<<endl;
      //定时器处理,第一条消息syn如果一直失败就表示连接失败，由上层继续发起连接，而不是传输过程数据分组丢失
      if(oustanding_size == 0){
        timer_start_tag = false;
      }else{
        timer_start_tag = true;
      }
      retransmissions_times = 0;
      cur_RTO_ms_ = initial_RTO_ms_;
    }
    // cout << "-------------------------receive end: " << oustanding_size <<endl;
  }

  (void)msg;
}

void TCPSender::tick( const size_t ms_since_last_tick )
{
  // Your code here.
  if(timer_start_tag){
    cur_RTO_ms_ -=ms_since_last_tick;
  }
  if(cur_RTO_ms_ <= 0){
    // 用以发送最旧的一条数据
    tick_tag = true;
    // 如果window_size为1，除了第一次，可能为空，执行指数退避策略
    retransmissions_times ++;
    if(nonzero_window_size >= 1){
      cur_RTO_ms_ = pow(2,retransmissions_times)* initial_RTO_ms_;
    }else{
      cur_RTO_ms_ = initial_RTO_ms_;
    }
  }
  (void)ms_since_last_tick;
}
