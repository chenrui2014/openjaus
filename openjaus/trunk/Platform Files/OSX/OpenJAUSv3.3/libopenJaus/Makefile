TARGET	= 	./lib/libopenJaus.so
CCFLAGS	=	-Wall -fPIC -fno-strict-aliasing -O2 -c -g -I ./include/ -I ../libjaus/include/
LFLAGS	=	-L ../libjaus/lib/
LIBS		=	-ljaus
SOURCES =	./src/utils/queue.c \
		./src/utils/multicastSocket.c \
		./src/utils/datagramPacket.c \
		./src/utils/timeLib.c \
		./src/utils/inetAddress.c \
		./src/utils/datagramSocket.c \
		./src/componentLibrary/ojCmpt.c \
		./src/nodeManagerInterface/nodeManagerInterface.c \
		./src/nodeManagerInterface/lmHandler.c \
		./src/nodeManagerInterface/scManager.c \
		./src/nodeManagerInterface/defaultJausMessageProcessor.c \
		./src/utils/FileLoader.cpp \
		./src/nodeManager/JausTransportQueue.cpp \
		./src/nodeManager/NodeManagerComponent.cpp \
		./src/nodeManager/CommunicatorComponent.cpp \
		./src/nodeManager/events/DebugEvent.cpp \
		./src/nodeManager/events/ErrorEvent.cpp \
		./src/nodeManager/events/NodeManagerEvent.cpp \
		./src/nodeManager/events/SystemTreeEvent.cpp \
		./src/nodeManager/events/JausMessageEvent.cpp \
		./src/nodeManager/NodeManager.cpp \
		./src/nodeManager/MessageRouter.cpp \
		./src/nodeManager/OjUdpComponentInterface.cpp \
		./src/nodeManager/LocalComponent.cpp \
		./src/nodeManager/JausCommunicationManager.cpp \
		./src/nodeManager/JausComponentCommunicationManager.cpp \
		./src/nodeManager/SystemTree.cpp \
		./src/nodeManager/JausSubsystemCommunicationManager.cpp \
		./src/nodeManager/JausNodeCommunicationManager.cpp \
		./src/nodeManager/JausUdpInterface.cpp \
		./src/nodeManager/JausTransportInterface.cpp

OBJECTS =	./Build/queue.o \
		./Build/multicastSocket.o \
		./Build/datagramPacket.o \
		./Build/timeLib.o \
		./Build/inetAddress.o \
		./Build/datagramSocket.o \
		./Build/ojCmpt.o \
		./Build/nodeManagerInterface.o \
		./Build/lmHandler.o \
		./Build/scManager.o \
		./Build/defaultJausMessageProcessor.o \
		./Build/FileLoader.o \
		./Build/JausTransportQueue.o \
		./Build/NodeManagerComponent.o \
		./Build/CommunicatorComponent.o \
		./Build/DebugEvent.o \
		./Build/ErrorEvent.o \
		./Build/NodeManagerEvent.o \
		./Build/SystemTreeEvent.o \
		./Build/JausMessageEvent.o \
		./Build/NodeManager.o \
		./Build/MessageRouter.o \
		./Build/OjUdpComponentInterface.o \
		./Build/LocalComponent.o \
		./Build/JausCommunicationManager.o \
		./Build/JausComponentCommunicationManager.o \
		./Build/SystemTree.o \
		./Build/JausSubsystemCommunicationManager.o \
		./Build/JausNodeCommunicationManager.o \
		./Build/JausUdpInterface.o \
		./Build/JausTransportInterface.o

default : all

all : $(TARGET)

clean :
	rm -f ./Build/*.o
	rm -f $(TARGET)

$(TARGET) : $(OBJECTS)
	g++ -shared $(LFLAGS) -o $(TARGET) $(OBJECTS) $(LIBS)

./Build/queue.o : ./src/utils/queue.c
	gcc $(CCFLAGS) -o ./Build/queue.o ./src/utils/queue.c

./Build/multicastSocket.o : ./src/utils/multicastSocket.c
	gcc $(CCFLAGS) -o ./Build/multicastSocket.o ./src/utils/multicastSocket.c

./Build/datagramPacket.o : ./src/utils/datagramPacket.c
	gcc $(CCFLAGS) -o ./Build/datagramPacket.o ./src/utils/datagramPacket.c

./Build/timeLib.o : ./src/utils/timeLib.c
	gcc $(CCFLAGS) -o ./Build/timeLib.o ./src/utils/timeLib.c

./Build/inetAddress.o : ./src/utils/inetAddress.c
	gcc $(CCFLAGS) -o ./Build/inetAddress.o ./src/utils/inetAddress.c

./Build/datagramSocket.o : ./src/utils/datagramSocket.c
	gcc $(CCFLAGS) -o ./Build/datagramSocket.o ./src/utils/datagramSocket.c

./Build/ojCmpt.o : ./src/componentLibrary/ojCmpt.c
	gcc $(CCFLAGS) -o ./Build/ojCmpt.o ./src/componentLibrary/ojCmpt.c

./Build/nodeManagerInterface.o : ./src/nodeManagerInterface/nodeManagerInterface.c
	gcc $(CCFLAGS) -o ./Build/nodeManagerInterface.o ./src/nodeManagerInterface/nodeManagerInterface.c

./Build/lmHandler.o : ./src/nodeManagerInterface/lmHandler.c
	gcc $(CCFLAGS) -o ./Build/lmHandler.o ./src/nodeManagerInterface/lmHandler.c

./Build/scManager.o : ./src/nodeManagerInterface/scManager.c
	gcc $(CCFLAGS) -o ./Build/scManager.o ./src/nodeManagerInterface/scManager.c

./Build/defaultJausMessageProcessor.o : ./src/nodeManagerInterface/defaultJausMessageProcessor.c
	gcc $(CCFLAGS) -o ./Build/defaultJausMessageProcessor.o ./src/nodeManagerInterface/defaultJausMessageProcessor.c

./Build/FileLoader.o : ./src/utils/FileLoader.cpp
	g++ $(CCFLAGS) -o ./Build/FileLoader.o ./src/utils/FileLoader.cpp

./Build/JausTransportQueue.o : ./src/nodeManager/JausTransportQueue.cpp
	g++ $(CCFLAGS) -o ./Build/JausTransportQueue.o ./src/nodeManager/JausTransportQueue.cpp

./Build/NodeManagerComponent.o : ./src/nodeManager/NodeManagerComponent.cpp
	g++ $(CCFLAGS) -o ./Build/NodeManagerComponent.o ./src/nodeManager/NodeManagerComponent.cpp

./Build/CommunicatorComponent.o : ./src/nodeManager/CommunicatorComponent.cpp
	g++ $(CCFLAGS) -o ./Build/CommunicatorComponent.o ./src/nodeManager/CommunicatorComponent.cpp

./Build/DebugEvent.o : ./src/nodeManager/events/DebugEvent.cpp
	g++ $(CCFLAGS) -o ./Build/DebugEvent.o ./src/nodeManager/events/DebugEvent.cpp

./Build/ErrorEvent.o : ./src/nodeManager/events/ErrorEvent.cpp
	g++ $(CCFLAGS) -o ./Build/ErrorEvent.o ./src/nodeManager/events/ErrorEvent.cpp

./Build/NodeManagerEvent.o : ./src/nodeManager/events/NodeManagerEvent.cpp
	g++ $(CCFLAGS) -o ./Build/NodeManagerEvent.o ./src/nodeManager/events/NodeManagerEvent.cpp

./Build/SystemTreeEvent.o : ./src/nodeManager/events/SystemTreeEvent.cpp
	g++ $(CCFLAGS) -o ./Build/SystemTreeEvent.o ./src/nodeManager/events/SystemTreeEvent.cpp

./Build/JausMessageEvent.o : ./src/nodeManager/events/JausMessageEvent.cpp
	g++ $(CCFLAGS) -o ./Build/JausMessageEvent.o ./src/nodeManager/events/JausMessageEvent.cpp

./Build/NodeManager.o : ./src/nodeManager/NodeManager.cpp
	g++ $(CCFLAGS) -o ./Build/NodeManager.o ./src/nodeManager/NodeManager.cpp

./Build/MessageRouter.o : ./src/nodeManager/MessageRouter.cpp
	g++ $(CCFLAGS) -o ./Build/MessageRouter.o ./src/nodeManager/MessageRouter.cpp

./Build/OjUdpComponentInterface.o : ./src/nodeManager/OjUdpComponentInterface.cpp
	g++ $(CCFLAGS) -o ./Build/OjUdpComponentInterface.o ./src/nodeManager/OjUdpComponentInterface.cpp

./Build/LocalComponent.o : ./src/nodeManager/LocalComponent.cpp
	g++ $(CCFLAGS) -o ./Build/LocalComponent.o ./src/nodeManager/LocalComponent.cpp

./Build/JausCommunicationManager.o : ./src/nodeManager/JausCommunicationManager.cpp
	g++ $(CCFLAGS) -o ./Build/JausCommunicationManager.o ./src/nodeManager/JausCommunicationManager.cpp

./Build/JausComponentCommunicationManager.o : ./src/nodeManager/JausComponentCommunicationManager.cpp
	g++ $(CCFLAGS) -o ./Build/JausComponentCommunicationManager.o ./src/nodeManager/JausComponentCommunicationManager.cpp

./Build/SystemTree.o : ./src/nodeManager/SystemTree.cpp
	g++ $(CCFLAGS) -o ./Build/SystemTree.o ./src/nodeManager/SystemTree.cpp

./Build/JausSubsystemCommunicationManager.o : ./src/nodeManager/JausSubsystemCommunicationManager.cpp
	g++ $(CCFLAGS) -o ./Build/JausSubsystemCommunicationManager.o ./src/nodeManager/JausSubsystemCommunicationManager.cpp

./Build/JausNodeCommunicationManager.o : ./src/nodeManager/JausNodeCommunicationManager.cpp
	g++ $(CCFLAGS) -o ./Build/JausNodeCommunicationManager.o ./src/nodeManager/JausNodeCommunicationManager.cpp

./Build/JausUdpInterface.o : ./src/nodeManager/JausUdpInterface.cpp
	g++ $(CCFLAGS) -o ./Build/JausUdpInterface.o ./src/nodeManager/JausUdpInterface.cpp

./Build/JausTransportInterface.o : ./src/nodeManager/JausTransportInterface.cpp
	g++ $(CCFLAGS) -o ./Build/JausTransportInterface.o ./src/nodeManager/JausTransportInterface.cpp
