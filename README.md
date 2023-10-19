# lab0：
1. 环境构建ubuntu要22以上， with g++ 12.2  ， supports the C++ 2020 standard  
2. 实现一个客户端socket的发送和接受请求和一个缓冲池，数据的读取和pop

# lab1:
![image.png](https://cdn.nlark.com/yuque/0/2023/png/1410510/1694825165166-24f9144a-fe41-411c-8943-526b089cb863.png#averageHue=%23fafafa&clientId=u957e7e86-e324-4&from=paste&height=439&id=u771b3e7b&originHeight=439&originWidth=812&originalType=binary&ratio=1&rotation=0&showTitle=false&size=91115&status=done&style=none&taskId=u7a9cb5cd-8eed-4bfa-80de-e945823c186&title=&width=812)

实现一个组装器Reassembler，存储已经到来但是还未发送给Writer(ByteStream)的数据，即图中的红色部分，如果从first_unassembled_index 顺序存储了，则马上发送个Writer
实验注意的点：

1. 注意裁剪first_index 小于first_unassemble_index的情况和超过first_unacceptable_index的情况
2. 注意需要设置一个状态标识的数组标识避免存储字符数组有0的情况
3. 注意关闭字节流，insert方面上面的注释要看



# lab2：
实现一个tcp Receiver  ，包括  wrapping integers  实现一个对绝对序号和tcp头部序号的转换，因为tcp 报文头部只有32bit的位置留给长度

1.  需要实现一个32bit 的包装转换器
> unwrap  按取模后的余数进行比较会有很多int和uint的转换，不建议
> window_size上限为UINT16_MAX，一个seqno一个字节，而tcp报文序号为uint32_t字节，因此窗口填满的数据发来，也不会超过uint32_t，因此可以选则离checkpoint最近的
> abs_seqno取离checkpoint最近的，是不是会有可能读取完后，迷失的报文到达，把abs_seqno unrwrap为 右端近的导致读取错我的数据？
> 

2.  为了防止旧的序号对新连接的重复影响，每个新连接的syn都是连接建立时随机的
3.  需要注意syn和fin各占一个序号

![image.png](https://cdn.nlark.com/yuque/0/2023/png/1410510/1695304543523-0f0ebd28-0fa2-401e-b323-9f0ed2936bba.png#averageHue=%23faf8f7&clientId=ud2c74ec9-740d-4&from=paste&height=292&id=u1ba3059e&originHeight=292&originWidth=702&originalType=binary&ratio=1&rotation=0&showTitle=false&size=50962&status=done&style=none&taskId=u59d502ca-5b88-46a3-8d8c-0fb2556845b&title=&width=702)

tcp 接收器的实现tcp_receiver 主要有两个功能:

1. receive
> 1. 需要注意标识syn接收后才往reassembler中插入数据，此时因为时syn，注意seqno+1
> 2. 往reassembler中插入数据时，记得absolute seqno 需要减1到stream index

2. send
> 1. _first_unassembled_index（stream index）先+1转成abs seqno，再wrap
> 2. 不能通过is_set_fin来判断，如果有冗余的fin先到，会导致多加1,用ackno比较则fin会被过滤，不会相等
> 3. window_size上限为UINT16_MAX，一个seqno一个字节，


# lab 3
tcp sender:

1. 记录返回的ackno和windowsSize的踪迹
2. 尽可能在window内发送多的数据
3. 记录发送，但还没返回的ack，gbn（累计确认）
4. 定时重传
> 过慢用户会有延迟，过快会浪费网络资源


对于超时时间的设定，是非常重要的
>  过早超时（延迟的ACK）也能够正常工作；但是效率较低，一半 的分组和确认是重复的；  

1. 每几毫秒， TCPSender’s tick method 将被调用来表示距离他上次被调用过了多久，不要使用操作系统或者cpu的定时/记时方法
2. TCPSender 初始化的时候，会传入 retransmission timeout (RTO)，rto会变化，但初始值不變
3. 需要自己实现timer，但时间流逝来自于tick方法的调用而不是自然时间流逝（为了测试）
4. 每次一个segment 发送，不管是第一次还是重传，都要启动计时
5. 当所有发送的数据被确认，停止定时器
6. 如果 tick  被调用时，定时器daoqi
> a. 重传最早的分组，发送时记得保存
> b.如果窗口大小不为0 ，如果为0 只需要暂时不往发送队列push message即可，主要时避免加重网络拥塞
> 1. 记录连续的重传（ “consecutive retransmissions”  ），避免加重网络拥塞 
> 2. 重新设置定时器并将Rto设置为两倍
> c. 定时器超时后，重新设置定时器,（如果b没重新设置）rto值不变

7. 当返回ack时（新的ackNo一定大于旧的）
> a. 重置rto
> b. 重启定时器
> c. 重置 “consecutive retransmissions”  


 void push( Reader& outbound stream )  

1. 在窗口范围内，尽可能发多的 TCPSenderMessages， 即 发送窗口>1  ，TCPSenderMessage  中内容尽量多一点
2. 如果窗口为空/大小为1，发送一个一字节的报文去获取是否有可用窗口（不需要重传）

 void receive( const TCPReceiverMessage& msg )  
记得移除发送端保存的已经确认的segment

 void send empty message():  
发送一个空的，但sequence number时正确的的消息，作为接收端需要确认某些消息的作用，eg：第一次时请求时假设为1

实验完成过程，注意的点：：

1. 无需考虑segment 部分确认的情况，实际TCP可以实现
2. 分组内容较少，且超时，不考虑将其放到新的分组中的情况，实际TCP可以实现
3. 没有playload/SYN/FIN的segment都不需要记录和重传


遇到的问题：

1. sequence_numbers_in_flight应该是push时放进所有准备好发送的队列就记录的，而不是maybe_send时记录的已经发送的但没接收到的，这样才能控制准备好发送的不会超过windowsize
2. int 作为当前时间单位，避免-=时溢出
3. Test #30: send_retx 时，因为window_size=0时，需要当成1，定时器不需要指数退让，所以需要另一个参数配合定时器超时窗口非0情况，但是好像测试用例时没有设置检查这个特殊情况，在test33
4. 判断流是否结束时，可以加上newMessage.sequence_length() + oustanding_size < window_size判断是否刚好读取后没有足够的位置放fin，receive需要清除内存（lab也没这个校验）

#  Lab  4  
实现一个networkInterface ，lab6也将用到
![image.png](https://cdn.nlark.com/yuque/0/2023/png/1410510/1697120771222-7016b4de-a9be-450d-992f-304222a85468.png#averageHue=%23f2f0ef&clientId=u0f6b6374-c803-4&from=paste&height=725&id=u78358474&originHeight=725&originWidth=771&originalType=binary&ratio=1&rotation=0&showTitle=false&size=89749&status=done&style=none&taskId=uc10f9e99-6f6e-47bc-be93-81af41d9632&title=&width=771)
将TCP segments 传送到另一个主机（假设本地是linux系统）：

1.  TCP-in-UDP-in-IP  ，将tcp segments塞到UDP body中，构造 UDPSocket  交由操作系统
2.  TCP-in-IP   实现一个 entire Internet datagram   调用 linux TUN 接口，由linux包装成 Ethernet  
3.  TCP-in-IP-in-Ethernet   第二点实现中，linux 如果路由表中没有对于ip，则会广播查询ip和 Ethernet  

但是在这个lab中，我们将会提供一个 Ethernet   交由 TAP  而不是提供一个IP 报文
主要工作是：

1. 查看当前映射中是否有对应的next hop ip和Ethernet映射，有则组装发送，没有则发送ARP请求
2. 接受返回的数据包，分为：IPV4、ARP Request和ARP REPLY
3. 定时清楚超时的arp请求和ip-ethernet map对应数据

实验完成过程，注意的点：

1. 用缓存保存映射关系，崩溃时要重新创建
2. 如果是ARP请求，插入IP 到Ethernet ip的映射，现在如果有已发送的是不是不用等待请求就可以将对应arp时间记录队列和等待ip队列处理掉
3. send_datagram发送arp请求时，dst以太网帧不知道才需要查，因此dst不需要赋值
4. recv_frame处理ARPMessage::OPCODE_REQUEST数据时，只有对应ip相等才需要返回arp请求，避免网络拥塞

# lab 5
会用到lab4，但不会用更之前的lab，总体功能：

1. 保存routing table（不需要考虑rip、ospf、bgp那些东西）
2. 转发数据报
> 1. 正确的下一跳
> 2. 正确的 outgoing NetworkInterface  

 add_route  (uint32_t route_prefix, uint8_t prefix_length, optional，next_hop, size_t interface_num); 
添加一个route到路由表
 “match”  ： prefix_length  + route_prefix  构成匹配 
“action”   ： next_hop  +interface_num 构成转发
> 需要注意下一跳路由器要是有问题，应该给个新的转发对象

 void route();  

1. 需要实现最长前缀匹配ip，发送到对应interface_num
2. 没有匹配则丢弃
3. ttl耗完了则丢弃
> 没有匹配和ttl耗完，在实验中都不需要回发icmp报文


实验完成过程，注意的点：

1. 注意 : 0.0.0.0/0 via 171.67.76.1   为默认网关
2. direct没有next hop,相当于本地的网卡，直接传dst和对应interface调用即可
3. 移动32位可能会出现异常,代表的是默认的route



# lab6 
测试之前的实现，能否完成 telnet  命令
2023版的Lab并没有实现TCPConnection，也就是大家熟知的三次握手、四次挥手。
而是按上述的将三次握手拆成了，两次单信道各自的syn和ack
四次挥手还存在，一端close的时候，会进入FIN_WAIT_2等待另一端的close，才会 exit gracefully  
客户端启动而服务端没启动，客户端会定时轮询
