#pragma once

#include "byte_stream.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"

class TCPSender
{
  Wrap32 isn_;
  uint64_t initial_RTO_ms_;
  // 下个未发送的序号（所有已经发送的大小）
  uint64_t abs_seq;
  // 重试次数
  uint64_t retransmissions_times;
  // 所有准备好发送的段
  std::deque<TCPSenderMessage> senderMessageDeque;
  // 发送但是没收到ack的段
  std::deque<TCPSenderMessage> sendedNoReceiveMessageDeque;
  //此时已知的窗口大小
  uint16_t window_size;
  //发送但未完成的字节数
  uint16_t oustanding_size;
  //是否重发
  bool tick_tag;
  //是否首次发送
  bool isn_tag;
   //是否结束了，防止结束时多次调用push
  bool isn_fin;
  //定时器是否启动;
  bool timer_start_tag;
  //当前倒计时时间
  int cur_RTO_ms_;
  //因为window_size=0时，需要当成1，所以需要另一个参数配合定时器超时窗口非0情况
  uint16_t nonzero_window_size;

public:
  /* Construct TCP sender with given default Retransmission Timeout and possible ISN */
  TCPSender( uint64_t initial_RTO_ms, std::optional<Wrap32> fixed_isn );

  /* Push bytes from the outbound stream */
  void push( Reader& outbound_stream );

  /* Send a TCPSenderMessage if needed (or empty optional otherwise) */
  std::optional<TCPSenderMessage> maybe_send();

  /* Generate an empty TCPSenderMessage */
  TCPSenderMessage send_empty_message() const;

  /* Receive an act on a TCPReceiverMessage from the peer's receiver */
  void receive( const TCPReceiverMessage& msg );

  /* Time has passed by the given # of milliseconds since the last time the tick() method was called. */
  void tick( uint64_t ms_since_last_tick );

  /* Accessors for use in testing */
  uint64_t sequence_numbers_in_flight() const;  // How many sequence numbers are outstanding?
  uint64_t consecutive_retransmissions() const; // How many consecutive *re*transmissions have happened?
};
