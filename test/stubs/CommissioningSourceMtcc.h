#ifndef DQM_SiStripCommissioningSourcesMtcc_CommissioningSourceMtcc_H
#define DQM_SiStripCommissioningSourcesMtcc_CommissioningSourceMtcc_H
#include "CondCore/DBCommon/interface/Exception.h"
#include "CondFormats/SiStripObjects/interface/SiStripFedCabling.h"
#include "CalibFormats/SiStripObjects/interface/SiStripFecCabling.h"
#include "CondFormats/SiStripObjects/interface/SiStripPedestals.h"
#include "CondFormats/SiStripObjects/interface/SiStripNoises.h"
#include "DataFormats/SiStripDigi/interface/SiStripEventSummary.h"

#include "DataFormats/SiStripCommon/interface/SiStripEnumeratedTypes.h"

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
class CommissioningSourceMtcc : public edm::EDAnalyzer {

 public: // ----- public interface -----
  
  /** May of task objects, identified through FedChanelId */
  typedef std::map<unsigned int, CommissioningTaskMtcc*> TaskMap;
  
  CommissioningSourceMtcc( const edm::ParameterSet& );
  ~CommissioningSourceMtcc();
  
  void beginJob( edm::EventSetup const& );
  void analyze( const edm::Event&, const edm::EventSetup& );
  void endJob();

 
 private: // ----- private methods -----

  /** Private default constructor. */
  CommissioningSourceMtcc();
  void createDirs();
  void createTask( sistrip::Task task );
  void writePed();
 private: // ----- data members -----
  std::vector < std::pair<uint16_t, uint16_t> > rightpairs;
  std::string inputModuleLabel_;
  /** Interface to Data Quality Monitoring framework. */
  DaqMonitorBEInterface* dqm_;
  /** Identifies commissioning task. */
  std::string task_; 
  /** Map of task objects, identified through FedChanKey. */
  TaskMap tasks_;
  /** */
  int updateFreq_;
  /** */
  std::string filename_;
  /** */
  uint32_t run_;
  /** */
  bool firstEvent_;
  /** */
  SiStripFedCabling* fedCabling_;
  /** */
  SiStripFecCabling* fecCabling_;
  /** */

  std::string userEnv_;  
  std::string passwdEnv_; 
  
  //cuts
  float cutForNoisy_;
  float cutForDead_;
  float cutForNonGausTails_;

};

#endif // DQM_SiStripCommissioningSourcesMtcc_CommissioningSourceMtcc_H

