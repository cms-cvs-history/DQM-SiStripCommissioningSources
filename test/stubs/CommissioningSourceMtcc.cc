#include "DQM/SiStripCommissioningSources/test/stubs/CommissioningSourceMtcc.h"
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
  connect_(pset.getUntrackedParameter<std::string>("connect","sqlite_file:./sistrippedestals.db")), 
  catalog_(pset.getUntrackedParameter<std::string>("catalog","")), 
  tag_p(pset.getUntrackedParameter<std::string>("tag_p","SiStripPedestals_v1")), 
  tag_n(pset.getUntrackedParameter<std::string>("tag_n","SiStripNoises_v1")), 
  message_level_(pset.getUntrackedParameter<unsigned int>("messagelevel",0)),
  auth_(pset.getUntrackedParameter<unsigned int>("authenticationMethod",0)),
  userEnv_("CORAL_AUTH_USER=" + pset.getUntrackedParameter<string>("userEnv","me")),
  passwdEnv_("CORAL_AUTH_PASSWORD="+ pset.getUntrackedParameter<string>("passwdEnv","mypass")),
  RunStart_(pset.getUntrackedParameter<int>("RunStart",10)),
  
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
  rightpairs=OrderedPairs(setup);

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

  
  if ( summary->fedReadoutMode() == SiStripEventSummary::VIRGIN_RAW ) {
    event.getByLabel( inputModuleLabel_, "VirginRaw", raw );
  } else if ( summary->fedReadoutMode() == SiStripEventSummary::PROC_RAW ) {
    event.getByLabel( inputModuleLabel_, "ProcRaw", raw );
 } else if ( summary->fedReadoutMode() == SiStripEventSummary::SCOPE_MODE ) {
    event.getByLabel( inputModuleLabel_, "ScopeMode", raw );
  } else if ( summary->fedReadoutMode() == SiStripEventSummary::ZERO_SUPPR ) {
  } else {
    edm::LogError("CommissioningSourceMtcc") << "[CommissioningSourceMtcc::analyze]"
					 << " Unknown FED readout mode!";
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
void CommissioningSourceMtcc::createTask( SiStripEventSummary::Task task ) {
  LogDebug("Commissioning") << "[CommissioningSourceMtcc::createTask]";
  
  // Check DQM service is available
  dqm_ = edm::Service<DaqMonitorBEInterface>().operator->();
  if ( !dqm_ ) { 
    edm::LogError("Commissioning") << "[CommissioningSourceMtcc::createTask] Null pointer to DQM interface!"; 
    return; 
  }

  // Check commissioning task is known
  if ( task == SiStripEventSummary::UNKNOWN_TASK && task_ == "UNKNOWN" ) {
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
		if ( task == SiStripEventSummary::PEDESTALS )    { tasks_[key] = new PedestalsTaskMtcc( dqm_, conn,
													cutForNoisy_, 
													cutForDead_, 
													cutForNonGausTails_ ); }
		else if ( task == SiStripEventSummary::UNKNOWN_TASK ) {
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
  SiStripNoises * noise = new SiStripNoises();
  
  SiStripPedestalsVector theSiStripVector_p;
  SiStripNoiseVector theSiStripVector_n;

  vector< pair<uint16_t,uint16_t> >::iterator ipair;
  for (ipair=rightpairs.begin();ipair!=rightpairs.end();ipair++){
    uint32_t detid= fedCabling_->connection((*ipair).first,(*ipair).second).detId();
    uint16_t apv= fedCabling_->connection((*ipair).first,(*ipair).second).apvPairNumber();
    uint16_t nApv= fedCabling_->connection((*ipair).first,(*ipair).second).nApvPairs();
    uint32_t fed_key = SiStripReadoutKey::key((*ipair).first,(*ipair).second);
    if(apv==0) { 
      theSiStripVector_p.clear();
      theSiStripVector_n.clear();
    }
    for (unsigned int il=0;il<256;il++){
      SiStripPedestals::SiStripData sistripdata_p;
      PedestalsTaskMtcc* pedestals_ = dynamic_cast<PedestalsTaskMtcc*>(tasks_[fed_key]);
      float thisped = pedestals_->getPedestals()->getBinContent(il+1);
      float thisnoise = pedestals_->getCMSnoise()->getBinContent(il+1);
      int flag = 0;					
      if (pedestals_->getFlag(il) != 0){flag = 1;}           
      if (flag == 1){
         cout << " ped and noise for " << il << " are " << thisped << " and " << thisnoise << " Flag " << flag <<endl;}
      sistripdata_p.Data = ped->EncodeStripData(
						thisped,
						thisnoise,
						2,
						5,
						flag
						);
      theSiStripVector_p.push_back(sistripdata_p);	
      SiStripNoises::SiStripData sistripdata_n;
      sistripdata_n.setData(thisnoise, flag); 
      theSiStripVector_n.push_back(sistripdata_n);	
    }
    if (apv==nApv-1)  {
      ped->m_pedestals.insert(std::pair<uint32_t, SiStripPedestalsVector > (detid,theSiStripVector_p));
      noise->m_noises.insert(std::pair<uint32_t, SiStripNoiseVector > (detid,theSiStripVector_n));
    }
  }
  
  
  
  
  cout<<"Now writing to DB"<<endl;
  try {
    loader=new cond::ServiceLoader;  
    
    if( auth_==1 ){
      loader->loadAuthenticationService( cond::XML );
    }else{
      loader->loadAuthenticationService( cond::Env );
    }
    switch (message_level_) {
    case 0 :
      loader->loadMessageService(cond::Error);
      break;    
    case 1:
      loader->loadMessageService(cond::Warning);
      break;
    case 2:
      loader->loadMessageService( cond::Info );
      break;
    case 3:
      loader->loadMessageService( cond::Debug );
      break;  
    default:
      loader->loadMessageService();
    }

    session=new cond::DBSession(connect_);
    session->setCatalog(catalog_);
    session->connect(cond::ReadWriteCreate);

    pwriter   =new cond::DBWriter(*session, "SiStripPedestals");
    nwriter   =new cond::DBWriter(*session, "SiStripNoises");
    iovwriter =new cond::DBWriter(*session, "IOV");
    session->startUpdateTransaction();

    cond::IOV* pedIOV= new cond::IOV; 
    cond::IOV* noiseIOV= new cond::IOV; 

    cout << "markWrite pedestals..." << endl;
    string pedTok = pwriter->markWrite<SiStripPedestals>(ped); 
    //      string pedTok = pwriter->markWrite<SiStripPedestals>(iter->second); 
    cout << pedTok << endl;

    cout << "markWrite noises..." << endl;
    //      string noiseTok = nwriter->markWrite<SiStripNoises>(iter_n->second);
    string noiseTok = nwriter->markWrite<SiStripNoises>(noise);
    //      iter_n++;
    cout << noiseTok << endl;
    //      count++; 
    //      int iiov = (count!=iov_ped.size()) ? iter->first : edm::IOVSyncValue::endOfTime().eventID().run();   //set last iov valid forever
    int iiov;    
    //if (RunStart_>0)
    //  iiov = RunStart_;
    //else
      iiov = edm::IOVSyncValue::endOfTime().eventID().run();   //set last iov valid forever
    
    cout << "Associate IOV... " << iiov << endl;
    pedIOV->iov.insert(std::make_pair(iiov, pedTok));
    noiseIOV->iov.insert(std::make_pair(iiov, noiseTok));
    //   }    
    cout << "iov size " << pedIOV->iov.size() << endl;
    cout << "markWrite IOV..." << endl;
    string pedIOVTok = iovwriter->markWrite<cond::IOV>(pedIOV); 
    string noiseIOVTok = iovwriter->markWrite<cond::IOV>(noiseIOV);  
    cout << "Commit..." << endl;
    session->commit();//commit all in one
    cout << "Done." << endl;
    cout << "pedIOV size " << pedIOV->iov.size() << endl;
    cout << "noiseIOV size " << noiseIOV->iov.size() << endl;

    session->disconnect();
    cout << "Add MetaData... " << endl;
    metadataSvc = new cond::MetaData(connect_,*loader);
    metadataSvc->connect();
    metadataSvc->addMapping(tag_p,pedIOVTok);
    metadataSvc->addMapping(tag_n,noiseIOVTok);
    metadataSvc->disconnect();
    cout << "Done." << endl;
  }catch(const cond::Exception& e){
    std::cout<<"cond::Exception: " << e.what()<<std::endl;
  } catch (cms::Exception& e) {
    cout << "cms::Exception:  " << e.what() << endl;
  } catch (exception &e) {
    cout << "std::exception:  " << e.what() << endl;
  } catch (...) {
    cout << "Unknown error caught" << endl;
  }

  delete session;
  delete pwriter;
  delete iovwriter;
  delete metadataSvc;
  delete loader;
}

vector < pair<uint16_t, uint16_t> >  CommissioningSourceMtcc::OrderedPairs(const edm::EventSetup& setup){


  vector< pair<uint16_t,uint16_t> >chanfedpairs;
  vector<uint16_t>::const_iterator ifed;
  for ( ifed = fedCabling_->feds().begin(); ifed != fedCabling_->feds().end(); ifed++ ) {
    for ( uint16_t ichan = 0; ichan < 96; ichan++ ) {
      // Create FED key and check if non-zero
      uint32_t fed_key = SiStripReadoutKey::key( *ifed, ichan );
      cout << " fed key   " << fed_key << endl;
      if ( fed_key ) { 
	uint32_t detid = fedCabling_->connection(*ifed, ichan).detId(); 
	uint16_t ifl=  *ifed;
	uint16_t ich=ichan;

	if (detid!=0)     chanfedpairs.push_back(make_pair(ifl,ich));
      }
    }
  }
  stable_sort( chanfedpairs.begin(),chanfedpairs.end(),OrderChannels(setup));

  return chanfedpairs;
}
