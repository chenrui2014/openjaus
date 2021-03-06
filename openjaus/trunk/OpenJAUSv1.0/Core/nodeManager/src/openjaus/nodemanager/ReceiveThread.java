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
// File Name: ReceiveThread.java
//
// Written By: Tom Galluzzo (galluzzo AT gmail DOT com)
//
// Version: 3.2
//
// Date: 08/04/06
//
// Description: This thread is responsible for reading packets off of a UDP socket and placing them on a Queue

package openjaus.nodemanager;
import java.net.*;
import java.util.Enumeration;
import org.apache.log4j.Logger;

import openjaus.libjaus.message.*;

public class ReceiveThread extends Thread
{
	/** Logger that knows our class name */
	static private final Logger log = Logger.getLogger(ReceiveThread.class);

	//static private final Counter counter = new Counter("OJReceiveThread");
	
	static long receiveCount = 0;
    DatagramSocket socket;
	InetAddress socketIpAddress;
	int socketPort;
    Queue queue;
	DataRepository dataRepository;
	
	public ReceiveThread(String name, DatagramSocket socket, Queue queue)
	{
		this.setName(name);
		this.socket = socket;
		this.queue = queue;
		this.socketIpAddress = null;
		this.socketPort = 0;
		dataRepository = NodeManager.getDataRepository();
		dataRepository.put("Messages Received Count", new Long(receiveCount));
		
		try
		{
		    socket.setSoTimeout(1000);
		}
		catch(Exception e)
		{
			dataRepository.put("ReceiveThread Exception", e);
			log.warn("could not set socket timeout",e);
		}
	}
	
	public void run()
	{
		try
		{
			log.info(this.getName()+"("+queue.getName()+") running");
		    // Initialize the datagram packet with a pre-sized array (Packets with more data will be truncated)
			int maxJausRecvSize = JausMessage.MAX_DATA_SIZE; 
			maxJausRecvSize += JausMessage.HEADER_SIZE_BYTES;
			maxJausRecvSize += JausMessage.UDP_HEADER_SIZE_BYTES;

			if(socket instanceof MulticastSocket)
			{
				//System.out.println("Inet: "+((MulticastSocket)socket).getNetworkInterface()+":"+socket.getLocalPort());
				
				Enumeration e = ((MulticastSocket)socket).getNetworkInterface().getInetAddresses();
				if(e.hasMoreElements())
				{
					socketIpAddress = (InetAddress) e.nextElement();
					socketPort = socket.getLocalPort();
				}
			}
			
			while(NodeManager.isRunning())
			{
				DatagramPacket packet = new DatagramPacket(new byte[maxJausRecvSize], maxJausRecvSize);
				try
				{
					socket.receive(packet);
				}
				catch(SocketTimeoutException e)
				{
					continue;
				}
				dataRepository.put("Messages Received Count", new Long(++receiveCount));
				
		//		System.out.println(this.getName()+" ("+queue.getName()+") packet from: " + packet.getAddress()+":"+packet.getPort()+" to: "+socketIpAddress);

				// If the packet is from another socket then push it on the node Receive Queue
				// Otherwise ignore it because Datagram and Multicast packets are not allowed to loop
				// on this socket. This prevents infinite message loops in the NodeManager
					
				if(socket instanceof MulticastSocket)
				{
					if(!packet.getAddress().equals(socketIpAddress))
					{
						queue.push(packet);
					}
					else if(packet.getPort() != socketPort)
					{
						queue.push(packet);
					}
				}
				else
				{
					if(!packet.getAddress().equals(NodeManager.getNodeSideIpAddress()))
					{
						queue.push(packet);
					}
					else if(packet.getPort() != NodeManager.getNodePort())
					{
						queue.push(packet);
					}
				}
			}
		    
			log.warn("ReceiveThread ("+queue.getName()+"): Shutting down");			
		}
		catch (Exception e)
		{
			dataRepository.put("ReceiveThread Exception", e);
			log.error("Exception in ReceiveThread", e);
			System.out.println(e);
		}
	}
	
	public long getReceiveCount()
	{
		return receiveCount;
	}
	
}
