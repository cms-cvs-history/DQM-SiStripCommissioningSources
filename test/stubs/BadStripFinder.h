#ifndef DQM_SiStripCommissioningSourcesMtcc_BadStripFinder_H
#define DQM_SiStripCommissioningSourcesMtcc_BadStripFinder_H

#include "DQM/SiStripCommissioningSources/test/stubs/StripMask.h"
#include "DQM/SiStripCommissioningSources/test/stubs/CommissioningTaskMtcc.h"
#include "DQMServices/Core/interface/MonitorElement.h"

class MonitorElement;

class BadStripFinder{
	
	public:
	BadStripFinder(float cutNoi, float cutDead, float cutGaus);
        void updatePedestals(MonitorElement* peds) { theStripMask_->uploadPedestals(peds); };
        void updateNoise(MonitorElement* noi) { theStripMask_->uploadNoise(noi); };
	void updateEntries(MonitorElement* entries) { theStripMask_->uploadEntries(entries); };
	int downloadFlag(uint16_t strip) { return(theStripMask_->getFlag(strip)); };
	void updateTotalEvents(uint32_t event){ theStripMask_->setTotalEvents(event); };
 	void check();

	private:
	StripMask* theStripMask_;
        float cutNoi_;
        float cutDead_;
        float cutGaus_;
	void uploadFlag(uint16_t strip, int flag) { theStripMask_->setFlag(strip, flag); };
 
};
#endif //DQM_SiStripCommissioningSourcesMtcc_BadStripFinder_H
