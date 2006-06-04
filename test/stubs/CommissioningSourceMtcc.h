#ifndef DQM_SiStripCommissioningSourcesMtcc_CommissioningSourceMtcc_H
#define DQM_SiStripCommissioningSourcesMtcc_CommissioningSourceMtcc_H
#include "CondCore/DBCommon/interface/DBWriter.h"
#include "CondCore/DBCommon/interface/DBSession.h"
#include "CondCore/DBCommon/interface/Exception.h"
#include "CondCore/DBCommon/interface/ServiceLoader.h"
#include "CondCore/DBCommon/interface/ConnectMode.h"
#include "CondCore/DBCommon/interface/MessageLevel.h"
#include "CondCore/MetaDataService/interface/MetaData.h"
#include "CondCore/IOVService/interface/IOV.h"
#include "CondFormats/SiStripObjects/interface/SiStripFedCabling.h"
#include "CalibFormats/SiStripObjects/interface/SiStripFecCabling.h"
#include "CondFormats/SiStripObjects/interface/SiStripPedestals.h"
#include "CondFormats/SiStripObjects/interface/SiStripNoises.h"
#include "DataFormats/SiStripDigi/interface/SiStripEventSummary.h"
#include "FWCore/Framework/interface/EDAnalyzer.h"
#include <string>
#include <map>
#include "FWCore/Framework/interface/ESHandle.h"
#include "FWCore/Framework/interface/Handle.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/Event.h"
#include "CondFormats/DataRecord/interface/SiStripFedCablingRcd.h"
class DaqMonitorBEInterface;
class CommissioningTaskMtcc;
class FedChannelConnection;


/**
   @class CommissioningSource
*/
class OrderChannels {
 public:
  OrderChannels( const edm::EventSetup& setup){    
    //   setup.get<SiStripFedCablingRcd>().get( fedCabling_ ); 
    edm::ESHandle<SiStripFedCabling> fed_cabling;
    setup.get<SiStripFedCablingRcd>().get( fed_cabling ); 
    fedCabling_ = const_cast<SiStripFedCabling*>( fed_cabling.product() ); 
  };
  bool operator()( pair<uint16_t, uint16_t> h1,
		   pair<uint16_t, uint16_t> h2)
    {      
      uint32_t detid1 = fedCabling_->connection(h1.first, h1.second).detId(); 
      uint16_t apv1 = fedCabling_->connection(h1.first, h1.second).apvPairNumber();
      uint32_t detid2 = fedCabling_->connection(h2.first, h2.second).detId(); 
      uint16_t apv2 = fedCabling_->connection(h2.first, h2.second).apvPairNumber();
     
      if(detid1==detid2){
	return apv1<apv2;
      }else{
	return detid1 < detid2;}
    }
 private:
  SiStripFedCabling* fedCabling_;
};

class CommissioningSourceMtcc : public edm::EDAnalyzer {

 public: // ----- public interface -----
  
  /** May of task objects, identified through FedChanelId */
  typedef map<unsigned int, CommissioningTaskMtcc*> TaskMap;
  
  CommissioningSourceMtcc( const edm::ParameterSet& );
  ~CommissioningSourceMtcc();
  
  void beginJob( edm::EventSetup const& );
  void analyze( const edm::Event&, const edm::EventSetup& );
  void endJob();

 
 private: // ----- private methods -----

  /** Private default constructor. */
  CommissioningSourceMtcc();
  vector < pair<uint16_t, uint16_t> > OrderedPairs(const edm::EventSetup& setup);
  void createDirs();
  void createTask( SiStripEventSummary::Task task );
  void writePed();
 private: // ----- data members -----
  vector < pair<uint16_t, uint16_t> > rightpairs;
  string inputModuleLabel_;
  /** Interface to Data Quality Monitoring framework. */
  DaqMonitorBEInterface* dqm_;
  /** Identifies commissioning task. */
  string task_; 
  /** Map of task objects, identified through FedChanKey. */
  TaskMap tasks_;
  /** */
  int updateFreq_;
  /** */
  string filename_;
  /** */
  uint32_t run_;
  /** */
  bool firstEvent_;
  /** */
  SiStripFedCabling* fedCabling_;
  /** */
  SiStripFecCabling* fecCabling_;
  /** */

  cond::ServiceLoader* loader;
  cond::DBSession* session;
  cond::DBWriter* pwriter;
  cond::DBWriter* nwriter;
  cond::DBWriter* iovwriter;
  cond::MetaData* metadataSvc;
  map<int, SiStripPedestals *> iov_ped;
  map<int, SiStripNoises *> iov_noise;
  string connect_;
  string catalog_;
  string tag_p;
  string tag_n;
  unsigned int message_level_;
  unsigned int auth_;
  string userEnv_;  
  string passwdEnv_; 
  int RunMax_;
  int RunRange_;
  int RunStart_;
  int RunStop_;
  

  //cuts
  float cutForNoisy_;
  float cutForDead_;
  float cutForNonGausTails_;

};

#endif // DQM_SiStripCommissioningSourcesMtcc_CommissioningSourceMtcc_H

