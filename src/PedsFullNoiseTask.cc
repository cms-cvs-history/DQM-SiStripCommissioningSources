
#include "DQM/SiStripCommissioningSources/interface/PedsFullNoiseTask.h"

#include "DataFormats/SiStripCommon/interface/SiStripConstants.h"
#include "DataFormats/SiStripCommon/interface/SiStripHistoTitle.h"
#include "DQM/SiStripCommon/interface/ExtractTObject.h"
#include "DQM/SiStripCommon/src/UpdateTProfile.cc"
#include "DQMServices/Core/interface/DQMStore.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"

#include "boost/lexical_cast.hpp"


// -----------------------------------------------------------------------------
//
PedsFullNoiseTask::PedsFullNoiseTask(DQMStore * dqm, const FedChannelConnection & conn, const edm::ParameterSet & pset) :
  CommissioningTask(dqm, conn, "PedsFullNoiseTask"),
  nstrips_(256)
{
  LogTrace(sistrip::mlDqmSource_)
    << "[PedsFullNoiseTask::" << __func__ << "]"
    << " Constructing object...";
  edm::ParameterSet params = pset.getParameter<edm::ParameterSet>("PedsFullNoiseParameters");
  nskip_ = params.getParameter<int>("NrEvToSkipAtStart");
  skipped_ = false,
  nevpeds_ = params.getParameter<int>("NrEvForPeds");
  pedsdone_ = false;
  nadcnoise_ = params.getParameter<int>("NrPosBinsNoiseHist");
  fillnoiseprofile_ = params.getParameter<bool>("FillNoiseProfile");
}

// -----------------------------------------------------------------------------
//
PedsFullNoiseTask::~PedsFullNoiseTask()
{
  LogTrace(sistrip::mlDqmSource_)
    << "[PedsFullNoiseTask::" << __func__ << "]"
    << " Destructing object...";
}

// -----------------------------------------------------------------------------
//
void PedsFullNoiseTask::book()
{

  LogTrace(sistrip::mlDqmSource_) << "[PedsFullNoiseTask::" << __func__ << "]";

  // pedestal profile histo
  pedhist_.isProfile_ = true;
  pedhist_.explicitFill_ = false;
  if (!pedhist_.explicitFill_) {
    pedhist_.vNumOfEntries_.resize(nstrips_,0);
    pedhist_.vSumOfContents_.resize(nstrips_,0);
    pedhist_.vSumOfSquares_.resize(nstrips_,0);
  }
  std::string titleped = SiStripHistoTitle( sistrip::EXPERT_HISTO,
                                            sistrip::PEDS_FULL_NOISE,
                                            sistrip::FED_KEY,
                                            fedKey(),
                                            sistrip::LLD_CHAN,
                                            connection().lldChannel(),
                                            sistrip::extrainfo::pedestals_).title();
  pedhist_.histo( dqm()->bookProfile( titleped, titleped,
                                      nstrips_, -0.5, nstrips_*1.-0.5,
                                      1025, 0., 1025. ) );

  // Noise profile
  noiseprof_.isProfile_ = true;
  noiseprof_.explicitFill_ = false;
  if (!noiseprof_.explicitFill_) {
    noiseprof_.vNumOfEntries_.resize(nstrips_,0);
    noiseprof_.vSumOfContents_.resize(nstrips_,0);
    noiseprof_.vSumOfSquares_.resize(nstrips_,0);
  }
  std::string titlenoise = SiStripHistoTitle( sistrip::EXPERT_HISTO, 
                                              sistrip::PEDS_FULL_NOISE, 
                                              sistrip::FED_KEY, 
                                              fedKey(),
                                              sistrip::LLD_CHAN, 
                                              connection().lldChannel(),
                                              sistrip::extrainfo::noiseProfile_ ).title();
  noiseprof_.histo( dqm()->bookProfile( titlenoise, titlenoise,
                                        nstrips_, -0.5, nstrips_*1.-0.5,
                                        1025, 0., 1025. ) );

  // noise 2D compact histo
  noisehist_.explicitFill_ = false;
  if (!noisehist_.explicitFill_) {
    noisehist_.vNumOfEntries_.resize((nstrips_+2)*2*(nadcnoise_+2), 0);
  }
  std::string titlenoise2d = SiStripHistoTitle( sistrip::EXPERT_HISTO, 
                                                sistrip::PEDS_FULL_NOISE, 
                                                sistrip::FED_KEY, 
                                                fedKey(),
                                                sistrip::LLD_CHAN, 
                                                connection().lldChannel(),
                                                sistrip::extrainfo::noise2D_ ).title();
  noisehist_.histo( dqm()->book2S( titlenoise2d, titlenoise2d,
                                   2*nadcnoise_, -nadcnoise_, nadcnoise_,
                                   nstrips_, -0.5, nstrips_*1.-0.5 ) );
  hist2d_ = (TH2S *) noisehist_.histo()->getTH2S();

}

// -----------------------------------------------------------------------------
//
void PedsFullNoiseTask::fill( const SiStripEventSummary & summary,
                         const edm::DetSet<SiStripRawDigi> & digis )
{

  // Check number of digis
  uint16_t nbins = digis.data.size();
  if (nbins != nstrips_) {
    edm::LogWarning(sistrip::mlDqmSource_)
      << "[PedsFullNoiseTask::" << __func__ << "]"
      << " " << nstrips_ << " digis expected, but got " << nbins << ". Skipping.";
    return;
  }

  // get the event number of the first event, not necessarily 1 (parallel processing on FUs)
  static uint32_t firstev = summary.event();

  // skipping events
  if (!skipped_) {
    if (summary.event() - firstev < nskip_) {
      return;
    } else { // when all events are skipped
      skipped_ = true;
      if (nskip_ > 0) LogTrace(sistrip::mlDqmSource_) << "[PedsFullNoiseTask::" << __func__ << "]"
                                                      << " Done skipping events. Now starting pedestals.";
    }
  }

  // determine pedestals - decoupled from noise determination
  if (!pedsdone_) {
    if (summary.event() - firstev < nskip_ + nevpeds_) {
      // estimate peds roughly
      for ( uint16_t istrip = 0; istrip < nstrips_; ++istrip ) {
        updateHistoSet( pedhist_, istrip, digis.data[istrip].adc() );
      }
      return;
    } else { // when pedestals are done
      pedsdone_ = true;
      peds_.clear();
      for ( uint16_t iapv = 0; iapv < 2; ++iapv ) {
        for ( uint16_t ibin = 0; ibin < 128; ++ibin ) {
          uint16_t istrip = (iapv*128)+ibin;
          peds_.push_back(static_cast<int16_t>(1.*pedhist_.vSumOfContents_.at(istrip)/pedhist_.vNumOfEntries_.at(istrip)));
        }
      }
      LogTrace(sistrip::mlDqmSource_) << "[PedsFullNoiseTask::" << __func__ << "]"
                                      << " Rough pedestals done. Now starting noise measurements.";
    }
  }

  // fill (or not) the old-style niose profile
  if (fillnoiseprofile_) {
    // Calc common mode for both APVs
    std::vector<int32_t> cm; cm.resize(2, 0);
    std::vector<uint16_t> adc;
    for ( uint16_t iapv = 0; iapv < 2; iapv++ ) { 
      adc.clear(); adc.reserve(128);
      for ( uint16_t ibin = 0; ibin < 128; ibin++ ) { 
        if ( (iapv*128)+ibin < nbins ) { 
          adc.push_back( digis.data.at((iapv*128)+ibin).adc() );
        }
      }
      sort( adc.begin(), adc.end() ); 
      // take median as common mode
      uint16_t index = adc.size()%2 ? adc.size()/2 : adc.size()/2-1;
      cm[iapv] = static_cast<int16_t>(adc[index]);
    }
    // 1D noise profile - see also further processing in the update() method
    for ( uint16_t istrip = 0; istrip < nstrips_; ++istrip ) {
      // calculate the noise in the old way, by subtracting the common mode, but without pedestal subtraction
      int16_t noiseval = static_cast<int16_t>(digis.data.at(istrip).adc()) - cm[istrip/128];
      updateHistoSet( noiseprof_, istrip, noiseval );
    }
  }

  // 2D noise histogram
  std::vector<int16_t> noisevals;
  std::vector<int16_t> noisevalssorted;
  for ( uint16_t iapv = 0; iapv < 2; ++iapv ) { 
    noisevals.clear(); noisevals.reserve(128);
    noisevalssorted.clear();
    for ( uint16_t ibin = 0; ibin < 128; ++ibin ) {
      uint16_t istrip = (iapv*128)+ibin;
      // store the pedestal
      updateHistoSet( pedhist_, istrip, digis.data.at(istrip).adc() );
      // calculate the noise after subtracting the pedestal
      noisevals[ibin] = static_cast<int16_t>(digis.data.at(istrip).adc()) - peds_.at(istrip);
      // now we still have a possible constant shift of the adc values with respect to 0, so we prepare to calculate the median of this shift
      noisevalssorted.push_back( noisevals[ibin] );
    }
    // get index for median determination for remaining CM subtraction
    sort( noisevalssorted.begin(), noisevalssorted.end() ); 
    uint16_t index = noisevalssorted.size()%2 ? noisevalssorted.size()/2 : noisevalssorted.size()/2-1;
    // now loop again to calculate the CM+pedestal subtracted noise values
    for ( uint16_t ibin = 0; ibin < 128; ++ibin ) {
      uint16_t istrip = (iapv*128)+ibin;
      // subtract the remaining common mode after subtraction of the rough pedestals
      int16_t noiseval = noisevals[ibin] - noisevalssorted[index];
      // retrieve the linear binnr through the histogram
      uint32_t binnr = hist2d_->GetBin(noiseval+nadcnoise_, istrip+1);
      // store the noise value in the 2D histo
      updateHistoSet( noisehist_, binnr ); // no value, so weight 1
    }
  }

}

// -----------------------------------------------------------------------------
//
void PedsFullNoiseTask::update()
{

  // pedestals 
  updateHistoSet( pedhist_ );

  if (fillnoiseprofile_) {
  // noise profile (does not use HistoSet directly, as want to plot noise as "contents", not "error")
  TProfile* histo = ExtractTObject<TProfile>().extract( noiseprof_.histo() );
    for ( uint16_t ii = 0; ii < noiseprof_.vNumOfEntries_.size(); ++ii ) {
      float mean    = 0.;
      float spread  = 0.;
      float entries = noiseprof_.vNumOfEntries_[ii];
      if ( entries > 0. ) {
        mean = noiseprof_.vSumOfContents_[ii] / entries;
        spread = sqrt( noiseprof_.vSumOfSquares_[ii] / entries - mean * mean );  // nice way to calculate std dev: Sum (x-<x>)^2 / N
      }
      float error = spread / sqrt(entries); // uncertainty on std.dev. when no uncertainty on mean
      UpdateTProfile::setBinContent( histo, ii+1, entries, spread, error );
    }
  }

  // noise 2D histo
  updateHistoSet( noisehist_ );

}
// -----------------------------------------------------------------------------
