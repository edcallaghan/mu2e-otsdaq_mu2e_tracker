//-----------------------------------------------------------------------------
// interactive interface for ROOT-based GUI
// mixes high- and low-level commands
// assume everything is happening on one node
// there could be one or two DTCs and only one CFO
//-----------------------------------------------------------------------------
#ifndef __trkdaq_cfo_interface_hh__
#define __trkdaq_cfo_interface_hh__

#define __CLING__ 1

#include "srcs/mu2e_pcie_utils/dtcInterfaceLib/DTC.h"
#include "srcs/mu2e_pcie_utils/cfoInterfaceLib/CFO.h"

namespace trkdaq {

  class CfoInterface { 
  public:
    static CfoInterface* fgInstance;

    CFOLib::CFO*         fCfo;
    int                  fPcieAddr;
//-----------------------------------------------------------------------------
// functions
//-----------------------------------------------------------------------------
  private:
    CfoInterface(int PcieAddr = -1, DTC_SimMode SimMode=DTCLib::DTC_SimMode_Disabled);
  public:
    virtual ~CfoInterface();

    static CfoInterface* Instance(int PcieAddr = -1);

    int          PcieAddr() { return fPcieAddr; }
    CFOLib::CFO* Cfo     () { return fCfo     ; }

    void         LaunchRunPlan();
    void         SetRunPlan    (const char* Fn);
//-----------------------------------------------------------------------------
// input file is a .txt file
// output file is a binary file with precompiled instructions
//-----------------------------------------------------------------------------
    void         CompileRunPlan(const char* InputFn, const char* OutputFn);
//-----------------------------------------------------------------------------
// TODO: need one more function which would 
// 1. generate off-spill run plan for N evens, 
// 2. compile and load it
// 'EWLength' in units of 25 ns (40 MHz clock ticks)
//-----------------------------------------------------------------------------
    void         SetOffspillRunPlan(int NEvents, int EWLength);
  };

};

#endif
