#include "JausUdpInterface.h"
#include "JausSubsystemCommunicationManager.h"
#include "JausNodeCommunicationManager.h"
#include "JausComponentCommunicationManager.h"
#include "ErrorEvent.h"
#include "JausMessageEvent.h"

JausUdpInterface::JausUdpInterface(FileLoader *configData, EventHandler *handler, JausCommunicationManager *commMngr)
{
	this->commMngr = commMngr;
	this->eventHandler = handler;
	this->name = JAUS_UDP_NAME;
	this->configData = configData;
	this->multicast = false;

	// Determine the type of our commMngr
	if(dynamic_cast<JausSubsystemCommunicationManager  *>(this->commMngr))
	{
		this->type = SUBSYSTEM_INTERFACE;
	}
	else if(dynamic_cast<JausNodeCommunicationManager *>(this->commMngr))
	{
		this->type = NODE_INTERFACE;
	}
	else if(dynamic_cast<JausComponentCommunicationManager *>(this->commMngr))
	{
		this->type = COMPONENT_INTERFACE;
	}
	else
	{
		this->type = UNKNOWN_INTERFACE;
	}

	// Setup our UDP Socket
	if(!this->openSocket())
	{
		abort();
	}

	// Setup our pThread
	this->setupThread();

	// Setup our receiveThread
	this->setupRecvThread();
}

JausUdpInterface::~JausUdpInterface(void)
{
	if(running)
	{
		this->stopThread();
	}
	this->closeSocket();

	// TODO: Check our threadIds to see if they terminated properly
}

InetAddress JausUdpInterface::getInetAddress(void)
{
	return this->socket->address;
}

bool JausUdpInterface::routeMessage(JausMessage message)
{
	DatagramPacket packet;
	packet = datagramPacketCreate();

	switch(this->type)
	{
		case SUBSYSTEM_INTERFACE:
			// if subs==BROADCAST send multicast
			if(message->destination->subsystem == JAUS_BROADCAST_SUBSYSTEM_ID)
			{
				if(this->multicast)
				{
					// Send multicast packet
					sendJausMessage(this->multicastData, message);
					jausMessageDestroy(message);
					return true;
				}
				else
				{
					// Unicast to all known subsystems
					HASH_MAP<int, UdpTransportData>::iterator iter;
					for(iter = addressMap.begin(); iter != addressMap.end(); iter++)
					{
						sendJausMessage(iter->second, message);
					}
					jausMessageDestroy(message);
					return true;
				}
			}
			else
			{
				// Unicast
				if(addressMap.find(message->destination->subsystem) != addressMap.end())
				{
					sendJausMessage(addressMap.find(message->destination->subsystem)->second, message);
					jausMessageDestroy(message);
					return true;
				}
				else
				{
					// Don't know how to send this message
					jausMessageDestroy(message);
					return false;
				}
			}
			break;

		case NODE_INTERFACE:
			// if node==BROADCAST send multicast
			if(message->destination->node == JAUS_BROADCAST_NODE_ID)
			{
				if(this->multicast)
				{
					//if(message->commandCode == JAUS_QUERY_SERVICES) printf("Sending Node Multicast JAUS_QUERY_SERVICES\n");
					// Send multicast packet
					sendJausMessage(this->multicastData, message);
					jausMessageDestroy(message);
					return true;
				}
				else
				{
					// Unicast to all known subsystems
					HASH_MAP<int, UdpTransportData>::iterator iter;
					for(iter = addressMap.begin(); iter != addressMap.end(); iter++)
					{
						sendJausMessage(iter->second, message);
					}
					jausMessageDestroy(message);
					return true;
				}
			}
			else
			{
				// Unicast
				if(addressMap.find(message->destination->node) != addressMap.end())
				{
					sendJausMessage(addressMap.find(message->destination->node)->second, message);
					jausMessageDestroy(message);
					return true;
				}
				else
				{
					// Don't know how to send this message
					jausMessageDestroy(message);
					return false;
				}
			}
			break;

		case COMPONENT_INTERFACE:
			// if cmpt == BROADCAST || inst == BROADCAST send unicast to all components
			if( message->destination->component == JAUS_BROADCAST_COMPONENT_ID ||
				message->destination->instance == JAUS_BROADCAST_INSTANCE_ID )
			{
				if(this->multicast)
				{
					// Send multicast packet
					sendJausMessage(this->multicastData, message);
					jausMessageDestroy(message);
					return true;
				}
				else
				{
					// Unicast to all known subsystems
					HASH_MAP<int, UdpTransportData>::iterator iter;
					for(iter = addressMap.begin(); iter != addressMap.end(); iter++)
					{
						sendJausMessage(iter->second, message);
					}
					jausMessageDestroy(message);
					return true;
				}
			}
			else
			{
				// Unicast
				if(addressMap.find(jausAddressHash(message->destination)) != addressMap.end())
				{
					sendJausMessage(addressMap.find(jausAddressHash(message->destination))->second, message);
					jausMessageDestroy(message);
					return true;
				}
				else
				{
					// Don't know how to send this message
					jausMessageDestroy(message);
					return false;
				}
			}
			break;

		default:
			// Unknown type
			// No routing behavior
			jausMessageDestroy(message);
			return false;
	}
}

void JausUdpInterface::run()
{
	// Lock our mutex
	pthread_mutex_lock(&threadMutex);

	while(this->running)
	{
		pthread_cond_wait(&threadConditional, &threadMutex);
		
		while(!this->queue.isEmpty())
		{
			// Pop a packet off the queue and send it off
			routeMessage(queue.pop());
		}
	}
	pthread_mutex_unlock(&threadMutex);
}

std::string JausUdpInterface::toString()
{
	char ret[256] = {0};
	char buf[80] = {0};
	if(this->socket)
	{
		inetAddressToString(this->socket->address, buf);
		sprintf(ret, "%s %s:%d", JAUS_UDP_NAME, buf, this->socket->port);
		return ret;
	}
	else
	{
		sprintf(ret, "%s Invalid.", JAUS_UDP_NAME);
		return ret;
	}
}

bool JausUdpInterface::openSocket(void)
{
	char categoryString[128] = {0};
	switch(this->type)
	{
		case SUBSYSTEM_INTERFACE:
			// Read Subsystem UDP Parameters
			sprintf(categoryString, "%s", "Subsystem_Communications");
			break;

		case NODE_INTERFACE:
			// Setup Node Configuration
			sprintf(categoryString, "%s", "Node_Communications");
			break;

		case COMPONENT_INTERFACE:
			// Read Component Configuration
			sprintf(categoryString, "%s", "Component_Communications");
			break;

		default:
			// Unknown type
			// TODO: Log Error
			return false;
	}

	this->portNumber = this->configData->GetConfigDataInt(categoryString, "JAUS_UDP_Port");
	this->ipAddress = inetAddressGetByString((char *)this->configData->GetConfigDataString(categoryString, "JAUS_UDP_IP").c_str());
	if(this->ipAddress == NULL)
	{
		this->ipAddress = inetAddressCreate();
		this->ipAddress->value = INADDR_ANY;
	}

	// Create Component Socket
	this->socket = multicastSocketCreate(this->portNumber, ipAddress);
	if(!this->socket)
	{
		// Error creating our socket
		char errorString[128] = {0};
		char buf[24] = {0};
		
		inetAddressToString(this->ipAddress, buf);
		sprintf(errorString, "Could not open socket: %s:%d", buf, this->portNumber);
		ErrorEvent *e = new ErrorEvent(ErrorEvent::Configuration, __FUNCTION__, __LINE__, errorString);
		this->eventHandler->handleEvent(e);
		return false;
	}
	else
	{
		this->ipAddress = socket->address;
		this->portNumber = socket->port;
	}

	// Setup Timeout
	multicastSocketSetTimeout(this->socket, this->configData->GetConfigDataInt(categoryString, "JAUS_UDP_Timeout_Sec"));

	// Setup TTL
	multicastSocketSetTTL(this->socket, this->configData->GetConfigDataInt(categoryString, "JAUS_UDP_TTL"));

	// Setup Multicast
	if(this->configData->GetConfigDataBool(categoryString, "JAUS_UDP_Multicast"))
	{
		this->multicast = true;
		this->multicastGroup  = inetAddressGetByString((char *)this->configData->GetConfigDataString(categoryString, "JAUS_UDP_Multicast_Group").c_str());
		if(multicastSocketJoinGroup(this->socket, this->multicastGroup) != 0)
		{
			// Error joining our group
			char errorString[128] = {0};
			char buf[24] = {0};
			
			inetAddressToString(this->multicastGroup, buf);
			sprintf(errorString, "Could not open socket: %s:%d", buf, this->portNumber);
			ErrorEvent *e = new ErrorEvent(ErrorEvent::Configuration, __FUNCTION__, __LINE__, errorString);
			this->eventHandler->handleEvent(e);
			return false;
		}

		// Setup Loopback
		multicastSocketSetLoopback(this->socket, LOOPBACK_DISABLED);

		// Setup Multicast UdpData
		multicastData.addressValue = multicastGroup->value;
		multicastData.port = this->socket->port;
	}
	return true;
}

void JausUdpInterface::sendJausMessage(UdpTransportData data, JausMessage message)
{
	DatagramPacket packet = NULL;
	int result;

	JausMessage tempMessage = jausMessageClone(message);
	JausMessageEvent *e = new JausMessageEvent(tempMessage, this, JausMessageEvent::Outbound);
	this->eventHandler->handleEvent(e);

	switch(this->type)
	{
		case SUBSYSTEM_INTERFACE:
		case NODE_INTERFACE:
			packet = datagramPacketCreate();
			packet->bufferSizeBytes = (int) jausMessageSize(message) + JAUS_OPC_UDP_HEADER_SIZE_BYTES;
			packet->buffer = (unsigned char *) calloc(packet->bufferSizeBytes, 1);
			packet->port = data.port;
			packet->address->value = data.addressValue;
			memset(packet->buffer, 0, packet->bufferSizeBytes);
			
			memcpy(packet->buffer, JAUS_OPC_UDP_HEADER, JAUS_OPC_UDP_HEADER_SIZE_BYTES);
			if(jausMessageToBuffer(message, packet->buffer + JAUS_OPC_UDP_HEADER_SIZE_BYTES, packet->bufferSizeBytes - JAUS_OPC_UDP_HEADER_SIZE_BYTES))
			{
				result = multicastSocketSend(this->socket, packet);
			}

			free(packet->buffer);
			datagramPacketDestroy(packet);
			return;

		case COMPONENT_INTERFACE:
			packet = datagramPacketCreate();
			packet->bufferSizeBytes = (int) jausMessageSize(message);
			packet->buffer = (unsigned char *) calloc(packet->bufferSizeBytes, 1);
			packet->port = data.port;
			packet->address->value = data.addressValue;
			memset(packet->buffer, 0, packet->bufferSizeBytes);

			if(jausMessageToBuffer(message, packet->buffer, packet->bufferSizeBytes))
			{
				result = multicastSocketSend(this->socket, packet);
			}

			free(packet->buffer);
			datagramPacketDestroy(packet);
			return;

		default:
			char errorString[128] = {0};
			sprintf(errorString, "Unknown socket type %d\n", this->type);
			ErrorEvent *e = new ErrorEvent(ErrorEvent::Configuration, __FUNCTION__, __LINE__, errorString);
			this->eventHandler->handleEvent(e);
			return;
	}
}

void JausUdpInterface::closeSocket(void)
{
	multicastSocketDestroy(this->socket);
}

void JausUdpInterface::setupRecvThread()
{
	pthread_attr_init(&this->recvThreadAttr);
	pthread_attr_setdetachstate(&this->recvThreadAttr, PTHREAD_CREATE_DETACHED);

	this->recvThreadId = pthread_create(&this->recvThread, &this->recvThreadAttr, UdpRecvThread, this);
}

void JausUdpInterface::recvThreadRun()
{
	DatagramPacket packet;
	JausMessage rxMessage;
	UdpTransportData data;
	int index = 0;
	long bytesRecv = 0;

	packet = datagramPacketCreate();
	packet->bufferSizeBytes = JAUS_HEADER_SIZE_BYTES + JAUS_MAX_DATA_SIZE_BYTES + JAUS_OPC_UDP_HEADER_SIZE_BYTES;
	packet->buffer = (unsigned char *) calloc(packet->bufferSizeBytes, 1);
	
	while(this->running)
	{
		index = 0;
		bytesRecv = multicastSocketReceive(this->socket, packet);
		
		if(bytesRecv > 0)
		{
			if(!strncmp((char *)packet->buffer, JAUS_OPC_UDP_HEADER, JAUS_OPC_UDP_HEADER_SIZE_BYTES)) // equals 1 if same
			{
				index += JAUS_OPC_UDP_HEADER_SIZE_BYTES;
			}

			rxMessage = jausMessageCreate();
			if(jausMessageFromBuffer(rxMessage, packet->buffer + index, packet->bufferSizeBytes - index))
			{
				JausMessage tempMessage = jausMessageClone(rxMessage);
				JausMessageEvent *e = new JausMessageEvent(tempMessage, this, JausMessageEvent::Inbound);
				this->eventHandler->handleEvent(e);
				
				// Add to transportMap
				switch(this->type)
				{
					case SUBSYSTEM_INTERFACE:
						data.addressValue = packet->address->value;
						data.port = JAUS_UDP_DATA_PORT;
						this->addressMap[rxMessage->source->subsystem] = data;
						break;

					case NODE_INTERFACE:
						data.addressValue = packet->address->value;
						data.port = JAUS_UDP_DATA_PORT;
						this->addressMap[rxMessage->source->node] = data;
						break;

					case COMPONENT_INTERFACE:
						data.addressValue = packet->address->value;
						data.port = packet->port;
						this->addressMap[jausAddressHash(rxMessage->source)] = data;
						break;

					default:
						// Unknown type
						break;
				}

				this->commMngr->receiveJausMessage(rxMessage, this);
			}
			else
			{
				jausMessageDestroy(rxMessage);
			}
		}
	}
}

void *UdpRecvThread(void *obj)
{
	JausUdpInterface *udpInterface = (JausUdpInterface *)obj;
	udpInterface->recvThreadRun();
	return NULL;
}
