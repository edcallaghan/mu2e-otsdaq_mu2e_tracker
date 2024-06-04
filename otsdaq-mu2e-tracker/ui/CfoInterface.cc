//-----------------------------------------------------------------------------
// interactive interface for ROOT-based GUI
// mixes high- and low-level commands
// assume everything is happening on one node
// there could be one or two DTCs and only one CFO
// most repeats Ryan's code, but w/o dependency on xdaq, GUI-based UI, and otsdaq everything
//-----------------------------------------------------------------------------
#ifndef __trkdaq_cfo_interface_cc__
#define __trkdaq_cfo_interface_cc__

#define __CLING__ 1

#include "TString.h"
#include "iostream"
#include "CfoInterface.hh"
#include "cfoInterfaceLib/CFO_Compiler.hh"

using namespace CFOLib;
using namespace DTCLib;

namespace trkdaq {

  CfoInterface* CfoInterface::fgInstance = nullptr;

//-----------------------------------------------------------------------------
  CfoInterface::CfoInterface(int PcieAddr, DTC_SimMode SimMode) {
    std::string expected_version("");              // dont check
    bool        skip_init       (false);
    std::string sim_file        ("mu2esim.bin");
    std::string uid             ("");

    fPcieAddr = PcieAddr;
    fCfo      = new CFO(SimMode,PcieAddr,expected_version,skip_init,uid);
  }

//-----------------------------------------------------------------------------
  CfoInterface::~CfoInterface()  {
    if (fgInstance) {
      delete fgInstance->fCfo;
      fgInstance = nullptr;
    }
  }

//-----------------------------------------------------------------------------
  CfoInterface* CfoInterface::Instance(int PcieAddr) {
    int pcie_addr = PcieAddr;
    if (pcie_addr < 0) {
//-----------------------------------------------------------------------------
// PCIE address is not specified, check environment
//-----------------------------------------------------------------------------
      if (getenv("CFOLIB_CFO") != nullptr) pcie_addr = atoi(getenv("CFOLIB_CFO"));
      else {
        printf (" ERROR: PcieAddr < 0 and $CFOLIB_CFO is not defined. BAIL out\n");
        return nullptr;
      }
    }

    if (fgInstance == nullptr) fgInstance = new CfoInterface(pcie_addr,DTC_SimMode_NoCFO);
      
    if (fgInstance->PcieAddr() != pcie_addr) {
      printf (" ERROR: CfoInterface::Instance has been already initialized with PcieAddress = %i. BAIL out\n", 
              fgInstance->PcieAddr());
      return nullptr;
    }
    else return fgInstance;
  }


//-----------------------------------------------------------------------------
// looks that it is only for the off-spill
//-----------------------------------------------------------------------------
  void CfoInterface::LaunchRunPlan() {
    fCfo->DisableBeamOnMode (CFO_Link_ID::CFO_Link_ALL);
    fCfo->DisableBeamOffMode(CFO_Link_ID::CFO_Link_ALL);

    fCfo->SoftReset();
    usleep(10);	

    fCfo->EnableBeamOffMode (CFO_Link_ID::CFO_Link_ALL);
  }
  
//-----------------------------------------------------------------------------
  void CfoInterface::CompileRunPlan(const char* InputFn, const char* OutputFn) {
    CFOLib::CFO_Compiler compiler;

    std::string fn1(InputFn );
    std::string fn2(OutputFn);
    
    compiler.processFile(fn1,fn2);
  }

//-----------------------------------------------------------------------------
// launch is a separate step, could be repeated multiple times
// this is a one-time initialization
//-----------------------------------------------------------------------------
  void CfoInterface::InitReadout(const char* RunPlan, int CfoLink, int NDtcs) {

    CFO_Link_ID cfo_link = CFO_Link_ID(CfoLink);

    fCfo->SoftReset();
    fCfo->EnableLink     (cfo_link,DTC_LinkEnableMode(true,true),NDtcs);
    // fCfo->SetMaxDTCNumber(cfo_link,NDtcs); // done in EnableLink
    SetRunPlan   (RunPlan);
  // cfo_launch_run_plan();
  }

//-----------------------------------------------------------------------------
  uint32_t CfoInterface::ReadRegister(uint16_t Register) {

    uint32_t data;
    int      timeout(150);
    
    mu2edev* dev = fCfo->GetDevice();
    dev->read_register(Register,timeout,&data);
    
    return data;
  }


//-----------------------------------------------------------------------------
  void CfoInterface::PrintRegister(uint16_t Register, const char* Title) {
    std::cout << Form("%s (0x%04x) : 0x%08x\n",Title,Register,ReadRegister(Register));
  }

//-----------------------------------------------------------------------------
  void CfoInterface::PrintStatus() {
    std::cout << Form("-----------------------------------------------------------------\n");
    PrintRegister(0x9004,"CFO version                                ");
    PrintRegister(0x9030,"Kernel driver version                      ");
    PrintRegister(0x9100,"CFO control register                       ");
    PrintRegister(0x9104,"DMA Transfer Length                        ");
    PrintRegister(0x9108,"SERDES loopback enable                     ");
    PrintRegister(0x9114,"CFO link enable                            ");
    PrintRegister(0x9128,"CFO PLL locked                             ");
    PrintRegister(0x9140,"SERDES RX CDR lock                         ");
    PrintRegister(0x9144,"Beam On Timer Preset                       ");
    PrintRegister(0x9148,"Enable Beam On Mode                        ");
    PrintRegister(0x914c,"Enable Beam Off Mode                       ");
    PrintRegister(0x918c,"Number of DTCs                             ");
    
    PrintRegister(0x9200,"Receive  Byte   Count Link 0               ");
    PrintRegister(0x9220,"Receive  Packet Count Link 0               ");
    PrintRegister(0x9240,"Transmit Byte   Count Link 0               ");
    PrintRegister(0x9260,"Transmit Packet Count Link 0               ");
  }

//-----------------------------------------------------------------------------
// TODO
//-----------------------------------------------------------------------------
  void CfoInterface::SetOffspillRunPlan(int NEvents, int EWLength) {
    printf("ERROR: %s not implemented yet",__func__);
  }

//-----------------------------------------------------------------------------
// first 8 bytes contain nbytes, but written into the CFO are 0x10000 bytes
// (sizeof(mu2e_databuff_t)
//-----------------------------------------------------------------------------
  void CfoInterface::SetRunPlan(const char* Fn) {

    std::ifstream file(Fn, std::ios::binary | std::ios::ate);

    // read binary file
    mu2e_databuff_t inputData;
    auto inputSize = file.tellg();
    uint64_t dmaSize = static_cast<uint64_t>(inputSize) + 8;
    file.seekg(0, std::ios::beg);

    memcpy(&inputData[0], &dmaSize, sizeof(uint64_t));
    file.read((char*) (&inputData[8]), inputSize);
    file.close();

    fCfo->GetDevice()->write_data(DTC_DMA_Engine_DAQ, inputData, sizeof(inputData));
    usleep(10);	
  }

};

#endif