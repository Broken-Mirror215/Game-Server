#pragma once
#include <queue>
#include <memory>
#include "../Tcpconnection/Tcpconnection.h"
#include "../Player/Player.h"
#include <unordered_map>
#include "../Room/Room.h"
#include <unordered_set>
#include <pthread.h>
using Connptr=std::shared_ptr<Tcpconnection>;
class gServer
{
public:
    gServer();

    //新连接建立的时候调用
    void onConnect(const Connptr&);

    //当Codec解码之后调用
    void onMessage(const Connptr&,const std::string& msg);

    void onDisconnect(const Connptr & conn);

private:
    void handlematch(const Connptr &);

    void handleroommsg(const Connptr&,const std::string & msg);


private:

    int _nextplayerid;
    int _nextRoomid;
    
    //匹配等待队列
    std::queue<std::shared_ptr<Player>> _matchqueue;

    //一个连接对应一个player
    std::unordered_map<Tcpconnection*,std::shared_ptr<Player>> _players;

    std::unordered_map<int,std::shared_ptr<Room>> _Rooms;

    //一个连接当前的所在房间
    std::unordered_map<Tcpconnection*,int> _connRooms;

    //防止自己连接！
    std::unordered_set<Tcpconnection*> _waitingConn;

    pthread_mutex_t _mutex;
};
