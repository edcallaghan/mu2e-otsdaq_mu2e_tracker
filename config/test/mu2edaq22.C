///////////////////////////////////////////////////////////////////////////////
// mu2edaq22 : not checked, see mu2edaq09.C for reference
// ---------
// init_run_configuration : the name and the call signature are is fixed 
//                          and can't be changed
// this is an example of why one can't just use DTCLIB_DTC - there are two of them
///////////////////////////////////////////////////////////////////////////////
#include "otsdaq-mu2e-tracker/Gui/DtcGui.hh"

int init_run_configuration(DtcGui* X) {
  int rc(0);

  printf("[init_run_configuration] : test : DtcGui : %s\n",gSystem->Getenv("HOSTNAME"));

  DtcGui::DtcData_t* dtc = (DtcGui::DtcData_t*) X->fDtcData;

  X->fNDtcs           = 2;              // installed on a machine

  dtc[0].fName        = "DTC";
  dtc[0].fPcieAddr    = 0;
  dtc[0].fLinkMask    = 0x11;            // ROC0

  dtc[1].fName        = "DTC";
  dtc[1].fPcieAddr    = 1;
  dtc[1].fLinkMask    = 0x1;  // 

  gSystem->Setenv("DTCLIB_DTC","0");

  return rc;
}
