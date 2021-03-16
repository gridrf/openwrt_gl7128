# Makefile for Linux etc.

bin = lora-station

OBJS = cmac.o aes.o LoRaMacCrypto.o Packet.o Base64.o PacketForwarder.o WebSocketServer.o LoRaWebSocket.o MessagerHandler.o Radio.o SX1276.o spi.o gpio.o GpioControl.o TimerEvent.o main.o

CXXFLAGS += -I/usr/include/json-c

LIBS +=-lpthread -ljson-c -lwebsockets

all : $(bin)

$(bin) : $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS) $(LIBS)

clean:
	rm -f *.o lora-station *.bin *~
