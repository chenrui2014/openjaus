/*****************************************************************************
 *  Copyright (c) 2008, University of Florida
 *  All rights reserved.
 *  
 *  This file is part of OpenJAUS.  OpenJAUS is distributed under the BSD 
 *  license.  See the LICENSE file for details.
 * 
 *  Redistribution and use in source and binary forms, with or without 
 *  modification, are permitted provided that the following conditions 
 *  are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of the University of Florida nor the names of its 
 *       contributors may be used to endorse or promote products derived from 
 *       this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ****************************************************************************/
// File Name: setGlobalWaypointMessage.c
//
// Written By: Danny Kent (jaus AT dannykent DOT com), Tom Galluzzo (galluzzo AT gmail DOT com)
//
// Version: 3.3.0
//
// Date: 07/09/08
//
// Description: This file defines the functionality of a SetGlobalWaypointMessage



#include <stdlib.h>
#include <string.h>
#include "jaus.h"

static const int commandCode = JAUS_SET_GLOBAL_WAYPOINT;
static const int maxDataSizeBytes = 21;

static JausBoolean headerFromBuffer(SetGlobalWaypointMessage message, unsigned char *buffer, unsigned int bufferSizeBytes);
static JausBoolean headerToBuffer(SetGlobalWaypointMessage message, unsigned char *buffer, unsigned int bufferSizeBytes);

static JausBoolean dataFromBuffer(SetGlobalWaypointMessage message, unsigned char *buffer, unsigned int bufferSizeBytes);
static int dataToBuffer(SetGlobalWaypointMessage message, unsigned char *buffer, unsigned int bufferSizeBytes);
static void dataInitialize(SetGlobalWaypointMessage message);
static unsigned int dataSize(SetGlobalWaypointMessage message);

// ************************************************************************************************************** //
//                                    USER CONFIGURED FUNCTIONS
// ************************************************************************************************************** //

// Initializes the message-specific fields
static void dataInitialize(SetGlobalWaypointMessage message)
{
	// Set initial values of message fields
	message->presenceVector = newJausByte(JAUS_BYTE_PRESENCE_VECTOR_ALL_ON);
	message->waypointNumber = newJausUnsignedShort(0);
	message->latitudeDegrees = newJausDouble(0);	// Scaled Integer (-90, 90)
	message->longitudeDegrees = newJausDouble(0); 	// Scaled Integer (-180, 180)
	message->elevationMeters = newJausDouble(0); 	// Scaled Integer (-10000, 35000)
	message->rollRadians = newJausDouble(0);		// Scaled Short (-JAUS_PI, JAUS_PI)
	message->pitchRadians = newJausDouble(0);		// Scaled Short (-JAUS_PI, JAUS_PI)
	message->yawRadians = newJausDouble(0);			// Scaled Short (-JAUS_PI, JAUS_PI)
}

// Return boolean of success
static JausBoolean dataFromBuffer(SetGlobalWaypointMessage message, unsigned char *buffer, unsigned int bufferSizeBytes)
{
	int index = 0;
	JausInteger tempInteger;
	JausShort tempShort;

	if(bufferSizeBytes == message->dataSize)
	{
		// Unpack Message Fields from Buffer
		// Use Presence Vector
		if(!jausByteFromBuffer(&message->presenceVector, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
		index += JAUS_BYTE_SIZE_BYTES;
		
		if(!jausUnsignedShortFromBuffer(&message->waypointNumber, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
		index += JAUS_UNSIGNED_SHORT_SIZE_BYTES;
		
		//unpack
		//JausDouble latitudeDegrees		// Scaled Integer (-90, 90)
		if(!jausIntegerFromBuffer(&tempInteger, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
		index += JAUS_INTEGER_SIZE_BYTES;
		message->latitudeDegrees = jausIntegerToDouble(tempInteger, -90, 90);
		
		//unpack
		//JausDouble longitudeDegrees		// Scaled Integer (-180, 180)
		if(!jausIntegerFromBuffer(&tempInteger, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
		index += JAUS_INTEGER_SIZE_BYTES;
		message->longitudeDegrees = jausIntegerToDouble(tempInteger, -180, 180);

		if(jausByteIsBitSet(message->presenceVector, JAUS_WAYPOINT_PV_ELEVATION_BIT))
		{
			//unpack
			//JausDouble elevationMeters	// Scaled Integer (-10000, 35000)
			if(!jausIntegerFromBuffer(&tempInteger, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
			index += JAUS_INTEGER_SIZE_BYTES;
			
			message->elevationMeters = jausIntegerToDouble(tempInteger, -10000, 35000);
		}

		if(jausByteIsBitSet(message->presenceVector, JAUS_WAYPOINT_PV_ROLL_BIT))
		{
			//unpack
			//JausDouble rollRadians // Scaled Short (-JAUS_PI, JAUS_PI)
			if(!jausShortFromBuffer(&tempShort, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
			index += JAUS_SHORT_SIZE_BYTES;

			message->rollRadians = jausShortToDouble(tempShort, -JAUS_PI, JAUS_PI);
		}
		
		if(jausByteIsBitSet(message->presenceVector, JAUS_WAYPOINT_PV_PITCH_BIT))
		{
			//unpack
			//JausDouble pitchRadians;		// Scaled Short (-JAUS_PI, JAUS_PI)
			if(!jausShortFromBuffer(&tempShort, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
			index += JAUS_SHORT_SIZE_BYTES;

			message->pitchRadians = jausShortToDouble(tempShort, -JAUS_PI, JAUS_PI);
		}
			
		if(jausByteIsBitSet(message->presenceVector, JAUS_WAYPOINT_PV_YAW_BIT))
		{
			//unpack
			//JausDouble yawRadians;			// Scaled Short (-JAUS_PI, JAUS_PI)
			if(!jausShortFromBuffer(&tempShort, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
			index += JAUS_SHORT_SIZE_BYTES;

			message->yawRadians = jausShortToDouble(tempShort, -JAUS_PI, JAUS_PI);
		}
		return JAUS_TRUE;
	}
	else
	{
		return JAUS_FALSE;
	}
}

// Returns number of bytes put into the buffer
static int dataToBuffer(SetGlobalWaypointMessage message, unsigned char *buffer, unsigned int bufferSizeBytes)
{
	int index = 0;
	JausInteger tempInteger;
	JausShort tempShort;

	if(bufferSizeBytes >= dataSize(message))
	{
		// Pack Message Fields to Buffer
		// Use Presence Vector
		if(!jausByteToBuffer(message->presenceVector, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
		index += JAUS_BYTE_SIZE_BYTES;
		
		if(!jausUnsignedShortToBuffer(message->waypointNumber, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
		index += JAUS_UNSIGNED_SHORT_SIZE_BYTES;
		
		//pack
		//JausDouble latitudeDegrees		// Scaled Integer (-90, 90)
		tempInteger = jausIntegerFromDouble(message->latitudeDegrees, -90, 90);
		if(!jausIntegerToBuffer(tempInteger, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
		index += JAUS_INTEGER_SIZE_BYTES;
		
		//pack
		//JausDouble longitudeDegrees		// Scaled Integer (-180, 180)
		tempInteger = jausIntegerFromDouble(message->longitudeDegrees, -180, 180);
		if(!jausIntegerToBuffer(tempInteger, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
		index += JAUS_INTEGER_SIZE_BYTES;
		
		if(jausByteIsBitSet(message->presenceVector, JAUS_WAYPOINT_PV_ELEVATION_BIT))
		{
			//pack
			//JausDouble elevationMeters	// Scaled Integer (-10000, 35000)
			tempInteger = jausIntegerFromDouble(message->elevationMeters, -10000, 35000);
			if(!jausIntegerToBuffer(tempInteger, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
			index += JAUS_INTEGER_SIZE_BYTES;
		}

		if(jausByteIsBitSet(message->presenceVector, JAUS_WAYPOINT_PV_ROLL_BIT))
		{
			//pack
			//JausDouble rollRadians // Scaled Short (-JAUS_PI, JAUS_PI)
			tempShort = jausShortFromDouble(message->rollRadians, -JAUS_PI, JAUS_PI);
			if(!jausShortToBuffer(tempShort, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
			index += JAUS_SHORT_SIZE_BYTES;
		}
		
		if(jausByteIsBitSet(message->presenceVector, JAUS_WAYPOINT_PV_PITCH_BIT))
		{
			//pack
			//JausDouble pitchRadians;		// Scaled Short (-JAUS_PI, JAUS_PI)
			tempShort = jausShortFromDouble(message->pitchRadians, -JAUS_PI, JAUS_PI);
			if(!jausShortToBuffer(tempShort, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
			index += JAUS_SHORT_SIZE_BYTES;
		}
			
		if(jausByteIsBitSet(message->presenceVector, JAUS_WAYPOINT_PV_YAW_BIT))
		{
			//pack
			//JausDouble yawRadians;			// Scaled Short (-JAUS_PI, JAUS_PI)
			tempShort = jausShortFromDouble(message->yawRadians, -JAUS_PI, JAUS_PI);
			if(!jausShortToBuffer(tempShort, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
			index += JAUS_SHORT_SIZE_BYTES;
		}
	}
	return index;
}

static unsigned int dataSize(SetGlobalWaypointMessage message)
{
	int index = 0;

	index += JAUS_BYTE_SIZE_BYTES;
		
	index += JAUS_UNSIGNED_SHORT_SIZE_BYTES;
		
	index += JAUS_INTEGER_SIZE_BYTES;
		
	index += JAUS_INTEGER_SIZE_BYTES;
		
	if(jausByteIsBitSet(message->presenceVector, JAUS_WAYPOINT_PV_ELEVATION_BIT))
	{
		index += JAUS_INTEGER_SIZE_BYTES;
	}

	if(jausByteIsBitSet(message->presenceVector, JAUS_WAYPOINT_PV_ROLL_BIT))
	{
		index += JAUS_SHORT_SIZE_BYTES;
	}
	
	if(jausByteIsBitSet(message->presenceVector, JAUS_WAYPOINT_PV_PITCH_BIT))
	{
		index += JAUS_SHORT_SIZE_BYTES;
	}
		
	if(jausByteIsBitSet(message->presenceVector, JAUS_WAYPOINT_PV_YAW_BIT))
	{
		index += JAUS_SHORT_SIZE_BYTES;
	}

	return index;
}

// ************************************************************************************************************** //
//                                    NON-USER CONFIGURED FUNCTIONS
// ************************************************************************************************************** //

SetGlobalWaypointMessage setGlobalWaypointMessageCreate(void)
{
	SetGlobalWaypointMessage message;

	message = (SetGlobalWaypointMessage)malloc( sizeof(SetGlobalWaypointMessageStruct) );
	if(message == NULL)
	{
		return NULL;
	}
	
	// Initialize Values
	message->properties.priority = JAUS_DEFAULT_PRIORITY;
	message->properties.ackNak = JAUS_ACK_NAK_NOT_REQUIRED;
	message->properties.scFlag = JAUS_NOT_SERVICE_CONNECTION_MESSAGE;
	message->properties.expFlag = JAUS_NOT_EXPERIMENTAL_MESSAGE;
	message->properties.version = JAUS_VERSION_3_3;
	message->properties.reserved = 0;
	message->commandCode = commandCode;
	message->destination = jausAddressCreate();
	message->source = jausAddressCreate();
	message->dataFlag = JAUS_SINGLE_DATA_PACKET;
	message->dataSize = maxDataSizeBytes;
	message->sequenceNumber = 0;
	
	dataInitialize(message);
	message->dataSize = dataSize(message);
	
	return message;	
}

void setGlobalWaypointMessageDestroy(SetGlobalWaypointMessage message)
{
	jausAddressDestroy(message->source);
	jausAddressDestroy(message->destination);
	free(message);
}

JausBoolean setGlobalWaypointMessageFromBuffer(SetGlobalWaypointMessage message, unsigned char* buffer, unsigned int bufferSizeBytes)
{
	int index = 0;
	
	if(headerFromBuffer(message, buffer+index, bufferSizeBytes-index))
	{
		index += JAUS_HEADER_SIZE_BYTES;
		if(dataFromBuffer(message, buffer+index, bufferSizeBytes-index))
		{
			return JAUS_TRUE;
		}
		else
		{
			return JAUS_FALSE;
		}
	}
	else
	{
		return JAUS_FALSE;
	}
}

JausBoolean setGlobalWaypointMessageToBuffer(SetGlobalWaypointMessage message, unsigned char *buffer, unsigned int bufferSizeBytes)
{
	if(bufferSizeBytes < setGlobalWaypointMessageSize(message))
	{
		return JAUS_FALSE; //improper size	
	}
	else
	{	
		message->dataSize = dataToBuffer(message, buffer+JAUS_HEADER_SIZE_BYTES, bufferSizeBytes - JAUS_HEADER_SIZE_BYTES);
		if(headerToBuffer(message, buffer, bufferSizeBytes))
		{
			return JAUS_TRUE;
		}
		else
		{
			return JAUS_FALSE; // headerToSetGlobalWaypointBuffer failed
		}
	}
}

SetGlobalWaypointMessage setGlobalWaypointMessageFromJausMessage(JausMessage jausMessage)
{
	SetGlobalWaypointMessage message;
	
	if(jausMessage->commandCode != commandCode)
	{
		return NULL; // Wrong message type
	}
	else
	{
		message = (SetGlobalWaypointMessage)malloc( sizeof(SetGlobalWaypointMessageStruct) );
		if(message == NULL)
		{
			return NULL;
		}
		
		message->properties.priority = jausMessage->properties.priority;
		message->properties.ackNak = jausMessage->properties.ackNak;
		message->properties.scFlag = jausMessage->properties.scFlag;
		message->properties.expFlag = jausMessage->properties.expFlag;
		message->properties.version = jausMessage->properties.version;
		message->properties.reserved = jausMessage->properties.reserved;
		message->commandCode = jausMessage->commandCode;
		message->destination = jausAddressCreate();
		*message->destination = *jausMessage->destination;
		message->source = jausAddressCreate();
		*message->source = *jausMessage->source;
		message->dataSize = jausMessage->dataSize;
		message->dataFlag = jausMessage->dataFlag;
		message->sequenceNumber = jausMessage->sequenceNumber;
		
		// Unpack jausMessage->data
		if(dataFromBuffer(message, jausMessage->data, jausMessage->dataSize))
		{
			return message;
		}
		else
		{
			return NULL;
		}
	}
}

JausMessage setGlobalWaypointMessageToJausMessage(SetGlobalWaypointMessage message)
{
	JausMessage jausMessage;
	
	jausMessage = (JausMessage)malloc( sizeof(struct JausMessageStruct) );
	if(jausMessage == NULL)
	{
		return NULL;
	}	
	
	jausMessage->properties.priority = message->properties.priority;
	jausMessage->properties.ackNak = message->properties.ackNak;
	jausMessage->properties.scFlag = message->properties.scFlag;
	jausMessage->properties.expFlag = message->properties.expFlag;
	jausMessage->properties.version = message->properties.version;
	jausMessage->properties.reserved = message->properties.reserved;
	jausMessage->commandCode = message->commandCode;
	jausMessage->destination = jausAddressCreate();
	*jausMessage->destination = *message->destination;
	jausMessage->source = jausAddressCreate();
	*jausMessage->source = *message->source;
	jausMessage->dataSize = dataSize(message);
	jausMessage->dataFlag = message->dataFlag;
	jausMessage->sequenceNumber = message->sequenceNumber;
	
	jausMessage->data = (unsigned char *)malloc(jausMessage->dataSize);
	jausMessage->dataSize = dataToBuffer(message, jausMessage->data, jausMessage->dataSize);
	
	return jausMessage;
}


unsigned int setGlobalWaypointMessageSize(SetGlobalWaypointMessage message)
{
	return (unsigned int)(dataSize(message) + JAUS_HEADER_SIZE_BYTES);
}

//********************* PRIVATE HEADER FUNCTIONS **********************//

static JausBoolean headerFromBuffer(SetGlobalWaypointMessage message, unsigned char *buffer, unsigned int bufferSizeBytes)
{
	if(bufferSizeBytes < JAUS_HEADER_SIZE_BYTES)
	{
		return JAUS_FALSE;
	}
	else
	{
		// unpack header
		message->properties.priority = (buffer[0] & 0x0F);
		message->properties.ackNak	 = ((buffer[0] >> 4) & 0x03);
		message->properties.scFlag	 = ((buffer[0] >> 6) & 0x01);
		message->properties.expFlag	 = ((buffer[0] >> 7) & 0x01);
		message->properties.version	 = (buffer[1] & 0x3F);
		message->properties.reserved = ((buffer[1] >> 6) & 0x03);
		
		message->commandCode = buffer[2] + (buffer[3] << 8);
	
		message->destination->instance = buffer[4];
		message->destination->component = buffer[5];
		message->destination->node = buffer[6];
		message->destination->subsystem = buffer[7];
	
		message->source->instance = buffer[8];
		message->source->component = buffer[9];
		message->source->node = buffer[10];
		message->source->subsystem = buffer[11];
		
		message->dataSize = buffer[12] + ((buffer[13] & 0x0F) << 8);

		message->dataFlag = ((buffer[13] >> 4) & 0x0F);

		message->sequenceNumber = buffer[14] + (buffer[15] << 8);
		
		return JAUS_TRUE;
	}
}

static JausBoolean headerToBuffer(SetGlobalWaypointMessage message, unsigned char *buffer, unsigned int bufferSizeBytes)
{
	JausUnsignedShort *propertiesPtr = (JausUnsignedShort*)&message->properties;
	
	if(bufferSizeBytes < JAUS_HEADER_SIZE_BYTES)
	{
		return JAUS_FALSE;
	}
	else
	{	
		buffer[0] = (unsigned char)(*propertiesPtr & 0xFF);
		buffer[1] = (unsigned char)((*propertiesPtr & 0xFF00) >> 8);

		buffer[2] = (unsigned char)(message->commandCode & 0xFF);
		buffer[3] = (unsigned char)((message->commandCode & 0xFF00) >> 8);

		buffer[4] = (unsigned char)(message->destination->instance & 0xFF);
		buffer[5] = (unsigned char)(message->destination->component & 0xFF);
		buffer[6] = (unsigned char)(message->destination->node & 0xFF);
		buffer[7] = (unsigned char)(message->destination->subsystem & 0xFF);

		buffer[8] = (unsigned char)(message->source->instance & 0xFF);
		buffer[9] = (unsigned char)(message->source->component & 0xFF);
		buffer[10] = (unsigned char)(message->source->node & 0xFF);
		buffer[11] = (unsigned char)(message->source->subsystem & 0xFF);
		
		buffer[12] = (unsigned char)(message->dataSize & 0xFF);
		buffer[13] = (unsigned char)((message->dataFlag & 0xFF) << 4) | (unsigned char)((message->dataSize & 0x0F00) >> 8);

		buffer[14] = (unsigned char)(message->sequenceNumber & 0xFF);
		buffer[15] = (unsigned char)((message->sequenceNumber & 0xFF00) >> 8);
		
		return JAUS_TRUE;
	}
}

