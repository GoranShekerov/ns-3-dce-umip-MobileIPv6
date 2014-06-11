/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010 Hajime Tazaki
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
 * Author: Hajime Tazaki <tazaki@sfc.wide.ad.jp>
 */
#ifndef MIP6D_HELPER_H
#define MIP6D_HELPER_H

#include "ns3/node-container.h"
#include "ns3/application-container.h"
#include "ns3/object-factory.h"
#include "ns3/boolean.h"
#include "ns3/ipv4-interface-container.h"
#include "ns3/address-utils.h"

namespace ns3 {

/**
 * \brief create a umip (mip6d) daemon as an application and associate it to a node
 *
 */
class Mip6dHelper
{
public:
  /**
   * Create a Mip6dHelper which is used to make life easier for people wanting
   * to use mip6d Applications.
   *
   */
  Mip6dHelper ();

  /**
   * Install a mip6d application on each Node in the provided NodeContainer.
   *
   * \param nodes The NodeContainer containing all of the nodes to get a mip6d
   *              application via ProcessManager.
   *
   * \returns A list of mip6d applications, one for each input node
   */
  ApplicationContainer Install (NodeContainer nodes);

  /**
   * Install a mip6d application on the provided Node.  The Node is specified
   * directly by a Ptr<Node>
   *
   * \param node The node to install the Application on.
   *
   * \returns An ApplicationContainer holding the mip6d application created.
   */
  ApplicationContainer Install (Ptr<Node> node);

  /**
   * Install a mip6d application on the provided Node.  The Node is specified
   * by a string that must have previosly been associated with a Node using the
   * Object Name Service.
   *
   * \param nodeName The node to install the ProcessApplication on.
   *
   * \returns An ApplicationContainer holding the mip6d application created.
   */
  ApplicationContainer Install (std::string nodeName);

  /**
   * \brief Configure ping applications attribute
   * \param name   attribute's name
   * \param value  attribute's value
   */
  void SetAttribute (std::string name, const AttributeValue &value);

  // For HA
  void EnableHA (NodeContainer nodes);
  void AddHaServedPrefix (Ptr<Node> node,
                          Ipv6Address prefix, Ipv6Prefix plen);

  // For MR
  void AddMobileNetworkPrefix (Ptr<Node> node,
                               Ipv6Address prefix, Ipv6Prefix plen);
  void AddEgressInterface (Ptr<Node> node, const char *ifname);
  void AddHomeAgentAddress (Ptr<Node> node, Ipv6Address addr);
  void AddHomeAddress (Ptr<Node> node,
                       Ipv6Address addr, Ipv6Prefix plen);
  void EnableMR (NodeContainer nodes);
  void EnableDSMIP6 (NodeContainer nodes);

  // For PMIP
  void AddMNProfileMAG (Ptr<Node> node, Mac48Address mn_id, 
                        Ipv6Address lma_addr,
                        Ipv6Address home_pfx, Ipv6Prefix home_plen);
  void EnableMAG (Ptr<Node> node, const char *ifname, Ipv6Address addr);
  void EnableLMA (Ptr<Node> node, const char *ifname);

  // Common
  void EnableDebug (NodeContainer nodes);
  void UseManualConfig (NodeContainer nodes);
  void SetBinary (NodeContainer nodes, std::string binary);

  // Enable MIPv6 in CN
  bool cn_mipv6;

private:
  /**
   * \internal
   */
  ApplicationContainer InstallPriv (Ptr<Node> node);
  void GenerateConfig (Ptr<Node> node);
};

} // namespace ns3

#endif /* MIP6D_HELPER_H */
