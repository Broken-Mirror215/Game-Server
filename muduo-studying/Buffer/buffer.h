#pragma once

#include <string>
#include <algorithm>
#include <vector>

class Buffer{
public:
    Buffer();

    size_t readable() const;
    size_t writeable() const;

    void Noneall();
    void Append(const char* data,size_t len);

    //返回可读区域的起点
    const char * peek();
    char * begin();

    std::string ClearAll();

    void ensureablewrite(size_t len);

    //在buffer里面消费len个字节
    void retrieve(size_t len);
    //从buffer里面取出len个字节
    std::string retrieveAsstring(size_t len);

private:
    std::vector<char> _Buffer;
    size_t _writeIndex;
    size_t _readIndex;
};