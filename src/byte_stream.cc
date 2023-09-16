#include <stdexcept>

#include "byte_stream.hh"
#include <iostream>



using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ),
 _eof(false),_errorS(false),_buffer(), _total_bytes_pushed(0),_total_bytes_popped(0){}

void Writer::push( string data )
{
  // Your code here.
  uint64_t reamin_size = available_capacity();
  //忘了加个min，浪费了我好久
  uint64_t min_size = min(data.length(),reamin_size);
  for(uint64_t i = 0; i < min_size; i++){
    _buffer.push_back(data[i]);
  }
  _total_bytes_pushed+=min_size;
  (void)data;
}

void Writer::close()
{
  // Your code here.
  _eof = true;
}

void Writer::set_error()
{
  // Your code here.
  _errorS = true;
}

bool Writer::is_closed() const
{
  // Your code here.
  return {_eof};
}

uint64_t Writer::available_capacity() const
{
  // Your code here.
  return {capacity_-_buffer.size()};
}

uint64_t Writer::bytes_pushed() const
{
  // Your code here.
  return {_total_bytes_pushed};
}

string_view Reader::peek() const
{
  // Your code here.
  return string_view(&_buffer.front(),1);
}

bool Reader::is_finished() const
{
  // Your code here.
  // 不止要eof，还要让缓存里的读取完
  if(_eof && _buffer.size() == 0){
    return true;
  }

  return false;
}

bool Reader::has_error() const
{
  // Your code here.
  return {_errorS};
}

void Reader::pop( uint64_t len )
{
  // Your code here.
  uint64_t min_size =min(len,_buffer.size());
  for(uint64_t i = 0; i < min_size; i++){
    _buffer.pop_front();
  }
  _total_bytes_popped+=min_size;
  (void)len;
}

uint64_t Reader::bytes_buffered() const
{
  // Your code here.
  return {_buffer.size()};
}

uint64_t Reader::bytes_popped() const
{
  // Your code here.
  return {_total_bytes_popped};
}
