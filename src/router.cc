#include "router.hh"

#include <iostream>
#include <limits>

using namespace std;

// route_prefix: The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
// prefix_length: For this route to be applicable, how many high-order (most-significant) bits of
//    the route_prefix will need to match the corresponding bits of the datagram's destination address?
// next_hop: The IP address of the next hop. Will be empty if the network is directly attached to the router (in
//    which case, the next hop address should be the datagram's final destination).
// interface_num: The index of the interface to send the datagram out on.
void Router::add_route( const uint32_t route_prefix,
                        const uint8_t prefix_length,
                        const optional<Address> next_hop,
                        const size_t interface_num )
{
  cerr << "DEBUG: adding route " << Address::from_ipv4_numeric( route_prefix ).ip() << "/"
       << static_cast<int>( prefix_length ) << " => " << ( next_hop.has_value() ? next_hop->ip() : "(direct)" )
       << " on interface " << interface_num << "\n";
  //保存数据到_route_table
  _route_table.emplace(make_pair(route_prefix,prefix_length),_route_data{next_hop,interface_num});
  (void)route_prefix;
  (void)prefix_length;
  (void)next_hop;
  (void)interface_num;
}
inline std::string concat( std::vector<Buffer>& buffers )
{
  return std::accumulate(
    buffers.begin(), buffers.end(), std::string {}, []( const std::string& x, const Buffer& y ) {
      return x + static_cast<std::string>( y );
    } );
}

void Router::route() {
  
  for(auto&  async:interfaces_){
    std::optional<InternetDatagram> ip_dgram;
    while (true)
    {
      ip_dgram = async.maybe_receive();
      if(ip_dgram == nullopt){
        break;
      }
      if(ip_dgram.has_value()){
        //找到下一跳对应的发送接口
        InternetDatagram dgram = ip_dgram.value();
        //ttl 用完则抛弃
        if(dgram.header.ttl > 0 ){
          dgram.header.ttl--;
        }
        if(dgram.header.ttl == 0){
          continue;
        }
        dgram.header.compute_checksum();
        //默认路由
        // std::pair<std::pair<uint32_t,uint8_t>,_route_data>  default_route;
        //最佳路由
        std::pair<std::pair<uint32_t,uint8_t>,_route_data>  best_route;
        //判断是否有匹配的路由
        bool has_best{false};
        for(auto&& route : _route_table){
          uint8_t len = 32- route.first.second;
          //移动32位可能会出现异常,代表的是默认的route
          if(!has_best &&len == 32){
            best_route = route;
            has_best = true;
            continue;
          }
          std::cout << "--------------------------route"<< route.first.first <<endl;
          if(len != 32 && (route.first.first >> len) == (dgram.header.dst >> len)){
            // std::cout << "--------------------------best sec"<< route.second.next_hop.value().to_string() <<endl;
            best_route = route;
            has_best = true;
          }
        }
        if(has_best){
          //如果有匹配的ip，则需要找对应的interface发送
          // std::cout << "--------------------------发送"<<  concat(dgram.payload) <<endl;
          //direct没有next hop
          interfaces_[best_route.second.interface_num].send_datagram(dgram,best_route.second.next_hop.value_or(Address::from_ipv4_numeric(dgram.header.dst)));
        }else{
          continue;
        }
      }
    }
  }
  
}


