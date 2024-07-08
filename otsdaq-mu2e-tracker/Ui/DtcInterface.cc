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

#include "TRACE/tracemf.h"
#define TRACE_NAME "DtcInterface"

using namespace DTCLib;
using namespace std;

namespace {
  // int gSleepTimeDTC      =  1000;  // [us]
  //  int gSleepTimeROC      =  2000;  // [us]
  int gSleepTimeROCReset =  4000;  // [us]
};

namespace trkdaq {


  DtcInterface* DtcInterface::fgInstance[2] = {nullptr, nullptr};

//-----------------------------------------------------------------------------
  DtcInterface::DtcInterface(int PcieAddr, uint LinkMask, bool SkipInit) {
    std::string expected_version("");              // dont check
    std::string sim_file        ("mu2esim.bin");
    std::string uid             ("");
      
    fPcieAddr = PcieAddr;
    fLinkMask = LinkMask;
    fDtc      = new DTC(DTC_SimMode_NoCFO,PcieAddr,LinkMask,expected_version,
                        SkipInit,sim_file,uid);
    fDtc->ClearCFOEmulationMode();
    fDtc->SoftReset();
  }

//-----------------------------------------------------------------------------
  DtcInterface::~DtcInterface() { }

//-----------------------------------------------------------------------------
  DtcInterface* DtcInterface::Instance(int PcieAddr, uint LinkMask, bool SkipInit) {
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

    if (fgInstance[pcie_addr] == nullptr) fgInstance[pcie_addr] = new DtcInterface(pcie_addr,LinkMask, SkipInit);
    
    if (fgInstance[pcie_addr]->PcieAddr() != pcie_addr) {
      printf (" ERROR: DtcInterface::Instance has been already initialized with PcieAddress = %i. BAIL out\n", 
              fgInstance[pcie_addr]->PcieAddr());
      return nullptr;
    }
    else return fgInstance[pcie_addr];
  }


//-----------------------------------------------------------------------------
// Source=0: sync to internal clock ; 1: RTF
// on success, returns 1
//-----------------------------------------------------------------------------
  int DtcInterface::ConfigureJA(int ClockSource, int Reset) {
    fDtc->SetJitterAttenuatorSelect(ClockSource,Reset);     // 0:internal clock sync, 1:RTF
    usleep(100000);
    int ok(0);
    for (int i=0; i<3; i++) {
      ok = fDtc->ReadJitterAttenuatorLocked();              // in case of success, returns true
      usleep(100000);
      if (ok == 1) break;
    }
    
    fDtc->FormatJitterAttenuatorCSR();

    if (ok == 0) printf("ERROR in DtcInterface::%s: failed to setup JA\n",__func__); 

    return ok;
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
    cout << Form("reg(%2i)             :     0x%04x : ROC pattern mode ?? \n",reg, dat);

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

    reg =  9;
    iw1 = dtc->ReadROCRegister(link,reg  ,100);
    iw2 = dtc->ReadROCRegister(link,reg+1,100);
    iw  = (iw2 << 16) | iw1;
    cout << Form("reg(%2i)<<16+reg(%2i) : 0x%08x : Num DATA REQ seen\n", reg+1,reg,iw);

    reg = 35;
    iw1 = dtc->ReadROCRegister(link,reg  ,100);
    iw2 = dtc->ReadROCRegister(link,reg+1,100);
    iw  = (iw2 << 16) | iw1;
    cout << Form("reg(%i)<<16+reg(%i) : 0x%08x : Num DATA REQ written to DDR\n", reg+1,reg,iw);

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
// according to Ryan, disabling the CFO emulation is critical, otherwise NMarkers
// would be cached for the next time
//-----------------------------------------------------------------------------
  void DtcInterface::InitEmulatedCFOReadoutMode(int EWLength, int NMarkers, int FirstEWTag) {
    //                                 int EWMode, int EnableClockMarkers, int EnableAutogenDRP) {

    int EWMode             = 1;
    int EnableClockMarkers = 0;
    // int EnableAutogenDRP   = 1;

    fDtc->DisableCFOEmulation();
    fDtc->DisableAutogenDRP();
    
    fDtc->SoftReset();                     // write bit 31

    fDtc->SetCFOEmulationEventWindowInterval(EWLength);  
    fDtc->SetCFOEmulationNumHeartbeats      (NMarkers);
    fDtc->SetCFOEmulationTimestamp          (DTC_EventWindowTag((uint64_t) FirstEWTag));
    fDtc->SetCFOEmulationEventMode          (EWMode);
    fDtc->SetCFO40MHzClockMarkerEnable      (DTC_Link_ALL,EnableClockMarkers);

    fDtc->EnableAutogenDRP();                                      // r_0x9100:bit_23 = 1

    fDtc->SetCFOEmulationMode();                                   // r_0x9100:bit_15 = 1 
    fDtc->EnableCFOEmulation();                                    // r_0x9100:bit_30 = 1 
    fDtc->EnableReceiveCFOLink();                                  // r_0x9114:bit_14 = 1
  }

//-----------------------------------------------------------------------------
  void DtcInterface::InitExternalCFOReadoutMode(int SampleEdgeMode) {

    fDtc->SoftReset();                  // write 0x9100:bit_31=1

                                        // which ROC links should be enabled ? - all active ?
    int EnableClockMarkers = 0;         // for now
    fDtc->SetCFO40MHzClockMarkerEnable(DTC_Link_ALL,EnableClockMarkers);

    fDtc->SetExternalCFOSampleEdgeMode(SampleEdgeMode);
    
    fDtc->EnableAutogenDRP();

    fDtc->ClearCFOEmulationMode();      // r_0x9100:bit_15 = 0
    // dtc->SetCFOEmulationMode();      // r_0x9100:bit_15 = 1

    fDtc->DisableCFOEmulation  ();      // r_0x9100:bit_30 = 0
    // dtc->EnableCFOEmulation();       // r_0x9100:bit_30 = 1 

    fDtc->EnableReceiveCFOLink ();      // r_0x9114:bit_14 = 1
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

    // PrintRegister(0x9200,"Receive  Byte   Count Link 0               ");
    // PrintRegister(0x9220,"Receive  Packet Count Link 0               ");
    // PrintRegister(0x9240,"Transmit Byte   Count Link 0               ");
    // PrintRegister(0x9260,"Transmit Packet Count Link 0               ");

    // PrintRegister(0x9218,"Receive  Byte   Count CFO                  ");
    // PrintRegister(0x9238,"Receive  Packet Count CFO                  ");
    // PrintRegister(0x9258,"Transmit Byte   Count CFO                  ");
    // PrintRegister(0x9278,"Transmit Packet Count CFO                  ");

    PrintRegister(0x9308,"Jitter Attenuator CSR                      ");

    PrintRegister(0x9630,"TX Data Request Packet Count Link 0        ");
    PrintRegister(0x9631,"TX Data Request Packet Count Link 1        ");

    PrintRegister(0x9650,"TX Heartbeat    Packet Count Link 0        ");
    PrintRegister(0x9651,"TX Heartbeat    Packet Count Link 1        ");

    PrintRegister(0x9670,"RX Data Header  Packet Count Link 0        ");
    PrintRegister(0x9671,"RX Data Header  Packet Count Link 1        ");

    PrintRegister(0x9690,"RX Data         Packet Count Link 0        ");
    PrintRegister(0x9691,"RX Data         Packet Count Link 1        ");

    PrintRegister(0xa400,"TX Event Window Marker Count Link 0        ");
    PrintRegister(0xa404,"TX Event Window Marker Count Link 0        ");

    PrintRegister(0xa420,"RX Data Header Timeout Count Link 0        ");
    PrintRegister(0xa424,"RX Data Header Timeout Count Link 0        ");

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
// 
  void DtcInterface::RocConfigurePatternMode(int LinkMask) {

    if (LinkMask != 0) fLinkMask = LinkMask;
    
    RocReset(0);

    for (int i=0; i<6; i++) {
      int used = (fLinkMask >> 4*i) & 0x1;
      if (used != 0) {
        auto link = DTC_Link_ID(i);

        fDtc->WriteROCRegister(link, 8,0x0010,false,1000);              // configure ROC to send patterns
        // std::this_thread::sleep_for(std::chrono::microseconds(10*trk_daq::gSleepTimeROC));

        // fDtc->WriteROCRegister(link,30,0x0000,false,1000);                // r30: mode, write 0 into it 
        // // std::this_thread::sleep_for(std::chrono::microseconds(10*trk_daq::gSleepTimeROC));
      }
    }

    RocSetDataVersion(LinkMask,1); // Version --> R29
                                         // not sure if this is needed, later...
    std::this_thread::sleep_for(std::chrono::microseconds(2000));  
  }

//-----------------------------------------------------------------------------
// ROC reset : write 0x1 to R14 of each ROC specified as active by the mask
//-----------------------------------------------------------------------------
  void DtcInterface::RocReset(int LinkMask) {
    if (LinkMask != 0) fLinkMask = LinkMask;
    
    int tmo_ms(100);
    for (int i=0; i<6; i++) {
      int used = (fLinkMask >> 4*i) & 0x1;
      if (used != 0) {
        fDtc->WriteROCRegister(DTC_Link_ID(i),14,1,false,tmo_ms);  // 1 --> r14: reset ROC
      }
    }
    
    std::this_thread::sleep_for(std::chrono::microseconds(gSleepTimeROCReset));
  }

//-----------------------------------------------------------------------------
// Version --> R29 
//-----------------------------------------------------------------------------
  void DtcInterface::RocSetDataVersion(int Version, int LinkMask) {
    if (LinkMask != 0) fLinkMask = LinkMask;
    
    int tmo_ms(100);
    for (int i=0; i<6; i++) {
      int used = (fLinkMask >> 4*i) & 0x1;
      if (used != 0) {
        fDtc->WriteROCRegister(DTC_Link_ID(i),29,Version,false,tmo_ms);
      }
    }
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
// check if Fn exists 
//-----------------------------------------------------------------------------
      if((file = fopen(Fn,"r")) != NULL) {
        // file exists
        fclose(file);
        cout << "ERROR in " << __func__ << " : file " << Fn << " already exists, BAIL OUT" << endl;
        return;
      }
      else {
//-----------------------------------------------------------------------------
// Fn doesn't exist, open it 
//-----------------------------------------------------------------------------
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
        
        ts++;
      }
      catch (...) {
        TLOG(TLVL_ERROR) << "ERROR reading event_tag:" << event_tag.GetEventWindowTag(true) << " ts:" << ts << std::endl;
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
  void DtcInterface::SetBit(int Register, int Bit, int Value) {
    int tmo_ms(100);

    uint32_t data;
    fDtc->GetDevice()->read_register(Register,tmo_ms,&data);
    
    uint32_t w = (1 << Bit);
    
    data = (data ^ w) | (Value << Bit);
    fDtc->GetDevice()->write_register(Register,tmo_ms,data);
  }

//-----------------------------------------------------------------------------
// configure itself to use a CFO
//-----------------------------------------------------------------------------
  void DtcInterface::SetupCfoInterface(int CFOEmulationMode, int ForceCFOEdge,
                                       int EnableCFORxTx   , int EnableAutogenDRP) {
    // int tmo_ms(150);

    if (CFOEmulationMode == 0) fDtc->ClearCFOEmulationMode();
    else                       fDtc->SetCFOEmulationMode  ();

// ForceCFOEdge: defines bit_6 and bit_5 of the control register 0x9100
// bit_6: 1:force       0:auto
// bit_5: 0:rising edge 1:falling edge
// ForceCFOEdge = 0 : force use of the rising  edge
//              = 1 : force use of the falling edge
//              = 2 : auto
    
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

struct RocData_t {
  ushort  nb;
  ushort  header;
  ushort  n_data_packets;  // n data packets, 16 bytes each
  ushort  ewt[3];
  ushort  status;
  ushort  xxx2;
  ushort  data; // array, use it just for memory mapping
};
  
//-----------------------------------------------------------------------------
// returns number of found errors in the payload data
// assume ROC pattern generation
// 'Offset' is the
// PrintLevel =  0: print nothing
//            =  1: print all about errors
//            > 10: full printout 
//-----------------------------------------------------------------------------
int DtcInterface::ValidateDtcBlock(ushort* Data, ulong EwTag, ulong* Offset, int PrintLevel) {

  int nhits[64] = {
    1,   2,  3,  0,  0,  0,  7,  8,
    9,  10, 11, 12, 13, 14, 15, 16,
    0,  20, 21, 22, 12, 13, 11, 12,
    0,   0,  8,  4, 12, 11, 12, 13,
    16,  6,  3,  1, 12,  0, 16, 17,
    18, 19, 12,  1, 12, 12, 11, 11,
     0,  0,  0,  0, 13, 14, 10, 13,
    11, 14, 14, 15,  8,  9, 10, 32
  };

//-----------------------------------------------------------------------------
// check consistency of the lengths
// 1. total number of 2-byte words
//
//-----------------------------------------------------------------------------
  int ewt    = EwTag % 64 ;
  int nb_dtc = *Data;

  RocData_t* roc = (RocData_t*) (Data+0x18);

  int nb_rocs = 0;
  for (int i=0; i<6; i++) {
    int nb   = roc->nb;
    nb_rocs += nb;
    roc      = (RocData_t*) ( ((char*) roc) + roc->nb);
  }

  int nerr     = 0;

  if (nb_dtc != nb_rocs+0x30) {
    if (PrintLevel > 0) printf("ERROR: nb_dtc, nb_rocs : 0x%04x 0x%04x\n",nb_dtc,nb_rocs);
    nerr += 1;
  }
//-----------------------------------------------------------------------------
// event length checks out, check ROC payload
// check the ROC payload, assume a hit = 2 packets
//-----------------------------------------------------------------------------
  roc = (RocData_t*) (Data+0x18);
  for (int i=0; i<6; i++) {
    int nb = roc->nb;
//-----------------------------------------------------------------------------
// validate ROC header
//-----------------------------------------------------------------------------
    // ... TODO
    ulong ewtag_roc = ulong(roc->ewt[0]) | (ulong(roc->ewt[1]) << 16) | (ulong(roc->ewt[2]) << 32);

    if (ewtag_roc != EwTag) {
      if (PrintLevel > 0) printf("ERROR: roc EwTag ewt_roc : %i 0x%08lx 0x%08lx\n",i,EwTag,ewtag_roc);
      nerr += 1;
    }
    
    if (roc->nb > 0x10) { 
//-----------------------------------------------------------------------------
// non-zero payload
//-----------------------------------------------------------------------------
      uint*   pattern  = (uint*) &roc->data;
      if (PrintLevel > 10) printf("data[0]  = nb = 0x%04x\n",pattern[0]);
 
      int npackets     = roc->n_data_packets;
      int npackets_exp = nhits[ewt]*2;       // assume two packets per hit (this number is stored somewhere)

      if (npackets != npackets_exp) {
        if (PrintLevel > 0) printf("ERROR: EwTag roc npackets npackets_exp: 0x%08lx %i %5i %5i\n",
                                  EwTag,i,npackets,npackets_exp);
        nerr += 1;
      }
      
      if (PrintLevel > 10) {
        printf("EwTag, ewt, npackets, npackets_exp,  offset: %10lu %3i %2i %2i %10lu\n",
               EwTag,  ewt, npackets, npackets_exp, *Offset);
      }

      uint nw      = npackets*4;        // N 4-byte words
    
      for (uint i=0; i<nw; i++) {
        uint exp_pattern = (i+*Offset) & 0xffffffff;
    
        if (pattern[i] != exp_pattern) {
          nerr += 1;
          if (PrintLevel > 0) {
            printf("ERROR: EwTag, ewt i  offset payload[i]  exp_word: %10lu %3i %3i 0x%08x 0x%08x\n",
                   EwTag, ewt, i, *Offset,pattern[i],exp_pattern);
          }
        }
      }
    }
    roc = (RocData_t*) (((char*) roc) + roc->nb);
  }
  
  *Offset += 2*4*nhits[ewt];

  if (PrintLevel > 10) printf("EwTag = %10lx, nb_dtc = %i nerr = %i\n",EwTag,nb_dtc,nerr);

  return nerr;
}

};

#endif