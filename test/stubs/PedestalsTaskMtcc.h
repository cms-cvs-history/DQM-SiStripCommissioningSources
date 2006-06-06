#ifndef DQM_SiStripCommissioningSourcesMtcc_PedestalsTaskMtcc_h
#define DQM_SiStripCommissioningSourceMtccs_PedestalsTaskMtcc_h
#include "DQM/SiStripCommissioningSources/test/stubs/BadStripFinder.h"
#include "DQM/SiStripCommissioningSources/test/stubs/CommissioningTaskMtcc.h"

/**
   @class PedestalsTask
*/
class PedestalsTaskMtcc : public CommissioningTaskMtcc {

 public:
  
  PedestalsTaskMtcc( DaqMonitorBEInterface*, const FedChannelConnection& , float, float, float);
  virtual ~PedestalsTaskMtcc();

  MonitorElement* getPedestals() { return peds_.mePedestals_;};
  MonitorElement* getCMSnoise() { return peds_.meCommonModeSubtractedNoise_; };
  
  //get cuts
  float getCutForNoisy(){ return cutForNoisy_; };
  float getCutForDead(){ return cutForDead_; };
  float getCutForNonGausTails(){ return cutForNonGausTails_; };
  int getFlag(uint16_t strip){ return theBadStripFinder_.downloadFlag(strip); };

 private:
  
  HistoSet peds_;
  BadStripFinder theBadStripFinder_;
  virtual void book();
  virtual void fill( const SiStripEventSummary&,
		     const edm::DetSet<SiStripRawDigi>& );
  virtual void update();
  float cmn_[2]; 
  //HistoSet peds_;
  uint32_t tot_event_;
  
  //cuts
  float cutForNoisy_;
  float cutForDead_;
  float cutForNonGausTails_;


};

#endif // DQM_SiStripCommissioningSourcesMtcc_PedestalsTask_h

