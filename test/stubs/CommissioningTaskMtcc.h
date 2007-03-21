#ifndef DQM_SiStripCommissioningSourcesMtcc_CommissioningTaskMtcc_H
#define DQM_SiStripCommissioningSourcesMtcc_CommissioningTaskMtcc_H

#include "FWCore/Framework/interface/EventSetup.h"
#include "DataFormats/Common/interface/DetSet.h"
#include "DataFormats/SiStripDigi/interface/SiStripRawDigi.h"
#include "DataFormats/SiStripCommon/interface/SiStripEventSummary.h"
#include "CondFormats/SiStripObjects/interface/FedChannelConnection.h"
#include "DataFormats/SiStripCommon/interface/SiStripFedKey.h"
#include "boost/cstdint.hpp"
#include <string>
#include <iomanip>

class DaqMonitorBEInterface;
class MonitorElement;

using namespace std;

/**
   @class CommissioningTask
*/
class CommissioningTaskMtcc{

 public: // ----- public interface -----

  struct HistoSet {
    MonitorElement* meSumOfSquares_;
    MonitorElement* meSumOfContents_;
    MonitorElement* meNumOfEntries_;
    MonitorElement* meRawNoise_;
    MonitorElement* meCommonModeSubtractedNoise_;
    MonitorElement* mePedestals_;
    vector<uint32_t> vSumOfSquares_;
    vector<uint32_t> vSumOfSquaresOverflow_;
    vector<float> vSumOfContents_;
    vector<float> vNumOfEntries_;
    vector<float> vSumOfCMSSquares_;
    vector<float> vSumOfCMSContents_;
    uint32_t cluNumber_;

  };
  
  CommissioningTaskMtcc( DaqMonitorBEInterface*, 
		     const FedChannelConnection&,
		     const string& my_name );
  virtual ~CommissioningTaskMtcc();
  
  void bookHistograms();
  void fillHistograms( const SiStripEventSummary&, 
		       const edm::DetSet<SiStripRawDigi>& );
  
  void updateHistograms();
  
  /** Set histogram update frequency. */
  void updateFreq( const uint32_t& freq ) { updateFreq_ = freq; }

  /** Get histogram update frequency. */
  uint32_t getUpdateFreq() { return updateFreq_; };
  
  /** Set FED id and channel (for FED cabling task). */
  inline void fedChannel( const uint32_t& fed_key );
  
  /** Returns the name of this commissioning task. */
  const string& myName() const { return myName_; }

  //set cuts
  /*void cutForNoisy(float noi){ cutForNoisy_ = noi; };
  void cutForDead(float dead){cutForDead_ = dead; };
  void cutForNonGausTails(float gaus){ cutForNonGausTails_ = gaus; };*/
  
  
 protected: // ----- protected methods -----
  
  /** Updates vectors of HistoSet. */
  void updateHistoSet( HistoSet&, const uint32_t& bin, const uint32_t& value );
  /** Updates histograms (ME's) of HistoSet. */
  void updateHistoSet( HistoSet& );
  /** Returns const pointer to DQM back-end interface object. */
  inline DaqMonitorBEInterface* const dqm() const;
  /** */
  inline const FedChannelConnection& connection() const;
  
  /** Returns FEC key. */
  inline const uint32_t& fecKey() const;
  /** Returns FED key. */
  inline const uint32_t& fedKey() const;

  /** Returns FED id. */
  inline const uint32_t& fedId() const;
  /** Returns FED channel. */
  inline const uint32_t& fedCh() const;
  //CommissioningTaskMtcc() {;}

  // return cuts
  /*float getCutForDead(){ return cutForDead_; };
  float getCutForNoisy(){ return cutForNoisy_; };
  float getCutForNonGausTails(){ return cutForNonGausTails_; };*/



  
 private: // ----- private methods -----
  
  CommissioningTaskMtcc() {;}
  
  virtual void book();
  virtual void fill( const SiStripEventSummary&,
		     const edm::DetSet<SiStripRawDigi>& ) = 0;
  virtual void update() = 0;

 private: // ----- private data members -----

  //cuts
  /*float cutForDead_;
  float cutForNoisy_;
  float cutForNonGausTails_;*/


  DaqMonitorBEInterface* dqm_;
  uint32_t updateFreq_;
  uint32_t fillCntr_;
  FedChannelConnection connection_;
  uint32_t fedKey_;
  uint32_t fecKey_;
  bool booked_;
  pair<uint32_t,uint32_t> fedChannel_;
  string myName_;
  
};

// ----- inline methods -----

DaqMonitorBEInterface* const CommissioningTaskMtcc::dqm() const { return dqm_; }
const FedChannelConnection& CommissioningTaskMtcc::connection() const { return connection_; }

const uint32_t& CommissioningTaskMtcc::fecKey() const { return fecKey_; }
const uint32_t& CommissioningTaskMtcc::fedKey() const { return fedKey_; }

void CommissioningTaskMtcc::fedChannel( const uint32_t& fed_key ) { 
  SiStripFedKey path( fed_key ); 
  fedChannel_.first  = path.fedId(); 
  fedChannel_.second = path.fedChannel();
}
const uint32_t& CommissioningTaskMtcc::fedId() const { return fedChannel_.first; }
const uint32_t& CommissioningTaskMtcc::fedCh() const { return fedChannel_.second; }

#endif // DQM_SiStripCommissioningSources_CommissioningTaskMtcc_H

