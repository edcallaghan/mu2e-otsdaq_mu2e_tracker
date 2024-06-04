//-----------------------------------------------------------------------------
// interactive interface for ROOT-based GUI
// mixes high- and low-level commands
// assume everything is happening on one node
// there could be one or two DTCs and only one CFO
//-----------------------------------------------------------------------------
#ifndef __trkdaq_dtc_interface_cc__
#define __trkdaq_dtc_interface_cc__

#define __CLING__ 1

#include "iostream"
#include "DtcInterface.hh"
#include "TString.h"

using namespace DTCLib;
using namespace std;

namespace {
  // int gSleepTimeDTC      =  1000;  // [us]
  int gSleepTimeROC      =  2000;  // [us]
  int gSleepTimeROCReset =  4000;  // [us]
};

namespace trkdaq {


  DtcInterface* DtcInterface::fgInstance[2] = {nullptr, nullptr};

//-----------------------------------------------------------------------------
  DtcInterface::DtcInterface(int PcieAddr, uint LinkMask) {
    std::string expected_version("");              // dont check
    bool        skip_init       (false);
    std::string sim_file        ("mu2esim.bin");
    std::string uid             ("");
      
    fPcieAddr = PcieAddr;
    fDtc      = new DTC(DTC_SimMode_NoCFO,PcieAddr,LinkMask,expected_version,
                        skip_init,sim_file,uid);
  }

//-----------------------------------------------------------------------------
  DtcInterface::~DtcInterface() { }

//-----------------------------------------------------------------------------
  DtcInterface* DtcInterface::Instance(int PcieAddr, uint LinkMask) {
    int pcie_addr = PcieAddr;
    if (pcie_addr < 0) {
//-----------------------------------------------------------------------------
// PCIE address is not specified, check environment
//-----------------------------------------------------------------------------
      if (getenv("DTCLIB_DTC") != nullptr) pcie_addr = atoi(getenv("DTCLIB_DTC"));
      else {
        printf (" ERROR: PcieAddr < 0 and $DTCLIB_DTC is not defined. BAIL out\n");
        return nullptr;
      }
    }

    if (fgInstance[pcie_addr] == nullptr) fgInstance[pcie_addr] = new DtcInterface(pcie_addr,LinkMask);
    
    if (fgInstance[pcie_addr]->PcieAddr() != pcie_addr) {
      printf (" ERROR: DtcInterface::Instance has been already initialized with PcieAddress = %i. BAIL out\n", 
              fgInstance[pcie_addr]->PcieAddr());
      return nullptr;
    }
    else return fgInstance[pcie_addr];
  }


//-----------------------------------------------------------------------------
  void DtcInterface::PrintBuffer(const void* ptr, int nw) {

    // int     nw  = nbytes/2;
    ushort* p16 = (ushort*) ptr;
    int     n   = 0;
    
    for (int i=0; i<nw; i++) {
      if (n == 0) cout << Form(" 0x%08x: ",i*2);
      
      ushort  word = p16[i];
      cout << Form("0x%04x ",word);
      
      n   += 1;
      if (n == 8) {
        cout << std::endl;
        n = 0;
      }
    }
    
    if (n != 0) cout << std::endl;
  }

//-----------------------------------------------------------------------------
  void DtcInterface::PrintRocStatus(int Link) {
    cout << Form("-------------------- ROC %i registers:\n",Link);

    DTC* dtc = fDtc;

    DTC_Link_ID link = DTC_Link_ID(Link);
    uint32_t dat;

    uint reg = 0;
    dat = dtc->ReadROCRegister(link,reg,100);
    cout << Form("reg(%2i)             :     0x%04x : ALWAYS\n",reg, dat);

    reg = 8;
    dat = dtc->ReadROCRegister(link,reg,100);
    cout << Form("reg(%2i)             :     0x%04x :\n",reg, dat);

    reg = 18;
    dat = dtc->ReadROCRegister(link,reg,100);
    cout << Form("reg(%2i)             :     0x%04x :\n",reg, dat);

    reg = 16;
    dat = dtc->ReadROCRegister(link,reg,100);
    cout << Form("reg(%2i)             :     0x%04x :\n",reg, dat);

    reg =  7;
    dat = dtc->ReadROCRegister(link,reg,100);
    cout << Form("reg(%2i)             :     0x%04x : Fiber loss/lock counter\n",reg, dat);

    reg =  6;
    dat = dtc->ReadROCRegister(link,reg,100);
    cout << Form("reg(%2i)             :     0x%04x : Bad Markers counter\n",reg, dat);

    reg =  4;
    dat = dtc->ReadROCRegister(link,reg,100);
    cout << Form("reg(%2i)             :     0x%04x : Loopback coarse delay\n",reg, dat);

    uint iw, iw1, iw2;

    reg = 23;
    iw1 = dtc->ReadROCRegister(link,reg  ,100);
    iw2 = dtc->ReadROCRegister(link,reg+1,100);
    iw  = (iw2 << 16) | iw1;
    cout << Form("reg(%i)<<16+reg(%i) : 0x%08x : SIZE_FIFO_FULL [28]+STORE_POS[25:24]+STORE_CNT[19:0]\n",
                 reg+1,reg,iw);

    reg = 25;
    iw1 = dtc->ReadROCRegister(link,reg  ,100);
    iw2 = dtc->ReadROCRegister(link,reg+1,100);
    iw  = (iw2 << 16) | iw1;
    cout << Form("reg(%i)<<16+reg(%i) : 0x%08x : SIZE_FIFO_EMPTY[28]+FETCH_POS[25:24]+FETCH_CNT[19:0]\n",
                 reg+1,reg,iw);

    reg = 11;
    iw1 = dtc->ReadROCRegister(link,reg  ,100);
    iw2 = dtc->ReadROCRegister(link,reg+1,100);
    iw  = (iw2 << 16) | iw1;
    cout << Form("reg(%i)<<16+reg(%i) : 0x%08x : Num EWM seen\n", reg+1,reg,iw);

    reg = 64;
    iw1 = dtc->ReadROCRegister(link,reg  ,100);
    iw2 = dtc->ReadROCRegister(link,reg+1,100);
    iw  = (iw2 << 16) | iw1;
    cout << Form("reg(%i)<<16+reg(%i) : 0x%08x : Num windows seen\n", reg+1,reg,iw);

    reg = 27;
    iw1 = dtc->ReadROCRegister(link,reg  ,100);
    iw2 = dtc->ReadROCRegister(link,reg+1,100);
    iw  = (iw2 << 16) | iw1;
    cout << Form("reg(%i)<<16+reg(%i) : 0x%08x : Num HB seen\n", reg+1,reg,iw);

    reg = 29;
    iw1 = dtc->ReadROCRegister(link,reg  ,100);
    iw2 = dtc->ReadROCRegister(link,reg+1,100);
    iw  = (iw2 << 16) | iw1;
    cout << Form("reg(%i)<<16+reg(%i) : 0x%08x : Num null HB seen\n", reg+1,reg,iw);

    reg = 31;
    iw1 = dtc->ReadROCRegister(link,reg  ,100);
    iw2 = dtc->ReadROCRegister(link,reg+1,100);
    iw  = (iw2 << 16) | iw1;
    cout << Form("reg(%i)<<16+reg(%i) : 0x%08x : Num HB on hold\n", reg+1,reg,iw);

    reg = 33;
    iw1 = dtc->ReadROCRegister(link,reg  ,100);
    iw2 = dtc->ReadROCRegister(link,reg+1,100);
    iw  = (iw2 << 16) | iw1;
    cout << Form("reg(%i)<<16+reg(%i) : 0x%08x : Num PREFETCH seen\n\n", reg+1,reg,iw);

    reg = 35;
    iw1 = dtc->ReadROCRegister(link,reg  ,100);
    iw2 = dtc->ReadROCRegister(link,reg+1,100);
    iw  = (iw2 << 16) | iw1;
    cout << Form("reg(%i)<<16+reg(%i) : 0x%08x : Num DATA REQ seen\n", reg+1,reg,iw);

    reg = 13;
    dat = dtc->ReadROCRegister(link,reg,100);
    cout << Form("reg(%2i)             :     0x%04x : Num skipped DATA REQ\n",reg,dat);

    reg = 37;
    iw1 = dtc->ReadROCRegister(link,reg  ,100);
    iw2 = dtc->ReadROCRegister(link,reg+1,100);
    iw  = (iw2 << 16) | iw1;
    cout << Form("reg(%i)<<16+reg(%i) : 0x%08x : Num DATA REQ read from DDR\n", reg+1,reg,iw);

    reg = 39;
    iw1 = dtc->ReadROCRegister(link,reg  ,100);
    iw2 = dtc->ReadROCRegister(link,reg+1,100);
    iw  = (iw2 << 16) | iw1;
    cout << Form("reg(%i)<<16+reg(%i) : 0x%08x : Num DATA REQ sent to DTC\n", reg+1,reg,iw);


    reg = 41;
    iw1 = dtc->ReadROCRegister(link,reg  ,100);
    iw2 = dtc->ReadROCRegister(link,reg+1,100);
    iw  = (iw2 << 16) | iw1;
    cout << Form("reg(%i)<<16+reg(%i) : 0x%08x : Num DATA REQ with null data\n", reg+1,reg,iw);

    cout << Form("\n");
  
    reg = 43;
    iw1 = dtc->ReadROCRegister(link,reg  ,100);
    iw2 = dtc->ReadROCRegister(link,reg+1,100);
    iw  = (iw2 << 16) | iw1;
    cout << Form("reg(%i)<<16+reg(%i) : 0x%08x : Last spill tag\n", reg+1,reg,iw);

    //    uint16_t iw3;

    reg = 45;
    iw1 = dtc->ReadROCRegister(link,reg  ,100);
    iw2 = dtc->ReadROCRegister(link,reg+1,100);
    // iw3 = dtc->ReadROCRegister(link,reg+2,100);
  
    iw  = (iw2 << 16) | iw1;
    cout << Form("reg(%i)<<16+reg(%i) : 0x%08x : Last HB tag\n", reg+1,reg,iw);

    reg = 48;
    iw1 = dtc->ReadROCRegister(link,reg  ,100);
    iw2 = dtc->ReadROCRegister(link,reg+1,100);
    // iw3 = dtc->ReadROCRegister(link,reg+2,100);
  
    iw  = (iw2 << 16) | iw1;
    cout << Form("reg(%i)<<16+reg(%i) : 0x%08x : Last PREFETCH tag\n", reg+1,reg,iw);
  
    reg = 51;
    iw1 = dtc->ReadROCRegister(link,reg  ,100);
    iw2 = dtc->ReadROCRegister(link,reg+1,100);
    // iw3 = dtc->ReadROCRegister(link,reg+2,100);
  
    iw  = (iw2 << 16) | iw1;
    cout << Form("reg(%i)<<16+reg(%i) : 0x%08x : Last fetched tag\n", reg+1,reg,iw);

    reg = 54;
    iw1 = dtc->ReadROCRegister(link,reg  ,100);
    iw2 = dtc->ReadROCRegister(link,reg+1,100);
    // iw3 = dtc->ReadROCRegister(link,reg+2,100);
  
    iw  = (iw2 << 16) | iw1;
    cout << Form("reg(%i)<<16+reg(%i) : 0x%08x : Last DATA REQ tag\n", reg+1,reg,iw);

    reg = 57;
    iw1 = dtc->ReadROCRegister(link,reg  ,100);
    iw2 = dtc->ReadROCRegister(link,reg+1,100);
    // iw3 = dtc->ReadROCRegister(link,reg+2,100);
  
    iw  = (iw2 << 16) | iw1;
    cout << Form("reg(%i)<<16+reg(%i) : 0x%08x : OFFSET tag\n", reg+1,reg,iw);

    cout << std::endl;

    reg = 72;
    dat = dtc->ReadROCRegister(link,reg,100);
    cout << Form("reg(%2i)             :     0x%04x : Num HB tag inconsistencies\n",reg, dat);

    reg = 74;
    dat = dtc->ReadROCRegister(link,reg,100);
    cout << Form("reg(%2i)             :     0x%04x : Num HB tag lost\n",reg, dat);

    reg = 73;
    dat = dtc->ReadROCRegister(link,reg,100);
    cout << Form("reg(%2i)             :     0x%04x : Num DREQ tag inconsistencies\n",reg, dat);

    reg = 75;
    dat = dtc->ReadROCRegister(link,reg,100);
    cout << Form("reg(%2i)             :     0x%04x : Num DREQ tag lost\n",reg, dat);

    cout << "------------------------------------------------------------------------\n";
  }


//-----------------------------------------------------------------------------
  void DtcInterface::RocPatternConfig(int LinkMask) {
    for (int i=0; i<6; i++) {
      int used = (LinkMask >> 4*i) & 0x1;
      if (used != 0) {
        auto link = DTC_Link_ID(i);
        fDtc->WriteROCRegister(link,14,     1,false,1000);                // 1 --> r14: reset ROC

        // the delay should be hidden in the ROC firmware
        std::this_thread::sleep_for(std::chrono::microseconds(gSleepTimeROCReset));

        fDtc->WriteROCRegister(link, 8,0x0010,false,1000);              // configure ROC to send patterns
        // std::this_thread::sleep_for(std::chrono::microseconds(10*trk_daq::gSleepTimeROC));

        fDtc->WriteROCRegister(link,30,0x0000,false,1000);                // r30: mode, write 0 into it 
        // std::this_thread::sleep_for(std::chrono::microseconds(10*trk_daq::gSleepTimeROC));

        fDtc->WriteROCRegister(link,29,0x0001,false,1000);                // r29: data version, currently 1
        std::this_thread::sleep_for(std::chrono::microseconds(gSleepTimeROC));
      }
    }
                                        // not sure if this is needed, later...
    std::this_thread::sleep_for(std::chrono::microseconds(2000));  
  }

//-----------------------------------------------------------------------------
  void DtcInterface::InitEmulatedCFOReadoutMode(int EWLength, int NMarkers, int FirstEWTag) {
    //                                 int EWMode, int EnableClockMarkers, int EnableAutogenDRP) {

    int EWMode             = 1;
    int EnableClockMarkers = 0;
    // int EnableAutogenDRP   = 1;

    fDtc->SoftReset();                     // write bit 31

    fDtc->SetCFOEmulationEventWindowInterval(EWLength);  
    fDtc->SetCFOEmulationNumHeartbeats      (NMarkers);
    fDtc->SetCFOEmulationTimestamp          (DTC_EventWindowTag(FirstEWTag));
    fDtc->SetCFOEmulationEventMode          (EWMode);
    fDtc->SetCFO40MHzClockMarkerEnable      (DTC_Link_ALL,EnableClockMarkers);

    fDtc->EnableAutogenDRP();                                      // r_0x9100:bit_23 = 1

    fDtc->SetCFOEmulationMode();                                   // r_0x9100:bit_15 = 1 
    fDtc->EnableCFOEmulation();                                    // r_0x9100:bit_30 = 1 
    fDtc->EnableReceiveCFOLink();                                  // r_0x9114:bit_14 = 1
  }

//-----------------------------------------------------------------------------
  void DtcInterface::InitExternalCFOReadoutMode() {

    // which ROC links should be enabled ? - all active ?
    int EnableClockMarkers = 0; //for now
    fDtc->SetCFO40MHzClockMarkerEnable(DTC_Link_0,EnableClockMarkers);

    int SampleEdgeMode     = 0;
    fDtc->SetExternalCFOSampleEdgeMode(SampleEdgeMode);
    
    fDtc->EnableAutogenDRP();

    fDtc->ClearCFOEmulationMode();                                // r_0x9100:bit_15 = 0
    // dtc->SetCFOEmulationMode();                               // r_0x9100:bit_15 = 1

    fDtc->DisableCFOEmulation  ();                                // r_0x9100:bit_30 = 0
    // dtc->EnableCFOEmulation();                                // r_0x9100:bit_30 = 1 

    fDtc->EnableReceiveCFOLink ();                                // r_0x9114:bit_14 = 1
}

//-----------------------------------------------------------------------------
  void DtcInterface::LaunchRunPlan(int NEvents) {
    
  }

//-----------------------------------------------------------------------------
  void DtcInterface::PrintRegister(uint16_t Register, const char* Title) {
    std::cout << Form("%s (0x%04x) : 0x%08x\n",Title,Register,ReadRegister(Register));
  }

//-----------------------------------------------------------------------------
  void DtcInterface::PrintStatus() {
    cout << Form("-----------------------------------------------------------------\n");
    PrintRegister(0x9000,"DTC firmware link speed and design version ");
    PrintRegister(0x9004,"DTC version                                ");
    PrintRegister(0x9008,"Design status                              ");
    PrintRegister(0x900c,"Vivado version                             ");
    PrintRegister(0x9100,"DTC control register                       ");
    PrintRegister(0x9104,"DMA transfer length                        ");
    PrintRegister(0x9108,"SERDES loopback enable                     ");
    PrintRegister(0x9110,"ROC Emulation enable                       ");
    PrintRegister(0x9114,"Link Enable                                ");
    PrintRegister(0x9128,"SERDES PLL Locked                          ");
    PrintRegister(0x9140,"SERDES RX CDR lock (locked fibers)         ");
    PrintRegister(0x9144,"DMA Timeout Preset                         ");
    PrintRegister(0x9148,"ROC reply timeout                          ");
    PrintRegister(0x914c,"ROC reply timeout error                    ");
    PrintRegister(0x9158,"Event Builder Configuration                ");
    PrintRegister(0x91a8,"CFO Emulation Heartbeat Interval           ");
    PrintRegister(0x91ac,"CFO Emulation Number of HB Packets         ");
    PrintRegister(0x91bc,"CFO Emulation Number of Null HB Packets    ");
    PrintRegister(0x91f4,"CFO Emulation 40 MHz Clock Marker Interval ");
    PrintRegister(0x91f8,"CFO Marker Enables                         ");

    PrintRegister(0x9200,"Receive  Byte   Count Link 0               ");
    PrintRegister(0x9220,"Receive  Packet Count Link 0               ");
    PrintRegister(0x9240,"Transmit Byte   Count Link 0               ");
    PrintRegister(0x9260,"Transmit Packet Count Link 0               ");

    PrintRegister(0x9218,"Receive  Byte   Count CFO                  ");
    PrintRegister(0x9238,"Receive  Packet Count CFO                  ");
    PrintRegister(0x9258,"Transmit Byte   Count CFO                  ");
    PrintRegister(0x9278,"Transmit Packet Count CFO                  ");
  }

//-----------------------------------------------------------------------------
  uint32_t DtcInterface::ReadRegister(uint16_t Register) {

    uint32_t data;
    int      timeout(150);
    
    mu2edev* dev = fDtc->GetDevice();
    dev->read_register(Register,timeout,&data);
    
    return data;
  }

//-----------------------------------------------------------------------------
// ROC reset : write 0x1 to register 14
//-----------------------------------------------------------------------------
  void DtcInterface::ResetRoc(int Link) {
    int tmo_ms(1000);
    fDtc->WriteROCRegister(DTC_Link_ID(Link),14,1,false,tmo_ms);  // 1 --> r14: reset ROC
  }


//-----------------------------------------------------------------------------
// ROC reset : write 0x1 to register 14
//-----------------------------------------------------------------------------
  void DtcInterface::ReadSubevents(std::vector<std::unique_ptr<DTCLib::DTC_SubEvent>>& VSub, 
                                   ulong       FirstTS  ,
                                   int         PrintData, 
                                   const char* Fn       ) {
    ulong    ts       = FirstTS;
    bool     match_ts = false;
    // int      nevents  = 0;

    FILE*    file(nullptr);
    if (Fn != nullptr) {
//-----------------------------------------------------------------------------
// open output file
//-----------------------------------------------------------------------------
      if((file = fopen(Fn,"r")) != NULL) {
        // file exists
        fclose(file);
        cout << "ERROR in " << __func__ << " : file " << Fn << " already exists, BAIL OUT" << endl;
        return;
      }
      else {
        file = fopen(Fn,"w");
        if (file == nullptr) {
          cout << "ERROR in " << __func__ << " : failed to open " << Fn << " , BAIL OUT" << endl;
          return;
        }
      }
    }
//-----------------------------------------------------------------------------
// always read an event into the same external buffer (VSub), 
// so no problem with the memory management
//-----------------------------------------------------------------------------
    while(1) {
      DTC_EventWindowTag event_tag = DTC_EventWindowTag(ts);
      try {
        // VSub.clear();
        VSub   = fDtc->GetSubEventData(event_tag, match_ts);
        int sz = VSub.size();
        
        if (PrintData > 0) {
          cout << Form(">>>> ------- ts = %5li NDTCs:%2i",ts,sz);
          if (sz == 0) {
            cout << std::endl;
            break;
          }
        }
        
        ts++;
        int rs[6];
        for (int i=0; i<sz; i++) {
          DTC_SubEvent* ev = VSub[i].get();
          int      nb     = ev->GetSubEventByteCount();
          uint64_t ew_tag = ev->GetEventWindowTag().GetEventWindowTag(true);
          char*    data   = (char*) ev->GetRawBufferPointer();

          if (file) {
//-----------------------------------------------------------------------------
// write event to output file
//-----------------------------------------------------------------------------
            int nbb = fwrite(data,1,nb,file);
            if (nbb == 0) {
              cout << "ERROR in " << __func__ << " : failed to write event " << ew_tag 
                   << " , close file and BAIL OUT" << endl;
              fclose(file);
              return;
            }
          }

          char* roc_data  = data+0x30;

          for (int roc=0; roc<6; roc++) {
            int nb    = *((ushort*) roc_data);
            rs[roc]   = *((ushort*)(roc_data+0x0c));
            roc_data += nb;
          }
        
          if (PrintData > 0) {
            cout << Form(" DTC:%2i EWTag:%10li nbytes: %4i ROC status: 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x ",
                         i,ew_tag,nb,rs[0],rs[1],rs[2],rs[3],rs[4],rs[5]);
          }
          
          if (PrintData > 1) {
            cout << std::endl;
            PrintBuffer(ev->GetRawBufferPointer(),ev->GetSubEventByteCount()/2);
          }
        }
        if (PrintData > 0) cout << std::endl;
        
      }
      catch (...) {
        cout << "ERROR reading ts = %i" << ts << std::endl;
        break;
      }
    }

    fDtc->ReleaseAllBuffers(DTC_DMA_Engine_DAQ);
//-----------------------------------------------------------------------------
// to simplify first steps, assume that in a file writing mode all events 
// are read at once, so close the file on exit
//-----------------------------------------------------------------------------
    if (file) {
      fclose(file);
    }
  }


//-----------------------------------------------------------------------------
// configure itself to use a CFO
//-----------------------------------------------------------------------------
  void DtcInterface::SetupCfoInterface(int CFOEmulationMode, int ForceCFOEdge, int EnableCFORxTx, int EnableAutogenDRP) {
    // int tmo_ms(150);

    if (CFOEmulationMode == 0) fDtc->ClearCFOEmulationMode();
    else                       fDtc->SetCFOEmulationMode  ();

    fDtc->SetExternalCFOSampleEdgeMode(ForceCFOEdge);

    if (EnableCFORxTx == 0) {
      fDtc->DisableReceiveCFOLink ();
      fDtc->DisableTransmitCFOLink();
    }
    else {
      fDtc->EnableReceiveCFOLink  ();
      fDtc->EnableTransmitCFOLink ();
    }
    
    if (EnableAutogenDRP == 0) fDtc->DisableAutogenDRP();
    else                       fDtc->EnableAutogenDRP ();
  }

//-----------------------------------------------------------------------------
  void DtcInterface::PrintFireflyTemp() {
    int tmo_ms(50);

//-----------------------------------------------------------------------------
// read RX firefly temp
//------------------------------------------------------------------------------
    fDtc->GetDevice()->write_register(0x93a0,tmo_ms,0x00000100);
    std::this_thread::sleep_for(std::chrono::milliseconds(tmo_ms));
    fDtc->GetDevice()->write_register(0x9288,tmo_ms,0x50160000);
    std::this_thread::sleep_for(std::chrono::milliseconds(tmo_ms));
    fDtc->GetDevice()->write_register(0x928c,tmo_ms,0x00000002);
    std::this_thread::sleep_for(std::chrono::milliseconds(tmo_ms));
    fDtc->GetDevice()->write_register(0x93a0,tmo_ms,0x00000000);
    std::this_thread::sleep_for(std::chrono::milliseconds(tmo_ms));

    uint data, rx_temp, txrx_temp;

    fDtc->GetDevice()->read_register(0x9288,tmo_ms,&data);
    rx_temp = data & 0xff;
//-----------------------------------------------------------------------------
// read TX/RX firefly temp
//------------------------------------------------------------------------------
    fDtc->GetDevice()->write_register(0x93a0,tmo_ms,0x00000400);
    std::this_thread::sleep_for(std::chrono::milliseconds(tmo_ms));
    fDtc->GetDevice()->write_register(0x92a8,tmo_ms,0x50160000);
    std::this_thread::sleep_for(std::chrono::milliseconds(tmo_ms));
    fDtc->GetDevice()->write_register(0x92ac,tmo_ms,0x00000002);
    std::this_thread::sleep_for(std::chrono::milliseconds(tmo_ms));
    fDtc->GetDevice()->write_register(0x93a0,tmo_ms,0x00000000);
    std::this_thread::sleep_for(std::chrono::milliseconds(tmo_ms));

    fDtc->GetDevice()->read_register(0x92a8,tmo_ms,&data);
    txrx_temp = data & 0xff;

    cout << "rx_temp: " << rx_temp << " txrx_temp: " << txrx_temp << endl;
  }

};

#endif
