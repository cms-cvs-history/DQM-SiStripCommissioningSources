#include "DQM/SiStripCommissioningSources/test/stubs/SiStripHistoNamingSchemeMtcc.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include <iostream>
#include <sstream>
#include <iomanip>

// -----------------------------------------------------------------------------
// definition of static data members
const string SiStripHistoNamingSchemeMtcc::root_ = "/";
const string SiStripHistoNamingSchemeMtcc::top_ = "SiStrip";
const string SiStripHistoNamingSchemeMtcc::dir_ = "/";
const string SiStripHistoNamingSchemeMtcc::sep_ = "_";
const uint16_t SiStripHistoNamingSchemeMtcc::all_ = 0xFFFF;

const string SiStripHistoNamingSchemeMtcc::controlView_ = "ControlView";
const string SiStripHistoNamingSchemeMtcc::fecCrate_ = "FecCrate";
const string SiStripHistoNamingSchemeMtcc::fecSlot_ = "FecSlot";
const string SiStripHistoNamingSchemeMtcc::fecRing_ = "FecRing";
const string SiStripHistoNamingSchemeMtcc::ccuAddr_ = "CcuAddr";
const string SiStripHistoNamingSchemeMtcc::ccuChan_ = "CcuChan";

const string SiStripHistoNamingSchemeMtcc::readoutView_ = "ReadoutView";
const string SiStripHistoNamingSchemeMtcc::fedId_ = "FedId";
const string SiStripHistoNamingSchemeMtcc::fedChannel_ = "FedChannel";

const string SiStripHistoNamingSchemeMtcc::detectorView_ = "DetectorView"; //@@ necessary?

const string SiStripHistoNamingSchemeMtcc::pedestals_ = "Pedestals";
const string SiStripHistoNamingSchemeMtcc::unknownTask_ = "UnknownTask";

const string SiStripHistoNamingSchemeMtcc::fedKey_ = "FedKey";
const string SiStripHistoNamingSchemeMtcc::fecKey_ = "FecKey";
const string SiStripHistoNamingSchemeMtcc::detKey_ = "DetId"; //@@ necessary?
const string SiStripHistoNamingSchemeMtcc::unknownKey_ = "UnknownKey";

const string SiStripHistoNamingSchemeMtcc::sum2_ = "SumOfSquares";
const string SiStripHistoNamingSchemeMtcc::sum_ = "SumOfContents";
const string SiStripHistoNamingSchemeMtcc::num_ = "NumOfEntries";
const string SiStripHistoNamingSchemeMtcc::rawnoise_ = "RawNoise";
const string SiStripHistoNamingSchemeMtcc::cmnoise_ = "CommonModeSubtractedNoise";
const string SiStripHistoNamingSchemeMtcc::peds_ = "Pedestals";
const string SiStripHistoNamingSchemeMtcc::unknownContents_ = "UnknownContents";

const string SiStripHistoNamingSchemeMtcc::lldChan_ = "LldChan";
const string SiStripHistoNamingSchemeMtcc::apvPair_ = "ApvPair";
const string SiStripHistoNamingSchemeMtcc::apv_ = "Apv";
const string SiStripHistoNamingSchemeMtcc::unknownGranularity_ = "UnknownGranularity";

const string SiStripHistoNamingSchemeMtcc::gain_ = "Gain";
const string SiStripHistoNamingSchemeMtcc::digital_ = "Digital";

// -----------------------------------------------------------------------------
//
string SiStripHistoNamingSchemeMtcc::controlPath( uint16_t fec_crate,
					      uint16_t fec_slot,
					      uint16_t fec_ring,
					      uint16_t ccu_addr,
					      uint16_t ccu_chan ) { 
  stringstream folder;
  folder << controlView_;
  if ( fec_crate != all_ ) {
    folder << dir_ << fecCrate_ << fec_crate;
    if ( fec_slot != all_ ) {
      folder << dir_ << fecSlot_ << fec_slot;
      if ( fec_ring != all_ ) {
	folder << dir_ << fecRing_ << fec_ring;
	if ( ccu_addr != all_ ) {
	  folder << dir_ << ccuAddr_ << ccu_addr;
	  if ( ccu_chan != all_ ) {
	    folder << dir_ << ccuChan_ << ccu_chan;
	  }
	}
      }
    }
  }
  LogDebug("DQM") << "[SiStripHistoNamingSchemeMtcc::controlPath]  " << folder.str();
  return folder.str();
}

// -----------------------------------------------------------------------------
//
SiStripHistoNamingSchemeMtcc::ControlPath SiStripHistoNamingSchemeMtcc::controlPath( string directory ) {

  ControlPath path;
  path.fecCrate_ = all_;
  path.fecSlot_ = all_;
  path.fecRing_ = all_;
  path.ccuAddr_ = all_;
  path.ccuChan_ = all_;

  uint16_t index = 0;
  
  // Extract view 
  stringstream ss; ss << controlView_ << dir_;
  uint16_t size = controlView_.size() + dir_.size();
  if ( !directory.compare( index, size, ss.str() ) ) {
    unsigned short index = controlView_.size() + dir_.size();
    // Extract FEC crate
    if ( !directory.compare( index, fecCrate_.size(), fecCrate_ ) ) {
      index += fecCrate_.size();
      string fec_crate( directory, index, directory.find( directory, index ) - index );
      path.fecCrate_ = atoi( fec_crate.c_str() );
      index = directory.find( dir_, index ) + 1;
      // Extract FEC slot
      if ( !directory.compare( index, fecSlot_.size(), fecSlot_ ) ) {
	index += fecSlot_.size();
	string fec_slot( directory, index, directory.find( directory, index ) - index );
	path.fecSlot_ = atoi( fec_slot.c_str() );
	index = directory.find( dir_, index ) + 1;
	// Extract FEC ring
	if ( !directory.compare( index, fecRing_.size(), fecRing_ ) ) {
	  index += fecRing_.size();
	  string fec_ring( directory, index, directory.find( directory, index ) - index );
	  path.fecRing_ = atoi( fec_ring.c_str() );
	  index = directory.find( dir_, index ) + 1;
	  // Extract CCU address
	  if ( !directory.compare( index, ccuAddr_.size(), ccuAddr_ ) ) {
	    index += ccuAddr_.size();
	    string ccu_addr( directory, index, directory.find( directory, index ) - index );
	    path.ccuAddr_ = atoi( ccu_addr.c_str() );
	    index = directory.find( dir_, index ) + 1;
	    // Extract CCU channel
	    if ( !directory.compare( index, ccuChan_.size(), ccuChan_ ) ) {
	      index += ccuChan_.size();
	      string ccu_chan( directory, index, directory.find( directory, index ) - index );
	      path.ccuChan_ = atoi( ccu_chan.c_str() );
	      index = directory.find( dir_, index ) + 1;
	    }
	  }
	}
      }
    }
  } else {
    edm::LogError("DQM") << "[SiStripHistoNamingSchemeMtcc::controlPath]" 
			 << " Unexpected view! Not " << controlView_ << "!";
  }
 
  LogDebug("DQM") << "[SiStripHistoNamingSchemeMtcc::controlPath]" 
		  << "  FecCrate: " << path.fecCrate_
		  << "  FecSlot: " << path.fecSlot_
		  << "  FecRing: " << path.fecRing_
		  << "  CcuAddr: " << path.ccuAddr_
		  << "  CcuChan: " << path.ccuChan_;
  return path;
  
}

// -----------------------------------------------------------------------------
//
string SiStripHistoNamingSchemeMtcc::readoutPath( uint16_t fed_id,
					      uint16_t fed_channel ) { 
  
  stringstream folder;
  folder << readoutView_;
  if ( fed_id != all_ ) {
    folder << dir_ << fedId_ << fed_id;
    if ( fed_channel != all_ ) {
      folder << dir_ << fedChannel_ << fed_channel;
    }
  }
  return folder.str();
}

// -----------------------------------------------------------------------------
// 
string SiStripHistoNamingSchemeMtcc::histoTitle( Task        histo_task, 
					     Contents    histo_contents,
					     KeyType     key_type,
					     uint32_t    key_value,
					     Granularity granularity,
					     uint16_t    channel,
					     string      extra_info ) {
  
  stringstream title;

  stringstream task;
  if      ( histo_task == SiStripHistoNamingSchemeMtcc::PEDESTALS )     { task << pedestals_; }
  else if ( histo_task == SiStripHistoNamingSchemeMtcc::NO_TASK )       { /* add nothing */ }
  else if ( histo_task == SiStripHistoNamingSchemeMtcc::UNKNOWN_TASK )  { task << sep_ << unknownTask_; }
  else { edm::LogError("DQM") << "[SiStripHistoNamingSchemeMtcc::histoTitle]"
			      << " Unexpected histogram task!"; }
  title << task.str();

  stringstream contents;
  if      ( histo_contents == SiStripHistoNamingSchemeMtcc::SUM2 )             { contents << sep_ << sum2_; }
  else if ( histo_contents == SiStripHistoNamingSchemeMtcc::SUM )              { contents << sep_ << sum_; }
  else if ( histo_contents == SiStripHistoNamingSchemeMtcc::NUM )              { contents << sep_ << num_; }
  else if ( histo_contents == SiStripHistoNamingSchemeMtcc::RAWNOISE )              { contents << sep_ << rawnoise_; }
  else if ( histo_contents == SiStripHistoNamingSchemeMtcc::CMNOISE )              { contents << sep_ << cmnoise_; }
  else if ( histo_contents == SiStripHistoNamingSchemeMtcc::PEDS )              { contents << sep_ << peds_; }
  else if ( histo_contents == SiStripHistoNamingSchemeMtcc::COMBINED )         { /* add nothing */ }
  else if ( histo_contents == SiStripHistoNamingSchemeMtcc::UNKNOWN_CONTENTS ) { contents << sep_ << unknownContents_; }
  else { edm::LogError("DQM") << "[SiStripHistoNamingSchemeMtcc::histoTitle]"
			      << " Unexpected histogram contents!"; }
  title << contents.str();
  
  stringstream key;
  if      ( key_type == SiStripHistoNamingSchemeMtcc::FED )         { key << sep_ << fedKey_ << setfill('0') << setw(8) << hex << key_value; }
  else if ( key_type == SiStripHistoNamingSchemeMtcc::FEC )         { key << sep_ << fecKey_ << setfill('0') << setw(8) << hex << key_value; }
  else if ( key_type == SiStripHistoNamingSchemeMtcc::DET )         { key << sep_ << detKey_ << setfill('0') << setw(8) << hex << key_value; }
  else if ( key_type == SiStripHistoNamingSchemeMtcc::NO_KEY )      { /* add nothing */ }
  else if ( key_type == SiStripHistoNamingSchemeMtcc::UNKNOWN_KEY ) { key << sep_ << unknownKey_; }
  else { edm::LogError("DQM") << "[SiStripHistoNamingSchemeMtcc::histoTitle]"
			      << " Unexpected key type!"; }
  title << key.str();

  stringstream gran;
  if      ( granularity == SiStripHistoNamingSchemeMtcc::LLD_CHAN )     { gran << sep_ << lldChan_ << channel; }
  else if ( granularity == SiStripHistoNamingSchemeMtcc::APV_PAIR )     { gran << sep_ << apvPair_ << channel; }
  else if ( granularity == SiStripHistoNamingSchemeMtcc::APV )          { gran << sep_ << apv_     << channel; }
  else if ( granularity == SiStripHistoNamingSchemeMtcc::MODULE )       { /* add nothing */ }
  else if ( granularity == SiStripHistoNamingSchemeMtcc::UNKNOWN_GRAN ) { gran << sep_ << unknownGranularity_; }
  else { edm::LogError("DQM") << "[SiStripHistoNamingSchemeMtcc::histoTitle]"
			      << " Unexpected granularity!"; }
  title << gran.str();

  if ( extra_info != "" ) { title << sep_ << extra_info; }
  
  LogDebug("DQM") << "[SiStripHistoNamingSchemeMtcc::histoTitle] " << title.str();
  return title.str();
  
}

// -----------------------------------------------------------------------------
// 
SiStripHistoNamingSchemeMtcc::HistoTitle SiStripHistoNamingSchemeMtcc::histoTitle( string histo_title ) {
  
  HistoTitle title;
  title.task_        = SiStripHistoNamingSchemeMtcc::UNKNOWN_TASK;
  title.contents_    = SiStripHistoNamingSchemeMtcc::UNKNOWN_CONTENTS;
  title.keyType_     = SiStripHistoNamingSchemeMtcc::UNKNOWN_KEY;
  title.keyValue_    = 0;
  title.granularity_ = SiStripHistoNamingSchemeMtcc::UNKNOWN_GRAN;
  title.channel_     = 0;
  title.extraInfo_   = "";

  uint32_t position = 0;

  // Extract task 
  if ( histo_title.find( pedestals_, position ) != string::npos ) { 
    title.task_ = SiStripHistoNamingSchemeMtcc::PEDESTALS;
    position = histo_title.find( pedestals_, position ) + pedestals_.size();
  } 
  else if ( histo_title.find( unknownTask_, position ) != string::npos ) { 
    title.task_ = SiStripHistoNamingSchemeMtcc::UNKNOWN_TASK;
    position = histo_title.find( unknownTask_, position ) + unknownTask_.size();
  } else { 
    title.task_ = SiStripHistoNamingSchemeMtcc::NO_TASK; 
  } 

  // Extract contents
  if ( histo_title.find( sum2_, position ) != string::npos ) { 
    title.contents_ = SiStripHistoNamingSchemeMtcc::SUM2;
    position = histo_title.find( sum2_, position ) + sum2_.size();
  } else if ( histo_title.find( sum_, position ) != string::npos ) { 
    title.contents_ = SiStripHistoNamingSchemeMtcc::SUM;
    position = histo_title.find( sum_, position ) + sum_.size();
  } else if ( histo_title.find( num_, position ) != string::npos ) { 
    title.contents_ = SiStripHistoNamingSchemeMtcc::NUM;
    position = histo_title.find( num_, position ) + num_.size();
  } else if ( histo_title.find( rawnoise_, position ) != string::npos ) { 
    title.contents_ = SiStripHistoNamingSchemeMtcc::RAWNOISE;
    position = histo_title.find( rawnoise_, position ) + rawnoise_.size();
  } else if ( histo_title.find( cmnoise_, position ) != string::npos ) { 
    title.contents_ = SiStripHistoNamingSchemeMtcc::CMNOISE;
    position = histo_title.find( cmnoise_, position ) + cmnoise_.size();
  } else if ( histo_title.find( peds_, position ) != string::npos ) { 
    title.contents_ = SiStripHistoNamingSchemeMtcc::PEDS;
    position = histo_title.find( peds_, position ) + peds_.size();
  } else if ( histo_title.find( unknownContents_, position ) != string::npos ) { 
    title.contents_ = SiStripHistoNamingSchemeMtcc::UNKNOWN_CONTENTS;
    position = histo_title.find( unknownContents_, position ) + unknownContents_.size();
  } else { 
    title.contents_ = SiStripHistoNamingSchemeMtcc::COMBINED;
  }
  
  // Extract key type and value
  if ( histo_title.find( fedKey_, position ) != string::npos ) { 
    title.keyType_ = SiStripHistoNamingSchemeMtcc::FED; 
    position = histo_title.find( fedKey_, position ) + fedKey_.size();
  } else if ( histo_title.find( fecKey_, position ) != string::npos ) { 
    title.keyType_ = SiStripHistoNamingSchemeMtcc::FEC; 
    position = histo_title.find( fecKey_, position ) + fecKey_.size();
  } else if ( histo_title.find( detKey_, position ) != string::npos ) { 
    title.keyType_ = SiStripHistoNamingSchemeMtcc::DET; 
    position = histo_title.find( detKey_, position ) + detKey_.size();
  } else if ( histo_title.find( unknownKey_, position ) != string::npos ) { 
    title.keyType_ = SiStripHistoNamingSchemeMtcc::UNKNOWN_KEY; 
    position = histo_title.find( unknownKey_, position ) + unknownKey_.size();
  } else { 
    title.keyType_ = SiStripHistoNamingSchemeMtcc::NO_KEY;
  }
  if ( title.keyType_ != SiStripHistoNamingSchemeMtcc::NO_KEY && 
       title.keyType_ != SiStripHistoNamingSchemeMtcc::UNKNOWN_KEY ) { 
    stringstream ss; ss << histo_title.substr( position, 8 );
    ss >> hex >> title.keyValue_;
    position += 8;
  } 
  
  // Extract granularity and channel number
  if ( histo_title.find( lldChan_, position ) != string::npos ) { 
    title.granularity_ = SiStripHistoNamingSchemeMtcc::LLD_CHAN; 
    position = histo_title.find( lldChan_, position ) + lldChan_.size();
  } else if ( histo_title.find( apvPair_, position ) != string::npos ) { 
    title.granularity_ = SiStripHistoNamingSchemeMtcc::APV_PAIR; 
    position = histo_title.find( apvPair_, position ) + apvPair_.size();
  } else if ( histo_title.find( apv_, position ) != string::npos ) { 
    title.granularity_ = SiStripHistoNamingSchemeMtcc::APV; 
    position = histo_title.find( apv_, position ) + apv_.size();
  } else if ( histo_title.find( unknownGranularity_, position ) != string::npos ) { 
    title.granularity_ = SiStripHistoNamingSchemeMtcc::UNKNOWN_GRAN; 
    position = histo_title.find( unknownGranularity_, position ) + unknownGranularity_.size(); 
  } else { 
    title.granularity_ = SiStripHistoNamingSchemeMtcc::MODULE;
  }
  if ( title.granularity_ != SiStripHistoNamingSchemeMtcc::MODULE &&
       title.granularity_ != SiStripHistoNamingSchemeMtcc:: UNKNOWN_GRAN ) { 
    stringstream ss; 
    ss << histo_title.substr( position, histo_title.find( sep_, position ) - position );
    ss >> dec >> title.channel_;
    position += ss.str().size();
  } 
  
  // Extract any extra info
  if ( histo_title.find( sep_, position ) != string::npos ) { 
    title.extraInfo_ = histo_title.substr( histo_title.find( sep_, position )+1, string::npos ); 
  }
  
  // Return HistoTitle struct
  return title;
  
}

// -----------------------------------------------------------------------------
// 
string SiStripHistoNamingSchemeMtcc::task( SiStripHistoNamingSchemeMtcc::Task task ) {
  if      ( task == SiStripHistoNamingSchemeMtcc::PEDESTALS )     { return pedestals_; }
  else if ( task == SiStripHistoNamingSchemeMtcc::NO_TASK )       { return ""; }
  else if ( task == SiStripHistoNamingSchemeMtcc::UNKNOWN_TASK )  { return unknownTask_; }
  else { 
    edm::LogError("DQM") << "[SiStripHistoNamingSchemeMtcc::task]"
			 << " Unexpected histogram task!"; 
    return "";
  }
}

// -----------------------------------------------------------------------------
// 
SiStripHistoNamingSchemeMtcc::Task SiStripHistoNamingSchemeMtcc::task( string task ) {
  if      ( task == "" )          { return SiStripHistoNamingSchemeMtcc::NO_TASK; }
  else if ( task == pedestals_ )  { return SiStripHistoNamingSchemeMtcc::PEDESTALS; }
  else { return SiStripHistoNamingSchemeMtcc::UNKNOWN_TASK; }
}  

// -----------------------------------------------------------------------------
// 
string SiStripHistoNamingSchemeMtcc::contents( SiStripHistoNamingSchemeMtcc::Contents contents ) {
  if      ( contents == SiStripHistoNamingSchemeMtcc::COMBINED )          { return ""; }
  else if ( contents == SiStripHistoNamingSchemeMtcc::SUM2 )              { return sum2_; }
  else if ( contents == SiStripHistoNamingSchemeMtcc::SUM )               { return sum_; }
  else if ( contents == SiStripHistoNamingSchemeMtcc::NUM )               { return num_; }
  else if ( contents == SiStripHistoNamingSchemeMtcc::RAWNOISE )          { return rawnoise_; }
  else if ( contents == SiStripHistoNamingSchemeMtcc::CMNOISE )           { return cmnoise_; }
  else if ( contents == SiStripHistoNamingSchemeMtcc::PEDS )              { return peds_; }
  else if ( contents == SiStripHistoNamingSchemeMtcc::UNKNOWN_CONTENTS )  { return unknownContents_; }
  else { 
    edm::LogError("DQM") << "[SiStripHistoNamingSchemeMtcc::contents]"
			 << " Unexpected histogram contents!"; 
    return "";
  }
}

// -----------------------------------------------------------------------------
// 
SiStripHistoNamingSchemeMtcc::Contents SiStripHistoNamingSchemeMtcc::contents( string contents ) {
  if      ( contents == "" )    { return SiStripHistoNamingSchemeMtcc::COMBINED; }
  else if ( contents == sum2_ ) { return SiStripHistoNamingSchemeMtcc::SUM2; }
  else if ( contents == sum_ )  { return SiStripHistoNamingSchemeMtcc::SUM; }
  else if ( contents == num_ )  { return SiStripHistoNamingSchemeMtcc::NUM; }
  else if ( contents == rawnoise_ )  { return SiStripHistoNamingSchemeMtcc::RAWNOISE; }
  else if ( contents == cmnoise_ )  { return SiStripHistoNamingSchemeMtcc::CMNOISE; }
  else if ( contents == peds_ )  { return SiStripHistoNamingSchemeMtcc::PEDS; }
  else { return SiStripHistoNamingSchemeMtcc::UNKNOWN_CONTENTS; }
}  

// -----------------------------------------------------------------------------
// 
string SiStripHistoNamingSchemeMtcc::keyType( SiStripHistoNamingSchemeMtcc::KeyType key_type ) {
  if      ( key_type == SiStripHistoNamingSchemeMtcc::NO_KEY )       { return ""; }
  else if ( key_type == SiStripHistoNamingSchemeMtcc::FED )          { return fedKey_; }
  else if ( key_type == SiStripHistoNamingSchemeMtcc::FEC )          { return fecKey_; }
  else if ( key_type == SiStripHistoNamingSchemeMtcc::DET )          { return detKey_; }
  else if ( key_type == SiStripHistoNamingSchemeMtcc::UNKNOWN_KEY )  { return unknownKey_; }
  else { 
    edm::LogError("DQM") << "[SiStripHistoNamingSchemeMtcc::keyType]"
			 << " Unexpected histogram key type!"; 
    return "";
  }
}

// -----------------------------------------------------------------------------
// 
SiStripHistoNamingSchemeMtcc::KeyType SiStripHistoNamingSchemeMtcc::keyType( string key_type ) {
  if      ( key_type == "" )      { return SiStripHistoNamingSchemeMtcc::NO_KEY; }
  else if ( key_type == fedKey_ ) { return SiStripHistoNamingSchemeMtcc::FED; }
  else if ( key_type == fecKey_ ) { return SiStripHistoNamingSchemeMtcc::FEC; }
  else if ( key_type == detKey_ ) { return SiStripHistoNamingSchemeMtcc::DET; }
  else { return SiStripHistoNamingSchemeMtcc::UNKNOWN_KEY; }
}  

// -----------------------------------------------------------------------------
// 
string SiStripHistoNamingSchemeMtcc::granularity( SiStripHistoNamingSchemeMtcc::Granularity granularity ) {
  if      ( granularity == SiStripHistoNamingSchemeMtcc::MODULE )       { return ""; }
  else if ( granularity == SiStripHistoNamingSchemeMtcc::LLD_CHAN )     { return lldChan_; }
  else if ( granularity == SiStripHistoNamingSchemeMtcc::APV_PAIR )     { return apvPair_; }
  else if ( granularity == SiStripHistoNamingSchemeMtcc::APV )          { return apv_; }
  else if ( granularity == SiStripHistoNamingSchemeMtcc::UNKNOWN_GRAN ) { return unknownGranularity_; }
  else { 
    edm::LogError("DQM") << "[SiStripHistoNamingSchemeMtcc::granularity]"
			 << " Unexpected histogram granularity!"; 
    return "";
  }
}

// -----------------------------------------------------------------------------
// 
SiStripHistoNamingSchemeMtcc::Granularity SiStripHistoNamingSchemeMtcc::granularity( string granularity ) {
  if      ( granularity == "" )       { return SiStripHistoNamingSchemeMtcc::MODULE; }
  else if ( granularity == lldChan_ ) { return SiStripHistoNamingSchemeMtcc::LLD_CHAN; }
  else if ( granularity == apvPair_ ) { return SiStripHistoNamingSchemeMtcc::APV_PAIR; }
  else if ( granularity == apv_ )     { return SiStripHistoNamingSchemeMtcc::APV; }
  else { return SiStripHistoNamingSchemeMtcc::UNKNOWN_GRAN; }
}  

