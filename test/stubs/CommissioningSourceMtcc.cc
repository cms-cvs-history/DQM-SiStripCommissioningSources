#include "DQM/SiStripCommissioningSources/test/stubs/CommissioningSourceMtcc.h"
// edm
#include "FWCore/Framework/interface/ESHandle.h"
#include "FWCore/Framework/interface/Handle.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "CondCore/DBOutputService/interface/PoolDBOutputService.h"
// dqm
#include "DQMServices/Core/interface/DaqMonitorBEInterface.h"
#include "DQM/SiStripCommissioningSources/test/stubs/SiStripHistoNamingSchemeMtcc.h"
// conditions
#include "CondFormats/DataRecord/interface/SiStripFedCablingRcd.h"
#include "CondFormats/SiStripObjects/interface/SiStripFedCabling.h"
// calibrations
#include "CalibFormats/SiStripObjects/interface/SiStripFecCabling.h"
// data formats
#include "DataFormats/Common/interface/DetSetVector.h"
#include "DataFormats/SiStripDigi/interface/SiStripDigi.h"
#include "DataFormats/SiStripDetId/interface/SiStripControlKey.h"
#include "DataFormats/SiStripDetId/interface/SiStripReadoutKey.h"
// tasks
#include "DQM/SiStripCommissioningSources/test/stubs/PedestalsTaskMtcc.h"
// std, utilities
#include <boost/cstdint.hpp>
#include <memory>
#include <vector>
#include <sstream>
#include <iomanip>

#include <string>
#include <map>


using namespace std;
// -----------------------------------------------------------------------------
//
CommissioningSourceMtcc::CommissioningSourceMtcc( const edm::ParameterSet& pset ) :
  inputModuleLabel_( pset.getParameter<string>( "InputModuleLabel" ) ),
  dqm_(0),
  task_( pset.getUntrackedParameter<string>("CommissioningTask","UNDEFINED") ),
  tasks_(),
  updateFreq_( pset.getUntrackedParameter<int>("HistoUpdateFreq",100) ),
  filename_( pset.getUntrackedParameter<string>("RootFileName","Source") ),
  run_(0),
  firstEvent_(true),
  fecCabling_(0),
  //cablingTask_(false),
  userEnv_("CORAL_AUTH_USER=" + pset.getUntrackedParameter<string>("userEnv","me")),
  passwdEnv_("CORAL_AUTH_PASSWORD="+ pset.getUntrackedParameter<string>("passwdEnv","mypass")),
  
  cutForNoisy_(pset.getParameter<double>("cutForNoisy")),
  cutForDead_(pset.getParameter<double>("cutForDead")),
  cutForNonGausTails_(pset.getParameter<double>("cutForNonGausTails"))
{
  edm::LogInfo("CommissioningSourceMtcc") << "[CommissioningSourceMtcc::CommissioningSourceMtcc] Constructing object...";
  ::putenv( const_cast<char*>( userEnv_.c_str() ) );
  ::putenv( const_cast<char*>( passwdEnv_.c_str() ) );
}

// -----------------------------------------------------------------------------
//
CommissioningSourceMtcc::~CommissioningSourceMtcc() {
  edm::LogInfo("CommissioningSourceMtcc") << "[CommissioningSourceMtcc::~CommissioningSourceMtcc] Destructing object...";
}

// -----------------------------------------------------------------------------
// Retrieve DQM interface, control cabling and "control view" utility
// class, create histogram directory structure and generate "reverse"
// control cabling.
void CommissioningSourceMtcc::beginJob( const edm::EventSetup& setup ) {
  edm::LogInfo("Commissioning") << "[CommissioningSourceMtcc::beginJob]";

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
void CommissioningSourceMtcc::endJob() {

  writePed();
  edm::LogInfo("Commissioning") << "[CommissioningSourceMtcc::endJob]";
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
void CommissioningSourceMtcc::analyze( const edm::Event& event, 
				   const edm::EventSetup& setup ) {

  LogDebug("Commissioning") << "[CommissioningSourceMtcc::analyze]";
  
  edm::Handle<SiStripEventSummary> summary;
  event.getByLabel( inputModuleLabel_, summary );

  // Extract run number
  if ( event.id().run() != run_ ) { run_ = event.id().run(); }
 
  // Create commissioning task objects 
  if ( firstEvent_ ) { createTask( summary->task() ); firstEvent_ = false; }
 
  edm::Handle< edm::DetSetVector<SiStripRawDigi> > raw;
  //edm::Handle< edm::DetSetVector<SiStripDigi> > zs;

  
  if ( summary->fedReadoutMode() == sistrip::VIRGIN_RAW ) {
    event.getByLabel( inputModuleLabel_, "VirginRaw", raw );
    //std::cout << " sono in virgin" << std::endl;
  } else if ( summary->fedReadoutMode() == sistrip::PROC_RAW ) {
    event.getByLabel( inputModuleLabel_, "ProcRaw", raw );
    //std::cout << " sono in processed" << std::endl; 
 } else if ( summary->fedReadoutMode() == sistrip::SCOPE_MODE ) {
    event.getByLabel( inputModuleLabel_, "ScopeMode", raw );
    //std::cout << " sono in scope" << std::endl;
  } else if ( summary->fedReadoutMode() == sistrip::ZERO_SUPPR ) {
    //event.getByLabel( inputModuleLabel_, "ZeroSuppr", zs );
  } else {
    edm::LogError("CommissioningSourceMtcc") << "[CommissioningSourceMtcc::analyze]"
					 << " Unknown FED readout mode!";

    //%%%%%%%%%%%% REMOVE %%%%%%%%%%
    edm::LogError("CommissioningSourceMtcc") << "[CommissioningSourceMtcc::analyze]"
					 << "Force to take virgin mode"; 
    event.getByLabel( inputModuleLabel_, "VirginRaw", raw );
    //%%%%%%%%%%%% REMOVE %%%%%%%%%%
  }
  

  if ( &(*raw) == 0 ) {
    edm::LogError("SiStripCommissioningSource")
      << "[SiStripCommissioningSource::analyze]"
      << " NULL pointer to DetSetVector!";
    return;
  }

  //&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
  // Generate FEC key (if FED cabling task)
  uint32_t fec_key = 0;
  
  // Iterate through FED ids and channels
  vector<uint16_t>::const_iterator ifed;
  for ( ifed = fedCabling_->feds().begin(); ifed != fedCabling_->feds().end(); ifed++ ) {
    for ( uint16_t ichan = 0; ichan < 96; ichan++ ) {
      // Create FED key and check if non-zero
      uint32_t fed_key = SiStripReadoutKey::key( *ifed, ichan );
      if ( fed_key ) { 
	// Retrieve digis for given FED key and check if found
	vector< edm::DetSet<SiStripRawDigi> >::const_iterator digis = raw->find( fed_key );
	if ( digis != raw->end() ) { 
	  // Fill histograms for given FEC or FED key, depending on commissioning task
	  if ( tasks_.find(fed_key) != tasks_.end() ) { 
	    tasks_[fed_key]->fillHistograms( *summary, *digis );
	  } else {
	    SiStripReadoutKey::ReadoutPath path = SiStripReadoutKey::path( fec_key );
	    stringstream ss;
	    ss << "[CommissioningSourceMtcc::analyze]"
	       << " Commissioning task with FED key " 
	       << hex << setfill('0') << setw(8) << fed_key << dec
	       << " and FED id/ch " 
	       << path.fedId_ << "/"
	       << path.fedCh_ 
	       << " not found in list!"; 
	    edm::LogError("Commissioning") << ss.str();
	  }
	}
      }
    }
  }
  
}

// -----------------------------------------------------------------------------
//
void CommissioningSourceMtcc::createDirs() { 

  // Check DQM service is available
  dqm_ = edm::Service<DaqMonitorBEInterface>().operator->();
  if ( !dqm_ ) { 
    edm::LogError("Commissioning") << "[CommissioningSourceMtcc::createTask] Null pointer to DQM interface!"; 
    return; 
  }
  
  // Iterate through FEC cabling and create commissioning task objects
  for ( vector<SiStripFec>::const_iterator ifec = fecCabling_->fecs().begin(); ifec != fecCabling_->fecs().end(); ifec++ ) {
    for ( vector<SiStripRing>::const_iterator iring = (*ifec).rings().begin(); iring != (*ifec).rings().end(); iring++ ) {
      for ( vector<SiStripCcu>::const_iterator iccu = (*iring).ccus().begin(); iccu != (*iring).ccus().end(); iccu++ ) {
	for ( vector<SiStripModule>::const_iterator imodule = (*iccu).modules().begin(); imodule != (*iccu).modules().end(); imodule++ ) {
	  string dir = SiStripHistoNamingSchemeMtcc::controlPath( 0, // FEC crate 
							      (*ifec).fecSlot(),
							      (*iring).fecRing(),
							      (*iccu).ccuAddr(),
							      (*imodule).ccuChan() );
	  dqm_->setCurrentFolder( dir );
	  SiStripHistoNamingSchemeMtcc::ControlPath path = SiStripHistoNamingSchemeMtcc::controlPath( dir );
	  edm::LogInfo("Commissioning") << "[CommissioningSourceMtcc::createDirs]"
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
void CommissioningSourceMtcc::createTask( sistrip::Task task ) {
  LogDebug("Commissioning") << "[CommissioningSourceMtcc::createTask]";
  
  // Check DQM service is available
  dqm_ = edm::Service<DaqMonitorBEInterface>().operator->();
  if ( !dqm_ ) { 
    edm::LogError("Commissioning") << "[CommissioningSourceMtcc::createTask] Null pointer to DQM interface!"; 
    return; 
  }

  // Check commissioning task is known
  if ( task == sistrip::UNKNOWN_TASK && task_ == "UNKNOWN" ) {
    edm::LogError("Commissioning") << "[CommissioningSourceMtcc::createTask] Unknown commissioning task!"; 
    return; 
  }

  // Iterate through FEC cabling and create commissioning task objects
  for ( vector<SiStripFec>::const_iterator ifec = fecCabling_->fecs().begin(); ifec != fecCabling_->fecs().end(); ifec++ ) {
    for ( vector<SiStripRing>::const_iterator iring = (*ifec).rings().begin(); iring != (*ifec).rings().end(); iring++ ) {
      for ( vector<SiStripCcu>::const_iterator iccu = (*iring).ccus().begin(); iccu != (*iring).ccus().end(); iccu++ ) {
	for ( vector<SiStripModule>::const_iterator imodule = (*iccu).modules().begin(); imodule != (*iccu).modules().end(); imodule++ ) {
	  string dir = SiStripHistoNamingSchemeMtcc::controlPath( 0, // FEC crate 
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
	    uint32_t fed_key = SiStripReadoutKey::key( conn.fedId(), conn.fedCh() );

	    uint32_t key = fed_key;//cablingTask_ ? fec_key : fed_key;	    
	    // Create commissioning task objects
	    if ( tasks_.find( key ) == tasks_.end() ) {
	      
	      //Giulio
	      if      ( task_ == "PEDESTALS" )   { tasks_[key] = new PedestalsTaskMtcc( dqm_, conn, 
											cutForNoisy_, 
											cutForDead_, 
											cutForNonGausTails_  ); }
	      else if ( task_ == "UNDEFINED" )   {
		//  Use data stream to determine which task objects are created!
		if ( task == sistrip::PEDESTALS )    { tasks_[key] = new PedestalsTaskMtcc( dqm_, conn,
													cutForNoisy_, 
													cutForDead_, 
													cutForNonGausTails_ ); }
		else if ( task == sistrip::UNKNOWN_TASK ) {
		  edm::LogError("Commissioning") << "[CommissioningSourceMtcc::createTask]"
						 << " Unknown commissioning task in data stream! " << task_;
		}
	      } else {
		edm::LogError("Commissioning") << "[CommissioningSourceMtcc::createTask]"
					       << " Unknown commissioning task in .cfg file! " << task_;
	      }
	      
	      // Check if key is found and, if so, book histos and set update freq
	      if ( tasks_.find( key ) != tasks_.end() ) {
		tasks_[key]->bookHistograms(); 
		tasks_[key]->updateFreq( updateFreq_ ); 
	      } else {
		stringstream ss;
		ss << "[CommissioningSourceMtcc::createTask]"
		   << " Commissioning task with key " 
		   << hex << setfill('0') << setw(8) << key << dec
		   << " not found in list!"; 
		edm::LogError("Commissioning") << ss.str();
	      }

	    } else {
	      stringstream ss;
	      ss << "[CommissioningSourceMtcc::createTask]"
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
  
  edm::LogInfo("Commissioning") << "[CommissioningSourceMtcc]"
				<< " Number of task objects created: " << tasks_.size();
  return;

}

void CommissioningSourceMtcc::writePed(){
 
  SiStripPedestals * ped = new SiStripPedestals();
  SiStripNoises    * noise = new SiStripNoises();
  
  std::vector<char>  theSiStripVector_p;  
  std::vector<short> theSiStripVector_n;  

  for ( vector<SiStripFec>::const_iterator ifec = fecCabling_->fecs().begin(); ifec != fecCabling_->fecs().end(); ifec++ ) {
    for ( vector<SiStripRing>::const_iterator iring = (*ifec).rings().begin(); iring != (*ifec).rings().end(); iring++ ) {
      for ( vector<SiStripCcu>::const_iterator iccu = (*iring).ccus().begin(); iccu != (*iring).ccus().end(); iccu++ ) {
        for ( vector<SiStripModule>::const_iterator imodule = (*iccu).modules().begin(); imodule != (*iccu).modules().end(); imodule++ ){
          map< uint16_t, pair<uint16_t,uint16_t> > fedConMap = imodule->fedChannels();
    	  uint32_t detid= imodule->detId();
          uint16_t nApv= imodule->nApvPairs();
          for (uint16_t ipair = 0; ipair < nApv; ipair++){
             if(ipair==0) { 
              theSiStripVector_p.clear();
              theSiStripVector_n.clear();
             }
	     edm::LogInfo("Commissioning")<<"[CommissioningSourceMtcc::writePed] detid " << detid << " napvpairs " << nApv << " ipair " << ipair << std::endl;
             map< uint16_t, pair<uint16_t,uint16_t> >::iterator iterFedCon = fedConMap.find(imodule->lldChannel(ipair));
             if (iterFedCon!=fedConMap.end()){
               for (unsigned int il=0;il<256;il++){
                 uint32_t fed_key = SiStripReadoutKey::key(iterFedCon->second.first,iterFedCon->second.second); 
                 PedestalsTaskMtcc* pedestals_ = dynamic_cast<PedestalsTaskMtcc*>(tasks_[fed_key]);
                 float thisped = pedestals_->getPedestals()->getBinContent(il+1);
                 float thisnoise = pedestals_->getCMSnoise()->getBinContent(il+1);
                 int flag = 0;					
                 if (pedestals_->getFlag(il) != 0){flag = 1;}           
                 //if (flag == 1){
		 edm::LogInfo("Commissioning")<<"[CommissioningSourceMtcc::writePed] ped and noise for " << il << " are " << thisped << " and " << thisnoise << " Flag " << flag <<endl;
		    //}
		 ped->setData(thisped,2,5,theSiStripVector_p);
		 noise->setData(thisnoise,flag,theSiStripVector_n);
               }
             } else {
	       edm::LogInfo("Commissioning") << "Warning!! Found " << imodule->fedChannels().size()  
					     << " fibers instead of " << nApv  
					     << " on module with id " << detid	
					     << "    Missing Apv Pair " << ipair ;
	       for (unsigned int il=0;il<256;il++){
		 ped->setData(0,2,5,theSiStripVector_p);
		 noise->setData(0,1,theSiStripVector_n);
	       }
             }
	     if (ipair==nApv-1)  {
	       SiStripPedestals::Range range_p(theSiStripVector_p.begin(),theSiStripVector_p.end());
	       edm::LogInfo("Commissioning")<<"[CommissioningSourceMtcc::writePed] write pedestals for " << detid << " range " << range_p.second-range_p.first << std::endl;
	       if ( ! ped->put(detid,range_p) )
		 edm::LogError("Commissioning")<<"[CommissioningSourceMtcc::writePed] storing pedestals: detid already exists"<<std::endl;
	       SiStripNoises::Range range_n(theSiStripVector_n.begin(),theSiStripVector_n.end());
	       edm::LogInfo("Commissioning")<<"[CommissioningSourceMtcc::writePed] write noise for " << detid << " range " << range_n.second-range_n.first << std::endl;
	       if ( ! noise->put(detid,range_n) )
		 edm::LogError("Commissioning")<<"[CommissioningSourceMtcc::writePed] storing noise: detid already exists"<<std::endl;
	     }
	  }	
        }
      }     
    } 
  }
  
  //End now write sistrippedestals data in DB
  cout<<"Now writing to DB"<<endl;
  edm::Service<cond::service::PoolDBOutputService> mydbservice;
  
  if( mydbservice.isAvailable() ){
    try{
      size_t PedCallbackToken=mydbservice->callbackToken("SiStripPedestals");
      size_t NoiseCallbackToken=mydbservice->callbackToken("SiStripNoises");
      edm::LogInfo("Commissioning")<<"current time "<<mydbservice->currentTime()<<std::endl;
      edm::LogInfo("Commissioning")<<"end of time "<<mydbservice->endOfTime()<<std::endl;
      mydbservice->newValidityForNewPayload<SiStripPedestals>(ped,mydbservice->endOfTime(),PedCallbackToken);
      mydbservice->newValidityForNewPayload<SiStripNoises>(noise,mydbservice->endOfTime(),NoiseCallbackToken);
    }catch(const cond::Exception& er){
      edm::LogError("Commissioning")<<er.what()<<std::endl;
    }catch(const std::exception& er){
      edm::LogError("Commissioning")<<"caught std::exception "<<er.what()<<std::endl;
    }catch(...){
      edm::LogError("Commissioning")<<"Funny error"<<std::endl;
    }
  }else{
    edm::LogError("Commissioning")<<"Service is unavailable"<<std::endl;
  }
}
