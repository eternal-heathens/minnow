#include "reassembler.hh"
#include <algorithm>
#include <iostream>

using namespace std;

//Reassembler 主要作为一个滑动窗口，等待前序到来，然后按序输出给output
Reassembler::Reassembler():
_reassemblerBuf(),//output为整个ByteStream,_reassemblerBuf整个滑动窗口缓冲池，默认填充0
_flag(),//所以需要另外一个数组来帮忙避免原先是0的情况
first_unassembled_index(0),
first_unacceptable_index(0),
_init(true),
last_index(-1){
}

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring, Writer& output )
{
  std::cout << "--------------first_index: " << first_index << "-----data: " << data <<endl; 
  // Your code here.
  if(_init){
    _reassemblerBuf.resize(output.available_capacity());
    _flag.resize(output.available_capacity());
    _init = false; 
  }

  //最后索引标记，关闭writer
  if(is_last_substring == true){
    last_index = first_index+data.size();
  }

  first_unassembled_index = output.bytes_pushed();
  first_unacceptable_index = first_unassembled_index+output.available_capacity();

  if(!data.empty()){
    //处理超出的情况
    if(first_index>=first_unacceptable_index ||first_index+data.size() < first_unassembled_index){
      data = "";
    }else{
      //做数据重复部分和超出部分的裁剪
      uint64_t str_strat;
      uint64_t str_end;
      str_strat = first_index;
      if(first_index < first_unassembled_index){
        str_strat = first_unassembled_index;
      }
      str_end = first_unacceptable_index-1;
      if(first_index+data.length() < first_unacceptable_index){
        str_end = first_index+data.length()-1;
      }
      std::cout << "--------------str_strat: " << str_strat << "-----str_end: " << str_end <<endl; 
      for (uint64_t i = str_strat; i <= str_end; i++)
      {
        //滑动窗口最左侧为first_unassembled_index，unpop和poped数据为output.bytes_pushed
        // i-first_unassembled_index为对应的 i-first_index为首个未重复的data数据
        _reassemblerBuf[i-first_unassembled_index]=data[i-first_index];
        _flag[i-first_unassembled_index] = true;
      }
    }
  }

  string tmp ;

  while (_flag.front() == true)
  {
    tmp+=_reassemblerBuf.front();
    _reassemblerBuf.pop_front();
    _flag.pop_front();
    _reassemblerBuf.push_back(0);
    _flag.push_back(0);
  }
  output.push(tmp);
  
  if(output.bytes_pushed() == last_index){
    output.close();
  }

  
  (void)first_index;
  (void)data;
  (void)is_last_substring;
  (void)output;
}

uint64_t Reassembler::bytes_pending() const
{
  // Your code here.
  return static_cast<uint64_t>(count(_flag.begin(),_flag.end(),true));
}
