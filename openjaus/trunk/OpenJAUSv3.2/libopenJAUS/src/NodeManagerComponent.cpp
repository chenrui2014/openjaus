#include "NodeManagerComponent.h"
#include "JausComponentCommunicationManager.h"
#include "ErrorEvent.h"
#include "EventHandler.h"
#include "SystemTree.h"
#include "jaus.h"
#include "timeLib.h"

#define _USE_MATH_DEFINES
#include <math.h>

NodeManagerComponent::NodeManagerComponent(FileLoader *configData, EventHandler *handler, JausComponentCommunicationManager *cmptComms)
{
	int subsystemId, nodeId;

	this->eventHandler = handler;

	if(configData == NULL)
	{
		// OK, don't do this. This is bad.
		ErrorEvent *e = new ErrorEvent(ErrorEvent::NullPointer, __FUNCTION__, __LINE__, "configData is NULL");
		this->eventHandler->handleEvent(e);
		return;
	}

	if(cmptComms == NULL)
	{
		// OK, don't do this. This is bad.
		ErrorEvent *e = new ErrorEvent(ErrorEvent::NullPointer, __FUNCTION__, __LINE__, "cmptComms is NULL");
		this->eventHandler->handleEvent(e);
		return;
	}

	this->type = COMPONENT_INTERFACE;
	this->commMngr = cmptComms;
	this->configData = configData;
	this->name = "OpenJAUS Node Manager v2.0";
	this->cmptRateHz = NM_RATE_HZ;
	this->systemTree = cmptComms->getSystemTree();
	for(int i = 0; i < MAXIMUM_EVENT_ID; i++)
	{
		eventId[i] = false;
	}

	// NOTE: These two values should exist in the properties file and should be checked 
	// in the NodeManager class prior to constructing this object
	subsystemId = configData->GetConfigDataInt("JAUS", "SubsystemId");
	if(subsystemId < JAUS_MINIMUM_SUBSYSTEM_ID || subsystemId > JAUS_MAXIMUM_SUBSYSTEM_ID)
	{
		// Invalid ID
		ErrorEvent *e = new ErrorEvent(ErrorEvent::Configuration, __FUNCTION__, __LINE__, "Invalid SubsystemId");
		this->eventHandler->handleEvent(e);
		return;
	}

	nodeId = configData->GetConfigDataInt("JAUS", "NodeId");
	if(nodeId < JAUS_MINIMUM_NODE_ID || nodeId > JAUS_MAXIMUM_NODE_ID)
	{
		// Invalid ID
		ErrorEvent *e = new ErrorEvent(ErrorEvent::Configuration, __FUNCTION__, __LINE__, "Invalid NodeId");
		this->eventHandler->handleEvent(e);
		return;
	}

	this->cmpt = jausComponentCreate();
	if(!this->cmpt)
	{
		ErrorEvent *e = new ErrorEvent(ErrorEvent::Memory, __FUNCTION__, __LINE__, "Cannot create component");
		this->eventHandler->handleEvent(e);
		return;
	}

	this->cmpt->address->subsystem = subsystemId;
	this->cmpt->address->node = nodeId;
	this->cmpt->address->component = JAUS_NODE_MANAGER;
	this->cmpt->address->instance = JAUS_MINIMUM_INSTANCE_ID;
	this->setupJausServices();

	this->cmpt->node = systemTree->getNode(subsystemId, nodeId);
	this->cmpt->identification = (char *)this->name.c_str();

	this->startupState();
	this->setupThread();

	if(!systemTree->addComponent(this->cmpt->address, this->cmpt))
	{
		ErrorEvent *e = new ErrorEvent(ErrorEvent::Configuration, __FUNCTION__, __LINE__, "Cannot add Node Manager component");
		this->eventHandler->handleEvent(e);
	}
}

NodeManagerComponent::~NodeManagerComponent(void){}

bool NodeManagerComponent::processMessage(JausMessage message)
{
	if(!message || !message->destination)
	{
		ErrorEvent *e = new ErrorEvent(ErrorEvent::NullPointer, __FUNCTION__, __LINE__, "Invalid JausMessage.");
		this->eventHandler->handleEvent(e);
		return false;
	}

	//// Check for loopback of message from myself
	//if(jausAddressEqual(message->source, cmpt->address))
	//{
	//	// We sent a broadcast (*.*.255.255) message probably
	//	// Just eat it.
	//	jausMessageDestroy(message);
	//	return true;
	//}

	//char buf[80] = {0};
	//jausAddressToString(message->source, buf);
	//printf("NM Process %s from %s", jausMessageCommandCodeString(message), buf);
	//jausAddressToString(message->destination, buf);
	//printf(" to %s\n", buf);

	// TODO: Let's add our hack here for making every message look like a heartbeat!
	switch(message->commandCode)
	{
		case JAUS_SET_COMPONENT_AUTHORITY:
		case JAUS_SHUTDOWN:
		case JAUS_STANDBY:
		case JAUS_RESUME:
		case JAUS_RESET:
		case JAUS_SET_EMERGENCY:
		case JAUS_CLEAR_EMERGENCY:
			// These messages are ignored!
			jausMessageDestroy(message);
			return true;
		
		case JAUS_CREATE_SERVICE_CONNECTION:
			return processCreateServiceConnection(message);

		case JAUS_ACTIVATE_SERVICE_CONNECTION:
			return processActivateServiceConnection(message);

		case JAUS_SUSPEND_SERVICE_CONNECTION:
			return processSuspendServiceConnection(message);

		case JAUS_TERMINATE_SERVICE_CONNECTION:
			return processTerminateServiceConnection(message);

		case JAUS_REQUEST_COMPONENT_CONTROL:
			return processRequestComponentControl(message);

		case JAUS_QUERY_COMPONENT_AUTHORITY:
			return processQueryComponentAuthority(message);

		case JAUS_QUERY_COMPONENT_STATUS:
			return processQueryComponentStatus(message);

		case JAUS_QUERY_HEARTBEAT_PULSE:
			return processQueryHeartbeatPulse(message);

		case JAUS_REPORT_HEARTBEAT_PULSE:
			return processReportHeartbeatPulse(message);

		case JAUS_QUERY_CONFIGURATION:
			return processQueryConfiguration(message);

		case JAUS_QUERY_IDENTIFICATION:
			return processQueryIdentification(message);

		case JAUS_QUERY_SERVICES:
			return processQueryServices(message);

		case JAUS_REPORT_CONFIGURATION:
			return processReportConfiguration(message);

		case JAUS_REPORT_IDENTIFICATION:
			return processReportIdentification(message);

		case JAUS_REPORT_SERVICES:
			return processReportServices(message);

		case JAUS_CANCEL_EVENT:
			return processCancelEvent(message);

		case JAUS_CONFIRM_EVENT:
			return processConfirmEvent(message);

		case JAUS_CREATE_EVENT:
			return processCreateEvent(message);

		default:
			// Unhandled message received by node manager component
			jausMessageDestroy(message);
			return false;
	}
}

std::string NodeManagerComponent::toString()
{
	return "Node Manager Component";
}

JausAddress NodeManagerComponent::checkInLocalComponent(int cmptId)
{
	JausComponent component = jausComponentCreate();
	if(!component) return NULL;
	
	// Setup query address
	component->address->subsystem = this->cmpt->address->subsystem;
	component->address->node = this->cmpt->address->node;
	component->address->component = cmptId;
	component->address->instance = 0;

	// Query SystemTree for next valid instance ID
	component->address->instance = this->getCommunicationManager()->getSystemTree()->getNextInstanceId(component->address);
	
	// Check returned value
	if(component->address->instance == JAUS_INVALID_INSTANCE_ID)
	{
		jausComponentDestroy(component);
		return NULL;
	}
	else
	{
		if(this->commMngr->getSystemTree()->addComponent(component->address, component))
		{
			JausAddress address = jausAddressClone(component->address);
			jausComponentDestroy(component);
			
			sendNodeChangedEvents();
			return address;
		}
		else
		{
			jausComponentDestroy(component);
			return NULL;
		}
	}
}

void NodeManagerComponent::checkOutLocalComponent(JausAddress address)
{
	return checkOutLocalComponent(address->subsystem, address->node, address->component, address->instance);
}

void NodeManagerComponent::checkOutLocalComponent(int subsId, int nodeId, int cmptId, int instId)
{
	if(systemTree->removeComponent(subsId, nodeId, cmptId, instId))
	{
		sendNodeChangedEvents();
	}
}

void NodeManagerComponent::startupState()
{
}

void NodeManagerComponent::intializeState()
{
	// Nothing to do
	this->cmpt->state = JAUS_READY_STATE;
}

void NodeManagerComponent::standbyState()
{
	// Nothing to do
}

void NodeManagerComponent::readyState()
{
	// Nothing to do
}

void NodeManagerComponent::emergencyState()
{
	// Nothing to do
}

void NodeManagerComponent::failureState()
{
	// Nothing to do
}

void NodeManagerComponent::shutdownState()
{
	// Nothing to do
}

void NodeManagerComponent::allState()
{
	static double refreshTime = 0;
	generateHeartbeats();
	if(getTimeSeconds() >= refreshTime)
	{
		systemTree->refresh();
		refreshTime = getTimeSeconds() + REFRESH_TIME_SEC;
	}

	// TODO: Check for serviceConnections
}

bool NodeManagerComponent::processReportConfiguration(JausMessage message)
{
	// This function follows the flowchart designed for NM 2.0 by D. Kent and T. Galluzzo
	ReportConfigurationMessage reportConf = NULL;

	reportConf = reportConfigurationMessageFromJausMessage(message);
	if(!reportConf)
	{
		// Error unpacking the reportConf
		ErrorEvent *e = new ErrorEvent(ErrorEvent::Memory, __FUNCTION__, __LINE__, "Cannot unpack ReportConf");
		this->eventHandler->handleEvent(e);
		jausMessageDestroy(message);
		return false;
	}

	// Has Subs?
	if(!systemTree->hasSubsystem(reportConf->source))
	{
		// TODO: Throw Exception. Log Error.
		reportConfigurationMessageDestroy(reportConf);
		jausMessageDestroy(message);
		return false;
	}

	// My Subs?
	if(reportConf->source->subsystem != this->cmpt->address->subsystem)
	{
		systemTree->replaceSubsystem(reportConf->source, reportConf->subsystem);

		JausSubsystem subs = systemTree->getSubsystem(reportConf->source);

		for(int i = 0; i < subs->nodes->elementCount; i++)
		{
			JausNode node = (JausNode) subs->nodes->elementData[i];

			if(!jausNodeHasIdentification(node))
			{
				JausAddress address = jausAddressCreate();
				if(!address)
				{
					ErrorEvent *e = new ErrorEvent(ErrorEvent::Memory, __FUNCTION__, __LINE__, "Cannot create address");
					this->eventHandler->handleEvent(e);
					reportConfigurationMessageDestroy(reportConf);
					jausMessageDestroy(message);
					return false;
				}
				address->subsystem = reportConf->source->subsystem;
				address->node = reportConf->source->node;
				address->component = JAUS_NODE_MANAGER;
				address->instance = 1;

				sendQueryNodeIdentification(address);
				jausAddressDestroy(address);
			}

			for(int j = 0; j < node->components->elementCount; j++)
			{
				JausComponent cmpt = (JausComponent) node->components->elementData[j];
				
				if(!jausComponentHasIdentification(cmpt))
				{
					sendQueryComponentIdentification(cmpt->address);
				}

				if(!jausComponentHasServices(cmpt))
				{
					sendQueryComponentServices(cmpt->address);
				}
			} // End For(Components)
		} // End For(Nodes)

		jausSubsystemDestroy(subs);
		reportConfigurationMessageDestroy(reportConf);
		jausMessageDestroy(message);
		return true;
	} // End !MySubs?
	
	if(!systemTree->hasNode(reportConf->source))
	{
		// Report Conf from unknown node! This is an unsolicited report
		ErrorEvent *e = new ErrorEvent(ErrorEvent::Message, __FUNCTION__, __LINE__, "Report Conf from unknown node! This is an unsolicited report");
		this->eventHandler->handleEvent(e);
		reportConfigurationMessageDestroy(reportConf);
		jausMessageDestroy(message);
		return true;
	}

	// This is a node-level report
	// Should only have 1 node attached!
	if(reportConf->subsystem->nodes->elementCount > 1)
	{
		// Warning
		ErrorEvent *e = new ErrorEvent(ErrorEvent::Warning, __FUNCTION__, __LINE__, "Node-level Report Conf with more than 1 node included. Ignoring other nodes.");
		this->eventHandler->handleEvent(e);
	}

	JausNode node = (JausNode) reportConf->subsystem->nodes->elementData[0];
	
	// Replace Node
	systemTree->replaceNode(reportConf->source, node);

	for(int i = 0; i < node->components->elementCount; i++)
	{
		JausComponent cmpt = (JausComponent) node->components->elementData[i];
		
		if(!jausComponentHasIdentification(cmpt))
		{
			sendQueryComponentIdentification(cmpt->address);
		}

		if(!jausComponentHasServices(cmpt))
		{
			sendQueryComponentServices(cmpt->address);
		}
	}

	// Send subs changed events
	sendSubsystemChangedEvents();
	
	reportConfigurationMessageDestroy(reportConf);
	jausMessageDestroy(message);
	return true;
}


bool NodeManagerComponent::processReportIdentification(JausMessage message)
{
	// This function follows the flowchart designed for NM 2.0 by D. Kent and T. Galluzzo
	ReportIdentificationMessage reportId = NULL;
	
	reportId = reportIdentificationMessageFromJausMessage(message);
	if(!reportId)
	{
		// Error unpacking the reportId msg
		// TODO: Log Error. Throw Exception?
		jausMessageDestroy(message);
		return false;
	}

	switch(reportId->queryType)
	{
		case JAUS_QUERY_FIELD_SYSTEM_IDENTITY:
			// We don't care about this (actually we never ask for this, so this shouldn't happen)
			reportIdentificationMessageDestroy(reportId);
			jausMessageDestroy(message);
			return true;

		case JAUS_QUERY_FIELD_SS_IDENTITY:
			// Has Subs?
			if(!systemTree->hasSubsystem(reportId->source))
			{
				// Report ID from unknown Subsystem
				// TODO: Log Error. Throw Exception
				reportIdentificationMessageDestroy(reportId);
				jausMessageDestroy(message);
				return false;
			}

			// Add Identification
			systemTree->setSubsystemIdentification(reportId->source, reportId->identification);
			
			// Query Subs Conf & Setup event
			sendQuerySubsystemConfiguration(reportId->source, true);
			reportIdentificationMessageDestroy(reportId);
			jausMessageDestroy(message);
			return true;
			
		case JAUS_QUERY_FIELD_NODE_IDENTITY:
			// Has Node?
			if(!systemTree->hasNode(reportId->source))
			{
				// Report ID from unknown Node
				// TODO: Log Error. Throw Exception
				reportIdentificationMessageDestroy(reportId);
				jausMessageDestroy(message);
				return false;
			}

			// Add Identification
			systemTree->setNodeIdentification(reportId->source, reportId->identification);
			
			// Query Subs Conf & Setup event
			sendQueryNodeConfiguration(reportId->source, true);
			reportIdentificationMessageDestroy(reportId);
			jausMessageDestroy(message);
			return true;

		case JAUS_QUERY_FIELD_COMPONENT_IDENTITY:
			// Has Cmpt?
			if(!systemTree->hasComponent(reportId->source))
			{
				// Report ID from unknown Component
				// TODO: Log Error. Throw Exception
				reportIdentificationMessageDestroy(reportId);
				jausMessageDestroy(message);
				return false;
			}

			// Add Identification
			systemTree->setComponentIdentification(reportId->source, reportId->identification);
			reportIdentificationMessageDestroy(reportId);
			jausMessageDestroy(message);
			return true;

		default:
			// TODO: Log Error. Throw Exception.
			reportIdentificationMessageDestroy(reportId);
			jausMessageDestroy(message);
			return true;
	}

	reportIdentificationMessageDestroy(reportId);
	jausMessageDestroy(message);
	return true;
}

bool NodeManagerComponent::processReportServices(JausMessage message)
{
	ReportServicesMessage reportServices = NULL;

	reportServices = reportServicesMessageFromJausMessage(message);
	if(!reportServices)
	{
		// TODO: Log error. Throw Exception.
		jausMessageDestroy(message);
		return false;
	}

	if(systemTree->hasComponent(reportServices->source))
	{
		systemTree->setComponentServices(reportServices->source, reportServices->jausServices);
	}

	reportServicesMessageDestroy(reportServices);
	jausMessageDestroy(message);
	return true;
}

bool NodeManagerComponent::processReportHeartbeatPulse(JausMessage message)
{
	// This function follows the flowchart designed for NM 2.0 by D. Kent and T. Galluzzo
	//printf("Process HB from: %d.%d.%d.%d\n", message->source->subsystem, message->source->node, message->source->component, message->source->instance);
	
	// Has Subs?
	if(!systemTree->hasSubsystem(message->source))
	{
		// Add Subs
		systemTree->addSubsystem(message->source, NULL);
		
		// Query SubsId
		sendQuerySubsystemIdentification(message->source);
		jausMessageDestroy(message);
		return true;
	}

	// Update Subsystem Timestamp
	systemTree->updateSubsystemTimestamp(message->source);
	
	// Has SubsId?
	if(!systemTree->hasSubsystemIdentification(message->source))
	{
		// Query SubsId
		sendQuerySubsystemIdentification(message->source);
		jausMessageDestroy(message);
		return true;
	}

	// My Subs?
	if(message->source->subsystem != this->cmpt->address->subsystem)
	{
		// Has SubsConf?
		if(!systemTree->hasSubsystemConfiguration(message->source))
		{
			sendQuerySubsystemConfiguration(message->source, true);
			jausMessageDestroy(message);
			return true;
		}

		JausSubsystem subs = systemTree->getSubsystem(message->source);
		if(!subs)
		{
			// TODO: Throw Exception. Log Error.
			jausMessageDestroy(message);
			return false;
		}

		for(int i = 0; i < subs->nodes->elementCount; i++)
		{
			JausNode node = (JausNode) subs->nodes->elementData[i];

			if(!jausNodeHasIdentification(node))
			{
				JausAddress address = jausAddressCreate();
				if(!address)
				{
					// TODO: Throw Exception. Log Error.
					jausSubsystemDestroy(subs);
					jausMessageDestroy(message);
					return false;
				}
				address->subsystem = message->source->subsystem;
				address->node = node->id;
				address->component = JAUS_NODE_MANAGER;
				address->instance = 1;

				sendQueryNodeIdentification(address);
				jausAddressDestroy(address);
			}

			for(int j = 0; j < node->components->elementCount; j++)
			{
				JausComponent cmpt = (JausComponent) node->components->elementData[j];
				
				if(!jausComponentHasIdentification(cmpt))
				{
					sendQueryComponentIdentification(cmpt->address);
				}

				if(!jausComponentHasServices(cmpt))
				{
					sendQueryComponentServices(cmpt->address);
				}
			} // end For(Components)
		} // end For(Nodes)

		jausMessageDestroy(message);
		return true;
	} // End !MySubs

	// Has Node?
	if(!systemTree->hasNode(message->source))
	{
		// Add Node
		systemTree->addNode(message->source, NULL);

		// Query NodeID
		// Setup Query Address
		JausAddress address = jausAddressCreate();
		if(!address)
		{
			// TODO: Throw Exception. Log Error.
			jausMessageDestroy(message);
			return false;
		}
		address->subsystem = message->source->subsystem;
		address->node = message->source->node;
		address->component = JAUS_NODE_MANAGER;
		address->instance = 1;

		// Query Node ID
		sendQueryNodeIdentification(address);
		jausAddressDestroy(address);
		jausMessageDestroy(message);
		return true;
	}

	// Update Node Timestamp
	systemTree->updateNodeTimestamp(message->source);

	// Has NodeId?
	if(!systemTree->hasNodeIdentification(message->source))
	{
		// Query NodeID
		// Setup Query Address
		JausAddress address = jausAddressCreate();
		if(!address)
		{
			// TODO: Throw Exception. Log Error.
			jausMessageDestroy(message);
			return false;
		}
		address->subsystem = message->source->subsystem;
		address->node = message->source->node;
		address->component = JAUS_NODE_MANAGER;
		address->instance = 1;

		// Query Node ID
		sendQueryNodeIdentification(address);
		jausAddressDestroy(address);
		jausMessageDestroy(message);
		return true;
	}

	// My Node?
	if(message->source->node != this->cmpt->address->node)
	{
		// Has NodeConf?
		if(!systemTree->hasNodeConfiguration(message->source))
		{
			// Query NodeConf
			JausAddress address = jausAddressCreate();
			if(!address)
			{
				// TODO: Throw Exception. Log Error.
				jausMessageDestroy(message);
				return false;
			}
			address->subsystem = message->source->subsystem;
			address->node = message->source->node;
			address->component = JAUS_NODE_MANAGER;
			address->instance = 1;

			sendQueryNodeConfiguration(address, true);
			jausAddressDestroy(address);
			jausMessageDestroy(message);
			return true;
		}
		else
		{
			JausNode node = systemTree->getNode(message->source);
			
			// Check for component ID and Services
			for(int i = 0; i < node->components->elementCount; i++)
			{
				JausComponent cmpt = (JausComponent) node->components->elementData[i];
				if(!jausComponentHasIdentification(cmpt))
				{
					sendQueryComponentIdentification(cmpt->address);
				}

				if(!jausComponentHasServices(cmpt))
				{
					sendQueryComponentServices(cmpt->address);
				}
			}

			jausMessageDestroy(message);
			return false;
		} 
	}// End !MyNode

	// Has Component?
	if(!systemTree->hasComponent(message->source))
	{
		// Add Component
		systemTree->addComponent(message->source, NULL);
	}
	else
	{
		// Update Component Timestamp
		systemTree->updateComponentTimestamp(message->source);
	}

	if(!systemTree->hasComponentIdentification(message->source))
	{
		//printf("Send Query Cmpt Id\n");
		sendQueryComponentIdentification(message->source);
	}

	if(!systemTree->hasComponentServices(message->source))
	{
		//printf("Send Query Cmpt Services\n");
		sendQueryComponentServices(message->source);
	}

	jausMessageDestroy(message);
	return true;
}

bool NodeManagerComponent::processCreateEvent(JausMessage message)
{
	// Only support Configuration Changed events
	CreateEventMessage createEvent = NULL;
	QueryConfigurationMessage queryConf = NULL;
	ConfirmEventMessage confirmEvent = NULL;
	JausMessage txMessage = NULL;
	int nextEventId = -1;
	HASH_MAP <int, JausAddress>::iterator iterator;
	
	confirmEvent = confirmEventMessageCreate();
	if(!confirmEvent)
	{
		//TODO: Log Error. Throw Exception
		return false;
	}
	jausAddressCopy(confirmEvent->destination, message->source);
	jausAddressCopy(confirmEvent->source, cmpt->address);
	
	createEvent = createEventMessageFromJausMessage(message);
	if(!createEvent)
	{
		//TODO: Log Error. Throw Exception
		confirmEvent->responseCode = INVALID_EVENT_RESPONSE;
		txMessage = confirmEventMessageToJausMessage(confirmEvent);
		if(txMessage)
		{
			this->commMngr->receiveJausMessage(txMessage, this);
		}
		confirmEventMessageDestroy(confirmEvent);
		return false;
	}

	if(createEvent->messageCode != JAUS_QUERY_CONFIGURATION)
	{
		// Currently the NM only supports configuration changed events
		confirmEvent->responseCode = MESSAGE_UNSUPPORTED_RESPONSE;
		txMessage = confirmEventMessageToJausMessage(confirmEvent);
		if(txMessage)
		{
			this->commMngr->receiveJausMessage(txMessage, this);
		}
		confirmEventMessageDestroy(confirmEvent);
		return false;
	}
	confirmEvent->messageCode = JAUS_REPORT_CONFIGURATION;

	queryConf = queryConfigurationMessageFromJausMessage(createEvent->queryMessage);
	if(!queryConf)
	{
		// ERROR: Cannot unpack query message
		// TODO: Log Error. Throw Exception
		confirmEvent->responseCode = INVALID_EVENT_RESPONSE;
		txMessage = confirmEventMessageToJausMessage(confirmEvent);
		if(txMessage)
		{
			this->commMngr->receiveJausMessage(txMessage, this);
		}
		confirmEventMessageDestroy(confirmEvent);
		return false;
	}
	
	switch(queryConf->queryField)
	{
		case JAUS_SUBSYSTEM_CONFIGURATION:
			for(iterator = subsystemChangeList.begin(); iterator != subsystemChangeList.end(); iterator++)
			{
				if(jausAddressEqual(createEvent->source, iterator->second))
				{
					// Event already created
					confirmEvent->responseCode = SUCCESSFUL_RESPONSE;
					confirmEvent->eventId = iterator->first;
					break;
				}
			}

			nextEventId = getNextEventId();
			if(nextEventId >= 0)
			{
				confirmEvent->eventId = (JausByte) nextEventId;
				eventId[nextEventId] = true;
				subsystemChangeList[nextEventId] = jausAddressClone(createEvent->source);
				confirmEvent->responseCode = SUCCESSFUL_RESPONSE;
			}
			else
			{
				confirmEvent->responseCode = CONNECTION_REFUSED_RESPONSE;
				confirmEvent->eventId = 0;
			}
			break;
		
		case JAUS_NODE_CONFIGURATION:
			for(iterator = nodeChangeList.begin(); iterator != nodeChangeList.end(); iterator++)
			{
				if(jausAddressEqual(createEvent->source, iterator->second))
				{
					// Event already created
					confirmEvent->responseCode = SUCCESSFUL_RESPONSE;
					confirmEvent->eventId = iterator->first;
					break;
				}
			}

			nextEventId = getNextEventId();
			if(nextEventId >= 0)
			{
				confirmEvent->eventId = (JausByte) nextEventId;
				eventId[nextEventId] = true;
				nodeChangeList[nextEventId] = jausAddressClone(createEvent->source);
				confirmEvent->responseCode = SUCCESSFUL_RESPONSE;
			}
			else
			{
				confirmEvent->responseCode = CONNECTION_REFUSED_RESPONSE;
				confirmEvent->eventId = 0;
			}
			break;

		default:
			// TODO: Log Error. Throw Exception.
			// Unknown Query Type
			confirmEvent->responseCode = INVALID_EVENT_RESPONSE;
			txMessage = confirmEventMessageToJausMessage(confirmEvent);
			if(txMessage)
			{
				this->commMngr->receiveJausMessage(txMessage, this);
			}
			confirmEventMessageDestroy(confirmEvent);
			return false;
	}

	// Send response
	txMessage = confirmEventMessageToJausMessage(confirmEvent);
	if(txMessage)
	{
		this->commMngr->receiveJausMessage(txMessage, this);
	}

	queryConfigurationMessageDestroy(queryConf);
	createEventMessageDestroy(createEvent);
	return true;
}

bool NodeManagerComponent::processCancelEvent(JausMessage message)
{
	CancelEventMessage cancelEvent = NULL;

	cancelEvent = cancelEventMessageFromJausMessage(message);
	if(!cancelEvent)
	{
		// Error unpacking message
		// TODO: Throw Exception. Log Error.
		return false;
	}

	if(cancelEvent->messageCode == JAUS_QUERY_CONFIGURATION)
	{
		if(eventId[cancelEvent->eventId])
		{
			eventId[cancelEvent->eventId] = false;

			if(subsystemChangeList.find(cancelEvent->eventId) != subsystemChangeList.end())
			{
				// Remove that element
				subsystemChangeList.erase(subsystemChangeList.find(cancelEvent->eventId));
			}
			else if(nodeChangeList.find(cancelEvent->eventId) != nodeChangeList.end())
			{
				// Remove that element
				nodeChangeList.erase(nodeChangeList.find(cancelEvent->eventId));
			}
		}
	}

	cancelEventMessageDestroy(cancelEvent);
	return true;
}

bool NodeManagerComponent::processCreateServiceConnection(JausMessage message)
{
	// Not implemented right now
	return true;
}

bool NodeManagerComponent::processActivateServiceConnection(JausMessage message)
{
	// Not Implemented right now
	return true;
}

bool NodeManagerComponent::processSuspendServiceConnection(JausMessage message)
{
	// Not Implemented right now
	return true;
}

bool NodeManagerComponent::processTerminateServiceConnection(JausMessage message)
{
	// Not Implemented right now
	return true;
}

bool NodeManagerComponent::processRequestComponentControl(JausMessage message)
{
	RequestComponentControlMessage requestComponentControl = NULL;
	RejectComponentControlMessage rejectComponentControl = NULL;
	ConfirmComponentControlMessage confirmComponentControl = NULL;
	JausMessage txMessage = NULL;

	requestComponentControl = requestComponentControlMessageFromJausMessage(message);
	if(!requestComponentControl)
	{
		// Error unpacking message
		// TODO: Log Error. Throw Exception
		return false;
	}

	if(cmpt->controller.active)
	{
		if(requestComponentControl->authorityCode > cmpt->controller.authority) // Test for higher authority
		{	
			// Terminate control of current component
			rejectComponentControl = rejectComponentControlMessageCreate();
			jausAddressCopy(rejectComponentControl->source, cmpt->address);
			jausAddressCopy(rejectComponentControl->destination, cmpt->controller.address);
			txMessage = rejectComponentControlMessageToJausMessage(rejectComponentControl); 
			if(txMessage)
			{
				this->commMngr->receiveJausMessage(txMessage, this);
			}
			
			// Accept control of new component
			confirmComponentControl = confirmComponentControlMessageCreate();
			jausAddressCopy(confirmComponentControl->source, cmpt->address);
			jausAddressCopy(confirmComponentControl->destination, message->source);
			confirmComponentControl->responseCode = JAUS_CONTROL_ACCEPTED;
			txMessage = confirmComponentControlMessageToJausMessage(confirmComponentControl); 
			if(txMessage)
			{
				this->commMngr->receiveJausMessage(txMessage, this);
			}
			
			// Update cmpt controller information
			jausAddressCopy(cmpt->controller.address, message->source);
			cmpt->controller.authority = requestComponentControl->authorityCode;
		
			rejectComponentControlMessageDestroy(rejectComponentControl);
			confirmComponentControlMessageDestroy(confirmComponentControl);						
		}	
		else
		{
			if(!jausAddressEqual(message->source, cmpt->controller.address))
			{
				rejectComponentControl = rejectComponentControlMessageCreate();
				jausAddressCopy(rejectComponentControl->source, cmpt->address);
				jausAddressCopy(rejectComponentControl->destination, message->source);
				txMessage = rejectComponentControlMessageToJausMessage(rejectComponentControl); 
				if(txMessage)
				{
					this->commMngr->receiveJausMessage(txMessage, this);
				}

				rejectComponentControlMessageDestroy(rejectComponentControl);
			}
			else
			{
				// Reaccept control of new component
				confirmComponentControl = confirmComponentControlMessageCreate();
				jausAddressCopy(confirmComponentControl->source, cmpt->address);
				jausAddressCopy(confirmComponentControl->destination, message->source);
				confirmComponentControl->responseCode = JAUS_CONTROL_ACCEPTED;
				txMessage = confirmComponentControlMessageToJausMessage(confirmComponentControl); 
				if(txMessage)
				{
					this->commMngr->receiveJausMessage(txMessage, this);
				}
				
				confirmComponentControlMessageDestroy(confirmComponentControl);						
			}
		}
	}					
	else // Not currently under component control, so give control
	{
		confirmComponentControl = confirmComponentControlMessageCreate();
		jausAddressCopy(confirmComponentControl->source, cmpt->address);
		jausAddressCopy(confirmComponentControl->destination, message->source);
		confirmComponentControl->responseCode = JAUS_CONTROL_ACCEPTED;
		txMessage = confirmComponentControlMessageToJausMessage(confirmComponentControl); 
		if(txMessage)
		{
			this->commMngr->receiveJausMessage(txMessage, this);
		}

		jausAddressCopy(cmpt->controller.address, message->source);
		cmpt->controller.authority = requestComponentControl->authorityCode;
		cmpt->controller.active = JAUS_TRUE;

		confirmComponentControlMessageDestroy(confirmComponentControl);						
	}

	requestComponentControlMessageDestroy(requestComponentControl);
	return true;
}

bool NodeManagerComponent::processQueryComponentAuthority(JausMessage message)
{
	ReportComponentAuthorityMessage report = NULL;
	JausMessage txMessage = NULL;


	report = reportComponentAuthorityMessageCreate();
	if(!report)
	{
		// TODO: Throw Exception. Log Error.
		return false;
	}

	jausAddressCopy(report->source, cmpt->address);
	jausAddressCopy(report->destination, message->source);
	report->authorityCode = cmpt->authority;
	
	txMessage = reportComponentAuthorityMessageToJausMessage(report);	
	if(txMessage)
	{
		this->commMngr->receiveJausMessage(txMessage, this);
	}

	reportComponentAuthorityMessageDestroy(report);
	return true;
}

bool NodeManagerComponent::processQueryComponentStatus(JausMessage message)
{
	ReportComponentStatusMessage reportComponentStatus = NULL;
	JausMessage txMessage = NULL;


	reportComponentStatus = reportComponentStatusMessageCreate();
	jausAddressCopy(reportComponentStatus->source, cmpt->address);
	jausAddressCopy(reportComponentStatus->destination, message->source);
	reportComponentStatus->primaryStatusCode = cmpt->state;
	
	txMessage = reportComponentStatusMessageToJausMessage(reportComponentStatus);	
	if(txMessage)
	{
		this->commMngr->receiveJausMessage(txMessage, this);
	}

	reportComponentStatusMessageDestroy(reportComponentStatus);
	return true;
}

bool NodeManagerComponent::processQueryHeartbeatPulse(JausMessage message)
{

	JausMessage txMessage = NULL;
	ReportHeartbeatPulseMessage reportHeartbeat = NULL;

	reportHeartbeat = reportHeartbeatPulseMessageCreate();
	if(!reportHeartbeat)
	{
		// TODO: Log Error. Throw Exception.
		return false;
	}

	jausAddressCopy(reportHeartbeat->source, cmpt->address);
	jausAddressCopy(reportHeartbeat->destination, message->source);
	txMessage = reportHeartbeatPulseMessageToJausMessage(reportHeartbeat);
	if(txMessage)
	{
		this->commMngr->receiveJausMessage(txMessage, this);
	}
	return true;
}

bool NodeManagerComponent::processQueryConfiguration(JausMessage message)
{
	QueryConfigurationMessage queryConf = NULL;
	ReportConfigurationMessage reportConf = NULL;
	JausMessage txMessage = NULL;
	JausNode node = NULL;

	queryConf = queryConfigurationMessageFromJausMessage(message);
	if(!queryConf)
	{
		// TODO: Log Error. Throw Exception.
		// Error unpacking message	
		return false;
	}
	
	switch(queryConf->queryField)
	{
		case JAUS_SUBSYSTEM_CONFIGURATION:
			// Subsystem Configuration requests should go to the Communicator!
			// This is included for backwards compatibility and other implementation support
			if(this->commMngr->getMessageRouter()->subsystemCommunicationEnabled())
			{
				reportConf = reportConfigurationMessageCreate();
				if(!reportConf)
				{
					// TODO: Log Error. Throw Exception
					queryConfigurationMessageDestroy(queryConf);
					return false;
				}
				
				// Remove the subsystem created by the constructor
				jausSubsystemDestroy(reportConf->subsystem);

				// This call to the systemTree returns a copy, so safe to set this pointer to it
				reportConf->subsystem = systemTree->getSubsystem(this->cmpt->address->subsystem);
				if(reportConf->subsystem)
				{
					txMessage = reportConfigurationMessageToJausMessage(reportConf);
					jausAddressCopy(txMessage->source, cmpt->address);
					jausAddressCopy(txMessage->destination, reportConf->source);
					if(txMessage)
					{
						this->commMngr->receiveJausMessage(txMessage, this);
					}
				}

				reportConfigurationMessageDestroy(reportConf);
				queryConfigurationMessageDestroy(queryConf);
				return true;
			}
			else
			{
				// This NM is not connected to the subsystem network, 
				// therefore no one should be asking us for subsystem configuration
				// TODO: Log Error. Throw Exception.
				queryConfigurationMessageDestroy(queryConf);
				return false;
			}

		case JAUS_NODE_CONFIGURATION:
			reportConf = reportConfigurationMessageCreate();
			if(!reportConf)
			{
				// TODO: Log Error. Throw Exception
				queryConfigurationMessageDestroy(queryConf);
				return false;
			}
			
			// This call to the systemTree returns a copy, so safe to set this pointer to it
			node = systemTree->getNode(this->cmpt->address->subsystem, this->cmpt->address->node);
			if(node)
			{
				jausArrayAdd(reportConf->subsystem->nodes, node);
			}

			txMessage = reportConfigurationMessageToJausMessage(reportConf);
			jausAddressCopy(txMessage->source, cmpt->address);
			jausAddressCopy(txMessage->destination, queryConf->source);
			if(txMessage)
			{
				this->commMngr->receiveJausMessage(txMessage, this);
			}

			reportConfigurationMessageDestroy(reportConf);
			queryConfigurationMessageDestroy(queryConf);
			return true;

		default:
			// TODO: Log Error. Throw Exception.
			// Unknown query type
			queryConfigurationMessageDestroy(queryConf);
			return false;
	}
}

bool NodeManagerComponent::processQueryIdentification(JausMessage message)
{
	QueryIdentificationMessage queryId = NULL;
	ReportIdentificationMessage reportId = NULL;
	JausMessage txMessage = NULL;
	char *identification = NULL;

	queryId = queryIdentificationMessageFromJausMessage(message);
	if(!queryId)
	{
		// TODO: Log Error. Throw Exception.
		// Error unpacking message	
		return false;
	}
	
	switch(queryId->queryField)
	{
		case JAUS_QUERY_FIELD_SS_IDENTITY:
			// Subsystem Configuration requests should go to the Communicator!
			// This is included for backwards compatibility and other implementation support
			if(this->commMngr->getMessageRouter()->subsystemCommunicationEnabled())
			{
				reportId = reportIdentificationMessageCreate();
				if(!reportId)
				{
					// TODO: Log Error. Throw Exception
					queryIdentificationMessageDestroy(queryId);
					return false;
				}

				identification = systemTree->getSubsystemIdentification(cmpt->address);
				if(strlen(identification) < JAUS_IDENTIFICATION_LENGTH_BYTES)
				{
					sprintf(reportId->identification, "%s", identification);
				}
				else
				{
					memcpy(reportId->identification, identification, JAUS_IDENTIFICATION_LENGTH_BYTES-1);
					reportId->identification[JAUS_IDENTIFICATION_LENGTH_BYTES-1] = 0;
				}

				reportId->queryType = JAUS_QUERY_FIELD_SS_IDENTITY;
				jausAddressCopy(txMessage->source, cmpt->address);
				jausAddressCopy(txMessage->destination, queryId->source);
				txMessage = reportIdentificationMessageToJausMessage(reportId);
				if(txMessage)
				{
					this->commMngr->receiveJausMessage(txMessage, this);
				}

				reportIdentificationMessageDestroy(reportId);
				queryIdentificationMessageDestroy(queryId);
				return true;
			}
			else
			{
				// This NM is not connected to the subsystem network, 
				// therefore no one should be asking us for subsystem configuration
				// TODO: Log Error. Throw Exception.
				queryIdentificationMessageDestroy(queryId);
				return false;
			}
		case JAUS_QUERY_FIELD_NODE_IDENTITY:
			reportId = reportIdentificationMessageCreate();
			if(!reportId)
			{
				// TODO: Log Error. Throw Exception
				queryIdentificationMessageDestroy(queryId);
				return false;
			}
			
			identification = systemTree->getNodeIdentification(cmpt->address);
			if(strlen(identification) < JAUS_IDENTIFICATION_LENGTH_BYTES)
			{
				sprintf(reportId->identification, "%s", identification);
			}
			else
			{
				memcpy(reportId->identification, identification, JAUS_IDENTIFICATION_LENGTH_BYTES-1);
				reportId->identification[JAUS_IDENTIFICATION_LENGTH_BYTES-1] = 0;
			}

			reportId->queryType = JAUS_QUERY_FIELD_NODE_IDENTITY;
			txMessage = reportIdentificationMessageToJausMessage(reportId);
			jausAddressCopy(txMessage->source, cmpt->address);
			jausAddressCopy(txMessage->destination, queryId->source);
			if(txMessage)
			{
				this->commMngr->receiveJausMessage(txMessage, this);
			}

			reportIdentificationMessageDestroy(reportId);
			queryIdentificationMessageDestroy(queryId);
			return true;

		case JAUS_QUERY_FIELD_COMPONENT_IDENTITY:
			reportId = reportIdentificationMessageCreate();
			if(!reportId)
			{
				// TODO: Log Error. Throw Exception
				queryIdentificationMessageDestroy(queryId);
				return false;
			}
			
			identification = cmpt->identification;
			if(strlen(identification) < JAUS_IDENTIFICATION_LENGTH_BYTES)
			{
				sprintf(reportId->identification, "%s", identification);
			}
			else
			{
				memcpy(reportId->identification, identification, JAUS_IDENTIFICATION_LENGTH_BYTES-1);
				reportId->identification[JAUS_IDENTIFICATION_LENGTH_BYTES-1] = 0;
			}

			reportId->queryType = JAUS_QUERY_FIELD_COMPONENT_IDENTITY;
			txMessage = reportIdentificationMessageToJausMessage(reportId);
			jausAddressCopy(txMessage->source, cmpt->address);
			jausAddressCopy(txMessage->destination, queryId->source);
			if(txMessage)
			{
				this->commMngr->receiveJausMessage(txMessage, this);
			}

			reportIdentificationMessageDestroy(reportId);
			queryIdentificationMessageDestroy(queryId);
			return true;

		default:
			queryIdentificationMessageDestroy(queryId);
			return false;
	}
}

bool NodeManagerComponent::processQueryServices(JausMessage message)
{
	QueryServicesMessage queryServices = NULL;
	ReportServicesMessage reportServices = NULL;
	JausMessage txMessage = NULL;


	queryServices = queryServicesMessageFromJausMessage(message);
	if(!queryServices)
	{
		// TODO: Log Error. Throw Exception.
		return false;
	}

	// Respond with our services
	reportServices = reportServicesMessageCreate();
	if(!reportServices)
	{
		// TODO: Log Error. Throw Exception.
		queryServicesMessageDestroy(queryServices);
		return false;
	}

	jausAddressCopy(reportServices->destination, message->source);
	jausAddressCopy(reportServices->source, cmpt->address);
	jausServicesDestroy(reportServices->jausServices);
	reportServices->jausServices = jausServicesClone(cmpt->services);

	txMessage = reportServicesMessageToJausMessage(reportServices);
	if(txMessage)
	{
		this->commMngr->receiveJausMessage(txMessage, this);
	}

	reportServicesMessageDestroy(reportServices);
	queryServicesMessageDestroy(queryServices);
	return true;
}

bool NodeManagerComponent::processConfirmEvent(JausMessage message)
{
	// Not currently implemented
	return true;
}

void NodeManagerComponent::sendNodeChangedEvents()
{
	ReportConfigurationMessage reportConf = NULL;
	JausMessage txMessage = NULL;	
	HASH_MAP <int, JausAddress>::iterator iterator;
	
	JausNode thisNode = systemTree->getNode(this->cmpt->address);
	if(!thisNode)
	{
		// TODO: Record an error. Throw Exception
		return;
	}

	reportConf = reportConfigurationMessageCreate();
	if(!reportConf)
	{
		// TODO: Record an error. Throw Exception
		jausNodeDestroy(thisNode);
		return;
	}
	jausArrayAdd(reportConf->subsystem->nodes, (void *)thisNode);

	txMessage = reportConfigurationMessageToJausMessage(reportConf);
	jausAddressCopy(txMessage->source, cmpt->address);

	// TODO: Go through nodeChangeList looking for dead addresses
	for(iterator = nodeChangeList.begin(); iterator != nodeChangeList.end(); iterator++)
	{
		jausAddressCopy(txMessage->destination, iterator->second);
		this->commMngr->receiveJausMessage(jausMessageClone(txMessage), this);
	}
	
	jausMessageDestroy(txMessage);
	reportConfigurationMessageDestroy(reportConf);
}

void NodeManagerComponent::sendSubsystemChangedEvents()
{
	ReportConfigurationMessage reportConf = NULL;
	JausMessage txMessage = NULL;	
	HASH_MAP <int, JausAddress>::iterator iterator;

	JausSubsystem thisSubs = systemTree->getSubsystem(this->cmpt->address);
	if(!thisSubs)
	{
		// TODO: Record an error. Throw Exception
		return;
	}

	reportConf = reportConfigurationMessageCreate();
	if(!reportConf)
	{
		// TODO: Record an error. Throw Exception
		jausSubsystemDestroy(thisSubs);
		return;
	}
	jausSubsystemDestroy(reportConf->subsystem);
	reportConf->subsystem = thisSubs;

	txMessage = reportConfigurationMessageToJausMessage(reportConf);
	jausAddressCopy(txMessage->source, cmpt->address);

	// TODO: check subsystemChangeList for dead addresses
	for(iterator = subsystemChangeList.begin(); iterator != subsystemChangeList.end(); iterator++)
	{
		jausAddressCopy(txMessage->destination, iterator->second);
		this->commMngr->receiveJausMessage(jausMessageClone(txMessage), this);
	}

	jausMessageDestroy(txMessage);
	reportConfigurationMessageDestroy(reportConf);
}

void NodeManagerComponent::generateHeartbeats()
{
	static double nextSendTime = getTimeSeconds();
	ReportHeartbeatPulseMessage heartbeat;
	JausMessage nodeHeartbeat;
	JausMessage cmptHeartbeat;
	
	if(getTimeSeconds() >= nextSendTime)
	{
		heartbeat = reportHeartbeatPulseMessageCreate();
		if(!heartbeat)
		{
			// Error constructing message
			// TODO: Log Error.
			return;
		}
		jausAddressCopy(heartbeat->source, cmpt->address);

		nodeHeartbeat = reportHeartbeatPulseMessageToJausMessage(heartbeat);
		if(!nodeHeartbeat)
		{
			// Error constructing message
			// TODO: Log Error.
			reportHeartbeatPulseMessageDestroy(heartbeat);
			return;
		}

		cmptHeartbeat = reportHeartbeatPulseMessageToJausMessage(heartbeat);
		if(!cmptHeartbeat)
		{
			// Error constructing message
			// TODO: Log Error.
			jausMessageDestroy(nodeHeartbeat);
			reportHeartbeatPulseMessageDestroy(heartbeat);
			return;
		}

		// Send Heartbeat to other node managers on this subsystem
		nodeHeartbeat->destination->subsystem = cmpt->address->subsystem;
		nodeHeartbeat->destination->node = JAUS_BROADCAST_NODE_ID;
		nodeHeartbeat->destination->component = JAUS_NODE_MANAGER_COMPONENT;
		nodeHeartbeat->destination->instance = 1;
		this->commMngr->receiveJausMessage(nodeHeartbeat, this);

		// Send Heartbeat to components on this node
		cmptHeartbeat->destination->subsystem = cmpt->address->subsystem;
		cmptHeartbeat->destination->node = cmpt->address->node;
		cmptHeartbeat->destination->component = JAUS_BROADCAST_COMPONENT_ID;
		cmptHeartbeat->destination->instance = JAUS_BROADCAST_INSTANCE_ID;
		this->commMngr->receiveJausMessage(cmptHeartbeat, this);

		nextSendTime = getTimeSeconds() + 1.0;
		reportHeartbeatPulseMessageDestroy(heartbeat);
	}
}

bool NodeManagerComponent::sendQueryNodeIdentification(JausAddress address)
{
	QueryIdentificationMessage queryId = NULL;
	JausMessage txMessage = NULL;

	// TODO: Check timeout and request limit
	if( systemTree->hasNode(address) && 
		!systemTree->hasNodeIdentification(address))
	{
		// Create query message
		queryId = queryIdentificationMessageCreate();
		if(!queryId)
		{
			// Constructor Failed
			return false;
		}
		
		queryId->queryField = JAUS_QUERY_FIELD_NODE_IDENTITY;
		txMessage = queryIdentificationMessageToJausMessage(queryId);
		if(!txMessage)
		{
			// ToJausMessage Failed
			queryIdentificationMessageDestroy(queryId);
			return false;
		}

		jausAddressCopy(txMessage->destination, address);
		jausAddressCopy(txMessage->source, cmpt->address);
		this->commMngr->receiveJausMessage(txMessage, this);
		queryIdentificationMessageDestroy(queryId);
		return true;
	}
	return false;
}

bool NodeManagerComponent::sendQuerySubsystemIdentification(JausAddress address)
{
	QueryIdentificationMessage queryId = NULL;
	JausMessage txMessage = NULL;

	// TODO: Check timeout and request limit
	if( systemTree->hasSubsystem(address) && 
		!systemTree->hasSubsystemIdentification(address))
	{
		// Create query message
		queryId = queryIdentificationMessageCreate();
		if(!queryId)
		{
			// Constructor Failed
			return false;
		}
		
		queryId->queryField = JAUS_QUERY_FIELD_SS_IDENTITY;
		txMessage = queryIdentificationMessageToJausMessage(queryId);
		if(!txMessage)
		{
			// ToJausMessage Failed
			queryIdentificationMessageDestroy(queryId);
			return false;
		}

		jausAddressCopy(txMessage->destination, address);
		jausAddressCopy(txMessage->source, cmpt->address);
		this->commMngr->receiveJausMessage(txMessage, this);
		queryIdentificationMessageDestroy(queryId);
		return true;
	}
	return false;
}

bool NodeManagerComponent::sendQueryComponentIdentification(JausAddress address)
{
	QueryIdentificationMessage queryId = NULL;
	JausMessage txMessage = NULL;

	// TODO: Check timeout and request limit
	if(systemTree->hasComponent(address) &&
		!systemTree->hasComponentIdentification(address))
	{
		// Create query message
		queryId = queryIdentificationMessageCreate();
		if(!queryId)
		{
			// Constructor Failed
			return false;
		}
		
		queryId->queryField = JAUS_QUERY_FIELD_COMPONENT_IDENTITY;
		txMessage = queryIdentificationMessageToJausMessage(queryId);
		if(!txMessage)
		{
			// ToJausMessage Failed
			queryIdentificationMessageDestroy(queryId);
			return false;
		}

		jausAddressCopy(txMessage->destination, address);
		jausAddressCopy(txMessage->source, cmpt->address);
		this->commMngr->receiveJausMessage(txMessage, this);
		queryIdentificationMessageDestroy(queryId);
		return true;
	}
	return false;
}

bool NodeManagerComponent::sendQueryComponentServices(JausAddress address)
{
	QueryServicesMessage query = NULL;
	JausMessage txMessage = NULL;

	// TODO: Check timeout and request limit
	if( systemTree->hasComponent(address) &&
		!systemTree->hasComponentServices(address))
	{
		// Create query message
		query = queryServicesMessageCreate();
		if(!query)
		{
			// Constructor Failed
			return false;
		}
		
		txMessage = queryServicesMessageToJausMessage(query);
		if(!txMessage)
		{
			// ToJausMessage Failed
			queryServicesMessageDestroy(query);
			return false;
		}

		jausAddressCopy(txMessage->destination, address);
		jausAddressCopy(txMessage->source, cmpt->address);
		this->commMngr->receiveJausMessage(txMessage, this);
		queryServicesMessageDestroy(query);
		return true;
	}
	return false;
}
bool NodeManagerComponent::sendQueryNodeConfiguration(JausAddress address, bool createEvent)
{
	QueryConfigurationMessage query = NULL;
	JausMessage txMessage = NULL;
	CreateEventMessage createEventMsg = NULL;

	// TODO: Check timeout and request limit
	if( systemTree->hasNode(address) &&
		!systemTree->hasNodeConfiguration(address))
	{
		// Create query message
		query = queryConfigurationMessageCreate();
		if(!query)
		{
			// Constructor Failed
			return false;
		}
		query->queryField = JAUS_NODE_CONFIGURATION;

		txMessage = queryConfigurationMessageToJausMessage(query);
		if(!txMessage)
		{
			// ToJausMessage Failed
			queryConfigurationMessageDestroy(query);
			return false;
		}

		jausAddressCopy(txMessage->destination, address);
		jausAddressCopy(txMessage->source, cmpt->address);
		this->commMngr->receiveJausMessage(txMessage, this);
		
		if(createEvent)
		{
			createEventMsg = createEventMessageCreate();
			if(!createEventMsg)
			{
				queryConfigurationMessageDestroy(query);
				return false;
			}
			
			createEventMsg->messageCode = query->commandCode;
			createEventMsg->eventType = EVENT_EVERY_CHANGE_TYPE;
			createEventMsg->queryMessage = queryConfigurationMessageToJausMessage(query);
			if(!createEventMsg->queryMessage)
			{
				// Problem with queryConfigurationMessageToJausMessage
				createEventMessageDestroy(createEventMsg);
				queryConfigurationMessageDestroy(query);
				return false;
			}
			
			txMessage = createEventMessageToJausMessage(createEventMsg);
			if(!txMessage)
			{
				// Problem with createEventMsgMessageToJausMessage
				createEventMessageDestroy(createEventMsg);
				queryConfigurationMessageDestroy(query);
				return false;
			}

			jausAddressCopy(txMessage->destination, address);
			jausAddressCopy(txMessage->source, cmpt->address);
			this->commMngr->receiveJausMessage(txMessage, this);
		}

		queryConfigurationMessageDestroy(query);
		return true;
	}
	return false;
}

bool NodeManagerComponent::sendQuerySubsystemConfiguration(JausAddress address, bool createEvent)
{
	QueryConfigurationMessage query = NULL;
	JausMessage txMessage = NULL;
	CreateEventMessage createEventMsg = NULL;

	// TODO: Check timeout and request limit
	if( systemTree->hasSubsystem(address) &&
		!systemTree->hasSubsystemConfiguration(address))
	{
		// Create query message
		query = queryConfigurationMessageCreate();
		if(!query)
		{
			// Constructor Failed
			return false;
		}
		query->queryField = JAUS_SUBSYSTEM_CONFIGURATION;

		txMessage = queryConfigurationMessageToJausMessage(query);
		if(!txMessage)
		{
			// ToJausMessage Failed
			queryConfigurationMessageDestroy(query);
			return false;
		}

		jausAddressCopy(txMessage->destination, address);
		jausAddressCopy(txMessage->source, cmpt->address);
		this->commMngr->receiveJausMessage(txMessage, this); // Send Query Node Configuration

		if(createEvent)
		{
			createEventMsg = createEventMessageCreate();
			if(!createEventMsg)
			{
				queryConfigurationMessageDestroy(query);
				return false;
			}
			
			createEventMsg->messageCode = query->commandCode;
			createEventMsg->eventType = EVENT_EVERY_CHANGE_TYPE;
			createEventMsg->queryMessage = queryConfigurationMessageToJausMessage(query);
			if(!createEventMsg->queryMessage)
			{
				// Problem with queryConfigurationMessageToJausMessage
				createEventMessageDestroy(createEventMsg);
				queryConfigurationMessageDestroy(query);
				return false;
			}
			
			txMessage = createEventMessageToJausMessage(createEventMsg);
			if(!txMessage)
			{
				// Problem with createEventMsgMessageToJausMessage
				createEventMessageDestroy(createEventMsg);
				queryConfigurationMessageDestroy(query);
				return false;
			}

			jausAddressCopy(txMessage->destination, address);
			jausAddressCopy(txMessage->source, cmpt->address);
			this->commMngr->receiveJausMessage(txMessage, this);
		}

		queryConfigurationMessageDestroy(query);
		return true;
	}
	return false;
}

int NodeManagerComponent::getNextEventId()
{
	int i;
	for(i = 0; i < MAXIMUM_EVENT_ID; i++)
	{
		if(eventId[i] == false)
		{
			return i;
		}
	}
	return -1;
}

bool NodeManagerComponent::setupJausServices()
{
	JausService service;
	service = jausServiceCreate(0);
	if(!service) return false;
	jausServiceAddInputCommand(service, JAUS_SET_COMPONENT_AUTHORITY, NO_PRESENCE_VECTOR);
	jausServiceAddInputCommand(service, JAUS_SHUTDOWN, NO_PRESENCE_VECTOR);
	jausServiceAddInputCommand(service, JAUS_STANDBY, NO_PRESENCE_VECTOR);
	jausServiceAddInputCommand(service, JAUS_RESUME, NO_PRESENCE_VECTOR);
	jausServiceAddInputCommand(service, JAUS_RESET, NO_PRESENCE_VECTOR);
	jausServiceAddInputCommand(service, JAUS_SET_EMERGENCY, NO_PRESENCE_VECTOR);
	jausServiceAddInputCommand(service, JAUS_CLEAR_EMERGENCY, NO_PRESENCE_VECTOR);
	jausServiceAddInputCommand(service, JAUS_CREATE_SERVICE_CONNECTION, NO_PRESENCE_VECTOR);
	jausServiceAddInputCommand(service, JAUS_ACTIVATE_SERVICE_CONNECTION, NO_PRESENCE_VECTOR);
	jausServiceAddInputCommand(service, JAUS_SUSPEND_SERVICE_CONNECTION, NO_PRESENCE_VECTOR);
	jausServiceAddInputCommand(service, JAUS_TERMINATE_SERVICE_CONNECTION, NO_PRESENCE_VECTOR);
	jausServiceAddInputCommand(service, JAUS_REQUEST_COMPONENT_CONTROL, NO_PRESENCE_VECTOR);
	jausServiceAddInputCommand(service, JAUS_QUERY_COMPONENT_AUTHORITY, NO_PRESENCE_VECTOR);
	jausServiceAddInputCommand(service, JAUS_QUERY_COMPONENT_STATUS, NO_PRESENCE_VECTOR);
	jausServiceAddInputCommand(service, JAUS_QUERY_HEARTBEAT_PULSE, NO_PRESENCE_VECTOR);

	jausServiceAddOutputCommand(service, JAUS_CREATE_SERVICE_CONNECTION, NO_PRESENCE_VECTOR);
	jausServiceAddOutputCommand(service, JAUS_CONFIRM_SERVICE_CONNECTION, NO_PRESENCE_VECTOR);
	jausServiceAddOutputCommand(service, JAUS_TERMINATE_SERVICE_CONNECTION, NO_PRESENCE_VECTOR);
	jausServiceAddOutputCommand(service, JAUS_CONFIRM_COMPONENT_CONTROL, NO_PRESENCE_VECTOR);
	jausServiceAddOutputCommand(service, JAUS_REJECT_COMPONENT_CONTROL, NO_PRESENCE_VECTOR);
	jausServiceAddOutputCommand(service, JAUS_REPORT_COMPONENT_AUTHORITY, NO_PRESENCE_VECTOR);
	jausServiceAddOutputCommand(service, JAUS_REPORT_COMPONENT_STATUS, NO_PRESENCE_VECTOR);
	jausServiceAddOutputCommand(service, JAUS_REPORT_HEARTBEAT_PULSE, NO_PRESENCE_VECTOR);
	jausServiceAddOutputCommand(service, JAUS_QUERY_COMPONENT_STATUS, NO_PRESENCE_VECTOR);

	// Add Core Service
	jausServiceAddService(cmpt->services, service);

	service = jausServiceCreate(JAUS_NODE_MANAGER);
	if(!service) return false;
	jausServiceAddInputCommand(service, JAUS_QUERY_CONFIGURATION, NO_PRESENCE_VECTOR);
	jausServiceAddInputCommand(service, JAUS_QUERY_IDENTIFICATION, NO_PRESENCE_VECTOR);
	jausServiceAddInputCommand(service, JAUS_QUERY_SERVICES, NO_PRESENCE_VECTOR);
	jausServiceAddInputCommand(service, JAUS_REPORT_CONFIGURATION, NO_PRESENCE_VECTOR);
	jausServiceAddInputCommand(service, JAUS_REPORT_IDENTIFICATION, NO_PRESENCE_VECTOR);
	jausServiceAddInputCommand(service, JAUS_REPORT_SERVICES, NO_PRESENCE_VECTOR);
	jausServiceAddInputCommand(service, JAUS_CANCEL_EVENT, NO_PRESENCE_VECTOR);
	jausServiceAddInputCommand(service, JAUS_CONFIRM_EVENT, NO_PRESENCE_VECTOR);
	jausServiceAddInputCommand(service, JAUS_CREATE_EVENT, NO_PRESENCE_VECTOR);

	jausServiceAddOutputCommand(service, JAUS_QUERY_CONFIGURATION, NO_PRESENCE_VECTOR);
	jausServiceAddOutputCommand(service, JAUS_QUERY_IDENTIFICATION, NO_PRESENCE_VECTOR);
	jausServiceAddOutputCommand(service, JAUS_QUERY_SERVICES, NO_PRESENCE_VECTOR);
	jausServiceAddOutputCommand(service, JAUS_REPORT_CONFIGURATION, NO_PRESENCE_VECTOR);
	jausServiceAddOutputCommand(service, JAUS_REPORT_IDENTIFICATION, NO_PRESENCE_VECTOR);
	jausServiceAddOutputCommand(service, JAUS_REPORT_SERVICES, NO_PRESENCE_VECTOR);
	jausServiceAddOutputCommand(service, JAUS_CANCEL_EVENT, NO_PRESENCE_VECTOR);
	jausServiceAddOutputCommand(service, JAUS_CONFIRM_EVENT, NO_PRESENCE_VECTOR);
	jausServiceAddOutputCommand(service, JAUS_CREATE_EVENT, NO_PRESENCE_VECTOR);

	// Add Node Manager Service
	jausServiceAddService(cmpt->services, service);

	return true;
}


