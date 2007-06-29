#include "DQM/SiStripCommissioningSources/interface/OptoScanTask.h"
#include "DataFormats/SiStripCommon/interface/SiStripConstants.h"
#include "DataFormats/SiStripCommon/interface/SiStripHistoTitle.h"
#include "DQMServices/Core/interface/DaqMonitorBEInterface.h"
#include "CalibFormats/SiStripObjects/interface/SiStripFecCabling.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include <algorithm>
#include <math.h>

using namespace sistrip;

// -----------------------------------------------------------------------------
//
OptoScanTask::OptoScanTask( DaqMonitorBEInterface* dqm,
			    const FedChannelConnection& conn ) :
  CommissioningTask( dqm, conn, "OptoScanTask" ),
  opto_()
{}

// -----------------------------------------------------------------------------
//
OptoScanTask::~OptoScanTask() {
}

// -----------------------------------------------------------------------------
//
void OptoScanTask::book() {

  uint16_t nbins = 51; //@@ correct?
  uint16_t gains = 4;

  std::string title;

  // Resize std::vector of "Histo sets" to accommodate the 4 different gain
  // settings and the two different digital levels ("0" and "1").
  opto_.resize( gains );
  for ( uint16_t igain = 0; igain < opto_.size(); igain++ ) { opto_[igain].resize(2); }

  // Book histos and resize std::vectors within "Histo sets"
  for ( uint16_t igain = 0; igain < opto_.size(); igain++ ) { 
    for ( uint16_t ilevel = 0; ilevel < 2; ilevel++ ) { 

      //@@ new histo: rms of "baseline" (280 samples mius 8 for tick marks) as func of bias setting 
      
      std::stringstream extra_info; 
      extra_info << sistrip::gain_ << igain 
		 << sistrip::digital_ << ilevel;
      
      title = SiStripHistoTitle( sistrip::EXPERT_HISTO, 
				 sistrip::OPTO_SCAN, 
				 sistrip::FED_KEY, 
				 fedKey(),
				 sistrip::LLD_CHAN, 
				 connection().lldChannel(),
				 extra_info.str() ).title();
    
      opto_[igain][ilevel].histo_  = dqm()->bookProfile( title, title, 
							 nbins, -0.5, nbins*1.-0.5,
							 1025, 0., 1025. );

      opto_[igain][ilevel].vNumOfEntries_.resize(nbins,0);
      opto_[igain][ilevel].vSumOfContents_.resize(nbins,0);
      opto_[igain][ilevel].vSumOfSquares_.resize(nbins,0);
      
    }
  }
  
}

// -----------------------------------------------------------------------------
//
/*
  improve finding tick mark and baseline
  keep baseline "noise" along with tick hieght and base
  new histo plotting noise +/- spread in noise vs bias
*/
void OptoScanTask::fill( const SiStripEventSummary& summary,
			 const edm::DetSet<SiStripRawDigi>& digis ) {

  //@@ if scope mode length is in trigger fed, then 
  //@@ can add check here on number of digis
  if ( digis.data.empty() ) {
    edm::LogWarning(mlDqmSource_)
      << "[OptoScanTask::" << __func__ << "]"
      << " Unexpected number of digis! " 
      << digis.data.size(); 
  } else {
    
    // Retrieve opt bias and gain setting from SiStripEventSummary
    uint16_t gain = summary.lldGain();
    uint16_t bias = summary.lldBias();
    if ( gain >= opto_.size() ) { 
      opto_.resize( gain );
      for ( uint16_t igain = 0; igain < opto_.size(); igain++ ) { 
	if ( opto_[gain].size() != 2 ) { opto_[gain].resize( 2 ); }
      }
      edm::LogWarning(mlDqmSource_)  
	<< "[OptoScanTask::" << __func__ << "]"
	<< " Unexpected gain value! " << gain;
    }
//     LogTrace(mlDqmSource_) 
//       << "[OptoScanTask::" << __func__ << "]"
//       << " Gain: " << opto.first 
//       << " Bias: " << opto.second;
    
    // Find digital "0" and digital "1" levels from tick marks within scope mode data
    std::pair< uint16_t, uint16_t > digital_range;
    locateTicks( digis, digital_range, false );
//     LogTrace(mlDqmSource_)
//       << "[OptoScanTask::" << __func__ << "]"
//       << " Digital \"0\" level: " << digital_range.first 
//       << " Digital \"1\" level: " << digital_range.second;
    
    // Digital "0"
    if ( digital_range.first <= 1024 ) {
      updateHistoSet( opto_[gain][0], bias, digital_range.first );
    }
    
    // Digital "1"
    if ( digital_range.second <= 1024 ) {
      updateHistoSet( opto_[gain][1], bias, digital_range.second );
    }

  }

}


// -----------------------------------------------------------------------------
//
void OptoScanTask::update() {
  
  for ( uint16_t igain = 0; igain < opto_.size(); igain++ ) { 
    for ( uint16_t ilevel = 0; ilevel < opto_[igain].size(); ilevel++ ) { 
      updateHistoSet( opto_[igain][ilevel] );
    }
  }

}

// -----------------------------------------------------------------------------
//
void OptoScanTask::locateTicks( const edm::DetSet<SiStripRawDigi>& digis, 
				std::pair< uint16_t, uint16_t >& digital_range, 
				bool first_tick_only ) {

  //@@ RUBBISH!!!! simplify!!! find min/max, mid-range and push back into base and tick vectors (based on mid-range)
  
  // Copy ADC values and sort
  std::vector<uint16_t> adc; adc.reserve( digis.data.size() ); 
  for ( uint16_t iadc = 0; iadc < digis.data.size(); iadc++ ) { adc.push_back( digis.data[iadc].adc() ); }
  sort( adc.begin(), adc.end() );
  uint16_t size = adc.size();

  // Find mean and error
  float sum = 0, sum2 = 0, num = 0;
  for ( uint16_t iadc = static_cast<uint16_t>( 0.1*size ); 
	iadc < static_cast<uint16_t>( 0.9*adc.size() ); iadc++ ) {
    sum  += adc[iadc];
    sum2 += adc[iadc] * adc[iadc];
    num++;
  }
  float mean = 0, mean2 = 0, sigma = 0;
  if ( num ) { 
    mean = sum / num; 
    mean2 = sum2 / num;
    if ( mean2 > mean*mean ) { sigma = sqrt( mean2 - mean*mean ); }
  }

  // Identify samples belonging to "baseline" and "tick marks"
  float threshold = mean + sigma * 5.;
  //bool found_first_tick = false;
  std::vector<uint32_t> baseline; 
  std::vector<uint32_t> tickmark; 
  for ( uint16_t iadc = 0; iadc < adc.size(); iadc++ ) { 
    if ( adc[iadc] > threshold ) { tickmark.push_back( adc[iadc] ); }
    else { baseline.push_back( adc[iadc] ); }
  }

  // Define digital "0" and "1" levels (median values)
  if ( !baseline.empty() ) { 
    uint16_t sample = baseline.size()%2 ? baseline.size()/2 : baseline.size()/2-1;
    digital_range.first = baseline[ sample ]; // median
  } else { digital_range.first = 1025; }

  if ( !tickmark.empty() ) { 
    uint16_t sample = tickmark.size()%2 ? tickmark.size()/2 : tickmark.size()/2-1;
    digital_range.second = tickmark[ sample ]; // median
  } else { digital_range.second = 1025; }
  
}



