// Last commit: $Id: $

#ifndef DQM_SiStripCommissioningClients_testSiStripCommissioningSource_H
#define DQM_SiStripCommissioningClients_testSiStripCommissioningSource_H

#include "FWCore/Framework/interface/EDAnalyzer.h"
#include <boost/cstdint.hpp>
#include <sstream>

/**
   @class testSiStripCommissioningSource 
   @author R.Bainbridge
   @brief Simple class that tests SiStripCommissioningSource
*/
class testSiStripCommissioningSource : public edm::EDAnalyzer {

 public:
  
  testSiStripCommissioningSource( const edm::ParameterSet& );
  ~testSiStripCommissioningSource();
  
  void beginJob( edm::EventSetup const& );
  void analyze( const edm::Event&, const edm::EventSetup& );
  void endJob() {;}

 private:

  void directory( std::stringstream&, 
		  uint32_t run_number = 0 );
  
};

#endif // DQM_SiStripCommissioningClients_testSiStripCommissioningSource_H

