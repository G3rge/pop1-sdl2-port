# Prince of Persia 1 - Port (C++/SDL2) - Makefile para MSYS2
# Usable desde las shells MINGW64 o UCRT64 (usa $MINGW_PREFIX).
# Dependencias: mingw-w64-*-gcc y mingw-w64-*-SDL2

CXX   ?= g++
PREFIX ?= $(MINGW_PREFIX)
ifeq ($(PREFIX),)
  PREFIX := /usr
endif

CXXFLAGS := -O2 -Wall -I$(PREFIX)/include/SDL2
LDLIBS   := -lmingw32 -lSDL2main -lSDL2

SRCS := main.cpp dat.cpp img.cpp text.cpp level.cpp player.cpp tiles.cpp
OBJS := $(SRCS:.cpp=.o)
TARGET := dat_view.exe

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all run clean