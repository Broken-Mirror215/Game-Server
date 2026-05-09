#include "Game-server.h"

gServer::gServer()
:_nextplayerid(1),
_nextRoomid(1)
{
    pthread_mutex_init(&_mutex,nullptr);
}


void gServer::onConnect(const Connptr & conn){
    Tcpconnection* key=conn.get();
    pthread_mutex_lock(&_mutex);
    auto p1=std::make_shared<Player>(_nextplayerid++,conn);

    //利用玩家的fd创建一个玩家
    _players[key]=p1;
    int playid=p1->ID();
    pthread_mutex_unlock(&_mutex);
    std::cout<<"new player id is : "<<p1->ID()<<std::endl;
    conn->send("login success id is " + std::to_string(p1->ID()));
}


void gServer::onMessage(const Connptr& conn,const std::string& msg){
    if (msg=="MATCH"){
        handlematch(conn);
    }
    else{
        handleroommsg(conn,msg);
    }
}

void gServer::handlematch(const Connptr & conn){
    Tcpconnection * key=conn.get();
    std::shared_ptr<Player> currentplayer;
    std::shared_ptr<Player> nextplayer;
    pthread_mutex_lock(&_mutex);

    auto it=_players.find(key);
    if (it==_players.end())
    {
        pthread_mutex_unlock(&_mutex);
        conn->send("error find msg");
        return ;
    }

    currentplayer=it->second;
    //如果玩家进了房间，就别匹配
    if (_connRooms.find(key)!=_connRooms.end()){
        pthread_mutex_unlock(&_mutex);
        conn->send("err! player has been in the room !");
        return ;
    }


    //设计了防重复match的保护。
    if (_waitingConn.find(key)!=_waitingConn.end()){
        conn->send("player has already match");
        return ;
    }

    //如果队列没人，就进队然后等
    if (_matchqueue.empty()){
        _matchqueue.push(currentplayer);
        _waitingConn.insert(key);
        std::cout<<"player "<<currentplayer->ID()<<" waiting in mathch"<<std::endl;
        pthread_mutex_unlock(&_mutex);
        conn->send("waiting");
        return ;
    }

    //如果队伍有人，就取出来去匹配。
    nextplayer=_matchqueue.front();
    _matchqueue.pop();
    //AI提示到考虑不要自匹配。
    _waitingConn.erase(nextplayer->CON().get());
    _waitingConn.erase(currentplayer->CON().get());
    if (nextplayer == currentplayer)
    {
        pthread_mutex_unlock(&_mutex);
        conn->send("err not math self");
        return;
    }
    

    int roomid=_nextRoomid++;
    auto room=std::make_shared<Room>(roomid);
    room->Join(currentplayer);
    room->Join(nextplayer);
    _Rooms[roomid]=room;

    _connRooms[currentplayer->CON().get()]=roomid;
    _connRooms[nextplayer->CON().get()]=roomid;

    std::cout<<"the room created "<<roomid
            <<"player "<<currentplayer->ID()
            <<"is vs player "<<nextplayer->ID()<<std::endl;

    pthread_mutex_unlock(&_mutex);

    currentplayer->CON()->send(
        "match success roomid "+std::to_string(roomid)+
        "p1 id :"+std::to_string(currentplayer->ID())
        );

    nextplayer->CON()->send(
        "match success roomid "+std::to_string(roomid)+
        "p2 id :"+std::to_string(nextplayer->ID())
        );

}

void gServer::handleroommsg(const Connptr& conn,const std::string&msg){
    Tcpconnection* key=conn.get();
    std::shared_ptr<Player> player;
    std::shared_ptr<Room> room;
    int roomid=-1;

    pthread_mutex_lock(&_mutex);
    //先找到连接这个房间的玩家
    auto p1=_players.find(key);
    if (p1==_players.end()){
        pthread_mutex_unlock(&_mutex);
        conn->send("not found the player");
        return;
    }
    player=p1->second;

    //这是看玩家是不是在某个房间里面？
    auto rit=_connRooms.find(key);
    if (rit==_connRooms.end()){
        pthread_mutex_unlock(&_mutex);
        conn->send("the player not find in the room");
        return;
    }

    roomid=rit->second;
    auto roomIT=_Rooms.find(roomid);
    if (roomIT==_Rooms.end()){
        conn->send("room not find");
        return;
    }

    room=roomIT->second;

    //建议包装转发消息...
    std::string realmsg = "PLAYER_MSG from=" + std::to_string(player->ID()) + " " + msg;
    std::cout << "room " << roomid << " forward msg: " << realmsg << std::endl;

    //发给另一个玩家
    room->Forward(player,realmsg);
}

void gServer::onDisconnect(const Connptr & conn){
    Tcpconnection * key=conn.get();
    pthread_mutex_lock(&_mutex);
    auto it =_players.find(key);
    if (it==_players.end()){
        pthread_mutex_unlock(&_mutex);
        return ;
    }
    auto player=it->second;
    std::cout<<"the disconnect id is "<<player->ID()<<std::endl;
    
    _players.erase(key);
    _waitingConn.erase(key);
    _connRooms.erase(key);

    std::queue<std::shared_ptr<Player>> newQueue;
    while (!_matchqueue.empty()) {
        auto p = _matchqueue.front();
        _matchqueue.pop();

        if (p != player) {
            newQueue.push(p);
        }
    }
    _matchqueue.swap(newQueue);
    pthread_mutex_unlock(&_mutex);
}
