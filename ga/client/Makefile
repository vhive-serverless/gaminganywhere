
include ../Makefile.def

# gRPC-related variables
GRPC_INCLUDE = /usr/include/grpc  # Update this path according to your system
GRPC_LIB = /usr/local/lib  # Update this path according to your system

# Append gRPC-related flags to existing CFLAGS
CFLAGS += -I$(GRPC_INCLUDE)

# Append gRPC-related flags to existing LDFLAGS
LDFLAGS += -L$(GRPC_LIB) -lgrpc++ -lgrpc -lprotobuf

CFLAGS	+= -I../core $(AVCCF) $(L5CF) $(SDLCF)
LDFLAGS	= $(EXTRALDFLAGS) -L../core -lga $(AVCLD) $(SDLLD) $(L5LD) -Wl,-rpath,\$$ORIGIN

ifeq ($(OS), Linux)
CFLAGS	+= $(X11CF)
LDFLAGS	+= $(X11LD) -lrt -lpthread
endif

ifeq ($(OS), MSYS)
LDFLAGS	+= -lws2_32
endif

ifeq ($(OS), Darwin)
LDFLAGS	+= -framework Cocoa
endif

TARGET	= ga-client

all: $(TARGET)

.cpp.o:
	$(CXX) -c -g $(CXXFLAGS) $<

ga-client: ga-client.o rtspclient.o ctrl-sdl.o minih264.o minivp8.o qosreport.o
	$(CXX) -o $@ $^ $(LDFLAGS)

install: $(TARGET)
	mkdir -p ../../bin
	cp -f $(TARGET) ../../bin
	cp -f *.ttf ../../bin

clean:
	rm -f $(TARGET) *.o *~

