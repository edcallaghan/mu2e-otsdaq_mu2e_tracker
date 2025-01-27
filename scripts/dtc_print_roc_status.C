#define __CLING__ 1

#ifndef __dtc_print_roc_status_C__
#define __dtc_print_roc_status_C__

#include "dtc_init.C"

using namespace trkdaq;
//-----------------------------------------------------------------------------
// primary source: ~/test_stand/monica_002/var_read_all.sh
// last update to that:  May 15 2024, by Monica
//-----------------------------------------------------------------------------
void dtc_print_roc_status(int Link, int PcieAddress = -1) {

  cout << Form("-------------------- ROC %i registers:\n",Link);

  DTC* dtc = trkdaq::DtcInterface::Instance(PcieAddress)->Dtc();

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

  uint16_t iw3;

  reg = 45;
  iw1 = dtc->ReadROCRegister(link,reg  ,100);
  iw2 = dtc->ReadROCRegister(link,reg+1,100);
  iw3 = dtc->ReadROCRegister(link,reg+2,100);
  
  iw  = (iw2 << 16) | iw1;
  cout << Form("reg(%i)<<16+reg(%i) : 0x%08x : Last HB tag\n", reg+1,reg,iw);

  reg = 48;
  iw1 = dtc->ReadROCRegister(link,reg  ,100);
  iw2 = dtc->ReadROCRegister(link,reg+1,100);
  iw3 = dtc->ReadROCRegister(link,reg+2,100);
  
  iw  = (iw2 << 16) | iw1;
  cout << Form("reg(%i)<<16+reg(%i) : 0x%08x : Last PREFETCH tag\n", reg+1,reg,iw);
  
  reg = 51;
  iw1 = dtc->ReadROCRegister(link,reg  ,100);
  iw2 = dtc->ReadROCRegister(link,reg+1,100);
  iw3 = dtc->ReadROCRegister(link,reg+2,100);
  
  iw  = (iw2 << 16) | iw1;
  cout << Form("reg(%i)<<16+reg(%i) : 0x%08x : Last fetched tag\n", reg+1,reg,iw);

  reg = 54;
  iw1 = dtc->ReadROCRegister(link,reg  ,100);
  iw2 = dtc->ReadROCRegister(link,reg+1,100);
  iw3 = dtc->ReadROCRegister(link,reg+2,100);
  
  iw  = (iw2 << 16) | iw1;
  cout << Form("reg(%i)<<16+reg(%i) : 0x%08x : Last DATA REQ tag\n", reg+1,reg,iw);

  reg = 57;
  iw1 = dtc->ReadROCRegister(link,reg  ,100);
  iw2 = dtc->ReadROCRegister(link,reg+1,100);
  iw3 = dtc->ReadROCRegister(link,reg+2,100);
  
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
#endif
