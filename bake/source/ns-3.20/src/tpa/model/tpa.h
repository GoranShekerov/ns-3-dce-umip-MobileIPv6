/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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

#ifndef TPA_H
#define TPA_H

#include "ns3/object.h"
#include "ns3/packet.h"
#include <ns3/applications-module.h>

namespace ns3 {
/**
 * \Traffic Performance Analyzer (tpa) - class for analyzing traffic parametars
 *
 * Aimed to analyze traffic parametars, such as throughput,
 * delay and jitter, used with MIPv6 (ns-3-dce umip) implementation.
 * Primary used with modified OnOff Application, but could be eddited in orded to use it for other kinds of traffic.
 * The idea is to load the sent and received packets in the sinks of the ruuning simulation script
 * in the end communication nodes and to calculate the traffic performances.
 * 
 * Note:
 * The analyzing is limited by the size of the arrays that hold packet information,
 * correspondigly the application packet rate.
 * (Enough for me to simulate 2.5Mbps application UDP traffic with 512byte packets for 150 sec)
 * This static allocation takes about 5 to 10 MB RAM per MN node while simulating (fair enought :-),
 * It coul'd be modified to approximately calculate the number of packets needed and allocate enough space to optimize memory allocation.
 * Dinamic memory allocation arrises a lot of problems for me ;)
 *   
 */
class Tpa : public Object
{

public:
  static TypeId GetTypeId (void);
  Tpa ();
  virtual ~Tpa ();
  void SetTrafficType (std::string stype);
  void LoadSentPacket (Ptr<const Packet> p_loadedPacket, double timeNow);
  void LoadReceivedPacket (Ptr<const Packet> p_loadedPacket, double timeNow);
  void LoadControlPacket (Ptr<const Packet> p_loadedPacket, double timeNow);
  void PrintTrafficPerformances ();
  void PrintThroughput ();
  bool m_enable_column_labels;


private:
  void LoadSentEchoRequestPacket (Ptr<const Packet> p_lerp, double timeNow);
  void LoadReceivedEchoReplyPacket (Ptr<const Packet> p_lerp, double timeNow);
  void LoadSentOnOffPacket (Ptr<const Packet> p_loadedPacket, double timeNow);
  void LoadReceivedOnOffPacket (Ptr<const Packet> p_loadedPacket, double timeNow);
  void LoadSentUdpTracePacket (Ptr<const Packet> p_loadedPacket, double timeNow);
  void LoadReceivedUdpTracePacket (Ptr<const Packet> p_loadedPacket, double timeNow);
  double CalculateThroughput ();
  double CalculatePacketLossPrecentage ();
  double CalculateEndToEndDelayAvg ();
  double CalculateJitterAvg ();
  double CalculateR_Value ();
  double CalculateHandoverTime ();

  struct sentPacketParam
  {
    double   sentTime;
    uint32_t packetID;
  };
  struct receivedPacketParam
  {
    double   receivedTime;
    uint32_t packetID;
    uint32_t packetSize; // in bytes
  };

  sentPacketParam     sentDataArray[100000];
  receivedPacketParam receivedDataArray[100000];
  double delaysTempArray[100000];
  double jitterTempArray[100000];


  enum TrafficType_e{
    PING =    1,
    UDPCBR =  2,
    TCPCBR =  3,
    VOIP =    4,
    VIDEO_S = 5
  };
  
  uint8_t  m_trafficType;
  int      m_sentPacketsNumber;
  int      m_receivedPacketsNumber;
  int      m_next_sentPacketIndex;
  int      m_next_receivedPacketIndex;
  int      m_next_delayIndex;
  int      m_next_jitterIndex;
  int      m_receivedPacketSize;
  int      m_sentPacketSize;
  double   m_startTrafficTime;
  double   m_stopTrafficTime;
  double   m_throughput;
  double   m_packetLossPercentage;
  double   m_endToEndDelayAvg;
  double   m_rValue;
  double   m_Jitter;
  double   m_L3Ths; // L3 handover start time
  double   m_L3Thf; // L3 handover finish time
  double   m_L3Th;
  SeqTsHeader seqTs;
};

} // namespace ns3

#endif /* TPA_H */

