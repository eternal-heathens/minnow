#include "wrapping_integers.hh"

#include<cmath>
#include<stdlib.h>
#include<iostream>

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  // Your code here.
  (void)n;
  (void)zero_point;
  //static_cast<uint32_t>(zero_point.raw_value_+ (n%(1UL<<32)) )
  //uint64_t n直接强转成uint32_t，截断前面32位，只保留低32位，等效于%2^32
  return Wrap32 {zero_point.raw_value_+static_cast<uint32_t>(n)};
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  // Your code here.
  //正数或负数无所谓,负数时取模运算
  uint64_t abs_seqno = static_cast<uint64_t>(this->raw_value_-zero_point.raw_value_);
  // mod 2^32的余数,<<32实现截断前面32位，>>32实现保留低32位
  uint64_t remain = checkpoint<<32 >>32;
  //只按余数计算需要很多小于0的兼容计算，需要static_cast<int64_t>不优雅，换成对实际abs的边界值计算
  // int64_t closet  = 0;
  // int64_t leftSide = 0;
  // uint64_t rightSide = 0;
  // //如果abs_seqno为remian 的左边界
  // if(abs_seqno < remain ){
  //   leftSide = abs_seqno ;
  //   rightSide = abs_seqno+(1UL<<32);
  // }else{
  //    //如果abs_seqno为remian 的右边界
  //   rightSide = abs_seqno;
  //   //处理小于0的情况
  //   leftSide = -(static_cast<int64_t>(1UL<<32)-static_cast<int64_t>(abs_seqno));
  // }

  // if(remain - leftSide < rightSide-remain){
  //   closet = leftSide ;
  // }else{
  //   closet = rightSide;
  // }
  // (void)zero_point;
  // (void)checkpoint;
  // return {(checkpoint>>32<<32)==0 && closet<0?static_cast<int64_t>(1UL<<32)+closet:(checkpoint>>32<<32)+closet };
   uint64_t times_mod = checkpoint >> 32; // >>32等价于除以2^32
   uint64_t bound;
  // 先取得离checkpoint最近的边界的mod次数（times_mod是左边界mod次数）
  if ( remain < 1UL << 31 ) // remain属于[0,2^32-1]，mid=2^31(即1UL << 31)
    bound = times_mod;
  else
    bound = times_mod + 1;
  // 以该边界的左右边界作为base，还原出2个mod之前的abs_seqno值
  // <<32等价于乘上2^32
  uint64_t abs_seqno_l = abs_seqno + ( ( bound == 0 ? 0 : bound - 1 ) << 32 ); // 注意bound=0的特殊情况
  uint64_t abs_seqno_r = abs_seqno + ( bound << 32 );
  // 判断checkpoint离哪个abs_seqno值近就取那个
  if ( checkpoint < ( abs_seqno_l + abs_seqno_r ) / 2 )
    abs_seqno = abs_seqno_l;
  else
    abs_seqno = abs_seqno_r;
  (void)zero_point;
  (void)checkpoint;
  return abs_seqno;
}
