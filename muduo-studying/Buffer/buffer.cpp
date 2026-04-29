#include "buffer.h"
Buffer::Buffer()
:_Buffer(1024),
_writeIndex(0),
_readIndex(0)
{}


void Buffer::Noneall(){
    _writeIndex=0;
    _readIndex=0;
}

size_t Buffer::readable() const
{
    return _writeIndex-_readIndex;
}

size_t Buffer::writeable() const{
    return _Buffer.size()-_writeIndex;
}

const char* Buffer::peek(){
    return begin()+_readIndex;
}

char * Buffer::begin(){
    return _Buffer.data();
}

void Buffer::ensureablewrite(size_t len){
    if (writeable()<len){
        _Buffer.resize(_Buffer.size()+len);
    }
}

void Buffer::Append(const char* data,size_t len){
    ensureablewrite(len);
    std::copy(data,data+len,begin()+_writeIndex);
    _writeIndex+=len;
}

std::string Buffer::ClearAll(){
    std::string msg(peek(),readable());
    Noneall();
    return msg;
}

void Buffer::retrieve(size_t len){
    if (len<readable()){
        _readIndex+=len;
    }
    else
    {
        Noneall();
    }
}


std::string Buffer::retrieveAsstring(size_t len){
    if (len > readable())
    {
        len = readable();
    }

    std::string result(peek(), len);
    retrieve(len);
    return result;
}