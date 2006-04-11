#ifndef DQM_SiStripCommissioningSources_VpspScanTask_h
#define DQM_SiStripCommissioningSources_VpspScanTask_h

#include "DQM/SiStripCommissioningSources/interface/CommissioningTask.h"

/**
   @class VpspScanTask
*/
class VpspScanTask : public CommissioningTask {

 public:
  
  VpspScanTask( DaqMonitorBEInterface*, const FedChannelConnection& );
  virtual ~VpspScanTask();
  
 private:

  virtual void book( const FedChannelConnection& );
  virtual void fill( const SiStripEventSummary&,
		     const edm::DetSet<SiStripRawDigi>& );
  virtual void update();
  
  vector<HistoSet> vpsp_;
  
};

#endif // DQM_SiStripCommissioningSources_VpspScanTask_h
