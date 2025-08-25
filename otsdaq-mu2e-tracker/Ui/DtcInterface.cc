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
#include "vector"

// #include "artdaq-core-mu2e/Overlays/Decoders/TrackerDataDecoder.hh"
#include "artdaq-core-mu2e/Data/TrackerDataDecoder.hh"

#include "DtcInterface.hh"
#include "TString.h"    // includes ROOT's Form

#include "TRACE/tracemf.h"
#define  TRACE_NAME "DtcInterface"

using namespace DTCLib;
using namespace std;



namespace {
  bool initialized = 0;
};

namespace trkdaq {
                                        // channel readout sequence
                                        // the first 48 are readout by the digi FPGA on the CAL side
                                        // the rest 48 - by the FPGA on the HV side
  int adc_index[96] = {
    91, 85, 79, 73, 67, 61, 55, 49,          // lane 0
    43, 37, 31, 25, 19, 13,  7,  1,
    90, 84, 78, 72, 66, 60, 54, 48,
      
    42, 36, 30, 24, 18, 12,  6,  0,          // lane 1
    93, 87, 81, 75, 69, 63, 57, 51,
    45, 39, 33, 27, 21, 15,  9,  3,
      
    44, 38, 32, 26, 20, 14,  8,  2,          // lane 2
    92, 86, 80, 74, 68, 62, 56, 50,
    47, 41, 35, 29, 23, 17, 11,  5,
      
    95, 89, 83, 77, 71, 65, 59, 53,          // lane 3
    46, 40, 34, 28, 22, 16, 10,  4,
    94, 88, 82, 76, 70, 64, 58, 52
  };

  const char* kSpiVarName[TrkSpiDataNWords] = {
    "I3_3", "I2_5", "I1_8HV" , "IHV5_0",                          //  0
    "VDMBHV5_0", "V1_8HV"  , "V3_3HV", "V2_5" ,                   //  4
    "A0"     , "A1"  ,    "A2"  , "A3"  ,                         //  8
    "I1_8CAL", "I1_2"  , "ICAL5_0"  ,                             // 12
    "ADCSPARE",                                                   // 15
    "V3_3"  , "VCAL5_0", "V1_8CAL", "V1_0",                       // 16
    "ROCPCBTEMP", "HVPCBTEMP", "CALPCBTEMP", "RTD",               // 20
    "ROC_RAIL_1V", "ROC_RAIL_1_8V", "ROC_RAIL_2_5V", "ROC_TEMP",  // 24
    "CAL_RAIL_1V", "CAL_RAIL_1_8V", "CAL_RAIL_2_5V", "CAL_TEMP",  // 28
    "HV_RAIL_1V" , "HV_RAIL_1_8V" , "HV_RAIL_2_5V" , "HV_TEMP"    // 32
  };

  const char*   DtcInterface::fgSpiVarName[TrkSpiDataNWords];
  int           DtcInterface::fgFpga[96];
//-----------------------------------------------------------------------------
// default ROC readout mode:0
//-----------------------------------------------------------------------------
  DtcInterface::DtcInterface(int PcieAddr, uint LinkMask, bool SkipInit) 
    : mu2edaq::DtcInterface(PcieAddr, LinkMask, SkipInit) {
    fRocLaneMask     = 0xf;              // all lanes enabled
    fRocNHitsPerLane = 2;                // Monica's default for fRocReadoutMode=2
    if (not initialized) {
      for (int i=0; i<96; i++) {
        int ich     = adc_index[i];
        int fpga    = i/48;
        fgFpga[ich] = fpga;
      }
      initialized = true;
    }
  };

//-----------------------------------------------------------------------------
// in many cases, want SkipInit=false
//-----------------------------------------------------------------------------
  DtcInterface* DtcInterface::Instance(int PcieAddr, uint LinkMask, bool SkipInit) {
    int pcie_addr = PcieAddr;
    if (pcie_addr < 0) {
//-----------------------------------------------------------------------------
// PCIE address is not specified, check environment
//-----------------------------------------------------------------------------
      if (getenv("DTCLIB_DTC") != nullptr) {
        pcie_addr = atoi(getenv("DTCLIB_DTC"));
      }
      else {
        TLOG(TLVL_ERROR) << Form("PcieAddr < 0 and $DTCLIB_DTC is not defined. BAIL out\n");
        return nullptr;
      }
    }
//-----------------------------------------------------------------------------
// initialize the variable names just once
//-----------------------------------------------------------------------------
    if ((fgInstance[0] == nullptr) and (fgInstance[1] == nullptr)) {
      for (int i=0; i<TrkSpiDataNWords; i++) {
        fgSpiVarName[i] = kSpiVarName[i];
      }
    }
                                    
    TLOG(TLVL_DEBUG) << "inputs      : TRK PcieAddr: " << PcieAddr
                     << " pcie_addr:" << pcie_addr 
                     << " fgInstance[pcie_addr]:0x" << std::hex << fgInstance[pcie_addr] 
                     << " LinkMask:0x" << std::hex << LinkMask
                     << std::dec
                     << " SkipInit:" << SkipInit << std::endl;
    
    trkdaq::DtcInterface* dtc_i (nullptr);
    
    if (fgInstance[pcie_addr] == nullptr) {
      fgInstance[pcie_addr] = new DtcInterface(pcie_addr,LinkMask,SkipInit);
      dtc_i = (trkdaq::DtcInterface*) fgInstance[pcie_addr];
      TLOG(TLVL_DEBUG) << "instantiated: TRK pcie_addr:" << pcie_addr
                       << " fgInstance[pcie_addr]:0x" << std::hex << dtc_i  
                       << " dtc_i->fLinkMask:0x" << std::hex << dtc_i->fLinkMask; 
    }
    else {
//-----------------------------------------------------------------------------
// already unutualized, double-check
//-----------------------------------------------------------------------------
      if (fgInstance[pcie_addr]->PcieAddr() != pcie_addr) {
        TLOG(TLVL_ERROR) << Form("DtcInterface::Instance already initialized with PcieAddress = %i. BAIL out\n", 
                                 fgInstance[pcie_addr]->PcieAddr());
      }
      else {
        dtc_i = dynamic_cast<trkdaq::DtcInterface*>(fgInstance[pcie_addr]);
        TLOG(TLVL_DEBUG) << "instantiated: TRK pcie_addr:" << pcie_addr
                         << " fgInstance[pcie_addr]:0x" << std::hex << dtc_i  
                         << " dtc_i->fLinkMask:0x" << std::hex << dtc_i->fLinkMask; 
      }
    }
    return dtc_i;
  }

//-----------------------------------------------------------------------------
  roc_serial_t DtcInterface::ReadSerialNumber(const DTCLib::DTC_Link_ID& Link) {

    bool ok(false);
    for (int i=0; i<6; i++) {
      int enabled = (fLinkMask >> 4*i) & 1;
      if (enabled and (i == Link)) {
        ok = true;
      }
    }
    if (not ok) {
      TLOG(TLVL_ERROR) << "Link " << int(Link) << " is not enabled" << std::endl; 
      return "";
    }

    auto returned = this->ReadDeviceID(Link);

    stringstream ss;
    ss << "0x";

    // first 16 words are the serial number, print it in the right order
    for (int i = 15 ; i >= 0 ; i--){
      ss << hex << returned[i];

    }

    auto rv = ss.str();
    return rv;
  }

//-----------------------------------------------------------------------------
// fRocReadoutMode is supposed to be already set, don't reinitialize
// fRocReadoutMode = 0: read ROC-renerated patterns
//                 = 1: read digis

//-----------------------------------------------------------------------------
// this is fully tracker-specific
//-----------------------------------------------------------------------------
  int DtcInterface::InitRocReadoutMode() {
    int rc(0);
    
    TLOG(TLVL_DEBUG) << Form("-- START: fRocReadoutMode=%i\n",fRocReadoutMode);
//-----------------------------------------------------------------------------
// this should be the only place where we reset the ROC
// ROC readout mode (fixed_length << 4) | readout_mode
//-----------------------------------------------------------------------------
// 2025-01-19 PM    ResetLinks();       // forget it ! ... /*this seems to be necesary*/
    
    if (((fRocReadoutMode & 0xf) == 0) || ((fRocReadoutMode & 0xf) == 2)) {
      rc = MonicaVarPatternConfig();                  // readout ROC patterns
    }
    else if ((fRocReadoutMode & 0xf) == 1) {
      rc = MonicaVarLinkConfig();                      // readout ROC digis
      if (rc < 0) {
        TLOG(TLVL_ERROR) << "failed to configure the links, rc:" << rc;
        return rc;
      }
        

      // ostringstream sout;
      // PrintRocStatus(1,-1,sout);
      // TLOG(TLVL_DEBUG) << "after MonicaVarLinkConfig:\n" << sout.str();
      
      rc = MonicaDigiClear();                          //
      if (rc < 0) {
        return rc;
      }
    }
    else {
      TLOG(TLVL_DEBUG) << "unknown mode:" << fRocReadoutMode << "> BAIL OUT";
    }
    TLOG(TLVL_DEBUG) << Form("-- END: fRocReadoutMode=%i\n",fRocReadoutMode);

    return rc;
  }

//-----------------------------------------------------------------------------
// preserve historic naming convention- Monica named her script 'var_pattern_config'
//-----------------------------------------------------------------------------
  void DtcInterface::RocConfigurePatternMode() {
    MonicaVarPatternConfig();
  }

//-----------------------------------------------------------------------------
// ROC reset : write 0x1 to R14 of each ROC specified as active by the mask
// by default, don't redefine the link mask
//-----------------------------------------------------------------------------
  int DtcInterface::ResetLink(int Link) {
    int tmo_ms(100), rc(0);

    int lnk1(Link), lnk2(Link+1);
    if (Link == -1) {
      lnk1 = 0;
      lnk2 = 6;
    }
    for (int lnk=lnk1; lnk<lnk2; ++lnk) {
      try {
        fDtc->WriteROCRegister(DTC_Link_ID(lnk),14,1,false,tmo_ms);       // 1 --> r14: reset ROC
        std::this_thread::sleep_for(std::chrono::microseconds(fSleepTimeROCReset));
      }
      catch(...) {
        TLOG(TLVL_ERROR) << "Failed to reset link:" << lnk;
        rc = -1;
      }
    }
    return rc;
  }

//-----------------------------------------------------------------------------
// Version --> R29k
// as thre is no point inhaving different ROCs with different data versions, assume
// that specifying the mask means that we want it to be redefined
//-----------------------------------------------------------------------------
  void DtcInterface::RocSetDataVersion(int Version, int LinkMask) {
    if (LinkMask != 0) fLinkMask = LinkMask;
    
    int tmo_ms(100);
    for (int i=0; i<6; i++) {
      int enabled = (fLinkMask >> 4*i) & 0x1;
      if (enabled != 0) {
        fDtc->WriteROCRegister(DTC_Link_ID(i),29,Version,false,tmo_ms);
      }
    }
    std::this_thread::sleep_for(std::chrono::microseconds(fSleepTimeROCWrite));
  }

//-----------------------------------------------------------------------------
// assume that only one link is specified (not DTC_Link_ALL)
// read serial number and device info
//-----------------------------------------------------------------------------
  vector<roc_data_t> DtcInterface::ReadDeviceID(DTCLib::DTC_Link_ID Link, int PrintLevel, std::ostream& Stream) {
    vector<roc_data_t> rv;

    int ilink = int(Link);
    if (not LinkEnabled(ilink)) {
      Stream << "ERROR: Link " << ilink << " is not enabled" << std::endl; 
      return rv;
    }
                                        // reset only ROC in question
                                        // 2024-11-14: Monica tells reset is not needed
    //    this->ResetRoc(Link,0);
    // write nothing to trigger query
    vector<roc_data_t> empty;
    fDtc->WriteROCBlock(Link, 260, empty, false, false, 1000);
    std::this_thread::sleep_for(std::chrono::microseconds(fSleepTimeROCWrite));

    // read back payload
    rv = this->ReadROCBlockEnsured(Link, 260);

    if (PrintLevel & 0x1) {
      PrintBuffer(rv.data(),rv.size(),&Stream);
    }

    return rv;
  }


//-----------------------------------------------------------------------------
// align ROC fpga/adc signals, and optionally print summary table
// REG_FINDALIGNMENT=264
//-----------------------------------------------------------------------------
  Alignment DtcInterface::FindAlignment(DTC_Link_ID Link) {
    // write parameters into roc to initiate routine
    vector<roc_data_t> writeable = {
      4,                             // eye-monitor width
      0,                             // initial adc phase
      1,                             // flag to check adc patterns
      static_cast<uint16_t>(-1),     // for channel remapping; unused
      static_cast<uint16_t>(-1),     // for channel remapping; unused
      0xFFFF,                        // bitmask for channels  0 - 15
      0xFFFF,                        // bitmask for channels 16 - 31
      0xFFFF,                        // bitmask for channels 32 - 47
      0xFFFF,                        // bitmask for channels 48 - 63
      0xFFFF,                        // bitmask for channels 64 - 79
      0xFFFF,                        // bitmask for channels 80 - 95
    };

    // register 264: find alignment routine
    bool increment_address = false; // read via fifo
    fDtc->WriteROCBlock(Link, REG_FINDALIGNMENT, writeable, false, increment_address, 100);
    std::this_thread::sleep_for(std::chrono::microseconds(fSleepTimeROCWrite));

    // then, wait till reg 128 returns non-zero
    uint16_t u;
    while ((u = fDtc->ReadROCRegister(Link, 128, 100)) != 0x8000){
      // idle
    }

    vector<roc_data_t> returned = this->ReadROCBlockEnsured(Link,REG_FINDALIGNMENT);

    // return
    auto rv = Alignment(returned);
    return rv;
  }

//-----------------------------------------------------------------------------
// align ROC FPGA/ADC signals, and optionally print the summary
// 
// if 'Link' = -1, use the DTC link mask
// there is no practical need to pass a random link mask,
// so 'Link' is either all enabled DTC links, or a specific one
// 
// returns the number of channels with non-zero number of bit slip steps
//-----------------------------------------------------------------------------
  int DtcInterface::FindAlignments(int PrintLevel, int Link, std::ostream& Stream) {
    int n_slipped(0);
    
    int link_mask = fLinkMask;
    if (Link != -1) link_mask = 0x1 << 4*Link;

    
    for (int i = 0 ; i < 6 ; i++){
      int enabled = (link_mask >> 4*i) & 0x1;
      if (enabled == 0)                                     continue;
//-----------------------------------------------------------------------------
// perform one iteration
//-----------------------------------------------------------------------------
      auto link      = DTC_Link_ID(i);
      auto alignment = FindAlignment(link);

      int n_non_null   =  0;
      int nsteps_tot   =  0;
      int max_steps_ch = -1;
      int worst_ch     = -1;
      for (const auto& iteration: alignment.Iterations()) {
        const auto& channels = iteration.Channels();
        for (size_t i = 0 ; i < channels.size() ; i++){
          auto channel = channels[i];
          int nsteps     = (int) channel.BitSlipStep();
          if (nsteps > 0) n_non_null++;
          nsteps_tot  += nsteps;
          if (nsteps > max_steps_ch) {
            max_steps_ch = nsteps;
            worst_ch     = i;
          }
        }
      }

      if (PrintLevel & 0x2) {
        print_legacy_table(alignment,Stream);
      }

      if (PrintLevel & 0x1) {
        Stream << " link:" << i << " n_non_null:" << std::setw(3) << n_non_null
               << " nsteps_tot:" << std::setw(3) << nsteps_tot
               << " worst_ch:" << std::setw(3) << worst_ch
               << " max_steps_ch:" << std::setw(3) << max_steps_ch << std::endl;
      }
      n_slipped += n_non_null;
    }
    
    return n_slipped;
  }

//-----------------------------------------------------------------------------
// validate data taken in the tracker ROC pattern generation mode, focus on payload
// returns number of found errors in the payload data
// assume ROC pattern generation
// 'Offset' is the
// PrintLevel =  0: print nothing
//            =  1: print all about errors
//            > 10: full printout
// also returned NErrRoc[6]: number of errors per ROC
// returns nerrors, where does the error code goes ?
//-----------------------------------------------------------------------------
int DtcInterface::ValidateDigiPatterns (ushort* DtcData, ulong EwTag, ulong* Offset, int PrintLevel, int* NErrRoc) {

  int n_adc_packets(1);
  int nerr   = 0;
  
  RocData_t* roc = (RocData_t*) (DtcData+0x18);  // 0x30 bytes
  for (int i=0; i<6; i++) {
    // nb_roc[i]    = roc->header.byteCount;
    // nb_rocs_tot += nb_roc[i];

    int nhits        = roc->header.packetCount/(n_adc_packets+1);
    
    short* first_address = (short*) roc;
  
    for (int ihit=0; ihit<nhits; ihit++) {
      mu2e::TrackerDataDecoder::TrackerDataPacket* hit ;
      int offset          = ihit*(8+8*n_adc_packets);   // in 2-byte words
      //int offset_in_bytes = offset*2;
      hit     = (mu2e::TrackerDataDecoder::TrackerDataPacket*) (first_address+0x08+offset);
      if (hit->ErrorFlags != 0) {  // 4 bits
        nerr += 1;
      }
//-----------------------------------------------------------------------------
// check hit straaw ID - TODO: correct the chid check
//-----------------------------------------------------------------------------
      int ich = hit->StrawIndex;

      if (ich > 128) ich = ich-128;

      if (ich > 95) {
//-----------------------------------------------------------------------------
// non existing channel ID : flag an error, don't save the hit, but continue
//-----------------------------------------------------------------------------
        nerr += 1;
      }
      if (hit->NumADCPackets != n_adc_packets) {
        nerr += 1;
                                        // assume errors are localized within the ROC payload
        break;
      }
    }
    
    roc = (RocData_t*) ( ((char*) roc) + roc->header.byteCount);
  }
  return 0;
}

//-----------------------------------------------------------------------------
int DtcInterface::ValidateFixedPatterns(ushort* DtcData, ulong EwTag, ulong* Offset, int PrintLevel, int* NErrRoc) {
  //  int ewt    = EwTag % 64 ;
  int nb_dtc = *DtcData;
  
  int nerr   = 0;

  int nb_roc[6];
  int nb_rocs_tot = 0;
  int last_nb(-1);

  RocData_t* roc = (RocData_t*) (DtcData+0x18);  // 0x30 bytes
  for (int i=0; i<6; i++) {
    nb_roc[i]    = roc->header.byteCount;
    nb_rocs_tot += nb_roc[i];
//-----------------------------------------------------------------------------
// although some ROC may not respond,  all responding ones should report
// the same number of bytes
//-----------------------------------------------------------------------------
    if (roc->header.error_code() == 0) { 
      if ((last_nb > 0) and (nb_roc[i] != last_nb)) {
        nerr += 1;
        if (PrintLevel > 1) {
          printf("ERROR: EWtag, nb_dtc, i, nb_roc[i-1], nb[roc] : %10lu 0x%04x %i 0x%04x 0x%04x\n",
                 EwTag,nb_dtc,i,nb_roc[i-1], nb_roc[i]);
        }
      }
      last_nb = nb_roc[i];
    }
    roc = (RocData_t*) ( ((char*) roc) + roc->header.byteCount);
  }
  
                                        // DTC header is 0x30 bytes - 3 packets
  if (nb_dtc != nb_rocs_tot+0x30) {
    if (PrintLevel > 1) printf("ERROR: EWtag, nb_dtc, nb_rocs_tot : %10lu 0x%04x 0x%04x\n",EwTag,nb_dtc,nb_rocs_tot);
    nerr += 1;
  }
  
  return nerr;
}

//-----------------------------------------------------------------------------
int DtcInterface::ValidateVarPatterns  (ushort* DtcData, ulong EwTag, ulong* Offset, int PrintLevel, int* NErrRoc) {

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
  int nb_dtc = *DtcData;

  RocData_t* roc = (RocData_t*) (DtcData+0x18);

  int nb_rocs = 0;
  for (int i=0; i<6; i++) {
    int nb   = roc->header.byteCount;
    nb_rocs += nb;
    roc      = (RocData_t*) ( ((char*) roc) + nb);
  }

  int nerr   = 0;

  if (nb_dtc != nb_rocs+0x30) {
    if (PrintLevel > 1) printf("ERROR: EWtag, nb_dtc, nb_rocs : %10lu 0x%04x 0x%04x\n",EwTag,nb_dtc,nb_rocs);
    nerr += 1;
  }
//-----------------------------------------------------------------------------
// event length checks out, check ROC payload
// check the ROC payload, assume a hit = 2 packets
//-----------------------------------------------------------------------------
  roc = (RocData_t*) (DtcData+0x18);
  for (int iroc=0; iroc<6; iroc++) {
    NErrRoc[iroc] = 0;
    if (PrintLevel > 10) printf("  ---- roc # %i\n",iroc);
//-----------------------------------------------------------------------------
// offsets are the same for all non-emty ROC's in the DTC data block
//-----------------------------------------------------------------------------
    ulong offset = *Offset;
    //    int   nb     = roc->header.byteCount;
//-----------------------------------------------------------------------------
// validate ROC header
//-----------------------------------------------------------------------------
    // ... TODO
    ulong ewtag_roc = roc->header.ewtag();

    if (ewtag_roc != EwTag) {
      if (PrintLevel > 1) printf("ERROR: EwTag ewtag_roc roc : 0x%08lx 0x%08lx %i\n",EwTag,ewtag_roc,iroc);
      nerr          += 1;
      NErrRoc[iroc] += 1;
    }
    
    if (roc->header.byteCount > 0x10) { 
//-----------------------------------------------------------------------------
// non-zero payload
//-----------------------------------------------------------------------------
      uint32_t*   pattern  = (uint32_t*) &roc->data[0];
      if (PrintLevel > 10) printf("data[0]  = nb = 0x%04x\n",pattern[0]);
 
      int npackets     = roc->header.packetCount;
      int npackets_exp = nhits[ewt]*2;       // assume two packets per hit (this number is stored somewhere)

      if (npackets != npackets_exp) {
        if (PrintLevel > 1) printf("ERROR: EwTag roc npackets npackets_exp: 0x%08lx %i %5i %5i\n",
                                  EwTag,iroc,npackets,npackets_exp);
        nerr          += 1;
        NErrRoc[iroc] += 1;
      }
      
      if (PrintLevel > 10) {
        printf("EwTag, ewt, roc, npackets, npackets_exp,  offset: %10lu %3i %i %2i %2i %10lu\n",
               EwTag,  ewt, iroc, npackets, npackets_exp,  offset);
      }

      uint nw      = npackets*4;        // N 4-byte words
    
      for (uint iw=0; iw<nw; iw++) {
        uint exp_pattern = (iw+offset) & 0xffffffff;
    
        if (pattern[iw] != exp_pattern) {
          nerr          += 1;
          NErrRoc[iroc] += 1;
          if (PrintLevel > 1) {
            printf("ERROR: EwTag, ewt roc iw  offset payload[iw] exp_word: %10lu %3i %i %3i %10li 0x%08x 0x%08x\n",
                   EwTag, ewt, iroc, iw, offset,pattern[iw],exp_pattern);
          }
        }
      }
    }
    roc = (RocData_t*) (((char*) roc) + roc->header.byteCount);
  }
  
  *Offset += 2*4*nhits[ewt];

  if (PrintLevel > 10) printf("EwTag = %10lx, nb_dtc = %i nerr = %i nerr_roc: %5i %5i %5i %5i %5i %5i\n",
                              EwTag,nb_dtc,nerr,
                              NErrRoc[0],NErrRoc[1],NErrRoc[2],NErrRoc[3],NErrRoc[4],NErrRoc[5]);

  return nerr;
}

//-----------------------------------------------------------------------------
  int DtcInterface::MonicaDigiClear() {

    for (int i=0; i<6; i++) {
      int used = (fLinkMask >> 4*i) & 0x1;
      if (not used)                                           continue;
//-----------------------------------------------------------------------------
// link is active
//-----------------------------------------------------------------------------
      auto link = DTCLib::DTC_Link_ID(i);

      // rocUtil write_register -l $LINK -a 28 -w 16 > /dev/null
      fDtc->WriteROCRegister(link,28,0x10,false,1000); // 

      // Writing 0 & 1 to  address=16 for HV DIGIs ??? 
      // rocUtil write_register -l $LINK -a 27 -w  0 > /dev/null # write 0 
      // rocUtil write_register -l $LINK -a 26 -w  1 > /dev/null ## toggle INIT 
      // rocUtil write_register -l $LINK -a 26 -w  0 > /dev/null
      fDtc->WriteROCRegister(link,27,0x00,false,1000); // 
      fDtc->WriteROCRegister(link,26,0x01,false,1000); // toggle INIT 
      fDtc->WriteROCRegister(link,26,0x00,false,1000); // 
    

      // rocUtil write_register -l $LINK -a 27 -w  1 > /dev/null # write 1  
      // rocUtil write_register -l $LINK -a 26 -w  1 > /dev/null # toggle INIT
      // rocUtil write_register -l $LINK -a 26 -w  0 > /dev/null
      fDtc->WriteROCRegister(link,27,0x01,false,1000); // 
      fDtc->WriteROCRegister(link,26,0x01,false,1000); // 
      fDtc->WriteROCRegister(link,26,0x00,false,1000); // 
    
      // echo "Writing 0 & 1 to  address=16 for CAL DIGIs"
      // rocUtil write_register -l $LINK -a 25 -w 16 > /dev/null
      fDtc->WriteROCRegister(link,25,0x10,false,1000); // 
    
      // rocUtil write_register -l $LINK -a 24 -w  0 > /dev/null # write 0
      // rocUtil write_register -l $LINK -a 23 -w  1 > /dev/null # toggle INIT
      // rocUtil write_register -l $LINK -a 23 -w  0 > /dev/null
      fDtc->WriteROCRegister(link,24,0x00,false,1000); // 
      fDtc->WriteROCRegister(link,23,0x01,false,1000); // 
      fDtc->WriteROCRegister(link,23,0x00,false,1000); // 

      // rocUtil write_register -l $LINK -a 24 -w  1 > /dev/null # write 1
      // rocUtil write_register -l $LINK -a 23 -w  1 > /dev/null # toggle INIT
      // rocUtil write_register -l $LINK -a 23 -w  0 > /dev/null
      fDtc->WriteROCRegister(link,24,0x01,false,1000); // 
      fDtc->WriteROCRegister(link,23,0x01,false,1000); // 
      fDtc->WriteROCRegister(link,23,0x00,false,1000); // 
    }
    return 0;
  }

//-----------------------------------------------------------------------------
// LaneMask bits:
//           0x1 : CAL lane 0
//           0x2 : HV  lane 0
//           0x4 : CAL lane 1
//           0x8 : HV  lane 1
//
// origin: ~mu2etrk/test_stand/monica_002/var_link_config.sh from Mar 12 2024
//
//  -rwxr-xr-x  1 mu2etrk mu2e      1553 Mar 12 10:11 var_link_config.sh
//-----------------------------------------------------------------------------
// configure_ROC 'read' command should be followed by ROC reset
//-----------------------------------------------------------------------------
// to be added 
//-----------------------------------------------------------------------------
  int DtcInterface::MonicaVarLinkConfig(int LaneMask) {
    int rc(0);
    
    fRocReadoutMode = 1;                            // 1: read digis
                                        // bit 13 - disable reset of the counters by the HB next to the null HB
    // int lane_mask = 0x0300 | LaneMask;
    int lane_mask = 0x2300 | LaneMask;
    
    for (int i=0; i<6; i++) {
      int enabled = (fLinkMask >> 4*i) & 0x1;
      if (enabled) {
        fDtc->WriteROCRegister(DTC_Link_ID(i), 8,lane_mask,false,1000);              // enable lanes
        std::this_thread::sleep_for(std::chrono::microseconds(fSleepTimeROCWrite));
        TLOG(TLVL_INFO) << "wrote lane_mask:" << std::hex << lane_mask
                        << " to ROC:" << i <<" reg:8, read back:" << fDtc->ReadROCRegister(DTC_Link_ID(i), 8,100);
      }
    }
    
    std::this_thread::sleep_for(std::chrono::microseconds(fSleepTimeROCWrite));

    int data_version = 1;
    RocSetDataVersion(data_version);    // Version --> R29

    ResetLinks();                         // use fLinkMask
//-----------------------------------------------------------------------------
// according to Monica, this is the place for find_alignment and control_roc_read
// check if all lanes are ready to be read
//-----------------------------------------------------------------------------
    for (int i=0; i<6; i++) {
      int used = (fLinkMask >> 4*i) & 0x1;
      if (used != 0) {
        uint16_t u = fDtc->ReadROCRegister(DTC_Link_ID(i),18,100);
        if ((u >> 0x8) != LaneMask) {
          // try to recover - write 1, then - 0 to reg 13
          fDtc->WriteROCRegister(DTC_Link_ID(i), 13,0x1,false,1000);
          std::this_thread::sleep_for(std::chrono::microseconds(fSleepTimeROCWrite));
          fDtc->WriteROCRegister(DTC_Link_ID(i), 13,0x0,false,1000);
          std::this_thread::sleep_for(std::chrono::microseconds(fSleepTimeROCWrite));
          // and check again
          u = fDtc->ReadROCRegister(DTC_Link_ID(i),18,100);
          if ((u >> 0x8) != LaneMask) {
            // still in trouble
            TLOG(TLVL_ERROR) << Form("ROC on link %i is not ready to read the DIGIs  link mask is 0x%04x, call Monica and Richie\n",
                                     i,u);
            rc -= 1;
          }
        }
      }
    }
//-----------------------------------------------------------------------------
    return rc;
  }

//-----------------------------------------------------------------------------
// origin: test_stand/monica_002/var_pattern_config.sh from Feb 14 2024
//
//  -rwxr-xr-x  1 mu2etrk mu2e      1820 Feb 14 15:00 var_pattern_config.sh
// adding 0x2000 prevents ROC from reinitializing the pattern, so two subsequent
// buffer test runs would return different results
// lane mask default: 0xf
//-----------------------------------------------------------------------------
  int DtcInterface::MonicaVarPatternConfig(int LaneMask, int NHitsPerLane) {

    ResetLinks();                                      // use fLinkMask
    int version = 1;
    RocSetDataVersion(version);                      // Version --> R29

    int ro_mode            = (fRocReadoutMode >> 0) & 0xf;
    int var_pattern_length = (fRocReadoutMode >> 4) & 0xf;
    
    if ((ro_mode != 0) and (ro_mode != 2)) {
      TLOG(TLVL_ERROR) << "unknown mode:" << fRocReadoutMode << " BAIL OUT";
      return -1;
    }

    for (int i=0; i<6; i++) {
      int used = (fLinkMask >> 4*i) & 0x1;
      if (used != 0) {
        if (ro_mode == 0) {
//-----------------------------------------------------------------------------
// mask bit#04=1: variable length
// mask bit#12=0: 'ROC counter;
//-----------------------------------------------------------------------------
          fDtc->WriteROCRegister(DTC_Link_ID(i), 8,0x2010,false,1000); // configure ROC to send variable length patterns
          std::this_thread::sleep_for(std::chrono::microseconds(fSleepTimeROCWrite));
        }
        else {
//-----------------------------------------------------------------------------
// can only be Mode == 2
// set number of simulated hits per lane - where that number is coming from?
// have only 10 bits for the number  of hits
// mask bit#04=1: variable length
// mask bit#13=1: don't reset the conters when receiving a null HB
// mask bit#12=1: 'ROC checkerboard'
// mask bit#11=1: fixed length patters
// NHits : 10 LS bits in reg@15
//-----------------------------------------------------------------------------
          int lane_mask = LaneMask;
          if (lane_mask < 0) lane_mask = fRocLaneMask;
          uint16_t mask = 0x3800 | lane_mask;
          if (var_pattern_length == 1) mask = mask | 0x00000010;
          else                         mask = mask & 0xffffffef;
          fDtc->WriteROCRegister(DTC_Link_ID(i), 8,mask,false,1000);   // configure ROC to send fixed length patterns
          std::this_thread::sleep_for(std::chrono::microseconds(fSleepTimeROCWrite));

          int nhits = NHitsPerLane;
          if (nhits < 0) nhits = fRocNHitsPerLane;
          uint16_t w15 = (nhits & 0x3ff);
          fDtc->WriteROCRegister(DTC_Link_ID(i),15,w15,false,1000);
          std::this_thread::sleep_for(std::chrono::microseconds(fSleepTimeROCWrite));

          TLOG(TLVL_DEBUG) << "var_pattern_length:" << var_pattern_length
                           << " reg#08:0x" << std::hex << std::setw(4) << std::setfill('0') << mask
                           << " reg#15:0x" << std::hex << std::setw(4) << std::setfill('0') << w15;
        }
      }
    }

    return 0;
  }

//-----------------------------------------------------------------------------
// ROC reset : write 0x1 to register 14
//-----------------------------------------------------------------------------
  void DtcInterface::ReadSubevents(std::vector<std::unique_ptr<DTCLib::DTC_SubEvent>>& VSub, 
                                   ulong       FirstEWT   ,
                                   int         PrintLevel,
                                   int         Validate  ,
                                   const char* Fn        ) {
    ulong    ewt      = FirstEWT;
    bool     match_ts = false;
    int      nerr_tot  (0);
    ulong    nbytes_tot(0);
    ulong    offset    (0);               // used in validation mode
    int      nerr_roc[6], nerr_roc_tot[6];

    FILE*    file(nullptr);
    if (Fn != nullptr) {
//-----------------------------------------------------------------------------
// check if Fn exists 
//-----------------------------------------------------------------------------
      if((file = fopen(Fn,"r")) != NULL) {
        // file exists
        fclose(file);
        TLOG(TLVL_ERROR) << "file " << Fn << " already exists, BAIL OUT";
        return;
      }
      else {
//-----------------------------------------------------------------------------
// Fn doesn't exist, open it 
//-----------------------------------------------------------------------------
        file = fopen(Fn,"w");
        if (file == nullptr) {
          TLOG(TLVL_ERROR) <<  "failed to open " << Fn << " , BAIL OUT";
          return;
        }
      }
    }
//-----------------------------------------------------------------------------
// reset per-roc error counters
//-----------------------------------------------------------------------------
    for (int i=0; i<6; i++) {
      nerr_roc    [i] = 0;
      nerr_roc_tot[i] = 0;
    }
//-----------------------------------------------------------------------------
// always read an event into the same external buffer (VSub), 
// so no problem with the memory management
//-----------------------------------------------------------------------------
    int header_printed = 0;
    while(1) {
      // sleep(1);
      DTC_EventWindowTag event_tag = DTC_EventWindowTag(ewt);
      try {
        if (PrintLevel > 0) {
//-----------------------------------------------------------------------------
// print header
// if fValidate != 0, there is a lot of printout, so it is better to print header
// for every event
//-----------------------------------------------------------------------------
          if ((Validate and PrintLevel > 1) or (header_printed == 0)) {
            cout << Form("      event  DTC     EW Tag nbytes   nbytes_tot  link0   nb0  link1   nb1  link2   nb2  link3   nb3  link4   nb4  link5   nb5  nerr nerr_tot\n");
            cout << Form("--------------------------------------------------------------------------------------------------------------------------------------------\n");
            header_printed = 1;
          }
        }
        VSub   = fDtc->GetSubEventData(event_tag, match_ts);
        int sz = VSub.size();
        if (sz == 0) {
          if (PrintLevel > 0) {
            cout << Form(">>>> ------- ewt = %5li NDTCs:%2i END_OF_DATA\n",ewt,sz);
          }
          break;
        }
//-----------------------------------------------------------------------------
// a subevent contains data of a single DTC
//-----------------------------------------------------------------------------
        int rs[6];
        std::vector<uint8_t> dtc_block;
        
        for (int i=0; i<sz; i++) {
          DTC_SubEvent* ev  = VSub[i].get();
          uint64_t ew_tag   = ev->GetEventWindowTag().GetEventWindowTag(true);
          char*    raw_data = (char*) ev->GetRawBufferPointer();

          int      nbytes  = ev->GetSubEventByteCount();
//-----------------------------------------------------------------------------
// create a local copy of the DTC data block
//-----------------------------------------------------------------------------
          dtc_block.reserve(nbytes);
          memcpy(dtc_block.data(),raw_data,nbytes);

          nbytes_tot += nbytes;

          int nerr(0);
          
          if (Validate > 0) {
            // different readout modes - different validation
            if      ((fRocReadoutMode & 0xf) == 0) {
              nerr = ValidateVarPatterns((ushort*) dtc_block.data(),ew_tag,&offset,PrintLevel,nerr_roc);
            }
            else if ((fRocReadoutMode & 0xf) == 1) {
              nerr = ValidateDigiPatterns((ushort*) dtc_block.data(),ew_tag,&offset,PrintLevel,nerr_roc);
            }
            else if ((fRocReadoutMode & 0xf) == 2) {
              nerr = ValidateFixedPatterns((ushort*) dtc_block.data(),ew_tag,&offset,PrintLevel,nerr_roc);
            }
            
              
            nerr_tot += nerr;
            for (int ir=0; ir<6; ir++) nerr_roc_tot[ir] += nerr_roc[ir];
          }

          uint8_t* roc_data  = dtc_block.data()+0x30;

          int nb_roc[6];
          for (int roc=0; roc<6; roc++) {
            nb_roc[roc] = *((ushort*) roc_data);
            rs[roc]     = *((ushort*)(roc_data+0x0c));
            roc_data   += nb_roc[roc];
          }
        
          if (PrintLevel > 0) {
            cout << Form(" %10li  %2i  %10li %5i %13li 0x%04x %5i 0x%04x %5i 0x%04x %5i 0x%04x %5i 0x%04x %5i 0x%04x %5i %5i %8i %4i %4i %4i %4i %4i %4i\n",
                         ewt,i,ew_tag,nbytes,nbytes_tot,
                         rs[0],nb_roc[0],rs[1],nb_roc[1],rs[2],nb_roc[2],rs[3],nb_roc[3],rs[4],nb_roc[4],rs[5],nb_roc[5],
                         nerr,nerr_tot,
                         nerr_roc[0],nerr_roc[1],nerr_roc[2],nerr_roc[3],nerr_roc[4],nerr_roc[5] );
            if (((nerr > 0) and (PrintLevel > 1)) or (PrintLevel > 2)) {
              PrintBuffer(ev->GetRawBufferPointer(),ev->GetSubEventByteCount()/2);
            }
          }
          
          if (file) {
//-----------------------------------------------------------------------------
// write event to output file
//-----------------------------------------------------------------------------
            int nbb = fwrite(dtc_block.data(),1,nbytes,file);
            if (nbb == 0) {
              TLOG(TLVL_ERROR) << Form("failed to write event %10li , close file and BAIL OUT\n",ew_tag);
              fclose(file);
              return;
            }
          }
        }
        
        ewt++;                          // event in sequence
      }
      catch (...) {
        TLOG(TLVL_ERROR) << "ERROR reading event_tag:" << event_tag.GetEventWindowTag(true) << " ewt:" << ewt << std::endl;
        break;
      }
    }

    //    fDtc->ReleaseAllBuffers(DTC_DMA_Engine_DAQ);
//-----------------------------------------------------------------------------
// print summary
//-----------------------------------------------------------------------------
    ulong nev = ewt-FirstEWT;
    TLOG(TLVL_DEBUG) << Form("nevents: %10li nbytes_tot: %13li Validate:%i\n",nev, nbytes_tot,Validate)
                     << Form("nerr_tot:%10i nerr_roc_tot: %8i %8i %8i %8i %8i %8i\n",
                             nerr_tot,
                             nerr_roc_tot[0],nerr_roc_tot[1],nerr_roc_tot[2],
                             nerr_roc_tot[3],nerr_roc_tot[4],nerr_roc_tot[5]);
//-----------------------------------------------------------------------------
// to simplify first steps, assume that in a file writing mode all events 
// are read at once, so close the file on exit
//-----------------------------------------------------------------------------
    if (file) {
      fclose(file);
    }
  }

  
//-----------------------------------------------------------------------------  
// 2025-01-31: P.Murat: presently, calls to begin_dcs_transaction() and end_dcs_transaction()
// are just TODO reminders and don't do anything useful
//-----------------------------------------------------------------------------
  int DtcInterface::RocBlockRead(int Link, int Reg, std::vector<uint16_t>& Res, int NExpected) {
    int rc(0), nw(0);
//-----------------------------------------------------------------------------
// convert into enum
//-----------------------------------------------------------------------------
    TLOG(TLVL_DEBUG+1) << std::format("Link:{} Reg:{:03d} NExpected:{}",Link,Reg,NExpected);
    auto link_id  = DTC_Link_ID(Link);

    try {
      fDtc->GetDevice()->begin_dcs_transaction();
    
      fDtc->WriteROCRegister   (link_id,Reg,0x0000,false,100);
      std::this_thread::sleep_for(std::chrono::microseconds(fSleepTimeROCWrite));
    
      uint16_t u; 
      while ((u = fDtc->ReadROCRegister(link_id,128,100)) != 0x8000) {}; 
      //      TLOG(TLVL_DEBUG+1) << std::format("reg:{:03d} val:0x{:04x}\n",128,u);
//-----------------------------------------------------------------------------
// register 129: number of words to read, currently-  (+ 4) (ask Monica)
//-----------------------------------------------------------------------------
      nw = fDtc->ReadROCRegister(link_id,129,100);
      // TLOG(TLVL_DEBUG+1) << std::format("reg:{:03d} val:0x{:04x}\n",129,nw);

      nw -= 4;
      fDtc->ReadROCBlock(Res,link_id,Reg,nw,false,100);
    }
    catch(...) {
      TLOG(TLVL_ERROR) << "failed DCS transaction link:" << Link;
      rc = -2;
    }
    
    fDtc->GetDevice()->end_dcs_transaction();

    if ((rc == 0) and (NExpected > 0) and (nw != NExpected)) {
      TLOG(TLVL_ERROR) << "WRONG NUMBER OF WORDS: NExpected:" << NExpected << " nw:" << nw;
      rc = -1;
    }
//-----------------------------------------------------------------------------
// does the ROC need to be reset ? Monica says NO.
//-----------------------------------------------------------------------------
    return rc;
  }

//-----------------------------------------------------------------------------
// read a given block number from ROC DDR memory
// block size: 1 kB
// ROC reg 15: last memory block read
//-----------------------------------------------------------------------------
  int DtcInterface::ReadRocDDR(int Link, int Block, std::ostream& Stream) {
    int rc(0);

    DTC_Link_ID link_id = DTC_Link_ID(Link);
  
  // write block number to reg 33
    fDtc->WriteROCRegister(link_id,33,((Block      ) & 0xffff) ,false,1000);
    fDtc->WriteROCRegister(link_id,34,((Block >> 16) & 0xffff) ,false,1000);
  // cycle reg 32
    fDtc->WriteROCRegister(link_id,32, 0x01,false,1000);
    fDtc->WriteROCRegister(link_id,32, 0x00,false,1000);
  // success: reg 20:0x8080  reg21:nwords to read (512)
    int reg_20 = fDtc->ReadROCRegister (link_id,20,1000);         // ox8080
    int nw     = fDtc->ReadROCRegister (link_id,21,1000);         // number of 16-bit words in a 1 kByte block (512)
//-----------------------------------------------------------------------------
// at this point, if everything was OK (nw=512), can read the data
//-----------------------------------------------------------------------------
    Stream << std::format("--- read link:{} DDR block:{:10d} reg_21(nwords):{:d} reg_20:0x{:4x}",Link,Block,nw,reg_20);

    if (nw == 512) {
      std::vector<uint16_t> v;
      fDtc->ReadROCBlock(v,link_id,0x200,nw,false,1000);
      if (nw != 512) {
        Stream << std::format("ERROR:002 read {} instead of 512 words, try again\n",nw);
        rc = -1;
      }
      else {
        ULong64_t ewt = ULong64_t(v[0]) | (ULong64_t(v[1]) << 16) | (ULong64_t(v[2]) << 32);
        Stream << "  ewt:" << ewt << " len:" << v[3] << std::endl;
        PrintBuffer(v.data(),nw,&Stream);
      }
    }
    else {
      Stream << std::endl << "ERROR:001 wrong number of words nw:" << nw << " read (not 512)\n";
      rc = -1;
    }
    return rc;
  }


//-----------------------------------------------------------------------------
// wrapper for DTCLib::DTC::ReadROCBlock
//-----------------------------------------------------------------------------
  std::vector<roc_data_t> DtcInterface::ReadROCBlockEnsured(const DTC_Link_ID& Link, const roc_address_t& address){
    // register 129: number of words to read
    size_t nwords = static_cast<size_t>(fDtc->ReadROCRegister(Link, 129, 1000));
    nwords -= 4; // account for low-level headers already consumed on-chip

    std::vector<roc_data_t> rv;
    bool increment_address = false; // read via fifo
    fDtc->ReadROCBlock(rv, Link, address, nwords, increment_address, 10000);
    if (rv.size() != nwords){
      std::string msg = "Malformed block read";
      msg += " expected ";
      msg += std::to_string(nwords);
      msg += " words, received ";
      msg += std::to_string(rv.size());
      msg += " words";
      throw cet::exception("DtcInterface::ReadROCBlockEnsured") << msg;
    }

    // P.M. don't need to reset the DDR
    // // reset ddr memory
    // fDtc->WriteROCRegister(Link, 14, 0x01, false, 1000);
    // std::this_thread::sleep_for(std::chrono::microseconds(fSleepTimeROCWrite));

    // return
    return rv;
  }

 // This is just an example, needs to be implemented for each subsystem
  std::vector<std::string> DtcInterface::GetRocRegistersNames(bool history = false) {
    std::vector<std::string> roc_var_names;
    char var_name[128];
    // Basic ROC registers
    if(history) {
      for (int k=0; k<TrkSpiDataNWords; k++) {
        sprintf(var_name,"%s", SpiVarName(k));
        roc_var_names.push_back(var_name);
      }
    } else {
      for(const int& reg : RocRegisters) {
        sprintf(var_name,"reg_%03i",reg);
        roc_var_names.push_back(var_name);
      }
    }
    return roc_var_names;
  }

  // This is just an example, needs to be implemented for each subsystem
  std::vector<uint32_t> DtcInterface::GetRocRegisters(int ilink, bool history = false) {
    std::vector<uint32_t> roc_reg;
    // Basic ROC registers
    if(history) {
      try { 
        std::vector<uint16_t> spi_raw_data;
        ControlRoc_ReadSpi(spi_raw_data,ilink,0);
        
        for (int iw=0; iw<TrkSpiDataNWords; iw++) {
          roc_reg.emplace_back(spi_raw_data[iw]);
        }
      }
      catch(...) {
        TLOG(TLVL_ERROR) << "failed to read DTC:" << fPcieAddr << " ROC:" << ilink << " SPI";
//-----------------------------------------------------------------------------
// set ROC status to -1
//-----------------------------------------------------------------------------
        // TODO
      }
    } else {
      roc_reg.reserve(trkdaq::RocRegisters.size());
        try {
          for (const int reg : trkdaq::RocRegisters) {
            // ROC registers store 16-bit words, don't know how to declare an array
            // of shorts for ODBXX, use uint32_t
            uint32_t dat = fDtc->ReadROCRegister(DTCLib::DTC_Link_ID(ilink),reg,100); 
            roc_reg.emplace_back(dat);
          }
        } catch (...) {
          TLOG(TLVL_ERROR) << "failed to read DTC:" << fPcieAddr << " ROC:" << ilink << " registers";
        }
    }
    return roc_reg;
  }
  //-----------------------------------------------------------------------------
// This is just an example, needs to be implemented for each subsystem
  std::vector<float> DtcInterface::GetConvertedRocRegisters(int ilink, bool history = false) {
    std::vector<float> roc_reg;
    // Basic ROC registers
    if(history) {
      try { 
        std::vector<uint16_t> spi_raw_data;
        struct TrkSpiData_t   spi;
        ControlRoc_ReadSpi(spi_raw_data,ilink,0);
        ConvertSpiData    (spi_raw_data,&spi,0);
              
        std::vector<float> roc_spi;
              
        for (int iw=0; iw<TrkSpiDataNWords; iw++) {
          roc_spi.emplace_back(spi.Data(iw));
        }
      }
      catch(...) {
        TLOG(TLVL_ERROR) << "failed to read DTC:" << fPcieAddr << " ROC:" << ilink << " SPI";
//-----------------------------------------------------------------------------
// set ROC status to -1
//-----------------------------------------------------------------------------
        // TODO
      }
    } else {
        auto val = GetRocRegisters(ilink, history);
        return std::vector<float>(val.begin(), val.end());
    }
    return roc_reg;
  }

//-----------------------------------------------------------------------------
  std::string DtcInterface::GetRocID(int Link) {
    std::string roc_id("READ_ERROR");
    if (LinkEnabled(Link)) {
      ControlRoc_DeviceID_t devid;
      int rc = ControlRoc_ReadDeviceID(Link,devid);
      if (rc == 0) {
        roc_id = devid.DeviceSerial;
      }
    }
    else {
      TLOG(TLVL_ERROR) << "DTC:" << fPcieAddr << " Link:" << Link << " is not enabled";
    }
    return roc_id;
  }

//-----------------------------------------------------------------------------
  std::string DtcInterface::GetRocDesignInfo(int Link) {
    std::string design_info("READ_ERROR");
    
    if (LinkEnabled(Link)) {
      ControlRoc_DeviceID_t devid;
      int rc = ControlRoc_ReadDeviceID(Link,devid);
      if (rc == 0) {
        design_info = devid.DesignInfo;
      }
    }
    else {
      TLOG(TLVL_ERROR) << "DTC:" << fPcieAddr << " Link:" << Link << " is not enabled";
    }
    return design_info;
  }

//-----------------------------------------------------------------------------
  std::string DtcInterface::GetRocFwGitCommit(int Link) {
    std::string s("READ_ERROR");

    if (LinkEnabled(Link)) {
      ControlRoc_ReadGitCommit(s,Link);
    }
    else {
      TLOG(TLVL_ERROR) << "DTC:" << fPcieAddr << " Link:" << Link << " is not enabled";
    }
    return s;
  }

//-----------------------------------------------------------------------------
// ejc
  float DtcInterface::ProgramAndQueryThreshold(const int Link,
                                               const int ChannelID,
                                               const int PreampType,
                                               const DTCLib::roc_data_t dac){
    uint32_t mask_lo = 0x00000000;
    uint32_t mask_md = 0x00000000;
    uint32_t mask_hi = 0x00000000;
    if (ChannelID < 32){
      mask_lo += (1 << (ChannelID -  0));
    }
    else if (ChannelID < 64){
      mask_md += (1 << (ChannelID - 32));
    }
    else if (ChannelID < 96){
      mask_hi += (1 << (ChannelID - 64));
    }
    std::vector<float> queried;
    queried.reserve(96);
    this->ControlRoc_SetThreshold(Link, ChannelID, PreampType, dac);
    this->ControlRoc_ReadThresholds(Link, queried, mask_lo, mask_md, mask_hi);
    auto idx = 3*ChannelID + (1 - PreampType);
    auto rv = queried.at(idx);
    return rv;
  }

  bool DtcInterface::FindThreshold(const int Link,
                                   const int ChannelID,
                                   const int PreampType,
                                   const float threshold,
                                   const float tolerance,
                                   DTCLib::roc_data_t& out){
    roc_data_t lower = 0;
    roc_data_t upper = 1023;
    auto f = [this, Link, ChannelID, PreampType] (roc_data_t dac){
      this->ControlRoc_SetThreshold(Link, ChannelID, PreampType, dac);
      auto rv = this->ProgramAndQueryThreshold(Link, ChannelID, PreampType, dac);
      return rv;
    };

    auto dac = bisection_search(f, -threshold, tolerance, lower, upper);
    auto measured = this->ProgramAndQueryThreshold(Link, ChannelID, PreampType, dac);

    // return whether or not the search was successful
    auto rv = false;
    if (fabs((-measured) - threshold) < tolerance){
      rv = true;
      out = dac;
    }

    return rv;
  }

  bool DtcInterface::FindThreshold(const int Link,
                                   const int ChannelID,
                                   const int PreampType,
                                   const float threshold,
                                   const float tolerance){
    DTCLib::roc_data_t tmp;
    auto rv = this->FindThreshold(Link, ChannelID, PreampType,
                                  threshold, tolerance, tmp);
    return rv;
  }
};

#endif
