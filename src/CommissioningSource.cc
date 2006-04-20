#include "DQM/SiStripCommissioningSources/interface/CommissioningSource.h"
// edm
#include "FWCore/Framework/interface/ESHandle.h"
#include "FWCore/Framework/interface/Handle.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
// dqm
#include "DQMServices/Core/interface/DaqMonitorBEInterface.h"
#include "DQM/SiStripCommon/interface/SiStripHistoNamingScheme.h"
#include "DQM/SiStripCommon/interface/SiStripGenerateKey.h"
// conditions
#include "CondFormats/DataRecord/interface/SiStripFedCablingRcd.h"
#include "CondFormats/SiStripObjects/interface/SiStripFedCabling.h"
// calibrations
#include "CalibFormats/SiStripObjects/interface/SiStripFecCabling.h"
// data formats
#include "DataFormats/Common/interface/DetSetVector.h"
#include "DataFormats/SiStripDigi/interface/SiStripDigi.h"
// tasks
#include "DQM/SiStripCommissioningSources/interface/ApvTimingTask.h"
#include "DQM/SiStripCommissioningSources/interface/FedCablingTask.h"
#include "DQM/SiStripCommissioningSources/interface/FedTimingTask.h"
#include "DQM/SiStripCommissioningSources/interface/OptoScanTask.h"
#include "DQM/SiStripCommissioningSources/interface/PedestalsTask.h"
//#include "DQM/SiStripCommissioningSources/interface/PhysicsTask.h"
#include "DQM/SiStripCommissioningSources/interface/VpspScanTask.h"
// std, utilities
#include <boost/cstdint.hpp>
#include <memory>
#include <vector>
#include <sstream>
#include <iomanip>

// -----------------------------------------------------------------------------
//
CommissioningSource::CommissioningSource( const edm::ParameterSet& pset ) :
  inputModuleLabel_( pset.getParameter<string>( "InputModuleLabel" ) ),
  dqm_(0),
  task_( pset.getUntrackedParameter<string>("CommissioningTask","UNDEFINED") ),
  tasks_(),
  updateFreq_( pset.getUntrackedParameter<int>("HistoUpdateFreq",100) ),
  filename_( pset.getUntrackedParameter<string>("RootFileName","Source") ),
  run_(0),
  firstEvent_(true),
  fecCabling_(0),
  cablingTask_(false)
{
  edm::LogInfo("CommissioningSource") << "[CommissioningSource::CommissioningSource] Constructing object...";
}

// -----------------------------------------------------------------------------
//
CommissioningSource::~CommissioningSource() {
  edm::LogInfo("CommissioningSource") << "[CommissioningSource::~CommissioningSource] Destructing object...";
}

// -----------------------------------------------------------------------------
// Retrieve DQM interface, control cabling and "control view" utility
// class, create histogram directory structure and generate "reverse"
// control cabling.
void CommissioningSource::beginJob( const edm::EventSetup& setup ) {
  edm::LogInfo("Commissioning") << "[CommissioningSource::beginJob]";

  // Retrieve and store FED cabling, create FEC cabling
  edm::ESHandle<SiStripFedCabling> fed_cabling;
  setup.get<SiStripFedCablingRcd>().get( fed_cabling ); 
  fedCabling_ = const_cast<SiStripFedCabling*>( fed_cabling.product() ); 
  fecCabling_ = new SiStripFecCabling( *fed_cabling );
  
  // Create root directories according to control logical structure
  createDirs();

}

// -----------------------------------------------------------------------------
//
void CommissioningSource::endJob() {
  edm::LogInfo("Commissioning") << "[CommissioningSource::endJob]";
  for ( TaskMap::iterator itask = tasks_.begin(); itask != tasks_.end(); itask++ ) { 
    if ( itask->second ) { itask->second->updateHistograms(); }
  }
  if ( dqm_ ) { 
    string name;
    if ( filename_.find(".root",0) == string::npos ) { name = filename_; }
    else { name = filename_.substr( 0, filename_.find(".root",0) ); }
    stringstream ss; ss << name << "_" << setfill('0') << setw(7) << run_ << ".root";
    dqm_->save( ss.str() ); 
  }
  for ( TaskMap::iterator itask = tasks_.begin(); itask != tasks_.end(); itask++ ) { 
    if ( itask->second ) { delete itask->second; }
  }
  task_.clear();
}

// -----------------------------------------------------------------------------
//
void CommissioningSource::analyze( const edm::Event& event, 
				   const edm::EventSetup& setup ) {
  LogDebug("Commissioning") << "[CommissioningSource::analyze]";
  
  edm::Handle<SiStripEventSummary> summary;
  event.getByLabel( inputModuleLabel_, summary );

  // Extract run number
  if ( event.id().run() != run_ ) { run_ = event.id().run(); }
 
  // Create commissioning task objects 
  if ( firstEvent_ ) { createTask( summary->task() ); firstEvent_ = false; }
 
  edm::Handle< edm::DetSetVector<SiStripRawDigi> > raw;
  //edm::Handle< edm::DetSetVector<SiStripDigi> > zs;
  
  if ( summary->fedReadoutMode() == SiStripEventSummary::VIRGIN_RAW ) {
    event.getByLabel( inputModuleLabel_, "VirginRaw", raw );
  } else if ( summary->fedReadoutMode() == SiStripEventSummary::PROC_RAW ) {
    event.getByLabel( inputModuleLabel_, "ProcRaw", raw );
  } else if ( summary->fedReadoutMode() == SiStripEventSummary::SCOPE_MODE ) {
    event.getByLabel( inputModuleLabel_, "ScopeMode", raw );
  } else if ( summary->fedReadoutMode() == SiStripEventSummary::ZERO_SUPPR ) {
    //event.getByLabel( inputModuleLabel_, "ZeroSuppr", zs );
  } else {
    edm::LogError("CommissioningSource") << "[CommissioningSource::analyze]"
					 << " Unknown FED readout mode!";
  }
  
  // Generate FEC key (if FED cabling task)
  uint32_t fec_key = 0;
  if ( cablingTask_ ) {
    uint32_t id = summary->deviceId();
    fec_key = SiStripGenerateKey::controlKey( 0,                 // FEC crate
					      ((id>>27)&0x1F),   // FEC slot
					      ((id>>23)&0x0F),   // FEC ring
					      ((id>>16)&0x7F),   // CCU address
					      ((id>> 8)&0xFF),   // CCU channel
					      ((id>> 0)&0x03) ); // LLD channel
    SiStripGenerateKey::ControlPath path = SiStripGenerateKey::controlPath( fec_key );
    stringstream ss;
    ss << "[CommissioningSource::analyze]"
       << " Device id: " << setfill('0') << setw(8) << hex << id << dec
       << " FEC key: " << setfill('0') << setw(8) << hex << fec_key << dec
       << " crate/fec/ring/ccu/module/lld params: " 
       << path.fecCrate_ << "/"
       << path.fecSlot_ << "/"
       << path.fecRing_ << "/"
       << path.ccuAddr_ << "/"
       << path.ccuChan_ << "/"
       << path.lldChan_;
    LogDebug("Commissioning") << ss.str();
  }    
  
  // Iterate through FED ids and channels
  vector<uint16_t>::const_iterator ifed;
  for ( ifed = fedCabling_->feds().begin(); ifed != fedCabling_->feds().end(); ifed++ ) {
    for ( uint16_t ichan = 0; ichan < 96; ichan++ ) {
      // Create FED key and check if non-zero
      uint32_t fed_key = SiStripGenerateKey::fedKey( *ifed, ichan );
      if ( fed_key ) { 
	// Retrieve digis for given FED key and check if found
	vector< edm::DetSet<SiStripRawDigi> >::const_iterator digis = raw->find( fed_key );
	if ( digis != raw->end() ) { 
	  // Fill histograms for given FEC or FED key, depending on commissioning task
	  if ( cablingTask_ ) {
	    if ( tasks_.find(fec_key) != tasks_.end() ) { 
	      tasks_[fec_key]->fedChannel( fed_key );
	      tasks_[fec_key]->fillHistograms( *summary, *digis );
	    } else {
	      SiStripGenerateKey::ControlPath path = SiStripGenerateKey::controlPath( fec_key );
	      stringstream ss;
	      ss << "[CommissioningSource::analyze]"
		 << " Commissioning task with FEC key " 
		 << setfill('0') << setw(8) << hex << fec_key << dec
		 << " and crate/fec/ring/ccu/module/lld " 
		 << path.fecCrate_ << "/"
		 << path.fecSlot_ << "/"
		 << path.fecRing_ << "/"
		 << path.ccuAddr_ << "/"
		 << path.ccuChan_ << "/"
		 << path.lldChan_ 
		 << " not found in list!"; 
	      edm::LogError("Commissioning") << ss.str();
	    }
	  } else {
	    if ( tasks_.find(fed_key) != tasks_.end() ) { 
	      tasks_[fed_key]->fillHistograms( *summary, *digis );
	    } else {
	      pair<uint32_t,uint32_t> fed_ch = SiStripGenerateKey::fedChannel( fec_key );
	      stringstream ss;
	      ss << "[CommissioningSource::analyze]"
		 << " Commissioning task with FED key " 
		 << hex << setfill('0') << setw(8) << fed_key << dec
		 << " and FED id/ch " 
		 << fed_ch.first << "/"
		 << fed_ch.second 
		 << " not found in list!"; 
	      edm::LogError("Commissioning") << ss.str();
	    }
	  }
	}
      }
    }
  }
  
}

// -----------------------------------------------------------------------------
//
void CommissioningSource::createDirs() { 

  // Check DQM service is available
  dqm_ = edm::Service<DaqMonitorBEInterface>().operator->();
  if ( !dqm_ ) { 
    edm::LogError("Commissioning") << "[CommissioningSource::createTask] Null pointer to DQM interface!"; 
    return; 
  }
  
  // Iterate through FEC cabling and create commissioning task objects
  for ( vector<SiStripFec>::const_iterator ifec = fecCabling_->fecs().begin(); ifec != fecCabling_->fecs().end(); ifec++ ) {
    for ( vector<SiStripRing>::const_iterator iring = (*ifec).rings().begin(); iring != (*ifec).rings().end(); iring++ ) {
      for ( vector<SiStripCcu>::const_iterator iccu = (*iring).ccus().begin(); iccu != (*iring).ccus().end(); iccu++ ) {
	for ( vector<SiStripModule>::const_iterator imodule = (*iccu).modules().begin(); imodule != (*iccu).modules().end(); imodule++ ) {
	  string dir = SiStripHistoNamingScheme::controlPath( 0, // FEC crate 
							      (*ifec).fecSlot(),
							      (*iring).fecRing(),
							      (*iccu).ccuAddr(),
							      (*imodule).ccuChan() );
	  dqm_->setCurrentFolder( dir );
	  SiStripHistoNamingScheme::ControlPath path = SiStripHistoNamingScheme::controlPath( dir );
	  edm::LogInfo("Commissioning") << "[CommissioningSource::createDirs]"
					<< "  Created directory '" << dir 
					<< "' using params crate/slot/ring/ccu/chan " 
					<< 0 << "/" 
					<< (*ifec).fecSlot() << "/" 
					<< (*iring).fecRing() << "/" 
					<< (*iccu).ccuAddr() << "/"  
					<< (*imodule).ccuChan(); 
	}
      }
    }
  }
  
}

// -----------------------------------------------------------------------------
//
void CommissioningSource::createTask( SiStripEventSummary::Task task ) {
  LogDebug("Commissioning") << "[CommissioningSource::createTask]";
  
  // Check DQM service is available
  dqm_ = edm::Service<DaqMonitorBEInterface>().operator->();
  if ( !dqm_ ) { 
    edm::LogError("Commissioning") << "[CommissioningSource::createTask] Null pointer to DQM interface!"; 
    return; 
  }

  // Check commissioning task is known
  if ( task == SiStripEventSummary::UNKNOWN_TASK && task_ == "UNKNOWN" ) {
    edm::LogError("Commissioning") << "[CommissioningSource::createTask] Unknown commissioning task!"; 
    return; 
  }

  // Check if commissioning task is FED cabling 
  if ( task_ == "FED_CABLING" || ( task_ == "UNDEFINED" && task == SiStripEventSummary::FED_CABLING ) ) { cablingTask_ = true; }
  else { cablingTask_ = false; }

  // Iterate through FEC cabling and create commissioning task objects
  for ( vector<SiStripFec>::const_iterator ifec = fecCabling_->fecs().begin(); ifec != fecCabling_->fecs().end(); ifec++ ) {
    for ( vector<SiStripRing>::const_iterator iring = (*ifec).rings().begin(); iring != (*ifec).rings().end(); iring++ ) {
      for ( vector<SiStripCcu>::const_iterator iccu = (*iring).ccus().begin(); iccu != (*iring).ccus().end(); iccu++ ) {
	for ( vector<SiStripModule>::const_iterator imodule = (*iccu).modules().begin(); imodule != (*iccu).modules().end(); imodule++ ) {
	  string dir = SiStripHistoNamingScheme::controlPath( 0, // FEC crate 
							      (*ifec).fecSlot(),
							      (*iring).fecRing(),
							      (*iccu).ccuAddr(),
							      (*imodule).ccuChan() );
	  dqm_->setCurrentFolder( dir );
	  map< uint16_t, pair<uint16_t,uint16_t> >::const_iterator iconn;
	  for ( iconn = imodule->fedChannels().begin(); iconn != imodule->fedChannels().end(); iconn++ ) {
	    if ( !(iconn->second.first) ) { continue; } 
	    // Retrieve FED channel connection object in order to create key for task map
	    FedChannelConnection conn = fedCabling_->connection( iconn->second.first,
								 iconn->second.second );
	    uint32_t fed_key = SiStripGenerateKey::fedKey( conn.fedId(), conn.fedCh() );
	    uint32_t fec_key = SiStripGenerateKey::controlKey( conn.fecCrate(),
							       conn.fecSlot(),
							       conn.fecRing(),
							       conn.ccuAddr(),
							       conn.ccuChan(),
							       conn.lldChannel() );
	    uint32_t key = cablingTask_ ? fec_key : fed_key;
	    // Create commissioning task objects
	    if ( tasks_.find( key ) == tasks_.end() ) {
	      if      ( task_ == "FED_CABLING" ) { tasks_[key] = new FedCablingTask( dqm_, conn ); }
	      else if ( task_ == "PEDESTALS" )   { tasks_[key] = new PedestalsTask( dqm_, conn ); }
	      else if ( task_ == "APV_TIMING" )  { tasks_[key] = new ApvTimingTask( dqm_, conn ); }
	      else if ( task_ == "OPTO_SCAN" )   { tasks_[key] = new OptoScanTask( dqm_, conn ); }
	      else if ( task_ == "VPSP_SCAN" )   { tasks_[key] = new VpspScanTask( dqm_, conn ); }
	      else if ( task_ == "FED_TIMING" )  { tasks_[key] = new FedTimingTask( dqm_, conn ); }
	      else if ( task_ == "UNDEFINED" )   {
		//  Use data stream to determine which task objects are created!
		if      ( task == SiStripEventSummary::FED_CABLING )  { tasks_[key] = new FedCablingTask( dqm_, conn ); }
		else if ( task == SiStripEventSummary::PEDESTALS )    { tasks_[key] = new PedestalsTask( dqm_, conn ); }
		else if ( task == SiStripEventSummary::APV_TIMING )   { tasks_[key] = new ApvTimingTask( dqm_, conn ); } 
		else if ( task == SiStripEventSummary::OPTO_SCAN )    { tasks_[key] = new OptoScanTask( dqm_, conn ); }
		else if ( task == SiStripEventSummary::VPSP_SCAN )    { tasks_[key] = new VpspScanTask( dqm_, conn ); }
		else if ( task == SiStripEventSummary::FED_TIMING )   { tasks_[key] = new FedTimingTask( dqm_, conn ); }
		else if ( task == SiStripEventSummary::UNKNOWN_TASK ) {
		  edm::LogError("Commissioning") << "[CommissioningSource::createTask]"
						 << " Unknown commissioning task in data stream! " << task_;
		}
	      } else {
		edm::LogError("Commissioning") << "[CommissioningSource::createTask]"
					       << " Unknown commissioning task in .cfg file! " << task_;
	      }

	      // Check if key is found and, if so, book histos and set update freq
	      if ( tasks_.find( key ) != tasks_.end() ) {
		stringstream ss;
		ss << "[CommissioningSource::createTask]"
		   << " Created task '" << tasks_[key]->myName() << "' for key "
		   << hex << setfill('0') << setw(8) << key << dec 
		   << " in directory " << dir; 
		edm::LogInfo("Commissioning") << ss.str();
		tasks_[key]->bookHistograms(); 
		tasks_[key]->updateFreq( updateFreq_ ); 
	      } else {
		stringstream ss;
		ss << "[CommissioningSource::createTask]"
		   << " Commissioning task with key " 
		   << hex << setfill('0') << setw(8) << key << dec
		   << " not found in list!"; 
		edm::LogError("Commissioning") << ss.str();
	      }

	    } else {
	      stringstream ss;
	      ss << "[CommissioningSource::createTask]"
		 << " Task '" << tasks_[key]->myName()
		 << "' already exists for key "
		 << hex << setfill('0') << setw(8) << key << dec; 
	      edm::LogError("Commissioning") << ss.str();
	    }
	    
	  }
	}
      }
    }
  }
  
  edm::LogInfo("Commissioning") << "[CommissioningSource]"
				<< " Number of task objects created: " << tasks_.size();
  return;

}

