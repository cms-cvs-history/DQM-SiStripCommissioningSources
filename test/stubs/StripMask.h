#ifndef DQM_SiStripCommissioningSourcesMtcc_StripMask_H
#define DQM_SiStripCommissioningSourcesMtcc_StripMask_H

#include "DQMServices/Core/interface/MonitorElement.h"
#include <vector>
#include "boost/cstdint.hpp"

using namespace std;
class StripMask {
	public:
	StripMask(float cutNoisy, float cutDead, float cutGaus);
	int getFlag( uint16_t strip ) { return  vFlag_[strip]; };
	void setFlag( uint16_t strip, int flag ) { vFlag_[strip] = flag; };
	void uploadNoise(MonitorElement* noise){ theNoiSet_ = noise; };
	void uploadPedestals(MonitorElement* peds){ thePedSet_ = peds; };
	void uploadEntries(MonitorElement* entries){theNumOfEntries_ = entries; };
	void getCoupleMeanNoise();
        void setTotalEvents(uint32_t total){ totalEvent_ = total; };
	int check(uint16_t strip); 

	private:
	vector<int> vFlag_;
	uint32_t totalEvent_;
	float meanNoise_;
	float meanNumberOfGoodEvents_;
        //StripMask* theStripMask_;
	float cutNoisy_;
	float cutDead_;
	float cutGaus_;

        MonitorElement* thePedSet_;
        MonitorElement* theNoiSet_;
        MonitorElement* theNumOfEntries_;
};
#endif //DQM_SiStripCommissioningSourcesMtcc_StripMask_H
