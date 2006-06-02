#include "DQM/SiStripCommissioningSources/test/stubs/BadStripFinder.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#ifndef STRIPPERCOUPLE
#define STRIPPERCOUPLE 256
#endif

#define CUTFORNOISY 4
#define CUT_FOR_NON_GAUS_TAILS 2


BadStripFinder::BadStripFinder(float cutNoi, float cutDead, float cutGaus) {
	edm::LogInfo("Commissioning") << "Constructing BadStripFinder...";
	theStripMask_ = new StripMask(cutNoi, cutDead, cutGaus);
	cutNoi_ = cutNoi;
	cutDead_ = cutDead;
	cutGaus_ = cutGaus;
}


void BadStripFinder::check(){
	
	theStripMask_->getCoupleMeanNoise();
	for (uint16_t strip = 0; strip < STRIPPERCOUPLE; strip ++){
	  //if (downloadFlag(strip) == 0){
            uploadFlag(strip, theStripMask_->check(strip));
	    //if (theStripMask_->check(strip)==1) {
		//cout << "Uploaded one bad strip for extra noise" << endl;	
	    //}
	  //}
	}
}

	
