#include "network-scheduler.h"
#include <cmath>
#include <string>

namespace ns3 {
namespace lorawan {

NS_LOG_COMPONENT_DEFINE ("NetworkScheduler");

NS_OBJECT_ENSURE_REGISTERED (NetworkScheduler);

TypeId
NetworkScheduler::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::NetworkScheduler")
    .SetParent<Object> ()
    .AddConstructor<NetworkScheduler> ()
    .AddTraceSource ("ReceiveWindowOpened",
                     "Trace source that is fired when a receive window opportunity happens.",
                     MakeTraceSourceAccessor (&NetworkScheduler::m_receiveWindowOpened),
                     "ns3::Packet::TracedCallback")
    .SetGroupName ("lorawan");
  return tid;
}

NetworkScheduler::NetworkScheduler ()
{
}

NetworkScheduler::NetworkScheduler (Ptr<NetworkStatus> status,
                                    Ptr<NetworkController> controller) :
  m_status (status),
  m_controller (controller)
{
}

NetworkScheduler::~NetworkScheduler ()
{
}

void
NetworkScheduler::OnReceivedPacket (Ptr<const Packet> packet)
{
  NS_LOG_FUNCTION (packet);

  // Get the current packet's frame counter
  Ptr<Packet> packetCopy = packet->Copy ();
  LorawanMacHeader receivedMacHdr;
  packetCopy->RemoveHeader (receivedMacHdr);
  LoraFrameHeader receivedFrameHdr;
  receivedFrameHdr.SetAsUplink ();
  packetCopy->RemoveHeader (receivedFrameHdr);

  // Need to decide whether to schedule a receive window
  if (!m_status->GetEndDeviceStatus (packet)->HasReceiveWindowOpportunityScheduled ())
  {
    // Extract the address
    LoraDeviceAddress deviceAddress = receivedFrameHdr.GetAddress ();

    // Schedule OnReceiveWindowOpportunity event
    m_status->GetEndDeviceStatus (packet)->SetReceiveWindowOpportunity (
      Simulator::Schedule (Seconds (1),
                           &NetworkScheduler::OnReceiveWindowOpportunity,
                           this,
                           deviceAddress,
                           1)); // This will be the first receive window
  }
}


void
NetworkScheduler::OnReceivedPacket2 (Ptr<const Packet> packet, Ptr<Node> gateways [11])
{
  NS_LOG_FUNCTION (packet);
  // NS_LOG_INFO ("Size of gateways: " << sizeof(gateways));
  // for (int i = 0; i < 10; i++)
  // {
  //   // NS_LOG_INFO ("OnReceivedPacket2: iteration: " << i);
  //   // NS_LOG_INFO ("OnReceivedPacket2: length: " << sizeof(gateways)/sizeof(gateways[0]));
  //   // Get the mobility model associated with the device
  //   Ptr<MobilityModel> mobilityModel = gateways[i]->GetObject<MobilityModel>();

  //   // Check if the mobility model is not null before using it
  //   if (mobilityModel) {
  //       // Use the mobility model (e.g., get position, velocity, etc.)
  //       Vector position = mobilityModel->GetPosition();
  //       std::cout << "OnReceivedPacket2: Device Position: (" << position.x << ", " << position.y << ", " << position.z << ")" << std::endl;
  //   }
  // }

  // Get the current packet's frame counter
  Ptr<Packet> packetCopy = packet->Copy ();
  LorawanMacHeader receivedMacHdr;
  packetCopy->RemoveHeader (receivedMacHdr);
  LoraFrameHeader receivedFrameHdr;
  receivedFrameHdr.SetAsUplink ();
  packetCopy->RemoveHeader (receivedFrameHdr);

  // Need to decide whether to schedule a receive window
  if (!m_status->GetEndDeviceStatus (packet)->HasReceiveWindowOpportunityScheduled ())
  {
    // Extract the address
    LoraDeviceAddress deviceAddress = receivedFrameHdr.GetAddress ();

    // Schedule OnReceiveWindowOpportunity event
    m_status->GetEndDeviceStatus (packet)->SetReceiveWindowOpportunity (
      Simulator::Schedule (Seconds (1),
                           &NetworkScheduler::OnReceiveWindowOpportunity2,
                           this,
                           deviceAddress,
                           1,
                           gateways)); // This will be the first receive window
  }
}

void
NetworkScheduler::OnReceiveWindowOpportunity (LoraDeviceAddress deviceAddress, int window)
{
  NS_LOG_FUNCTION (deviceAddress);

  NS_LOG_DEBUG ("Opening receive window number " << window << " for device "
                                                 << deviceAddress);

  // Check whether we can send a reply to the device, again by using
  // NetworkStatus
  Address gwAddress = m_status->GetBestGatewayForDevice (deviceAddress, window);

  if (gwAddress == Address () && window == 1)
    {
      NS_LOG_DEBUG ("No suitable gateway found for first window.");

      // No suitable GW was found, but there's still hope to find one for the
      // second window.
      // Schedule another OnReceiveWindowOpportunity event
      m_status->GetEndDeviceStatus (deviceAddress)->SetReceiveWindowOpportunity (
        Simulator::Schedule (Seconds (1),
                             &NetworkScheduler::OnReceiveWindowOpportunity,
                             this,
                             deviceAddress,
                             2));     // This will be the second receive window
    }
  else if (gwAddress == Address () && window == 2)
    {
      // No suitable GW was found and this was our last opportunity
      // Simply give up.
      NS_LOG_DEBUG ("Giving up on reply: no suitable gateway was found " <<
                   "on the second receive window");

      // Reset the reply
      // XXX Should we reset it here or keep it for the next opportunity?
      m_status->GetEndDeviceStatus (deviceAddress)->RemoveReceiveWindowOpportunity();
      m_status->GetEndDeviceStatus (deviceAddress)->InitializeReply ();
    }
  else
    {
      // A gateway was found

      NS_LOG_DEBUG ("Found available gateway with address: " << gwAddress);

      m_controller->BeforeSendingReply (m_status->GetEndDeviceStatus
                                          (deviceAddress));

      // Check whether this device needs a response by querying m_status
      bool needsReply = m_status->NeedsReply (deviceAddress);

      if (needsReply)
        {
          NS_LOG_INFO ("A reply is needed");

          // Send the reply through that gateway
          m_status->SendThroughGateway (m_status->GetReplyForDevice
                                          (deviceAddress, window),
                                        gwAddress);

          // Reset the reply
          m_status->GetEndDeviceStatus (deviceAddress)->RemoveReceiveWindowOpportunity();
          m_status->GetEndDeviceStatus (deviceAddress)->InitializeReply ();
        }
    }
}

void
NetworkScheduler::OnReceiveWindowOpportunity2 (LoraDeviceAddress deviceAddress, int window, Ptr<Node> gateways [11])
{
  NS_LOG_FUNCTION (deviceAddress);

  NS_LOG_DEBUG ("Opening receive window number " << window << " for device "
                                                 << deviceAddress);

  // Check whether we can send a reply to the device, again by using
  // NetworkStatus
  Address* gwAddress2 = m_status->GetBestGatewayForDevice2 (deviceAddress, window);
  // std::cout << "OnReceiveWindowOpportunity2: Best GW addresses pointer: " << gwAddress2 << std::endl;
  for (int i = 0; i < 3; ++i) {
    std::cout << "--i--: " << gwAddress2[i] << std::endl;
  }

  int counter = 0;
  int random;
  srand(time(0));

  while (counter < 10) { // if the address is not valid (8 digit 00-00-00) keep trying untill we get a valid mac address (of 23 digits)
    random = rand() % 3;
    std::stringstream ss;
    ss << gwAddress2[random];
    std::cout << "OnReceiveWindowOpportunity2: Selected random: " << random << " and Best GW address: " << ss.str() << std::endl;
    std::cout << "--i--: " << ss.str().size() << std::endl;
    if (ss.str().size() == 23) { // check if its a valid MAC address
      break;
    }
    counter ++;
  }


  // IN BELOW LINES I GOT GATEWAYS IN THE PARAMETER AND CALCULATED DISTANCE OF EACH THEM FROM THE EDGE DEVICE... the purpose was to then sort the distances and select 2nd and 3rd best distance gateway. and then to choose between these 2 and the one we get from GetBestGatewayForDevice() method... but I instead changed GetBestGatewayForDevice() method to give me other 2 best gateways

  // double distances[10];
  // for (int i = 0; i < 10; i++)
  // {
  //   // Ptr<NetDevice> netDevice = gateways[i]->GetDevice(0);

  //   // Ptr<LoraNetDevice> nodeDevice = netDevice->GetObject<LoraNetDevice> ();
  //   // // Check if the LoraNetDevice is available
  //   // if (nodeDevice) {
  //   //   Address currentGwAddress = nodeDevice->GetAddress();
  //   //   NS_LOG_INFO ("Mac address of LoraNetDevice: " << currentGwAddress);

  //   //   if (currentGwAddress == gwAddress1) {
  //   //     NS_LOG_INFO ("Addresses are equal: ");
  //   //   } else {
  //   //     NS_LOG_INFO ("Addresses are not equal: ");
  //   //   }
  //   // }

  //   Ptr<MobilityModel> mobilityModel = gateways[i]->GetObject<MobilityModel>();

  //   // Check if the mobility model is not null before using it
  //   if (mobilityModel) {
  //     // Use the mobility model (e.g., get position, velocity, etc.)
  //     Vector position = mobilityModel->GetPosition();
      
  //     distances[i] = sqrt(pow(position.x - 10000, 2) + pow(position.y - 0, 2) + pow(position.z - 0, 2));

  //     std::cout << "OnReceiveWindowOpportunity2: Device Position: (" << position.x << ", " << position.y << ", " << position.z << ") and distance from ED is: " << distances[i] << std::endl;
  //   }
  // }

  Address gwAddress = gwAddress2[random];
  // Address gwAddress = gwAddress2[0];

  if (gwAddress == Address () && window == 1)
    {
      NS_LOG_DEBUG ("No suitable gateway found for first window.");

      // No suitable GW was found, but there's still hope to find one for the
      // second window.
      // Schedule another OnReceiveWindowOpportunity event
      m_status->GetEndDeviceStatus (deviceAddress)->SetReceiveWindowOpportunity (
        Simulator::Schedule (Seconds (1),
                             &NetworkScheduler::OnReceiveWindowOpportunity,
                             this,
                             deviceAddress,
                             2));     // This will be the second receive window
    }
  else if (gwAddress == Address () && window == 2)
    {
      // No suitable GW was found and this was our last opportunity
      // Simply give up.
      NS_LOG_DEBUG ("Giving up on reply: no suitable gateway was found " <<
                   "on the second receive window");

      // Reset the reply
      // XXX Should we reset it here or keep it for the next opportunity?
      m_status->GetEndDeviceStatus (deviceAddress)->RemoveReceiveWindowOpportunity();
      m_status->GetEndDeviceStatus (deviceAddress)->InitializeReply ();
    }
  else if (random != 0)
  // else
    {
      // A gateway was found

      NS_LOG_DEBUG ("Found available gateway with address: " << gwAddress);

      m_controller->BeforeSendingReply (m_status->GetEndDeviceStatus
                                          (deviceAddress));

      // Check whether this device needs a response by querying m_status
      bool needsReply = m_status->NeedsReply (deviceAddress);

      if (needsReply)
        {
          NS_LOG_INFO ("A reply is needed");

          // Send the reply through that gateway
          m_status->SendThroughGateway (m_status->GetReplyForDevice
                                          (deviceAddress, window),
                                        gwAddress);
          // m_status->SendThroughGateway2 (m_status->GetReplyForDevice
                                        //   (deviceAddress, window),
                                        // gwAddress);

          // Reset the reply
          m_status->GetEndDeviceStatus (deviceAddress)->RemoveReceiveWindowOpportunity();
          m_status->GetEndDeviceStatus (deviceAddress)->InitializeReply ();
        }
    }
}
}
}
