#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"

using namespace std;

// ethernet_address: Ethernet (what ARP calls "hardware") address of the interface
// ip_address: IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface( const EthernetAddress& ethernet_address, const Address& ip_address )
  : ethernet_address_( ethernet_address ), ip_address_( ip_address ),
  send_ethernet(),wait_ip_datagrams(),sended_arp_req(),ip_to_Ethernet(),arp_ttl(5000),ip_ethernet_ttl(30000)
{
  cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address_ ) << " and IP address "
       << ip_address.ip() << "\n";
}

// dgram: the IPv4 datagram to be sent
// next_hop: the IP address of the interface to send it to (typically a router or default gateway, but
// may also be another host if directly connected to the same network as the destination)

// Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) by using the
// Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
  uint32_t next_hop_ipv4 = next_hop.ipv4_numeric();
  // 看当前ip映射Ethernet mapping 是否生效
  EthernetFrame frame;
  if(ip_to_Ethernet.find(next_hop_ipv4) != ip_to_Ethernet.end() ){
    frame.header.type = EthernetHeader::TYPE_IPv4;
    frame.header.src = ethernet_address_;
    frame.header.dst = ip_to_Ethernet.find(next_hop_ipv4)->second.first;
    frame.payload = serialize(dgram);
    //放到发送队列中
   send_ethernet.push_back(frame);
  }else{
    //将IP adress 转成 32-bit ，写到ARP中
    if(sended_arp_req.find(next_hop_ipv4) == sended_arp_req.end()){
      ARPMessage arp_message;
      arp_message.opcode = ARPMessage::OPCODE_REQUEST;
      arp_message.sender_ethernet_address = ethernet_address_;
      arp_message.sender_ip_address = ip_address_.ipv4_numeric();
      //以太网帧不知道才需要查
      arp_message.target_ip_address = next_hop_ipv4;
      frame.header.type = EthernetHeader::TYPE_ARP;
      frame.header.src = ethernet_address_;
      frame.header.dst = ETHERNET_BROADCAST;
      frame.payload = serialize(arp_message);
      //dgram进入排队队列，arp请求记录时间
      wait_ip_datagrams[next_hop_ipv4].push_back(dgram);
      sended_arp_req[next_hop_ipv4] = arp_ttl;
      //放到发送队列中
      send_ethernet.push_back(frame);
    }
  }
  
  (void)dgram;
  (void)next_hop;
}

// frame: the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame( const EthernetFrame& frame )
{
  //如果目标地址不是广播地址也不是本机Ethernet address
  if(frame.header.dst != ETHERNET_BROADCAST && frame.header.dst != ethernet_address_){
    return nullopt;
  }
  // 如果是IPv4
 
  if(frame.header.type == EthernetHeader::TYPE_IPv4){
    InternetDatagram dgram;
    if(parse(dgram,frame.payload)){
      return dgram;
    };
  }
  // 如果是ARP
  if(frame.header.type == EthernetHeader::TYPE_ARP){
      //返回
      ARPMessage arp_msg;
      if(parse(arp_msg,frame.payload)){
        if(arp_msg.opcode == ARPMessage::OPCODE_REPLY){
          //插入IP 到Ethernet ip的映射
          ip_to_Ethernet.insert(std::pair<uint32_t,std::pair<EthernetAddress,uint64_t>>
          (arp_msg.sender_ip_address,std::pair<EthernetAddress,uint64_t>(frame.header.src,ip_ethernet_ttl)));
          
          //去掉存放已发送的arp帧的ip和时间
          sended_arp_req.erase(arp_msg.sender_ip_address);

          //将 IP Datagram排队队列 数据取出，包装成frame 发送到Ethernet的发送队列
          if(wait_ip_datagrams.find(arp_msg.sender_ip_address) != wait_ip_datagrams.end()){
            for(InternetDatagram dgram:wait_ip_datagrams.find(arp_msg.sender_ip_address)->second){
              EthernetFrame newframe;
              newframe.header.type = EthernetHeader::TYPE_IPv4;
              newframe.header.src = ethernet_address_;
              newframe.header.dst = ip_to_Ethernet[arp_msg.sender_ip_address].first;
              newframe.payload = serialize(dgram);
              send_ethernet.push_back(newframe);
            }
            wait_ip_datagrams.erase(arp_msg.sender_ip_address);
          }
        }else if (arp_msg.opcode == ARPMessage::OPCODE_REQUEST)
        {
          // 如果是ARP请求
          //插入IP 到Ethernet ip的映射
          //只有对应ip相等才需要返回arp请求，避免网络拥塞
          if(arp_msg.target_ip_address == ip_address_.ipv4_numeric()){
            ip_to_Ethernet.insert(std::pair<uint32_t,std::pair<EthernetAddress,uint64_t>>
            (arp_msg.sender_ip_address,std::pair<EthernetAddress,uint64_t>(frame.header.src,ip_ethernet_ttl)));
            //可以将arp返回的处理重复一遍？
            //包装返回
            ARPMessage back_arp_message;
            EthernetFrame newframe;
            back_arp_message.opcode = ARPMessage::OPCODE_REPLY;
            back_arp_message.sender_ethernet_address = ethernet_address_;
            back_arp_message.sender_ip_address = ip_address_.ipv4_numeric();
            back_arp_message.target_ethernet_address = arp_msg.sender_ethernet_address;
            back_arp_message.target_ip_address = arp_msg.sender_ip_address;
            newframe.header.type = EthernetHeader::TYPE_ARP;
            newframe.header.src = ethernet_address_;
            newframe.header.dst = arp_msg.sender_ethernet_address;
            newframe.payload = serialize(back_arp_message);
            send_ethernet.push_back(newframe);
          }
        }
      }
  }
  (void)frame;
  return nullopt;
}

// ms_since_last_tick: the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  //arp请求是否超时，不需要考虑是否永远不回来要不要丢弃
   for (auto it = sended_arp_req.begin(); it != sended_arp_req.end();){
    it->second-=ms_since_last_tick;
    if(it->second <=0){
      it = sended_arp_req.erase(it);
    }else{
      it++;
    }
   }
 
  //ip-ethernet map是否到期

  for (auto it = ip_to_Ethernet.begin(); it != ip_to_Ethernet.end();){
    it->second.second-=ms_since_last_tick;
    if(it->second.second <=0){
      it = ip_to_Ethernet.erase(it);
    }else{
      ++it;
    }
   }
  (void)ms_since_last_tick;
}

optional<EthernetFrame> NetworkInterface::maybe_send()
{
  if(send_ethernet.empty()){
    return nullopt;
  }
  optional<EthernetFrame> opEthernetFram(send_ethernet.front());
  send_ethernet.pop_front();
  return opEthernetFram;
}
