#include "DQM/SiStripCommissioningSources/test/stubs/StripMask.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "TMath.h"
#include <iostream>

#ifndef STRIPPERCOUPLE
#define STRIPPERCOUPLE 256
#endif

#define CUTFORNOISY 2
#define CUTFORDEAD 0.3
#define CUT_FOR_NON_GAUS_TAILS 0.95
#define CUTAVOIDSIGNAL 4

StripMask::StripMask(float cutNoisy, float cutDead, float cutGaus){
	edm::LogInfo("Commissioning") << "Constructing StripMask...";
	vFlag_.resize(STRIPPERCOUPLE, 0);
	cutNoisy_ = cutNoisy;
	cutDead_ = cutDead;
	cutGaus_ = cutGaus;
}

void StripMask::getCoupleMeanNoise(){
        int good = 0;
        meanNoise_ = 0;
	meanNumberOfGoodEvents_ = 0;
        for (uint16_t i = 0; i < STRIPPERCOUPLE; i++){
                //cout << "Flag: " << theStripMask_->getFlag(i) << endl;
                if (getFlag(i)==0){
                        good ++;
                        meanNoise_ += theNoiSet_->getBinContent(i+1);
			meanNumberOfGoodEvents_ += theNumOfEntries_->getBinContent(i + 1);
                }
        }
        meanNoise_ /= good;
	meanNumberOfGoodEvents_ /= good;
        //cout << "Mean noise " << meanNoise_ << endl;
}

int StripMask::check(uint16_t strip){
      if ( (meanNoise_ != 0) && (getFlag(strip) == 0) ){
	//cout << "noisy: " << cutNoisy_ << " dead " << cutDead_ << " gaus " << cutGaus_ << endl;
	float found = theNumOfEntries_->getBinContent(strip + 1);
        if (theNoiSet_->getBinContent(strip + 1) > cutNoisy_ * meanNoise_ ){
                cout << "Flagged one strip for extra noise" << endl;
		return 1;
        }
	else if ((found < meanNumberOfGoodEvents_)){
		double prob = (2*found + 1 + TMath::Sqrt(1 + 4 * found * (1 - found/meanNumberOfGoodEvents_)))/(2*(meanNumberOfGoodEvents_ + 1));
		if ( (2*prob -1 ) <
	            cutGaus_ * (2 * TMath::Erf(CUTAVOIDSIGNAL)-1)){
               		cout << "Prob " << 2*prob -1 << " expected " << 2*TMath::Erfc(CUTAVOIDSIGNAL) - 1 << endl;
			cout << "Flagged one strip for non gaussian noise" << endl;
			return 1;
		}
		else return 0;
	}
	else if (theNoiSet_->getBinContent(strip + 1) < cutDead_ * meanNoise_){
		cout << "Flagged one dead strip" << endl;
		return 1;
	}
	else return 0;
      }
      else return getFlag(strip);
}
