#include "DQM/SiStripCommissioningSources/test/stubs/PedestalsTaskMtcc.h"
#include "DQM/SiStripCommissioningSources/test/stubs/SiStripHistoNamingSchemeMtcc.h"
#include "DQM/SiStripCommon/interface/SiStripGenerateKey.h"
#include "DQMServices/Core/interface/DaqMonitorBEInterface.h"
#include "CalibFormats/SiStripObjects/interface/SiStripFecCabling.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include <math.h>
#ifndef STRIPPERCOUPLE
#define STRIPPERCOUPLE 256
#endif
#define CUTAVOIDSIGNAL 4
#define EVENTS_TO_BEGIN 200

// -----------------------------------------------------------------------------
//
PedestalsTaskMtcc::PedestalsTaskMtcc( DaqMonitorBEInterface* dqm,
			      const FedChannelConnection& conn, 
		              float cutNoisy,
			      float cutDead,
		              float cutGaus ) :
  CommissioningTaskMtcc( dqm, conn, "PedestalsTaskMtcc" ),
  peds_(),
  theBadStripFinder_( cutNoisy, 
		      cutDead, 
		      cutGaus ),
  cutForNoisy_(cutNoisy),
  cutForDead_(cutDead),
  cutForNonGausTails_(cutGaus)
{
  edm::LogInfo("Commissioning") << "[PedestalsTaskMtcc::PedestalsTaskMtcc] Constructing object...";
  //peds_.vSumOfContents_old_.resize(STRIPPERCOUPLE, 0);
  //peds_.vNumOfEntries_old_.resize(STRIPPERCOUPLE, 1);
  peds_.vSumOfSquares_.resize(STRIPPERCOUPLE,0);
  peds_.vSumOfContents_.resize(STRIPPERCOUPLE,0);
  peds_.vNumOfEntries_.resize(STRIPPERCOUPLE,0);
  peds_.vSumOfCMSContents_.resize(STRIPPERCOUPLE, 0);
  peds_.vSumOfCMSSquares_.resize(STRIPPERCOUPLE, 0);
  //peds_.vSumOfSquares_old_.resize(STRIPPERCOUPLE, 0);
  tot_event_ = 0;
  edm::LogInfo("CommissioningSourceMtcc") << "dead " << cutDead  << " noisy " << cutNoisy  
					<< " gaus " << cutGaus;
}

// -----------------------------------------------------------------------------
//
PedestalsTaskMtcc::~PedestalsTaskMtcc() {
  edm::LogInfo("Commissioning") << "[PedestalsTaskMtcc::PedestalsTaskMtcc] Destructing object...";
}

// -----------------------------------------------------------------------------
//
void PedestalsTaskMtcc::book() {
  edm::LogInfo("Commissioning") << "[PedestalsTaskMtcc::book]";
  
  uint16_t nbins = 256;
  
  string title;
  
  title = SiStripHistoNamingSchemeMtcc::histoTitle( SiStripHistoNamingSchemeMtcc::PEDESTALS, 
						SiStripHistoNamingSchemeMtcc::SUM2, 
						SiStripHistoNamingSchemeMtcc::FED, 
						fedKey(),
						SiStripHistoNamingSchemeMtcc::LLD_CHAN, 
						connection().lldChannel() );
  peds_.meSumOfSquares_ = dqm()->book1D( title, title, nbins, -0.5, nbins*1.-0.5 );
  
  title = SiStripHistoNamingSchemeMtcc::histoTitle( SiStripHistoNamingSchemeMtcc::PEDESTALS, 
						SiStripHistoNamingSchemeMtcc::SUM, 
						SiStripHistoNamingSchemeMtcc::FED, 
						fedKey(),
						SiStripHistoNamingSchemeMtcc::LLD_CHAN, 
						connection().lldChannel() );
  peds_.meSumOfContents_ = dqm()->book1D( title, title, nbins, -0.5, nbins*1.-0.5 );

  title = SiStripHistoNamingSchemeMtcc::histoTitle( SiStripHistoNamingSchemeMtcc::PEDESTALS, 
						SiStripHistoNamingSchemeMtcc::NUM, 
						SiStripHistoNamingSchemeMtcc::FED, 
						fedKey(),
						SiStripHistoNamingSchemeMtcc::LLD_CHAN, 
						connection().lldChannel() );
  peds_.meNumOfEntries_ = dqm()->book1D( title, title, nbins, -0.5, nbins*1.-0.5 );

  title = SiStripHistoNamingSchemeMtcc::histoTitle( SiStripHistoNamingSchemeMtcc::PEDESTALS, 
						SiStripHistoNamingSchemeMtcc::RAWNOISE, 
						SiStripHistoNamingSchemeMtcc::FED, 
						fedKey(),
						SiStripHistoNamingSchemeMtcc::LLD_CHAN, 
						connection().lldChannel() );
  peds_.meRawNoise_ = dqm()->book1D( title, title, nbins, -0.5, nbins*1.-0.5 );

  title = SiStripHistoNamingSchemeMtcc::histoTitle( SiStripHistoNamingSchemeMtcc::PEDESTALS, 
						SiStripHistoNamingSchemeMtcc::CMNOISE, 
						SiStripHistoNamingSchemeMtcc::FED, 
						fedKey(),
						SiStripHistoNamingSchemeMtcc::LLD_CHAN, 
						connection().lldChannel() );
  peds_.meCommonModeSubtractedNoise_ = dqm()->book1D( title, title, nbins, -0.5, nbins*1.-0.5 );
  
  title = SiStripHistoNamingSchemeMtcc::histoTitle( SiStripHistoNamingSchemeMtcc::PEDESTALS, 
						SiStripHistoNamingSchemeMtcc::PEDS, 
						SiStripHistoNamingSchemeMtcc::FED, 
						fedKey(),
						SiStripHistoNamingSchemeMtcc::LLD_CHAN, 
						connection().lldChannel() );
  peds_.mePedestals_ = dqm()->book1D( title, title, nbins, -0.5, nbins*1.-0.5 );
  
  peds_.vSumOfSquares_.resize(nbins,0);
  peds_.vSumOfSquaresOverflow_.resize(nbins,0);
  peds_.vSumOfContents_.resize(nbins,0);
  peds_.vNumOfEntries_.resize(nbins,0);
    
}

// -----------------------------------------------------------------------------
//
void PedestalsTaskMtcc::fill( const SiStripEventSummary& summary,
			  const edm::DetSet<SiStripRawDigi>& digis ) {
  LogDebug("Commissioning") << "[PedestalsTaskMtcc::fill]";

  tot_event_ ++;

  //  cout << "filling event " << tot_event_ << " det " << digis.id << " data size " << digis.data.size() << endl;

  if ( digis.data.size() != peds_.vNumOfEntries_.size() ) {
    edm::LogError("Commissioning") << "[PedestalsTaskMtcc::fill]" 
				   << " Unexpected number of digis! " 
				   << digis.data.size(); 
  }
  uint16_t nbins = STRIPPERCOUPLE;
  // Check number of digis
  //  uint16_t nbins = peds_.vNumOfEntries_.size();
  if ( digis.data.size() < nbins ) { nbins = digis.data.size(); }
  //float cmn[2] = {0, 0};
  if (tot_event_ > getUpdateFreq()){  // if pedestals are already initalized common mode callulation starts
    
    
    cmn_[0] = 0;
    cmn_[1] = 0;
    int strip_counter[2] = {0, 0};
    for (uint16_t ibin_cmn = 0; ibin_cmn < nbins; ibin_cmn++){  // loop over the strips ro calculate common mode
      int apv;
      if (ibin_cmn < nbins/2) apv = 0;
      else if (ibin_cmn >= nbins/2) apv = 1;
      
      if ( (abs(digis.data[ibin_cmn].adc()-peds_.mePedestals_->getBinContent(ibin_cmn+1)) 
	  < CUTAVOIDSIGNAL * peds_.meRawNoise_->getBinContent(ibin_cmn+1)) 
	  &&
	  (theBadStripFinder_.downloadFlag(ibin_cmn) == 0) ) {
	cmn_[apv] += ((float) digis.data[ibin_cmn].adc()-peds_.mePedestals_->getBinContent(ibin_cmn+1));
	strip_counter[apv]++;
      }
    
    }
    cmn_[0] = cmn_[0]/((strip_counter[0]>0) ? strip_counter[0]:1);
    cmn_[1] = cmn_[1]/((strip_counter[1]>0) ? strip_counter[1]:1);
    LogDebug("Commissioning") << 
		"CMN0 = " << cmn_[0] << " strip 0 " << strip_counter[0] << 
		" CMN1 = " << cmn_[1] << " strip 1 " << strip_counter[1];
    // Fill vectors
    for ( uint16_t ibin = 0; ibin < nbins; ibin++ ) {
     if  ( (abs(digis.data[ibin].adc()-peds_.mePedestals_->getBinContent(ibin+1))
          < CUTAVOIDSIGNAL * peds_.meRawNoise_->getBinContent(ibin+1) )  // avoid possible clusters
	&&
	(theBadStripFinder_.downloadFlag(ibin) == 0) ){       
       //if (theBadStripFinder_.downloadFlag(ibin) == 0){     //check if it is a good strip
         peds_.vSumOfSquares_[ibin] += digis.data[ibin].adc() * digis.data[ibin].adc();
         peds_.vSumOfContents_[ibin] += digis.data[ibin].adc();
         peds_.vNumOfEntries_[ibin]++;
         int apv;
         if (ibin < nbins/2) apv = 0;
         else if (ibin >= nbins/2) apv = 1;
         peds_.vSumOfCMSContents_[ibin] += (float)digis.data[ibin].adc() - cmn_[apv];
         peds_.vSumOfCMSSquares_[ibin] += ((float)digis.data[ibin].adc() - cmn_[apv])*
					((float)digis.data[ibin].adc() - cmn_[apv]);
       //}
     }
     //else theBadStripFinder_.newPossibleSignal(ibin);
    }
  }
  else {  // if we are in the first iteration we have to initalize Pedestals and Raw noise
    for ( uint16_t ibin = 0; ibin < nbins; ibin++ ) {   // loop over the strips to fill vectors
	peds_.vSumOfSquares_[ibin] += digis.data[ibin].adc() * digis.data[ibin].adc();
	peds_.vSumOfContents_[ibin] += digis.data[ibin].adc();
	peds_.vNumOfEntries_[ibin]++;
    }
  }
}

// -----------------------------------------------------------------------------
//
void PedestalsTaskMtcc::update() {
  LogDebug("Commissioning") << "[PedestalsTaskMtcc::update]";
  
  //cout << "Updating at event " <<  tot_event_ << " *** Update Frequency " << getUpdateFreq() << endl;

  //cout << "Inizio l'update" << endl;
  
  //  updateHistoSet( peds_ );

  if (tot_event_ > getUpdateFreq()){
     theBadStripFinder_.updatePedestals(peds_.mePedestals_);
     theBadStripFinder_.updateNoise(peds_.meCommonModeSubtractedNoise_);
     theBadStripFinder_.updateEntries(peds_.meNumOfEntries_);
     theBadStripFinder_.updateTotalEvents(tot_event_ - getUpdateFreq());
     //theBadStripFinder_.getCoupleMeanNoise();
     theBadStripFinder_.check();
  }

  for ( uint16_t ibin = 0; ibin < peds_.vNumOfEntries_.size(); ibin++ ) {
    peds_.meSumOfSquares_->setBinContent( ibin+1, peds_.vSumOfSquares_[ibin] );
    peds_.meSumOfContents_->setBinContent( ibin+1, peds_.vSumOfContents_[ibin] );
    peds_.meNumOfEntries_->setBinContent( ibin+1, peds_.vNumOfEntries_[ibin] );
  }


  //uint16_t nbins = STRIPPERCOUPLE;
  for ( uint16_t ibin = 0; ibin < peds_.vNumOfEntries_.size(); ibin++ ) {
    float entries = peds_.vNumOfEntries_[ibin];
    if (entries > 0) {
      peds_.meRawNoise_->setBinContent( ibin+1, sqrt((peds_.vSumOfSquares_[ibin]/entries-
						      (peds_.vSumOfContents_[ibin]/entries)*
						      (peds_.vSumOfContents_[ibin]/entries))) );
      peds_.mePedestals_->setBinContent( ibin+1, peds_.vSumOfContents_[ibin]/entries);
      //     peds_.meCommonModeSubtractedNoise_->setBinContent( ibin+1, sqrt((peds_.vSumOfCMSSquares_[ibin]/entries)+
      // 								(peds_.vSumOfContents_[ibin]/entries)
      // 								*(peds_.vSumOfContents_[ibin]/entries) -
      // 								2*(peds_.vSumOfContents_[ibin]/entries)*
      // 									    (peds_.vSumOfCMSContents_[ibin]/entries)) );

      //Added by Domenico
      if (tot_event_ > getUpdateFreq()){ // if not in the first iteration fills common mode subtracted histos
  
	//theBadStripFinder_.check(ibin);  
        if (theBadStripFinder_.downloadFlag(ibin) == 0){ 
           peds_.meCommonModeSubtractedNoise_->setBinContent( ibin+1, sqrt(
							  peds_.vSumOfCMSSquares_[ibin]/(entries - getUpdateFreq())
							   -
							  peds_.vSumOfCMSContents_[ibin]/(entries - getUpdateFreq())* 
							  peds_.vSumOfCMSContents_[ibin]/(entries - getUpdateFreq())
							  ) // the first updateFreq_ events are not user for this calulation 
							 );
        }
	else peds_.meCommonModeSubtractedNoise_->setBinContent( ibin+1, 0);
      }
    } else {
      //      cout << "trovato un canale con 0 entries " << ibin <<" " << entries<<  endl;
    }
  }
}


