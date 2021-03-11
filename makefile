# 条件赋值 ?= 
CXX ?= g++
VERSION ?= -std=c++11

DEBUG ?= 1
ifeq ($(DEBUG), 1)
	CXXFLAGS += -g -std=c++11
else
	CXXFLAGS += -O2
endif

TARGET := webserver
OBJ := ./main.cpp ./config/*.cpp ./http/*.cpp  ./log/*.cpp ./mysql/*cpp ./timer/*.cpp 

all: $(OBJ)
	$(CXX) $(CXXFLAGS) $(VERSION) -o $(TARGET) $^  -lpthread -lmysqlclient

clean:
	rm  -r $(TARGET)
