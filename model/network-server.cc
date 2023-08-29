/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2018 University of Padova
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Authors: Davide Magrin <magrinda@dei.unipd.it>
 *          Martina Capuzzo <capuzzom@dei.unipd.it>
 */

#include "ns3/network-server.h"
#include "ns3/net-device.h"
#include "ns3/point-to-point-net-device.h"
#include "ns3/packet.h"
#include "ns3/lorawan-mac-header.h"
#include "ns3/lora-frame-header.h"
#include "ns3/lora-device-address.h"
#include "ns3/network-status.h"
#include "ns3/lora-frame-header.h"
#include "ns3/node-container.h"
#include "ns3/class-a-end-device-lorawan-mac.h"
#include "ns3/mac-command.h"

namespace ns3 {
namespace lorawan {

NS_LOG_COMPONENT_DEFINE ("NetworkServer");

NS_OBJECT_ENSURE_REGISTERED (NetworkServer);

TypeId
NetworkServer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::NetworkServer")
    .SetParent<Application> ()
    .AddConstructor<NetworkServer> ()
    .AddTraceSource ("ReceivedPacket",
                     "Trace source that is fired when a packet arrives at the Network Server",
                     MakeTraceSourceAccessor (&NetworkServer::m_receivedPacket),
                     "ns3::Packet::TracedCallback")
    .SetGroupName ("lorawan");
  return tid;
}

NetworkServer::NetworkServer () :
  m_status (Create<NetworkStatus> ()),
  m_controller (Create<NetworkController> (m_status)),
  m_scheduler (Create<NetworkScheduler> (m_status, m_controller))
{
  NS_LOG_FUNCTION_NOARGS ();
}

NetworkServer::~NetworkServer ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

void
NetworkServer::StartApplication (void)
{
  NS_LOG_FUNCTION_NOARGS ();
}

void
NetworkServer::StopApplication (void)
{
  NS_LOG_FUNCTION_NOARGS ();
}

void
NetworkServer::AddGateway (Ptr<Node> gateway, Ptr<NetDevice> netDevice)
{
  NS_LOG_FUNCTION (this << gateway);

  // Get the PointToPointNetDevice
  Ptr<PointToPointNetDevice> p2pNetDevice;
  NS_LOG_INFO("gwIter: " << gwIter);
  gateways[gwIter] = gateway;
  // gateways.push_back(gateway);

  // // Get the mobility model associated with the device
  // Ptr<MobilityModel> mobilityModel = gateway->GetObject<MobilityModel>();

  // // Check if the mobility model is not null before using it
  // if (mobilityModel) {
  //   // Use the mobility model (e.g., get position, velocity, etc.)
  //   Vector position = mobilityModel->GetPosition();
  //   std::cout << "Device Position: (" << position.x << ", " << position.y << ", " << position.z << ")" << std::endl;
  // }
  
  gwIter += 1;
  
  for (uint32_t i = 0; i < gateway->GetNDevices (); i++)
    {
      p2pNetDevice = gateway->GetDevice (i)->GetObject<PointToPointNetDevice> ();
      if (p2pNetDevice != 0)
        {
          // We found a p2pNetDevice on the gateway
          break;
        }
    }

  // Get the gateway's LoRa MAC layer (assumes gateway's MAC is configured as first device)
  Ptr<GatewayLorawanMac> gwMac = gateway->GetDevice (0)->GetObject<LoraNetDevice> ()->
    GetMac ()->GetObject<GatewayLorawanMac> ();
  NS_ASSERT (gwMac != 0);
  NS_LOG_INFO("AddGateway: GatewayLorawanMac:  " << gwMac);

  // Get the Address
  Address gatewayAddress = p2pNetDevice->GetAddress ();

  // Address gatewayAddress = Mac48Address("11:00:01:02:03:02");
  // p2pNetDevice->SetAddress (gatewayAddress);

  // Create new gatewayStatus
  Ptr<GatewayStatus> gwStatus = Create<GatewayStatus> (gatewayAddress,
                                                       netDevice,
                                                       gwMac);

  m_status->AddGateway (gatewayAddress, gwStatus);
}

void
NetworkServer::AddNodes (NodeContainer nodes)
{
  NS_LOG_FUNCTION_NOARGS ();

  // For each node in the container, call the function to add that single node
  NodeContainer::Iterator it;
  for (it = nodes.Begin (); it != nodes.End (); it++)
    {
      AddNode (*it);
    }
}

void
NetworkServer::AddNode (Ptr<Node> node)
{
  NS_LOG_FUNCTION (this << node);

  // Get the LoraNetDevice
  Ptr<LoraNetDevice> loraNetDevice;
  for (uint32_t i = 0; i < node->GetNDevices (); i++)
    {
      loraNetDevice = node->GetDevice (i)->GetObject<LoraNetDevice> ();
      if (loraNetDevice != 0)
        {
          // We found a LoraNetDevice on the node
          break;
        }
    }

  // Get the MAC
  Ptr<ClassAEndDeviceLorawanMac> edLorawanMac =
    loraNetDevice->GetMac ()->GetObject<ClassAEndDeviceLorawanMac> ();

  // Update the NetworkStatus about the existence of this node
  m_status->AddNode (edLorawanMac);
}

bool
NetworkServer::Receive (Ptr<NetDevice> device, Ptr<const Packet> packet,
                        uint16_t protocol, const Address& address)
{
  NS_LOG_FUNCTION (this << packet << protocol << address);

  // Create a copy of the packet
  Ptr<Packet> myPacket = packet->Copy ();

  // Fire the trace source
  m_receivedPacket (packet);

  // for (int i = 0; i < 10; i++)
  // {
  //   // NS_LOG_INFO ("Receive: iteration: " << i);
  //   // NS_LOG_INFO ("Receive: length: " << sizeof(locations)/sizeof(locations[0]));
  //   // NS_LOG_INFO ("Receive: locations00: " << locations[0][0]);
  //   // NS_LOG_INFO ("Receive: locations01: " << locations[0][1]);
  //   // NS_LOG_INFO ("Receive: locations02: " << locations[0][2]);
  //   // NS_LOG_INFO ("Receive: length: " << sizeof(gateways)/sizeof(gateways[0]));
  //   // Get the mobility model associated with the device
  //   Ptr<MobilityModel> mobilityModel = gateways[i]->GetObject<MobilityModel>();

  //   // Check if the mobility model is not null before using it
  //   if (mobilityModel) {
  //       // Use the mobility model (e.g., get position, velocity, etc.)
  //       Vector position = mobilityModel->GetPosition();
  //       std::cout << "Receive: Device Position: (" << position.x << ", " << position.y << ", " << position.z << ")" << std::endl;
  //   }
  // }

  // Inform the scheduler of the newly arrived packet
  // m_scheduler->OnReceivedPacket (packet);
  m_scheduler->OnReceivedPacket2 (packet, gateways);

  // Inform the status of the newly arrived packet
  m_status->OnReceivedPacket (packet, address);

  // Inform the controller of the newly arrived packet
  m_controller->OnNewPacket (packet);

  return true;
}

void
NetworkServer::AddComponent (Ptr<NetworkControllerComponent> component)
{
  NS_LOG_FUNCTION (this << component);

  m_controller->Install (component);
}

Ptr<NetworkStatus>
NetworkServer::GetNetworkStatus (void)
{
  return m_status;
}

}
}
