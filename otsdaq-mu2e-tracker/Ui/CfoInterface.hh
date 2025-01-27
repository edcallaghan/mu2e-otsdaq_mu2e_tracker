//-----------------------------------------------------------------------------
// interactive interface for ROOT-based GUI
// mixes high- and low-level commands
// assume everything is happening on one node
// there could be one or two DTCs and only one CFO
//-----------------------------------------------------------------------------
#ifndef __trkdaq_cfo_interface_hh__
#define __trkdaq_cfo_interface_hh__

#define __CLING__ 1

#include "dtcInterfaceLib/DTC.h"
#include "cfoInterfaceLib/CFO.h"

namespace trkdaq {

  class CfoInterface { 
  public:
    static CfoInterface* fgInstance;

    CFOLib::CFO*         fCfo;
    int                  fPcieAddr;
    uint                 fDtcMask;      // DTC mask, 4 bits per N(DTCs)
//-----------------------------------------------------------------------------
// functions
//-----------------------------------------------------------------------------
  private:
    CfoInterface(int PcieAddr = -1, uint DtcMask = 0x1, DTC_SimMode SimMode=DTCLib::DTC_SimMode_Disabled,
                 bool SkipInit = false);
  public:
    
    virtual ~CfoInterface();

    static CfoInterface* Instance(int PcieAddr = -1, uint DtcMask = 0);

    CFOLib::CFO* Cfo     () { return fCfo     ; }

    int          ConfigureJA(int ClockSource, int Reset = 1);

    void         InitReadout(const char* RunPlan, uint DtcMask = 0xffffffff);

    void         Halt();
    void         LaunchRunPlan();
    void         SetRunPlan   (const char* Fn);

    int          PcieAddr     () { return fPcieAddr; }
    void         PrintRegister(uint16_t Register, const char* Title = "") ;
    void         PrintStatus  ();
    uint32_t     ReadRegister (uint16_t Register);
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
