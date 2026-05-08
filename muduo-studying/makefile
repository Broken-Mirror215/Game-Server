CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -g
LDFLAGS := -pthread
TARGET := server

SRCS := \
	Acceptor/Accetpor.cpp \
	Buffer/buffer.cpp \
	Channel/Channel.cpp \
	Codec/Codec.cpp \
	Epoller/Epoller.cpp \
	Eventloop/Eventloop.cpp \
	EventloopThread/EventloopThread.cpp \
	EventloopThreadpool/EventloopThreadpool.cpp \
	Game-Server/Game-Server.cpp \
	main/main.cpp \
	Player/Player.cpp \
	Room/Room.cpp \
	Socket/Socket.cpp \
	Tcpconnection/Tcpconnection.cpp \
	Tcpserver/Tcpserver.cpp

all: $(SRCS)
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(TARGET) $(LDFLAGS)

clean:
	rm -f $(TARGET)

.PHONY: all clean
