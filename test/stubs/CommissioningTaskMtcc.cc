#include "DQM/SiStripCommissioningSources/test/stubs/CommissioningTaskMtcc.h"
#include "DQMServices/Core/interface/DaqMonitorBEInterface.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "DataFormats/SiStripDetId/interface/SiStripControlKey.h"

#include <iostream>
#include <string> 

using namespace std;

// -----------------------------------------------------------------------------
//
CommissioningTaskMtcc::CommissioningTaskMtcc( DaqMonitorBEInterface* dqm,
				      const FedChannelConnection& conn,
				      const string& my_name ) :
  dqm_(dqm),
  updateFreq_(0),
  fillCntr_(0),
  connection_(conn),
  fedKey_(0),
  fecKey_(0),
  booked_(false),
  fedChannel_(0,0),
  myName_(my_name)
{
  LogDebug("Commissioning") << "[CommissioningTaskMtcc::CommissioningTaskMtcc]" 
			    << " Constructing object for FED id/ch " 
			    << connection_.fedId() << "/" 
			    << connection_.fedCh();
  fedKey_ = SiStripReadoutKey::key( connection_.fedId(), 
				    connection_.fedCh() );
  fecKey_ = SiStripControlKey::key( connection_.fecCrate(),
				    connection_.fecSlot(),
				    connection_.fecRing(),
				    connection_.ccuAddr(),
				    connection_.ccuChan(),
				    connection_.lldChannel() );
}

// -----------------------------------------------------------------------------
//
CommissioningTaskMtcc::~CommissioningTaskMtcc() {
  LogDebug("Commissioning") << "[CommissioningTaskMtcc::CommissioningTaskMtcc]" 
			    << " Destructing object for FED id/ch " 
			    << connection_.fedId() << "/" 
			    << connection_.fedCh();
}

// -----------------------------------------------------------------------------
//
void CommissioningTaskMtcc::book() {
  edm::LogError("Commissioning") << "[CommissioningTaskMtcc::book]"
				 << " This virtual method should always be over-ridden!";
}

// -----------------------------------------------------------------------------
//
void CommissioningTaskMtcc::bookHistograms() {
  edm::LogInfo("Commissioning") << "[CommissioningTaskMtcc::book]"
				<< " Booking histograms for FED id/ch: "
				<< connection_.fedId() << "/"
				<< connection_.fedCh();
  book();
  booked_ = true;

//   SiStripHistoNamingScheme::HistoTitle histo_title = SiStripHistoNamingScheme::histoTitle( title );
//   cout << "HistoTitle components: " 
//        << SiStripHistoNamingScheme::task( histo_title.task_ ) << " " 
//        << SiStripHistoNamingScheme::contents( histo_title.contents_ ) << " " 
//        << SiStripHistoNamingScheme::keyType( histo_title.keyType_ ) << " " 
//        << setfill('0') << setw(8) << hex << histo_title.keyValue_ << dec << " " 
//        << SiStripHistoNamingScheme::granularity( histo_title.granularity_ ) << " " 
//        << histo_title.channel_ << " " 
//        << histo_title.extraInfo_ << endl;

}

// -----------------------------------------------------------------------------
//
void CommissioningTaskMtcc::fillHistograms( const SiStripEventSummary& summary,
					const edm::DetSet<SiStripRawDigi>& digis ) {
  LogDebug("Commissioning") << "[CommissioningTaskMtcc::fillHistograms]";
  if ( !booked_ ) {
    edm::LogError("Commissioning") << "[CommissioningTaskMtcc::fillHistograms]"
				   << " Attempting to fill histos that haven't been booked yet!";
    return;
  }
  fillCntr_++;
  fill( summary, digis ); 
  if ( updateFreq_ ) { if ( !(fillCntr_%updateFreq_) ) update(); }
}

// -----------------------------------------------------------------------------
//
void CommissioningTaskMtcc::updateHistograms() {
  LogDebug("Commissioning") << "[CommissioningTaskMtcc::updateHistograms]"
			    << " Updating histograms...";
  update();
}

// -----------------------------------------------------------------------------
//
void CommissioningTaskMtcc::updateHistoSet( HistoSet& histo_set, 
					const uint32_t& bin,
					const uint32_t& value ) {
  
  if ( bin >= histo_set.vNumOfEntries_.size() ) { 
    edm::LogError("Commissioning") << "[VpspScanTask::fill]" 
				   << "  Unexpected bin! " << bin;
    return;
  }
  
  uint32_t remaining = 0xFFFFFFFF - histo_set.vSumOfSquares_[bin];
  uint32_t squared_value = value * value;

  // Set squared contents (and overflow if necessary)
  if ( remaining <= squared_value ) { 
    histo_set.vSumOfSquaresOverflow_[bin] +=1;
    histo_set.vSumOfSquares_[bin] += (squared_value-remaining);
  } else { 
    histo_set.vSumOfSquares_[bin] += squared_value;
  }
  
  // Set contents and entries
  histo_set.vSumOfContents_[bin] += value;
  histo_set.vNumOfEntries_[bin]++;

//   cout << bin << " " 
//        << histo_set.vSumOfSquaresOverflow_[bin] << " " 
// 	 << histo_set.vSumOfSquares_[bin] << " " 
// 	 << histo_set.vSumOfContents_[bin] << " " 
// 	 << histo_set.vNumOfEntries_[bin] << endl;
  
}

// -----------------------------------------------------------------------------
//
void CommissioningTaskMtcc::updateHistoSet( HistoSet& histo_set ) {
  
  if ( !histo_set.meSumOfSquares_ ||
       !histo_set.meSumOfContents_ ||
       !histo_set.meNumOfEntries_ ) {
    edm::LogError("Commissioning") << "[CommissioningTaskMtcc::updateHistoSet]" 
				   << "  NULL pointer to ME!";
    return;
  }
  
  for ( uint32_t ibin = 0; ibin < histo_set.vNumOfEntries_.size(); ibin++ ) {
    histo_set.meSumOfSquares_->setBinContent( ibin+1, histo_set.vSumOfSquares_[ibin]*1. );
    histo_set.meSumOfContents_->setBinContent( ibin+1, histo_set.vSumOfContents_[ibin]*1. );
    histo_set.meNumOfEntries_->setBinContent( ibin+1, histo_set.vNumOfEntries_[ibin]*1. );
    for ( uint32_t ientries = 0; ientries < histo_set.vSumOfSquaresOverflow_[ibin]; ientries++ ) {
      histo_set.meSumOfSquares_->Fill( ibin+1, (float)0xFFFFFFFF ); 
      histo_set.meSumOfSquares_->Fill( ibin+1, 1. );
    }
  }
  
}

