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
// File Name: jausComponent.h
//
// Written By: Danny Kent (jaus AT dannykent DOT com), Tom Galluzzo (galluzzo AT gmail DOT com)
//
// Version: 3.2
//
// Date: 08/04/06
//
// Description: This file defines basic functions related to a JausComponent structure

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "cimar/jaus.h"

JausComponent jausComponentCreate(void)
{
	JausComponent component;
	
	component = (JausComponent) calloc( 1, sizeof(JausComponentStruct) );
	if(component == NULL)
	{
		return NULL;
	}

	// Init Values
	component->identification = NULL;
	component->address = jausAddressCreate();
	if(!component->address)
	{
		cError("JausComponent:%d: Error allocating memory for component->address.\n", __LINE__);
		free(component);
		return NULL;
	}
	
	component->node = NULL;
	component->state = JAUS_UNDEFINED_STATE;
	component->authority = JAUS_DEFAULT_AUTHORITY;
	component->services = jausServicesCreate();
	if(!component->services)
	{
		cError("JausComponent:%d: Error allocating memory for component->services.\n", __LINE__);
		jausAddressDestroy(component->address);
		free(component);
		return NULL;
	}

	component->controller.active = JAUS_FALSE;
	component->controller.authority = JAUS_DEFAULT_AUTHORITY;
	component->controller.state = JAUS_UNDEFINED_STATE;
	component->controller.address = jausAddressCreate();
	if(!component->controller.address)
	{
		cError("JausComponent:%d: Error allocating memory for component->controller.address.\n", __LINE__);
		jausServicesDestroy(component->services);
		jausAddressDestroy(component->address);
		free(component);
		return NULL;
	}

	component->port = 0;
	jausComponentUpdateTimestamp(component);

	return component;
}

void jausComponentDestroy(JausComponent component)
{
	if(component)
	{
		jausServicesDestroy(component->services);
		jausAddressDestroy(component->controller.address);
		jausAddressDestroy(component->address);
		free(component);
	}	
}

JausComponent jausComponentClone(JausComponent component)
{
	JausComponent clone;
	
	clone = (JausComponent)malloc( sizeof(JausComponentStruct) );
	if(clone == NULL)
	{
		return NULL;
	}

	// Init Values
	if(component->identification)
	{
		sprintf(clone->identification, "%s", component->identification);
	}
	else
	{
		clone->identification = NULL;
	}
	clone->address = jausAddressCreate();
	clone->address->id = component->address->id;
	clone->node = component->node;
	clone->state = component->state;
	clone->authority = component->authority;

	clone->controller.active = component->controller.active;
	clone->controller.authority = component->controller.authority;
	clone->controller.state = component->controller.state;
	clone->controller.address = jausAddressCreate();
	clone->controller.address->id = component->controller.address->id;

	clone->port = component->port;
	clone->services = jausServicesDuplicate(component->services);
	
	jausComponentUpdateTimestamp(clone);

	return clone;
}

char *jausComponentGetTypeString(JausComponent component)
{
	switch(component->address->component)
	{	
		case JAUS_NODE_MANAGER:
				return "NodeManager";
		case JAUS_SYSTEM_COMMANDER:
				return "SystemCommander";
		case JAUS_SUBSYSTEM_COMMANDER:
				return "SubsystemCommander";
		case JAUS_COMMUNICATOR:
				return "Communicator";
		case JAUS_GLOBAL_POSE_SENSOR:
				return "GlobalPoseSensor";
		case JAUS_LOCAL_POSE_SENSOR:
				return "LocalPosSensor";
		case JAUS_VELOCITY_STATE_SENSOR:
				return "VelocityStateSensor";
		case JAUS_PRIMITIVE_DRIVER:
				return "PrimitiveDriver";
		case JAUS_REFLEXIVE_DRIVER:
				return "ReflexiveDriver";
		case JAUS_GLOBAL_VECTOR_DRIVER:
				return "GlobalVectorDriver";
		case JAUS_LOCAL_VECTOR_DRIVER:
				return "LocalVectorDriver";
		case JAUS_GLOBAL_WAYPOINT_DRIVER:
				return "GlobalWaypointDriver";
		case JAUS_LOCAL_WAYPOINT_DRIVER:
				return "LocalWaypointDriver";
		case JAUS_GLOBAL_PATH_SEGMENT_DRIVER:
				return "GlobalPathSegmentDriver";
		case JAUS_LOCAL_PATH_SEGMENT_DRIVER:
				return "LocalPathSegmentDriver";
		case JAUS_PRIMATIVE_MANIPULATOR:
				return "PrimativeManipulator";
		case JAUS_MANIPULATOR_JOINT_POSITION_SENSOR:
				return "ManipulatorJointPositionSensor";
		case JAUS_MANIPULATOR_JOINT_VELOCITY_SENSOR:
				return "ManipulatorJointVelocitySensor";
		case JAUS_MANIPULATOR_JOINT_FORCE_TORQUE_SENSOR:
				return "ManipulatorJointForce/TorqueSensor";
		case JAUS_MANIPULATOR_JOINT_POSITIONS_DRIVER:
				return "ManipulatorJointPositionsDriver";
		case JAUS_MANIPULATOR_END_EFFECTOR_DRIVER:
				return "ManipulatorEndEffectorDriver";
		case JAUS_MANIPULATOR_JOINT_VELOCITIES_DRIVER:
				return "ManipulatorJointVelocitiesDriver";
		case JAUS_MANIPULATOR_END_EFFECTOR_VELOCITY_STATE_DRIVER:
				return "ManipulatorEndEffectorVelocityStateDriver";
		case JAUS_MANIPULATOR_JOINT_MOVE_DRIVER:
				return "ManipulatorJointMoveDriver";
		case JAUS_MANIPULATOR_END_EFFECTOR_DISCRETE_POSE_DRIVER:
				return "ManipulatorEndEffectorDiscretePoseDriver";
		case JAUS_VISUAL_SENSOR:
				return "VisualSensor";
		case JAUS_RANGE_SENSOR:
				return "RangeSensor";
		default:
				return "Experimental/UnkownComponent";
	}
}

// Updates the input JausComponent's timestamp to the current time in seconds
void jausComponentUpdateTimestamp(JausComponent component)
{
	component->timestamp = getTimeSeconds();  
}

// Test if the component has not been updated in COMPONENT_TIMEOUT_SEC seconds
JausBoolean jausComponentIsTimedOut(JausComponent component)
{
	return (getTimeSeconds() - JAUS_COMPONENT_TIMEOUT_SEC) > component->timestamp ? JAUS_TRUE : JAUS_FALSE;
}

int jausComponentToString(JausComponent component, char *buf)
{
	if(component->identification)
	{
		if(component->port == 0)
		{
			return sprintf(buf, "%s (%s)-%d.%d", component->identification, 
												 jausComponentGetTypeString(component), 
												 component->address->component, 
												 component->address->instance);
		}
		else
		{
			return sprintf(buf, "%s (%s)-%d.%d (Port: %d)", component->identification, 
												 jausComponentGetTypeString(component), 
												 component->address->component, 
												 component->address->instance, 
												 component->port);
		}
	}
	else
	{
		if(component->port == 0)
		{
			return sprintf(buf, "%s-%d.%d", jausComponentGetTypeString(component), 
												component->address->component, 
												component->address->instance);
		}
		else
		{
			return sprintf(buf, "%s-%d.%d (Port: %d)", jausComponentGetTypeString(component), 
												component->address->component, 
												component->address->instance, 
												component->port);
		}
	}
}
