/*****************************************************************************
 *  Copyright (c) 2006, University of Florida.
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
// File Name: setVelocityStateMessage.c
//
// Written By: Danny Kent (jaus AT dannykent DOT com), Tom Galluzzo (galluzzo AT gmail DOT com)
//
// Version: 3.2
//
// Date: 08/04/06
//
// Description: This file defines the functionality of a SetVelocityStateMessage



#include <stdlib.h>
#include <string.h>
#include "cimar/jaus.h"

static const int commandCode = JAUS_SET_VELOCITY_STATE;
static const int maxDataSizeBytes = 30;

static JausBoolean headerFromBuffer(SetVelocityStateMessage message, unsigned char *buffer, unsigned int bufferSizeBytes);
static JausBoolean headerToBuffer(SetVelocityStateMessage message, unsigned char *buffer, unsigned int bufferSizeBytes);

static JausBoolean dataFromBuffer(SetVelocityStateMessage message, unsigned char *buffer, unsigned int bufferSizeBytes);
static int dataToBuffer(SetVelocityStateMessage message, unsigned char *buffer, unsigned int bufferSizeBytes);
static void dataInitialize(SetVelocityStateMessage message);
static void dataDestroy(SetVelocityStateMessage message);

// ************************************************************************************************************** //
//                                    USER CONFIGURED FUNCTIONS
// ************************************************************************************************************** //

// Initializes the message-specific fields
static void dataInitialize(SetVelocityStateMessage message)
{
	// Set initial values of message fields
	message->presenceVector = newJausShortPresenceVector();
	message->velocityXMps = newJausDouble(0);	// Scaled Int (-65.534, 65.534) 	Mps = Meters per Second
	message->velocityYMps = newJausDouble(0);	// Scaled Int (-65.534, 65.534) 	Mps = Meters per Second
	message->velocityZMps = newJausDouble(0);	// Scaled Int (-65.534, 65.534) 	Mps = Meters per Secon
	message->velocityRmsMps = newJausDouble(0);	// Scaled UInt (0, 100) 			Mps = Meters per Second
	message->rollRateRps = newJausDouble(0);	// Scaled Short (-32.767, 32.767) 	Rps = Radians per Second
	message->pitchRateRps = newJausDouble(0);	// Scaled Short (-32.767, 32.767) 	Rps = Radians per Second
	message->yawRateRps = newJausDouble(0);		// Scaled Short (-32.767, 32.767) 	Rps = Radians per Second
	message->rateRmsRps = newJausDouble(0);		// Scaled UShort (0, JAUS_PI) 		Rps = Radians per Second
	message->timeStamp = newJausUnsignedInteger(0);
}

// Destructs the message-specific fields
static void dataDestroy(SetVelocityStateMessage message)
{
	// Free message fields
	// None
}

// Return boolean of success
static JausBoolean dataFromBuffer(SetVelocityStateMessage message, unsigned char *buffer, unsigned int bufferSizeBytes)
{
	int index = 0;
	JausInteger tempInt;
	JausShort tempShort;
	JausUnsignedShort tempUShort;
	JausUnsignedInteger tempUInt;
		
	if(bufferSizeBytes == message->dataSize)
	{
		// Unpack Message Fields from Buffer
		// Use Presence Vector
		if(!jausShortPresenceVectorFromBuffer(&message->presenceVector, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
		index += JAUS_SHORT_PRESENCE_VECTOR_SIZE_BYTES;

		if(jausShortPresenceVectorIsBitSet(message->presenceVector, JAUS_VELOCITY_PV_VELOCITY_X_BIT))
		{
			//unpack
			if(!jausIntegerFromBuffer(&tempInt, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
			index += JAUS_INTEGER_SIZE_BYTES;
			
			// Scaled Int (-65.534, 65.534)
			message->velocityXMps = jausIntegerToDouble(tempInt, -65.534, 65.534);
		}

		if(jausShortPresenceVectorIsBitSet(message->presenceVector, JAUS_VELOCITY_PV_VELOCITY_Y_BIT))
		{
			//unpack
			if(!jausIntegerFromBuffer(&tempInt, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
			index += JAUS_INTEGER_SIZE_BYTES;
			
			// Scaled Int (-65.534, 65.534)
			message->velocityYMps = jausIntegerToDouble(tempInt, -65.534, 65.534);
		}

		if(jausShortPresenceVectorIsBitSet(message->presenceVector, JAUS_VELOCITY_PV_VELOCITY_Z_BIT))
		{
			//unpack
			if(!jausIntegerFromBuffer(&tempInt, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
			index += JAUS_INTEGER_SIZE_BYTES;
			
			// Scaled Int (-65.534, 65.534)
			message->velocityZMps = jausIntegerToDouble(tempInt, -65.534, 65.534);
		}

		if(jausShortPresenceVectorIsBitSet(message->presenceVector, JAUS_VELOCITY_PV_VELOCITY_RMS_BIT))
		{
			//unpack
			if(!jausUnsignedIntegerFromBuffer(&tempUInt, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
			index += JAUS_UNSIGNED_INTEGER_SIZE_BYTES;
			
			// Scaled UInt (0, 100)
			message->velocityRmsMps = jausUnsignedIntegerToDouble(tempUInt, 0, 100);
		}

		if(jausShortPresenceVectorIsBitSet(message->presenceVector, JAUS_VELOCITY_PV_ROLL_RATE_BIT))
		{
			// unpack
			if(!jausShortFromBuffer(&tempShort, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
			index += JAUS_SHORT_SIZE_BYTES;
			
			// Scaled Unsigned Short (-32.767, 32.767)
			message->rollRateRps = jausShortToDouble(tempShort, -32.767, 32.767);
		}

		if(jausShortPresenceVectorIsBitSet(message->presenceVector, JAUS_VELOCITY_PV_PITCH_RATE_BIT))
		{
			// unpack
			if(!jausShortFromBuffer(&tempShort, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
			index += JAUS_SHORT_SIZE_BYTES;
			
			// Scaled Unsigned Short (-32.767, 32.767)
			message->pitchRateRps = jausShortToDouble(tempShort, -32.767, 32.767);
		}

		if(jausShortPresenceVectorIsBitSet(message->presenceVector, JAUS_VELOCITY_PV_YAW_RATE_BIT))
		{
			// unpack
			if(!jausShortFromBuffer(&tempShort, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
			index += JAUS_SHORT_SIZE_BYTES;
			
			// Scaled Unsigned Short (-32.767, 32.767)
			message->yawRateRps = jausShortToDouble(tempShort, -32.767, 32.767);
		}

		if(jausShortPresenceVectorIsBitSet(message->presenceVector, JAUS_VELOCITY_PV_RATE_RMS_BIT))
		{
			// unpack
			if(!jausUnsignedShortFromBuffer(&tempUShort, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
			index += JAUS_UNSIGNED_SHORT_SIZE_BYTES;
			
			// Scaled Unsigned Short (0, JAUS_PI)
			message->rateRmsRps = jausUnsignedShortToDouble(tempUShort, 0, JAUS_PI);
		}

		if(jausShortPresenceVectorIsBitSet(message->presenceVector, JAUS_VELOCITY_PV_TIME_STAMP_BIT))
		{
			//unpack
			if(!jausUnsignedIntegerFromBuffer(&message->timeStamp, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
			index += JAUS_UNSIGNED_INTEGER_SIZE_BYTES;
		}
		return JAUS_TRUE;
	}
	else
	{
		return JAUS_FALSE;
	}
}

// Returns number of bytes put into the buffer
static int dataToBuffer(SetVelocityStateMessage message, unsigned char *buffer, unsigned int bufferSizeBytes)
{
	int index = 0;
	JausInteger tempInt;
	JausShort tempShort;
	JausUnsignedShort tempUShort;
	JausUnsignedInteger tempUInt;

	if(bufferSizeBytes >= message->dataSize)
	{
		// Pack Message Fields to Buffer
		// Use Presence Vector
		if(!jausShortPresenceVectorToBuffer(message->presenceVector, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
		index += JAUS_SHORT_PRESENCE_VECTOR_SIZE_BYTES;

		if(jausShortPresenceVectorIsBitSet(message->presenceVector, JAUS_VELOCITY_PV_VELOCITY_X_BIT))
		{
			// Scaled Int (-65.534, 65.534)
			tempInt = jausIntegerFromDouble(message->velocityXMps, -65.534, 65.534);

			//pack
			if(!jausIntegerToBuffer(tempInt, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
			index += JAUS_INTEGER_SIZE_BYTES;
		}

		if(jausShortPresenceVectorIsBitSet(message->presenceVector, JAUS_VELOCITY_PV_VELOCITY_Y_BIT))
		{
			// Scaled Int (-65.534, 65.534)
			tempInt = jausIntegerFromDouble(message->velocityYMps, -65.534, 65.534);

			//pack
			if(!jausIntegerToBuffer(tempInt, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
			index += JAUS_INTEGER_SIZE_BYTES;
		}

		if(jausShortPresenceVectorIsBitSet(message->presenceVector, JAUS_VELOCITY_PV_VELOCITY_Z_BIT))
		{
			// Scaled Int (-65.534, 65.534)
			tempInt = jausIntegerFromDouble(message->velocityZMps, -65.534, 65.534);

			//pack
			if(!jausIntegerToBuffer(tempInt, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
			index += JAUS_INTEGER_SIZE_BYTES;
		}

		if(jausShortPresenceVectorIsBitSet(message->presenceVector, JAUS_VELOCITY_PV_VELOCITY_RMS_BIT))
		{
			// Scaled UInt (0, 100)
			tempUInt = jausUnsignedIntegerFromDouble(message->velocityRmsMps, 0, 100);

			//pack
			if(!jausUnsignedIntegerToBuffer(tempUInt, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
			index += JAUS_UNSIGNED_INTEGER_SIZE_BYTES;
		}

		if(jausShortPresenceVectorIsBitSet(message->presenceVector, JAUS_VELOCITY_PV_ROLL_RATE_BIT))
		{
			// Scaled Short (-32.767, 32.767)
			tempShort = jausShortFromDouble(message->rollRateRps, -32.767, 32.767);

			// pack
			if(!jausShortToBuffer(tempShort, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
			index += JAUS_SHORT_SIZE_BYTES;
		}

		if(jausShortPresenceVectorIsBitSet(message->presenceVector, JAUS_VELOCITY_PV_PITCH_RATE_BIT))
		{
			// Scaled Short (-32.767, 32.767)
			tempShort = jausShortFromDouble(message->pitchRateRps, -32.767, 32.767);

			// pack
			if(!jausShortToBuffer(tempShort, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
			index += JAUS_SHORT_SIZE_BYTES;
		}

		if(jausShortPresenceVectorIsBitSet(message->presenceVector, JAUS_VELOCITY_PV_YAW_RATE_BIT))
		{
			// Scaled Unsigned Short (-32.767, 32.767)
			tempShort = jausShortFromDouble(message->yawRateRps, -32.767, 32.767);

			// pack
			if(!jausShortToBuffer(tempShort, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
			index += JAUS_SHORT_SIZE_BYTES;
		}

		if(jausShortPresenceVectorIsBitSet(message->presenceVector, JAUS_VELOCITY_PV_RATE_RMS_BIT))
		{
			// Scaled Unsigned Short (0, JAUS_PI)
			tempUShort = jausUnsignedShortToDouble(message->rateRmsRps, 0, JAUS_PI);

			// pack
			if(!jausUnsignedShortToBuffer(tempUShort, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
			index += JAUS_UNSIGNED_SHORT_SIZE_BYTES;
		}

		if(jausShortPresenceVectorIsBitSet(message->presenceVector, JAUS_VELOCITY_PV_TIME_STAMP_BIT))
		{
			//pack
			if(!jausUnsignedIntegerToBuffer(message->timeStamp, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
			index += JAUS_UNSIGNED_INTEGER_SIZE_BYTES;
		}
	}

	return index;
}

// ************************************************************************************************************** //
//                                    NON-USER CONFIGURED FUNCTIONS
// ************************************************************************************************************** //

SetVelocityStateMessage setVelocityStateMessageCreate(void)
{
	SetVelocityStateMessage message;

	message = (SetVelocityStateMessage)malloc( sizeof(SetVelocityStateMessageStruct) );
	if(message == NULL)
	{
		return NULL;
	}
	
	// Initialize Values
	message->priority = JAUS_DEFAULT_PRIORITY;
	message->ackNak = JAUS_ACK_NAK_NOT_REQUIRED;
	message->scFlag = JAUS_NOT_SERVICE_CONNECTION_MESSAGE;
	message->expFlag = JAUS_NOT_EXPERIMENTAL_MESSAGE;
	message->version = JAUS_VERSION_3_2;
	message->reserved = 0;
	message->commandCode = commandCode;
	message->destination = jausAddressCreate();
	message->source = jausAddressCreate();
	message->dataFlag = JAUS_SINGLE_DATA_PACKET;
	message->dataSize = maxDataSizeBytes;
	message->sequenceNumber = 0;
	
	dataInitialize(message);
	
	return message;	
}

void setVelocityStateMessageDestroy(SetVelocityStateMessage message)
{
	dataDestroy(message);
	jausAddressDestroy(message->source);
	jausAddressDestroy(message->destination);
	free(message);
}

JausBoolean setVelocityStateMessageFromBuffer(SetVelocityStateMessage message, unsigned char* buffer, unsigned int bufferSizeBytes)
{
	int index = 0;
	
	if(!strncmp((char *)buffer, JAUS_UDP_HEADER, JAUS_UDP_HEADER_SIZE_BYTES)) // equals 1 if same
	{
		index += JAUS_UDP_HEADER_SIZE_BYTES;
	}

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

JausBoolean setVelocityStateMessageToBuffer(SetVelocityStateMessage message, unsigned char *buffer, unsigned int bufferSizeBytes)
{
	if(bufferSizeBytes < setVelocityStateMessageSize(message))
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
			return JAUS_FALSE; // headerToSetVelocityStateBuffer failed
		}
	}
}

JausBoolean setVelocityStateMessageToUdpBuffer(SetVelocityStateMessage message, unsigned char *buffer, unsigned int bufferSizeBytes)
{
	if(bufferSizeBytes < setVelocityStateMessageUdpSize(message))
	{
		return JAUS_FALSE; // improper size
	}
	else
	{
		strncpy((char *)buffer, JAUS_UDP_HEADER, JAUS_UDP_HEADER_SIZE_BYTES); //copies the UDP header into the buffer
		return setVelocityStateMessageToBuffer(message, buffer+JAUS_UDP_HEADER_SIZE_BYTES, bufferSizeBytes - JAUS_UDP_HEADER_SIZE_BYTES);
	}
}

SetVelocityStateMessage setVelocityStateMessageFromJausMessage(JausMessage jausMessage)
{
	SetVelocityStateMessage message;
	
	if(jausMessage->commandCode != commandCode)
	{
		return NULL; // Wrong message type
	}
	else
	{
		message = (SetVelocityStateMessage)malloc( sizeof(SetVelocityStateMessageStruct) );
		if(message == NULL)
		{
			return NULL;
		}
		
		message->properties = jausMessage->properties;
		message->commandCode = jausMessage->commandCode;
		message->destination = jausAddressCreate();
		message->destination->id = jausMessage->destination->id;
		message->source = jausAddressCreate();
		message->source->id = jausMessage->source->id;
		message->dataControl = jausMessage->dataControl;
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

JausMessage setVelocityStateMessageToJausMessage(SetVelocityStateMessage message)
{
	JausMessage jausMessage;
	
	jausMessage = (JausMessage)malloc( sizeof(JausMessageStruct) );
	if(jausMessage == NULL)
	{
		return NULL;
	}	
	
	jausMessage->properties = message->properties;
	jausMessage->commandCode = message->commandCode;
	jausMessage->destination = jausAddressCreate();
	jausMessage->destination->id = message->destination->id;
	jausMessage->source = jausAddressCreate();
	jausMessage->source->id = message->source->id;
	jausMessage->dataControl = message->dataControl;
	jausMessage->sequenceNumber = message->sequenceNumber;
	
	jausMessage->data = (unsigned char *)malloc(message->dataSize);
	jausMessage->dataSize = dataToBuffer(message, jausMessage->data, message->dataSize);
	
	return jausMessage;
}

unsigned int setVelocityStateMessageUdpSize(SetVelocityStateMessage message)
{
	return (unsigned int)(message->dataSize + JAUS_HEADER_SIZE_BYTES + JAUS_UDP_HEADER_SIZE_BYTES);
}

unsigned int setVelocityStateMessageSize(SetVelocityStateMessage message)
{
	return (unsigned int)(message->dataSize + JAUS_HEADER_SIZE_BYTES);
}

//********************* PRIVATE HEADER FUNCTIONS **********************//

static JausBoolean headerFromBuffer(SetVelocityStateMessage message, unsigned char *buffer, unsigned int bufferSizeBytes)
{
	if(bufferSizeBytes < JAUS_HEADER_SIZE_BYTES)
	{
		return JAUS_FALSE;
	}
	else
	{
		// unpack header
		message->properties = buffer[0] + (buffer[1] << 8);
		message->commandCode = buffer[2] + (buffer[3] << 8);
	
		message->destination->instance = buffer[4];
		message->destination->component = buffer[5];
		message->destination->node = buffer[6];
		message->destination->subsystem = buffer[7];
	
		message->source->instance = buffer[8];
		message->source->component = buffer[9];
		message->source->node = buffer[10];
		message->source->subsystem = buffer[11];
		
		message->dataControl = buffer[12] + (buffer[13] << 8);
		message->sequenceNumber = buffer[14] + (buffer[15] << 8);
		
		return JAUS_TRUE;
	}
}

static JausBoolean headerToBuffer(SetVelocityStateMessage message, unsigned char *buffer, unsigned int bufferSizeBytes)
{
	if(bufferSizeBytes < JAUS_HEADER_SIZE_BYTES)
	{
		return JAUS_FALSE;
	}
	else
	{	
		buffer[0] = (unsigned char)(message->properties & 0xFF);
		buffer[1] = (unsigned char)((message->properties & 0xFF00) >> 8);

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
		
		buffer[12] = (unsigned char)(message->dataControl & 0xFF);
		buffer[13] = (unsigned char)((message->dataControl & 0xFF00) >> 8);

		buffer[14] = (unsigned char)(message->sequenceNumber & 0xFF);
		buffer[15] = (unsigned char)((message->sequenceNumber & 0xFF00) >> 8);
		
		return JAUS_TRUE;
	}
}

