#include "PluginManager/ModuleDef.h"
#include "FWCore/Framework/interface/MakerMacros.h"

#include "DQM/SiStripCommissioningSources/test/stubs/CommissioningSourceMtcc.h"

DEFINE_SEAL_MODULE();
DEFINE_ANOTHER_FWK_MODULE(CommissioningSourceMtcc)

