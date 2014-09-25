/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 * Author: Goran Shekerov <g_sekerov@yahoo.com>
 */

#include "tpa.h"
#include <iomanip>  // this is needed for std::setprecision()
#include <ns3/ethernet-header.h>
#include <ns3/wifi-mac-header.h>
#include <ns3/wifi-module.h>
#include <ns3/llc-snap-header.h>
#include <ns3/ipv6-header.h>
#include <ns3/icmpv6-header.h>
#include <ns3/udp-header.h>
#include <iomanip>
#include <math.h>
#include <ns3/ipv6-extension-header.h>
#include <fstream>


namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (Tpa);

TypeId
Tpa::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::Tpa")
    .SetParent<Object> ()
    .AddConstructor<Tpa> ()
    ;
  return tid;
}

Tpa::Tpa ()
{
  m_trafficType = 55;
  m_receivedPacketSize = 0;
  m_sentPacketSize = 0;
  m_enable_column_labels = true;
  m_next_sentPacketIndex = 0;
  m_next_receivedPacketIndex = 0;
  m_next_delayIndex = 0;
  m_next_jitterIndex = 0;
}

Tpa::~Tpa ()
{
}

void
Tpa::SetTrafficType (std::string stype)
{
  if (stype == "PING")     {m_trafficType = PING;}   //std::cout << "Traffic type set for Ping application" << std::endl; 
  if (stype == "UDPCBR")   {m_trafficType = UDPCBR;} //std::cout << "Traffic type set for OnOff application used as CBR" << std::endl;
  if (stype == "TCPCBR")   {m_trafficType = TCPCBR;} //std::cout << "Traffic type set for OnOff application used as CBR" << std::endl;
  if (stype == "VOIP")     {m_trafficType = VOIP;}   //std::cout << "Traffic type set for OnOff application used for VoIP" << std::endl;
  if (stype == "VIDEO_S")    {m_trafficType = VIDEO_S;}  //std::cout << "Traffic type set for UdpTrace application; Video streaming" << std::endl;
  if (m_trafficType == 55) {std::cout << "Traffic type Syntax Error" << std::endl; }
}

void 
Tpa::LoadSentPacket (Ptr<const Packet> p_loadedPacket, double timeNow)
{
  switch (m_trafficType)
  {
    case PING:
      LoadSentEchoRequestPacket (p_loadedPacket, timeNow);
      break;
    case UDPCBR:
      LoadSentOnOffPacket (p_loadedPacket, timeNow);
      break;

    case VOIP:
      LoadSentOnOffPacket (p_loadedPacket, timeNow);
      break;
    case VIDEO_S:
      LoadSentUdpTracePacket (p_loadedPacket, timeNow);
      break;
    default:
      std::cout << "Traffic type in Tpa::LoadSentPacket NOT implemented yet" << std::endl;
  }
}

void
Tpa::LoadReceivedPacket (Ptr<const Packet> p_loadedPacket, double timeNow)
{
  switch (m_trafficType)
  {
    case PING:
      LoadReceivedEchoReplyPacket (p_loadedPacket, timeNow);
      break;
    case UDPCBR:
      LoadReceivedOnOffPacket (p_loadedPacket, timeNow);
      break;

    case VOIP:
      LoadReceivedOnOffPacket (p_loadedPacket, timeNow);
      break;
    case VIDEO_S:
      LoadReceivedUdpTracePacket (p_loadedPacket, timeNow);
      break;
    default:
      std::cout << "Traffic type in Tpa::LoadReceivedPacket NOT implemented yet" << std::endl;
  }
}



void
Tpa::PrintTrafficPerformances ()
{
  //Calculating performances
  m_sentPacketsNumber = m_next_sentPacketIndex;
  m_receivedPacketsNumber = m_next_receivedPacketIndex;
  m_startTrafficTime = receivedDataArray[0].receivedTime;
  m_stopTrafficTime = receivedDataArray[m_next_receivedPacketIndex - 1].receivedTime;
  m_throughput = CalculateThroughput ();
  m_packetLossPercentage = CalculatePacketLossPrecentage ();
  m_endToEndDelayAvg = CalculateEndToEndDelayAvg ();
  m_Jitter = CalculateJitterAvg ();
  m_rValue = CalculateR_Value ();
  m_L3Th = CalculateHandoverTime ();


/*
  for (int i = 0; i < m_sentPacketsNumber; i++) { std::cout <<  "Echo request sent with ID= " << sentDataArray[i].packetID << " time:" << sentDataArray[i].sentTime << std::endl;}
  for (int j = 0; j < m_receivedPacketsNumber; j++) { std::cout <<  "Echo reply received with ID= " << receivedDataArray[j].packetID << " time:" << 
receivedDataArray[j].receivedTime << std::endl;}
int m_elementsNumber_delaysTempArray = delaysTempArray.size ();
  for (int k = 0; k < m_elementsNumber_delaysTempArray; k++) { std::cout << k << "- delay" << delaysTempArray[k] << std::endl;}
*/

if (m_enable_column_labels) // disable when generating results with bash scripts
  {
    //Printing performances
    std::cout << std::endl;
    std::cout << std::left << std::setw(10) << "Th[Kbps]"; //1 - Throughput
    std::cout << std::left << std::setw(8) << "Pl[%]";    //2 - Packet-Loss
    std::cout << std::left << std::setw(8) << "D[ms]";    //3 - Avg-Delay
    std::cout << std::left << std::setw(8) << "J[ms]";    //4 - Avg-Jitter
    std::cout << std::left << std::setw(8) << "H[s]";    //5 - Handover-delay
    std::cout << std::left << std::setw(8) << "R";        //6 - R_Value
    std::cout << std::left << std::setw(8) << "Ns";       //7 - Sent packets
    std::cout << std::left << std::setw(8) << "Nr";       //8 - Received packets
    std::cout << std::left << std::setw(8) << "Nd";       //9 - Dropped
    std::cout << std::left << std::setw(8) << "T[s]";     //10- Application time
    std::cout << std::endl;
  }

  std::cout << std::left << std::setw(10) << std::fixed << std::setprecision(2) << m_throughput; //1
  std::cout << std::left << std::setw(8)  << m_packetLossPercentage;  //2
  std::cout << std::left << std::setw(8)  << m_endToEndDelayAvg;      //3
  std::cout << std::left << std::setw(8)  << m_Jitter;                //4
  std::cout << std::left << std::setw(8)  << m_L3Th;                  //5
  std::cout << std::left << std::setw(8)  << int (m_rValue + 0.5);    //6
  std::cout << std::left << std::setw(8)  << m_sentPacketsNumber;     //7 
  std::cout << std::left << std::setw(8)  << m_receivedPacketsNumber; //8
  std::cout << std::left << std::setw(8)  << m_sentPacketsNumber - m_receivedPacketsNumber;      //9
  std::cout << std::left << std::setw(8)  << int ((m_stopTrafficTime - m_startTrafficTime) / 1000.0 + 0.5);    //10
  std::cout << std::endl; // for bash scripts, the new line is inserted from script
  //std::cout << "\n" << std::endl;
 
  //Output result to file (for parsing)
  std::ofstream r_out("/root/workspace/bake/source/ns-3-dce/tempresults.txt");
  
  r_out << std::fixed << std::setprecision(2) << m_throughput << "*"; //1
  r_out << m_packetLossPercentage << "*";  //2
  r_out << m_endToEndDelayAvg << "*";      //3
  r_out << m_Jitter << "*";                //4
  r_out << m_L3Th << "*";                  //5
  r_out << int (m_rValue + 0.5) << "*";    //6
  r_out << m_sentPacketsNumber << "*";     //7 
  r_out << m_receivedPacketsNumber << "*";//8
  r_out << m_sentPacketsNumber - m_receivedPacketsNumber << "*";      //9
  r_out << int ((m_stopTrafficTime - m_startTrafficTime) / 1000.0 + 0.5);    //10
  //std::cout << std::endl;
}


void 
Tpa::PrintThroughput ()
{
  std::ofstream tout("/root/workspace/bake/source/ns-3-dce/Throughput.txt");
  tout << "#Time_interval    Throughput[Kbps]" << std::endl;

  uint32_t m_throughput; // in bytes
  double next_packet_received_time;

  for (double i = 0; i < (m_stopTrafficTime)/1000; i=i+1 )
    {
      static double j = 1;
      m_throughput = 0;

      do
        {
          static int k = 0;
          m_throughput = m_throughput + receivedDataArray[k].packetSize;
          if (k < m_receivedPacketsNumber) {k = k+1;} else {return;}
          next_packet_received_time = (receivedDataArray[k].receivedTime)/1000; // ms/1000 = sec 
        }while(next_packet_received_time < j);

      tout << int (j) << "        " << (m_throughput * 8)/1024  << std::endl; // in Kbps      
      j = j + 1;
    }
}


void 
Tpa::LoadControlPacket (Ptr<const Packet> p_lcp, double timeNow)  // lcp - loaded control packet
{
  if (timeNow > 16000) // msec
    {    

      uint32_t packetSize = p_lcp -> GetSize();

      if (packetSize == 50) // Association Response packet
        {
          Ptr<Packet> packet = p_lcp->Copy ();
          WifiMacHeader wifimachdr;  packet->PeekHeader(wifimachdr);
          Mac48Address mnmac("00:00:00:00:00:10"); // this is the MAC of the MN in the script, needed when backround traffic is introduced
            if ( wifimachdr.IsAssocResp () and (wifimachdr.GetAddr1() == mnmac))
              {
                m_L3Ths = timeNow;
              } 
        }
 
     if (packetSize == 116)  // BA when MN moves to Foreign Network
        {
          m_L3Thf = timeNow; 
        }

      if (packetSize == 92)  // BA when MN moves to Home Network (RS packet might be 92 bytes of size)
        {
          Ptr<Packet> packet = p_lcp->Copy ();
          WifiMacHeader wifimachdr;  packet->RemoveHeader(wifimachdr);
          LlcSnapHeader llcsnaphdr;  packet->RemoveHeader(llcsnaphdr);
          Ipv6Header ipv6hdr;  packet->PeekHeader (ipv6hdr);
          if (ipv6hdr.GetNextHeader () ==  135)  //  Mobile IPv6 header type 135 
            {
              m_L3Thf = timeNow;
            }
        }
    }   
}

//Private

void
Tpa::LoadSentEchoRequestPacket (Ptr<const Packet> p_lerp, double timeNow) //lerp - loaded echo request packet
{
  Ptr<Packet> packet = p_lerp->Copy (); 
  EthernetHeader ethhdr;  packet->RemoveHeader (ethhdr); 
  Ipv6Header ipv6hdr;  packet->RemoveHeader (ipv6hdr);

  if (ipv6hdr.GetNextHeader () == 43) // Type 2 Routing IPv6 Extension Header in case of Route Optimization 
    {
      //Ipv6ExtensionRoutingHeader ipv6eh;
      Ipv6ExtensionDestinationHeader ipv6eh; // The Ipv6ExtensionRoutingHeader doesnt work, probably a bug?
      packet->RemoveHeader (ipv6eh);
    }

  Icmpv6Header icmp6hdr;  packet->PeekHeader (icmp6hdr);
  if (icmp6hdr.GetType () == Icmpv6Header::ICMPV6_ECHO_REQUEST)
    {
      static sentPacketParam spktPar = {};

      spktPar.sentTime = timeNow;  

      Icmpv6Echo icmp6EchoHdr;  packet->PeekHeader (icmp6EchoHdr);
      spktPar.packetID = icmp6EchoHdr.GetSeq ();
    
      sentDataArray[m_next_sentPacketIndex] = spktPar;
      m_next_sentPacketIndex = m_next_sentPacketIndex + 1;
     }  
}

void
Tpa::LoadReceivedEchoReplyPacket (Ptr<const Packet> lerp, double timeNow)
{  
  // Echo-replys in CN in order to calculate the RTT Dealy
  Ptr<Packet> packet = lerp->Copy ();    
  EthernetHeader ethhdr;  packet->RemoveHeader (ethhdr); 
  Ipv6Header ipv6hdr;  packet->RemoveHeader (ipv6hdr);

  if (ipv6hdr.GetNextHeader () == 60) // Destination Option IPv6 Extension Header in case of Route Optimization
    {
      Ipv6ExtensionDestinationHeader ipv6doeh;
      packet->RemoveHeader (ipv6doeh);  
    }

  Icmpv6Header icmp6hdr;  packet->PeekHeader (icmp6hdr);
  if (icmp6hdr.GetType () == Icmpv6Header::ICMPV6_ECHO_REPLY)
    {       
      static receivedPacketParam rpktPar = {};

      rpktPar.receivedTime = timeNow;  

      Icmpv6Echo icmp6EchoHdr;  packet->PeekHeader (icmp6EchoHdr);
      rpktPar.packetID = icmp6EchoHdr.GetSeq ();
    
      receivedDataArray[m_next_receivedPacketIndex] = rpktPar;
      m_next_receivedPacketIndex = m_next_receivedPacketIndex + 1;
      //std::cout << "" << std::endl; 
    } 
}

void
Tpa::LoadSentOnOffPacket (Ptr<const Packet> p_lofp, double timeNow)
{
  if(p_lofp->GetSize() > 200)
  {  // filter OnOff packets from the rest (80 IP6-IP6 + 8 UDP + 172 VoIP payload = 260 bytes)
     // all other controll packets are less than 200 bytes 
    Ptr<Packet> packet = p_lofp->Copy ();   
    EthernetHeader ethhdr;  packet->RemoveHeader (ethhdr); 
    Ipv6Header ipv6hdr;     packet->RemoveHeader (ipv6hdr);
/* Waiting for dce 1.3
    if (ipv6hdr.GetNextHeader () == 43) // Type 2 Routing IPv6 Extension Header in case of Route Optimization 
      {
        //Ipv6ExtensionRoutingHeader ipv6eh;
        Ipv6ExtensionDestinationHeader ipv6eh; // The Ipv6ExtensionRoutingHeader doesnt work, probably a bug?
        packet->RemoveHeader (ipv6eh);
      }
*/
    UdpHeader udphdr;       packet->RemoveHeader(udphdr);
                            packet->PeekHeader(seqTs);

    static sentPacketParam spktPar = {};
    spktPar.sentTime = timeNow;  
    spktPar.packetID = seqTs.GetSeq();
    sentDataArray[m_next_sentPacketIndex] = spktPar;
    m_next_sentPacketIndex = m_next_sentPacketIndex + 1;
  } 
}

void
Tpa::LoadReceivedOnOffPacket (Ptr<const Packet> p_lofp, double timeNow) // lofp - load on-off packet
{
  if(p_lofp->GetSize() > 200)
  {  //filter OnOff packets from the rest (80 IP6-IP6 + 8 UDP + 172 VoIP payload = 260 bytes)

    Ptr<Packet> packet = p_lofp->Copy ();
    static receivedPacketParam rpktPar = {};
    rpktPar.packetSize = packet->GetSize () + 32;  // the Wifi header (802.11 data + LLC) = 32 bytes are removed before the sink
   
    Ipv6Header ipv6hdr1;  packet->RemoveHeader(ipv6hdr1);

/* Waiting for dce 1.3
    if (ipv6hdr1.GetNextHeader () == 43) // Type 2 Routing IPv6 Extension Header in case of Route Optimization 
      {
        //Ipv6ExtensionRoutingHeader ipv6eh;
        Ipv6ExtensionDestinationHeader ipv6eh; // The Ipv6ExtensionRoutingHeader doesnt work, probably a bug?
        packet->RemoveHeader (ipv6eh);
      }
*/
    if (ipv6hdr1.GetNextHeader () ==  41) // IPv6 type = 41
      {
        Ipv6Header ipv6hdr2;  packet->RemoveHeader(ipv6hdr2);
      }


    UdpHeader udphdr;     packet->RemoveHeader(udphdr); 
                          packet->PeekHeader(seqTs);   

    rpktPar.receivedTime = timeNow;  
    rpktPar.packetID = seqTs.GetSeq();
    receivedDataArray[m_next_receivedPacketIndex] = rpktPar;
    m_next_receivedPacketIndex = m_next_receivedPacketIndex + 1;

  } 
}


void
Tpa::LoadSentUdpTracePacket (Ptr<const Packet> p_lp, double timeNow)
{

    Ptr<Packet> packet = p_lp->Copy ();   
    EthernetHeader ethhdr;  packet->RemoveHeader (ethhdr); 
    Ipv6Header ipv6hdr;     packet->RemoveHeader (ipv6hdr);
    if (ipv6hdr.GetNextHeader () ==  17) // UDP
      {    
        UdpHeader udphdr; packet->RemoveHeader(udphdr);
        packet->PeekHeader(seqTs);

        static sentPacketParam spktPar = {};
        spktPar.sentTime = timeNow;  
        spktPar.packetID = seqTs.GetSeq();
        sentDataArray[m_next_sentPacketIndex] = spktPar;
        m_next_sentPacketIndex = m_next_sentPacketIndex + 1;
      } 
}


void
Tpa::LoadReceivedUdpTracePacket (Ptr<const Packet> p_lp, double timeNow) 
{
    Ptr<Packet> packet = p_lp->Copy ();
    static receivedPacketParam rpktPar = {};
    rpktPar.packetSize = packet->GetSize () + 32;  // the Wifi header (802.11 data + LLC) = 32 bytes are removed before the sink
   
    Ipv6Header ipv6hdr1;  packet->RemoveHeader(ipv6hdr1);
    uint8_t m_next_header = ipv6hdr1.GetNextHeader ();
    if (m_next_header ==  41) // IPv6 in IPv6
      {
        Ipv6Header ipv6hdr2;  packet->RemoveHeader(ipv6hdr2);
        m_next_header = ipv6hdr2.GetNextHeader ();
      }
    if (m_next_header ==  17) // UDP
      {
        UdpHeader udphdr; packet->RemoveHeader(udphdr); 
        packet->PeekHeader(seqTs);   
    
        rpktPar.receivedTime = timeNow;  
        rpktPar.packetID = seqTs.GetSeq();
        receivedDataArray[m_next_receivedPacketIndex] = rpktPar;
        m_next_receivedPacketIndex = m_next_receivedPacketIndex + 1;

      } 
}

//************************
//************Calculating
double 
Tpa::CalculateThroughput ()
{
  uint32_t temp_received_troughput = 0;
  for (int i=0; i < m_receivedPacketsNumber; i++)
    {
      temp_received_troughput = temp_received_troughput + receivedDataArray[i].packetSize; // in bytes
    }

  return (temp_received_troughput * 8 / 1024) / ((m_stopTrafficTime - m_startTrafficTime) / 1000); // [Kbps]

  // return  m_receivedPacketsNumber * (m_receivedPacketSize * 8 / 1024 ) / ((m_stopTrafficTime - m_startTrafficTime) / 1000);
  // bytes*8bits/1024=[Kbps] (1000=k; 1024=K)
  // (m_stopTrafficTime - m_startTrafficTime)[ms]/1000 = [s]
}

double 
Tpa::CalculatePacketLossPrecentage ()
{
  return (m_sentPacketsNumber - m_receivedPacketsNumber) / double (m_sentPacketsNumber) * 100;
}

double
Tpa::CalculateEndToEndDelayAvg ()
{
  // End-to-End delay [ms] calculation -- Everything here is in Milli seconds [ms]
  double m_idelay = 0.0;
  for (int i = 0; i < m_sentPacketsNumber; i++)
    {
      for (int j=0; j < m_receivedPacketsNumber; j++)
        {
          if (receivedDataArray[j].packetID == sentDataArray[i].packetID)
            { 
              m_idelay = receivedDataArray[j].receivedTime - sentDataArray[i].sentTime;
              delaysTempArray[m_next_delayIndex] = m_idelay;
              m_next_delayIndex = m_next_delayIndex + 1;
              break;
            }
        }    

    }
  int m_elementsNumber_delaysTempArray = m_next_delayIndex;
  if (m_receivedPacketsNumber != m_elementsNumber_delaysTempArray) 
    {
      std::cout << "m_receivedPacketsNumber"  << m_receivedPacketsNumber << std::endl;
    } 
  double m_delaySum = 0;
  for (int k=0; k < m_elementsNumber_delaysTempArray; k++)
    {
      m_delaySum = m_delaySum + delaysTempArray[k];
    }

  return m_delaySum / m_elementsNumber_delaysTempArray;
}

double 
Tpa::CalculateJitterAvg ()
{
  double m_iJitter = 0.0;
  int m_elementsNumber_delaysTempArray = m_next_delayIndex;
  // the jitter between second and first packet delay is not included 
  for (int i = 0; i < m_elementsNumber_delaysTempArray - 2; i++) 
    {
      m_iJitter = abs(delaysTempArray[i + 1] - delaysTempArray[i]);
      jitterTempArray[m_next_jitterIndex] = m_iJitter;
      m_next_jitterIndex = m_next_jitterIndex + 1;
    }

  int m_elementsNumber_jitterTempArray = m_next_jitterIndex;
  if (m_elementsNumber_jitterTempArray != m_elementsNumber_delaysTempArray - 2)
    {
      std::cout << "Error - mismatch calculating! Tpa::CalculateJitterAvg" << std::endl;
    } 
  double m_jitterSum = 0;
  for (int j=0; j < m_elementsNumber_jitterTempArray; j++)
    {
      m_jitterSum = m_jitterSum + jitterTempArray[j];
    }

  return m_jitterSum / m_elementsNumber_jitterTempArray;
}

double 
Tpa::CalculateR_Value () // Simple analysis of the impact of packet loss and delay on voice transmission quality
{
  if (m_endToEndDelayAvg / 2 < 165)
    {
      return 92.68 - 22 * log(1 + 0.22 * m_packetLossPercentage);
    }
  else 
    {
      return 92.68 - (0.1 * m_endToEndDelayAvg / 2 - 15.9) - 22 * log(1 + 0.22 * m_packetLossPercentage);
      std::cout << "Some error, should not enter this code" << std::endl;
    }
}

double 
Tpa::CalculateHandoverTime ()
{
  return (m_L3Thf - m_L3Ths)/1000; // /1000 => in seconds
}


} // namespace ns3
