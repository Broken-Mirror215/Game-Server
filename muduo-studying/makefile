CXX := g++

TARGET := server
BUILD_DIR := build

CXXFLAGS := -std=c++17 -Wall -Wextra -g
LDFLAGS := -pthread

SRC_DIRS := \
	Acceptor \
	Buffer \
	Channel \
	Codec \
	Epoller \
	Eventloop \
	EventloopThread \
	Game-Server \
	main \
	Player \
	Room \
	Socket \
	Tcpconnection \
	Tcpserver

SRCS := $(foreach dir,$(SRC_DIRS),$(wildcard $(dir)/*.cpp))
OBJS := $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

.PHONY: all clean run rebuild print

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

run: all
	./$(TARGET)

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

rebuild: clean all

print:
	@echo "SRCS = $(SRCS)"
	@echo "OBJS = $(OBJS)"

-include $(DEPS)
