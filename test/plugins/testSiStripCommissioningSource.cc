// Last commit: $Id: $

#include "DQM/SiStripCommissioningSources/test/plugins/testSiStripCommissioningSource.h"
#include "FWCore/Framework/interface/Event.h" 
#include "DataFormats/SiStripCommon/interface/SiStripConstants.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include <iostream>
#include <sstream>
#include <sys/types.h>
#include <unistd.h>
#include <iomanip>

#include <arpa/inet.h>
#include <sys/unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>

using namespace sistrip;

// -----------------------------------------------------------------------------
// 
testSiStripCommissioningSource::testSiStripCommissioningSource( const edm::ParameterSet& pset ) 
{
  edm::LogVerbatim(mlTest_)
    << "[testSiStripCommissioningSource::" << __func__ << "]"
    << " Constructing object...";
}

// -----------------------------------------------------------------------------
// 
testSiStripCommissioningSource::~testSiStripCommissioningSource() {
  edm::LogVerbatim(mlTest_)
    << "[testSiStripCommissioningSource::" << __func__ << "]"
    << " Destructing object...";
}

// -----------------------------------------------------------------------------
// 
void testSiStripCommissioningSource::beginJob( const edm::EventSetup& setup ) {
  
  std::stringstream ss;
  ss << "[testSiStripCommissioningSource::" << __func__ << "]"
     << " Initializing...";
  edm::LogVerbatim(mlTest_) << ss.str();

}

// -----------------------------------------------------------------------------
// 
void testSiStripCommissioningSource::analyze( const edm::Event& event, 
					      const edm::EventSetup& setup ) {
  LogTrace(mlTest_) 
    << "[testSiStripCommissioningSource::" << __func__ << "]"
    << " Analyzing run/event "
    << event.id().run() << "/"
    << event.id().event();
  
  // Do this once only
  if ( event.id().event() == 1 ) {
    std::stringstream str;
    directory(str);
    edm::LogVerbatim(mlDqmSource_) 
      << "[testSiStripCommissioningSource::" << __func__ << "]"
      << " String is:" << std::endl
      << " "
      << str.str();
  }
  
}

// -----------------------------------------------------------------------------
// 
void testSiStripCommissioningSource::directory( std::stringstream& dir,
						uint32_t run_number ) {

  // Get details about host
  char hn[256];
  gethostname( hn, sizeof(hn) );
  struct hostent* he;
  he = gethostbyname(hn);

  // Extract host name and ip
  std::string host_name;
  std::string host_ip;
  if ( he ) { 
    host_name = std::string(he->h_name);
    host_ip = std::string( inet_ntoa( *(struct in_addr*)(he->h_addr) ) );
  } else {
    host_name = "unknown.cern.ch";
    host_ip = "255.255.255.255";
  }

  // Reformat IP address
  std::string::size_type pos = 0;
  std::stringstream ip;
  //for ( uint16_t ii = 0; ii < 4; ++ii ) {
  while ( pos != std::string::npos ) {
    std::string::size_type tmp = host_ip.find(".",pos);
    if ( tmp != std::string::npos ) {
      ip << std::setw(3)
	 << std::setfill('0')
	 << host_ip.substr( pos, tmp-pos ) 
	 << ".";
      pos = tmp+1; // skip the delimiter "."
    } else {
      ip << std::setw(3)
	 << std::setfill('0')
	 << host_ip.substr( pos );
      pos = std::string::npos;
    }
  }
  
  // Get pid
  pid_t pid = getpid();

  // Construct string
  dir << std::setw(8) 
      << std::setfill('0') 
      << run_number
      << "_" 
      << ip.str()
      << "_"
      << std::setw(5) 
      << std::setfill('0') 
      << pid;
  
  // Debug
  std::stringstream sss;
  sss << "[testSiStripCommissioningSource::" << __func__ << "]" << std::endl
      << " run number : " << run_number << std::endl
      << " host name  : " << host_name << std::endl
      << " host ip    : " << host_ip << std::endl
      << " pid        : " << pid;
  edm::LogVerbatim(mlDqmSource_) << sss.str();

}


