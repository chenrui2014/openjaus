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
// File Name: properties.c
//
// Written By: Tom Galluzzo (galluzzo AT gmail DOT com)
//
// Version: 3.2
//
// Date: 08/04/06
//
// Description:	This file describes the functionality associated with a Properties object.
// Inspired by the class of the same name in the JAVA language.

#if defined(__GNUC__)
	#define _GNU_SOURCE
#endif

#if defined(__APPLE__) || defined(WIN32)
	#include "getline.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "properties.h"

Properties propertiesCreate(void)
{
	return NULL;
}

void propertiesDestroy(Properties properties)
{
	Properties deadProperty;
	
	while(properties)
	{
		free(properties->property);
		free(properties->value);
		deadProperty = properties;
		properties = properties->nextProperty;
		free(deadProperty);
	}
}

Properties propertiesLoad(Properties properties, FILE *file)
{
	const char delimeters[] = "\t\n\r=";
	char *buf = NULL;
	char *tempProperty, *tempValue;
	size_t bufSizeChars = 0;
	Properties property = properties;
	Properties firstProperty = properties;
	
	while( getline(&buf, &bufSizeChars, file ) > -1)
	{
		tempProperty = strtok(buf, delimeters);
		tempValue = strtok(NULL, delimeters);
		if(tempProperty && tempProperty[0] != '#')
		{
			if(property)
			{
				property->nextProperty = (Properties)malloc( sizeof(PropertyStruct) );
				property = property->nextProperty;

				property->property = (char *)malloc( strlen(tempProperty) + 1 ); 
				sprintf(property->property, "%s", tempProperty);

				property->value = (char *)malloc( strlen(tempValue) + 1 ); 
				sprintf(property->value, "%s", tempValue);

				property->nextProperty = NULL;
			}
			else
			{
				property = (Properties)malloc( sizeof(PropertyStruct) );

				property->property = (char *)malloc( strlen(tempProperty) + 1 );
				sprintf(property->property, "%s", tempProperty);

				property->value = (char *)malloc( strlen(tempValue) + 1 ); 
				sprintf(property->value, "%s", tempValue);

				property->nextProperty = NULL;
				firstProperty = property;
			}
			// printf("%s = %s\n", tempProperty, tempValue);	
		}
	}
	
	if(buf)
	{
		free(buf);
	}
	
	return firstProperty; 
}

char *propertiesGetProperty(Properties properties, char * testProperty)
{
	while(properties)
	{
		if(strcmp(properties->property, testProperty))
		{
			properties = properties->nextProperty;
		}
		else
		{
			return properties->value;
		}
	}
	
	return NULL;
}
