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

#include "ns3/object-factory.h"
#include "ns3/simulator.h"
#include "mip6d-helper.h"
#include "ns3/dce-application-helper.h"
#include "ns3/names.h"
#include "ns3/linux-socket-fd-factory.h"
#include <fstream>
#include <map>
#include <sys/stat.h>

namespace ns3 {

class Mip6dConfig : public Object
{
private:
  static int index;
  std::string router_id;
public:
  Mip6dConfig ()
    : m_haenable (false),
      m_mrenable (false),
      m_magenable (false),
      m_debug (false),
      m_usemanualconf (false),
      m_ha_served_pfx (""),
      m_dsmip6enable (false),
      m_binary ("mip6d")
  {
    m_mr_mobile_pfx = new std::vector<std::string> ();
    m_mr_egress_if = new std::vector<std::string> ();
    m_mag_mn_profiles = new std::vector<Mip6dConfig::pmipMNprofile_t> ();
  }
  ~Mip6dConfig ()
  {
  }

  static TypeId
  GetTypeId (void)
  {
    static TypeId tid = TypeId ("ns3::Mip6dConfig")
      .SetParent<Object> ()
      .AddConstructor<Mip6dConfig> ()
    ;
    return tid;
  }
  TypeId
  GetInstanceTypeId (void) const
  {
    return GetTypeId ();
  }

  typedef struct
  {
    std::string m_mn_id;
    std::string m_lma_addr;
    std::string m_home_prefix;
  } pmipMNprofile_t;

  bool m_haenable;
  bool m_mrenable;
  bool m_magenable;
  bool m_debug;
  bool m_usemanualconf;
  bool m_dsmip6enable;
  std::string m_binary;
  std::string m_ha_served_pfx;
  std::vector<std::string> *m_mr_mobile_pfx;
  std::vector<std::string> *m_mr_egress_if;
  std::string m_mn_ha_addr;
  std::string m_mr_home_addr;
  std::string m_lma_mag_ifname;
  std::string m_mag_egress_gaddr;
  std::string m_mag_egress_ifname;
  std::vector<pmipMNprofile_t> *m_mag_mn_profiles;

  virtual void
  Print (std::ostream& os) const
  {
    os << "# IPsec configuration - NO IPSEC AT THE MOMENT" << std::endl
       << "UseMnHaIPsec disabled;" << std::endl
       << "KeyMngMobCapability disabled;" << std::endl
       << "# EOF" << std::endl;
  }
};
std::ostream& operator << (std::ostream& os, Mip6dConfig const& config)
{
  config.Print (os);
  return os;
}


Mip6dHelper::Mip6dHelper ()
{
  cn_mipv6 = false;
}

void
Mip6dHelper::SetAttribute (std::string name, const AttributeValue &value)
{
}

// HomeAgent
void
Mip6dHelper::EnableHA (NodeContainer nodes)
{
  for (uint32_t i = 0; i < nodes.GetN (); i++)
    {
      Ptr<Mip6dConfig> mip6d_conf = nodes.Get (i)->GetObject<Mip6dConfig> ();
      if (!mip6d_conf)
        {
          mip6d_conf = CreateObject<Mip6dConfig> ();
          nodes.Get (i)->AggregateObject (mip6d_conf);
        }
      mip6d_conf->m_haenable = true;

      Ptr<LinuxSocketFdFactory> kern = nodes.Get (i)->GetObject<LinuxSocketFdFactory>();
      Simulator::ScheduleWithContext (nodes.Get (i)->GetId (), Seconds (0.1),
                                      MakeEvent (&LinuxSocketFdFactory::Set, kern,
                                                 ".net.ipv6.conf.all.forwarding", "1"));
      Simulator::ScheduleWithContext (nodes.Get (i)->GetId (), Seconds (0.1),
                                      MakeEvent (&LinuxSocketFdFactory::Set, kern,
                                                 ".net.ipv6.conf.all.proxy_ndp", "1"));
    }

  return;
}

void
Mip6dHelper::AddHaServedPrefix (Ptr<Node> node,
                                Ipv6Address prefix, Ipv6Prefix plen)
{
  Ptr<Mip6dConfig> mip6d_conf = node->GetObject<Mip6dConfig> ();
  if (!mip6d_conf)
    {
      mip6d_conf = CreateObject<Mip6dConfig> ();
      node->AggregateObject (mip6d_conf);
    }

  std::ostringstream oss;
  prefix.Print (oss);
  oss << "/" << (uint32_t)plen.GetPrefixLength ();

  mip6d_conf->m_ha_served_pfx = oss.str ();

  return;
}

// MobileRouter
void
Mip6dHelper::EnableMR (NodeContainer nodes)
{
  //  for (uint32_t i = 0; i < nodes.GetN (); i++)
  for (uint32_t i = 0; i < nodes.GetN (); i++)
    {
      Ptr<Mip6dConfig> mip6d_conf = nodes.Get (i)->GetObject<Mip6dConfig> ();
      if (!mip6d_conf)
        {
          mip6d_conf = CreateObject<Mip6dConfig> ();
          nodes.Get (i)->AggregateObject (mip6d_conf);
        }
      mip6d_conf->m_mrenable = true;

      Ptr<LinuxSocketFdFactory> kern = nodes.Get (i)->GetObject<LinuxSocketFdFactory>();
      Simulator::ScheduleWithContext (nodes.Get (i)->GetId (), Seconds (0.1),
                                      MakeEvent (&LinuxSocketFdFactory::Set, kern,
                                                 ".net.ipv6.conf.all.forwarding", "1"));
    }

  return;
}

void
Mip6dHelper::AddMobileNetworkPrefix (Ptr<Node> node,
                                     Ipv6Address prefix, Ipv6Prefix plen)
{
  Ptr<Mip6dConfig> mip6d_conf = node->GetObject<Mip6dConfig> ();
  if (!mip6d_conf)
    {
      mip6d_conf = CreateObject<Mip6dConfig> ();
      node->AggregateObject (mip6d_conf);
    }

  std::ostringstream oss;
  prefix.Print (oss);
  oss << "/" << (uint32_t)plen.GetPrefixLength ();

  mip6d_conf->m_mr_mobile_pfx->push_back (oss.str ());

  return;
}

void
Mip6dHelper::AddEgressInterface (Ptr<Node> node, const char *ifname)
{
  Ptr<Mip6dConfig> mip6d_conf = node->GetObject<Mip6dConfig> ();
  if (!mip6d_conf)
    {
      mip6d_conf = CreateObject<Mip6dConfig> ();
      node->AggregateObject (mip6d_conf);
    }

  mip6d_conf->m_mr_egress_if->push_back (std::string (ifname));

  return;
}

void
Mip6dHelper::AddHomeAgentAddress (Ptr<Node> node, Ipv6Address addr)
{
  Ptr<Mip6dConfig> mip6d_conf = node->GetObject<Mip6dConfig> ();
  if (!mip6d_conf)
    {
      mip6d_conf = CreateObject<Mip6dConfig> ();
      node->AggregateObject (mip6d_conf);
    }

  std::ostringstream oss;
  addr.Print (oss);
  mip6d_conf->m_mn_ha_addr = oss.str ();

  return;
}

void
Mip6dHelper::AddHomeAddress (Ptr<Node> node,
                             Ipv6Address addr, Ipv6Prefix plen)
{
  Ptr<Mip6dConfig> mip6d_conf = node->GetObject<Mip6dConfig> ();
  if (!mip6d_conf)
    {
      mip6d_conf = CreateObject<Mip6dConfig> ();
      node->AggregateObject (mip6d_conf);
    }

  std::ostringstream oss;
  addr.Print (oss);
  oss << "/" << (uint32_t)plen.GetPrefixLength ();
  mip6d_conf->m_mr_home_addr = oss.str ();

  return;
}

// Local Mobility Anchor (LMA)
void
Mip6dHelper::EnableLMA (Ptr<Node> node, const char *ifname)
{
  Ptr<Mip6dConfig> mip6d_conf = node->GetObject<Mip6dConfig>();
  if (!mip6d_conf)
    {
      mip6d_conf = CreateObject<Mip6dConfig> ();
      node->AggregateObject (mip6d_conf);
    }

  mip6d_conf->m_haenable = true;
  mip6d_conf->m_lma_mag_ifname = ifname;
}

// Mobile Access Gateway (MAG)
void
Mip6dHelper::EnableMAG (Ptr<Node> node, 
                        const char *ifname, Ipv6Address addr)
{
  Ptr<Mip6dConfig> mip6d_conf = node->GetObject<Mip6dConfig>();
  if (!mip6d_conf)
    {
      mip6d_conf = CreateObject<Mip6dConfig> ();
      node->AggregateObject (mip6d_conf);
    }

  mip6d_conf->m_magenable = true;
  std::ostringstream oss;
  addr.Print (oss);
  mip6d_conf->m_mag_egress_gaddr = oss.str ();
  mip6d_conf->m_mag_egress_ifname = ifname;
}

void
Mip6dHelper::AddMNProfileMAG (Ptr<Node> node, Mac48Address mn_id, 
                              Ipv6Address lma_addr,
                              Ipv6Address home_pfx, Ipv6Prefix home_plen)
{
  Ptr<Mip6dConfig> mip6d_conf = node->GetObject<Mip6dConfig>();
  if (!mip6d_conf)
    {
      mip6d_conf = CreateObject<Mip6dConfig> ();
      node->AggregateObject (mip6d_conf);
    }

  std::ostringstream oss;
  Mip6dConfig::pmipMNprofile_t profile;

  oss << mn_id;
  profile.m_mn_id = oss.str ();

  oss.str ("");
  lma_addr.Print (oss);
  profile.m_lma_addr = oss.str ();

  oss.str ("");
  home_pfx.Print (oss);
  oss << "/" << (uint32_t)home_plen.GetPrefixLength ();
  profile.m_home_prefix = oss.str ();

  mip6d_conf->m_mag_mn_profiles->push_back (profile);

  return;
}

// DSMIP
void
Mip6dHelper::EnableDSMIP6 (NodeContainer nodes)
{
  for (uint32_t i = 0; i < nodes.GetN (); i++)
    {
      Ptr<Mip6dConfig> mip6d_conf = nodes.Get (i)->GetObject<Mip6dConfig> ();
      if (!mip6d_conf)
        {
          mip6d_conf = CreateObject<Mip6dConfig> ();
          nodes.Get (i)->AggregateObject (mip6d_conf);
        }
      mip6d_conf->m_dsmip6enable = true;
    }

  return;
}

void
Mip6dHelper::EnableDebug (NodeContainer nodes)
{
  for (uint32_t i = 0; i < nodes.GetN (); i++)
    {
      Ptr<Mip6dConfig> mip6d_conf = nodes.Get (i)->GetObject<Mip6dConfig> ();
      if (!mip6d_conf)
        {
          mip6d_conf = CreateObject<Mip6dConfig> ();
          nodes.Get (i)->AggregateObject (mip6d_conf);
        }
      mip6d_conf->m_debug = true;
    }
  return;
}

void
Mip6dHelper::UseManualConfig (NodeContainer nodes)
{
  for (uint32_t i = 0; i < nodes.GetN (); i++)
    {
      Ptr<Mip6dConfig> mip6d_conf = nodes.Get (i)->GetObject<Mip6dConfig> ();
      if (!mip6d_conf)
        {
          mip6d_conf = new Mip6dConfig ();
          nodes.Get (i)->AggregateObject (mip6d_conf);
        }
      mip6d_conf->m_usemanualconf = true;
    }
  return;
}

void
Mip6dHelper::SetBinary (NodeContainer nodes, std::string binary)
{
  for (uint32_t i = 0; i < nodes.GetN (); i++)
    {
      Ptr<Mip6dConfig> mip6d_conf = nodes.Get (i)->GetObject<Mip6dConfig> ();
      if (!mip6d_conf)
        {
          mip6d_conf = new Mip6dConfig ();
          nodes.Get (i)->AggregateObject (mip6d_conf);
        }
      mip6d_conf->m_binary = binary;
    }
  return;
}

void
Mip6dHelper::GenerateConfig (Ptr<Node> node)
{
  Ptr<Mip6dConfig> mip6d_conf = node->GetObject<Mip6dConfig> ();

  if (mip6d_conf->m_usemanualconf)
    {
      return;
    }

  // config generation
  std::stringstream conf_dir, conf_file;
  // FIXME XXX
  conf_dir << "files-" << node->GetId () << "";
  ::mkdir (conf_dir.str ().c_str (), S_IRWXU | S_IRWXG);
  conf_dir << "/etc/";
  ::mkdir (conf_dir.str ().c_str (), S_IRWXU | S_IRWXG);

  conf_file << conf_dir.str () << "/mip6d.conf";
  std::ofstream conf;
  conf.open (conf_file.str ().c_str ());

  if (mip6d_conf->m_haenable)
    {
      conf << "NodeConfig HA;" << std::endl
           << "Interface \"sim0\";" << std::endl
           << "HaAcceptMobRtr enabled;" << std::endl
           << "#BindingAclPolicy 2001:1:2:3::1000 (2001:1:2:5::/64) allow;" << std::endl
           << "DefaultBindingAclPolicy allow;" << std::endl;

      if (!mip6d_conf->m_lma_mag_ifname.empty ())
        conf << "LMAInterfaceMAG \"" << mip6d_conf->m_lma_mag_ifname << "\";" << std::endl;

      if (!mip6d_conf->m_ha_served_pfx.empty ())
        conf << "HaServedPrefix " << mip6d_conf->m_ha_served_pfx << ";" << std::endl;

      if (mip6d_conf->m_dsmip6enable)
        {
          conf << "HaAcceptDsmip6 enabled;" << std::endl;
        }
      if (mip6d_conf->m_dsmip6enable)
        {
          conf << "# The IPv4 address of the HA or the HA name can be given." << std::endl;
          conf << "HomeAgentV4Address 192.168.10.1;" << std::endl;
        }
    }
  // PMIP MAG
  else if (mip6d_conf->m_magenable)
    {
      conf << "NodeConfig MN;" << std::endl
           << "DoRouteOptimizationCN disabled;" << std::endl
           << "DoRouteOptimizationMN disabled;" << std::endl
           << "UseCnBuAck disabled;" << std::endl
           << "OptimisticHandoff enabled;" << std::endl
           << "MnMaxHaBindingLife 60;" << std::endl;

      conf << "MAGInterfaceLMA \"" << mip6d_conf->m_mag_egress_ifname << "\";" << std::endl;
      conf << "MAGEgressGlobalAddress " << mip6d_conf->m_mag_egress_gaddr << ";" << std::endl;


      for (std::vector<Mip6dConfig::pmipMNprofile_t>::iterator i = mip6d_conf->m_mag_mn_profiles->begin (); 
           i != mip6d_conf->m_mag_mn_profiles->end (); ++i)
        {
          conf << "MNIdentifier \"" << (*i).m_mn_id <<"\" {" << std::endl;
          conf << "       PMIPEnabled 1;" << std::endl;
          conf << "       LMAAddress " << (*i).m_lma_addr << ";" << std::endl;
          conf << "       HomeNetworkPrefix1 " << (*i).m_home_prefix << ";" << std::endl;
          conf << "       PMIPInterface1 \"sim0(XXX)\";" << std::endl;
          conf << "       HomePrefixLifetime 460.0;" << std::endl;
          conf << "}" << std::endl;
        }
    }
  // CMIP-MN or MR (NEMO)
  else
    {
      if (mip6d_conf->m_dsmip6enable)
        {
          conf << "MnUseDsmip6 enabled;" << std::endl;
        }
//*************************
      if (!cn_mipv6)
        {
          conf << "NodeConfig MN;" << std::endl
               //<< "DoRouteOptimizationCN enabled;" << std::endl
               << "DoRouteOptimizationMN enabled;" << std::endl
               << "UseCnBuAck enabled;" << std::endl
               << "MnDiscardHaParamProb enabled;" << std::endl;
               //<< "MobRtrUseExplicitMode enabled;" << std::endl;
        }
      if (cn_mipv6)
        {
          conf << "NodeConfig CN;" << std::endl
               << "DoRouteOptimizationCN enabled;" << std::endl
               //<< "DoRouteOptimizationMN enabled;" << std::endl
               << "UseCnBuAck enabled;" << std::endl
               << "MnDiscardHaParamProb enabled;" << std::endl;
               //<< "MobRtrUseExplicitMode enabled;" << std::endl;
        }
//**************************

      for (std::vector<std::string>::iterator i = mip6d_conf->m_mr_egress_if->begin ();
           i != mip6d_conf->m_mr_egress_if->end (); ++i)
        {
          if (mip6d_conf->m_dsmip6enable)
            {
              conf << "Interface \"" << (*i) << "\"{" << std::endl;
              conf << "  # Specify if this interface should use DHCP to get an IPv4 CoA" << std::endl;
              conf << "  UseDhcp enabled;" << std::endl;
              conf << "}" << std::endl;
            }
          else
            {
              conf << "Interface \"" << (*i) << "\";" << std::endl;
            }
        }


      conf << "MnRouterProbes 1;" << std::endl
           << "MnHomeLink \"sim0\" {" << std::endl;
      if (mip6d_conf->m_dsmip6enable)
        {
          conf << "# The IPv4 address of the HA or the HA name can be given." << std::endl
               << "# If both are given, the HomeAgentName field is ignored." << std::endl
               << "HomeAgentV4Address 192.168.10.1;" << std::endl
               << "# HomeAgentName <FQDN of the Home Agent>;" << std::endl;
        }

      if (mip6d_conf->m_mrenable)
        {
          conf << "	IsMobRtr enabled;" << std::endl;
        }
      conf << "	HomeAgentAddress " << mip6d_conf->m_mn_ha_addr << ";" << std::endl;

      conf << "	HomeAddress " << mip6d_conf->m_mr_home_addr;
      
      if (mip6d_conf->m_mrenable)
        {
          conf << "(";
          for (std::vector<std::string>::iterator i = mip6d_conf->m_mr_mobile_pfx->begin ();
               i != mip6d_conf->m_mr_mobile_pfx->end (); ++i)
            {
              if (i != mip6d_conf->m_mr_mobile_pfx->begin ())
                {
                  conf << "," ;
                }
              conf << (*i);
            }
          conf << ")" << std::endl;
        }
      conf << ";" << std::endl;
      conf << "}" << std::endl;
    }

  if (mip6d_conf->m_debug)
    {
      conf << "DebugLevel 10;" << std::endl;
    }

  conf << *mip6d_conf;
  conf.close ();

}


ApplicationContainer
Mip6dHelper::Install (Ptr<Node> node)
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
Mip6dHelper::Install (std::string nodeName)
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
Mip6dHelper::Install (NodeContainer c)
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

ApplicationContainer
Mip6dHelper::InstallPriv (Ptr<Node> node)
{
  DceApplicationHelper process;
  ApplicationContainer apps;

  Ptr<Mip6dConfig> mip6d_conf = node->GetObject<Mip6dConfig> ();
  if (!mip6d_conf)
    {
      mip6d_conf = new Mip6dConfig ();
      node->AggregateObject (mip6d_conf);
    }
  GenerateConfig (node);

  process.ResetArguments ();
  process.SetBinary (mip6d_conf->m_binary);
  process.ParseArguments ("-c /etc/mip6d.conf -d 10");
  process.SetStackSize (1 << 16);
  apps.Add (process.Install (node));
  apps.Get (0)->SetStartTime (Seconds (1.0 + 0.01 * node->GetId ()));
  node->AddApplication (apps.Get (0));

  return apps;
}

} // namespace ns3
