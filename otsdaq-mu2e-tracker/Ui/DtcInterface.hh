//-----------------------------------------------------------------------------
// interactive interface for ROOT-based GUI
// mixes high- and low-level commands
// assume everything is happening on one node
// there could be one or two DTCs and only one CFO
//-----------------------------------------------------------------------------
#ifndef __trkdaq_dtc_interface_hh__
#define __trkdaq_dtc_interface_hh__

#define __CLING__ 1

#include <string>
#include <vector>
#include <sstream>
#include "iostream"
#include "dtcInterfaceLib/DTC.h"
#include "artdaq-core-mu2e/Overlays/DTC_Types/DTC_Link_ID.h"
#include "artdaq-core-mu2e/Overlays/DTC_Packets/DTC_RocDataHeaderPacket.h"

#include "otsdaq-mu2e-tracker/Ui/ControlRocTypes.hh"

#include "otsdaq-mu2e-tracker/ParseAlignment/Alignment.hh"
#include "otsdaq-mu2e-tracker/ParseAlignment/PrintLegacyTable.hh"
#include "otsdaq-mu2e-tracker/Ui/DtcInterfaceBase.hh"

#include "otsdaq-mu2e-tracker/Ui/BisectionSearch.hh"

namespace trkdaq {
  using roc_serial_t = std::string;

     // some ROC registers are listed in decimal format, and some - in hex
    static const std::vector<int> RocRegisters = {
       0,   18,    8,   15,   16,    7,      6,    4,
      23,   24,   25,   26,   11,   12,     65,   65,   17,   28,
      29,   30,   31,   32,   33,   34,      9,   10,   35,   36,
      13,
      37,   38,   38,   40,   41,   42,     43,   44,   45,   46,
      48,   49,   51,   52,   54,   55,     57,   58,
      72,   73,   74,   75,
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95
  };

  class DtcInterface : public mu2edaq::DtcInterface { 
    private:
      DtcInterface(int PcieAddr, uint LinkMask, bool SkipInit);
    public:
//-----------------------------------------------------------------------------
// ROC functions : if LinkMask=0, use fLinkMask
//-----------------------------------------------------------------------------
    void                       RocConfigurePatternMode();
    void                       RocSetDataVersion      (int Version, int LinkMask=0);

    static const char*         fgSpiVarName[TrkSpiDataNWords]; //
    static       int           fgFpga[96];                     // 0:CAL or 1:HV
//-----------------------------------------------------------------------------
// functions
//-----------------------------------------------------------------------------
  public:
    static       DtcInterface* Instance             (int PcieAddr, uint LinkMask = 0x11, bool SkipInit = false);

    static const char*         SpiVarName           (int I) { return fgSpiVarName[I]; }
    static const char*         SpiVarNamePrintBuffer(int I) { return fgSpiVarName[I]; }
//-----------------------------------------------------------------------------
// generic interface to control_ROC.py commands.
// When/if we figure how to do it better, we'll implement a better solution
//-----------------------------------------------------------------------------
    int          ControlRoc(const char* Command, void* Parameters);

    // need: digi_rw -h 0 -w 1 -a 0x82 -d 0x1388
    int          ControlRoc_DigiRW (ControlRoc_DigiRW_Input_t*  Input          ,
                                    ControlRoc_DigiRW_Output_t* Output         ,
                                    int                         LinkMask   = -1,
                                    int                         PrintLevel =  0,
                                    std::ostream&               Stream     = std::cout);
//-----------------------------------------------------------------------------
// Channel = 0-95: read settings of a given preamp channel: gain_cal, gain_hv, thr_cal, thr_hv,
//                 4 words in total
//         = -1  : read settings of all channels : gain_cal[96], gain_hv[96], thr_cal[hv], thr_hv[cal],
//                 386 words in total
//-----------------------------------------------------------------------------
    int          ControlRoc_DumpSettings(int                   Link           ,
                                         int                   Channel    = -1,
                                         int                   PrintLevel =  0,
                                         std::ostream&         Stream     = std::cout);

    int          ControlRoc_ReadSettings(int                    Link           ,
                                         int                    Channel        ,
                                         std::vector<uint16_t>& Data           ,
                                         int                    PrintLevel =  0,
                                         std::ostream&          Stream     = std::cout);

    int          ControlRoc_PulserOn (int Link              ,
                                      int FirstChannelMask = 0x10,      // first channel:- #4
                                      int DutyCycle        = 10  ,
                                      int PulserDelay      = 1000,
                                      int PrintLevel       = 0x2,
                                      std::ostream& Stream = std::cout);
    
    int          ControlRoc_PulserOff(int Link, int PrintLevel = 0, std::ostream& Stream= std::cout);

                                        // always a single link
    int          ControlRoc_Rates    (int                     Link            ,
                                      std::vector<uint16_t>*  Output          ,
                                      int                     PrintLevel = 0x2,
                                      ControlRoc_Rates_t*     Par        = nullptr,
                                      std::ostream*           Stream     = nullptr);

    int          ControlRoc_Read   (ControlRoc_Read_Input_t0* Par        = nullptr,
                                    int                       Link       = 0    ,
                                    int                       PrintLevel = 0    ,
                                    std::ostream&             Stream     = std::cout);

    int          ControlRoc_ReadDeviceID(int                    Link,
                                         ControlRoc_DeviceID_t& DevId,
                                         int                    PrintLevel = 0,
                                         std::ostream&          Stream     = std::cout);
//-----------------------------------------------------------------------------
// if Line = -1, not interested in the output, only in the printout
// if OK, the read functions return Nwords
//   -1: failure
//-----------------------------------------------------------------------------
    int          ControlRoc_ReadGitCommit(std::string&       GitCommit       ,
                                          int                Link       = -1 ,
                                          int                PrintLevel = 0  ,
                                          std::ostream&      Stream     = std::cout);
    
    int          ControlRoc_ReadIlp(std::vector<uint16_t>&   RawData         ,
                                    int                      Link       = -1 ,
                                    int                      PrintLevel = 0  ,
                                    std::ostream&            Stream     = std::cout);
    
    int          ControlRoc_GetKey (std::vector<uint16_t>&   RawData         ,
                                    int                      Link       = -1 ,
                                    int                      PrintLevel = 0  ,
                                    std::ostream&            Stream     = std::cout);
    
    int          ControlRoc_ReadSpi(std::vector<uint16_t>&   SpiRawData     ,
                                    int                      Link       = -1,
                                    int                      PrintLevel = 0 ,
                                    std::ostream&            Stream     = std::cout);

    int          ControlRoc_ReadSpi_1(TrkSpiData_t*          Spi,
                                      int                    Link       = -1,
                                      int                    PrintLevel = 0 ,
                                      std::ostream&          Stream     = std::cout);
//-----------------------------------------------------------------------------
// measure thresholds returns an array of thresholds, which needs to be parsed
// so far, do it internally
// PrintLevel: bit 0: hex printout, bit 1: parsed printout
//-----------------------------------------------------------------------------
    int          ControlRoc_MeasureThresholds(int           Link                   ,
                                              uint32_t      MaskC      = 0xFFFFFFFF,
                                              uint32_t      MaskD      = 0xFFFFFFFF,
                                              uint32_t      MaskE      = 0xFFFFFFFF,
                                              int           PrintLevel = 0x2       ,
                                              std::ostream& Stream     = std::cout);

    int          ControlRoc_ReadThresholds   (int                        Link      ,
                                              std::vector<float>&        Thr       ,
                                              uint32_t      MaskC      = 0xFFFFFFFF,
                                              uint32_t      MaskD      = 0xFFFFFFFF,
                                              uint32_t      MaskE      = 0xFFFFFFFF,
                                              int           PrintLevel = 0x2       ,
                                              std::ostream& Stream     = std::cout );

    int          ControlRoc_PrintThresholds  (int                        Link      ,
                                              std::vector<float>&        Thr       ,
                                              uint32_t      MaskC      = 0xFFFFFFFF,
                                              uint32_t      MaskD      = 0xFFFFFFFF,
                                              uint32_t      MaskE      = 0xFFFFFFFF,
                                              int           PrintLevel = 0x2       ,
                                              std::ostream& Stream     = std::cout );
//-----------------------------------------------------------------------------
// supposedly, PulseHeight = V/3.3*1024 or 1024 = 3.3V
//-----------------------------------------------------------------------------
    int          ControlRoc_SetCalDac        (int Link, int FirstChannelMask, int PulseHeight,
                                              int PrintLevel = 0, std::ostream& Stream = std::cout);
//-----------------------------------------------------------------------------
// PreampType: 0:HV 1:CAL, or vice versa
// do one channel at a time
// shall we think of a block operation ? or not ? - channels could be masked OFFx
//-----------------------------------------------------------------------------
    int          ControlRoc_SetGain      (int Link, int ChannelID, int PreampType, int Gain     , int PrintLevel = 0);
    int          ControlRoc_SetThreshold (int Link, int ChannelID, int PreampType, int Threshold, int PrintLevel = 0);
//-----------------------------------------------------------------------------
// block operation: set thresholds and gains for all channels on a given DRAC
// 4 x 96 16 bit words. Gain cal, Gain HV, threshold CAL, threshold HV
// It is a block write of 384 values to address 275 (0x113). It does not return anything
// but you should still request that reg=128 read 0x8000 while reg=129 should stay at the default empty value of 0x1000
//-----------------------------------------------------------------------------
    int          ControlRoc_SetThresholds(int Link, uint16_t* TG, int PrintLevel = 0, std::ostream& Stream = std::cout);
// ejc
    float ProgramAndQueryThreshold(const int Link,
                                   const int ChannelID,
                                   const int PreampType,
                                   const DTCLib::roc_data_t dac);
    bool FindThreshold(const int Link,
                       const int ChannelID,
                       const int PreampType,
                       const float threshold,
                       const float tolerance,
                       DTCLib::roc_data_t& out);

    bool FindThreshold(const int Link,
                       const int ChannelID,
                       const int PreampType,
                       const float threshold,
                       const float tolerance);

    int          ConvertSpiData(const std::vector<uint16_t>& RawData,
                                TrkSpiData_t*                Data   ,
                                int                          PrintLevel = 0,
                                std::ostream&                Stream     = std::cout);

    virtual std::vector<std::string> GetRocRegistersNames     (bool history = false)            override;
    virtual std::vector<uint32_t>    GetRocRegisters          (int ilink, bool history = false) override;
    virtual std::vector<float>       GetConvertedRocRegisters (int ilink, bool history = false) override;

    virtual std::string              GetRocID         (int Link) override;
    virtual std::string              GetRocDesignInfo (int Link) override;
    virtual std::string              GetRocFwGitCommit(int Link) override;
//-----------------------------------------------------------------------------
// assume that to be printed are 'nw' uint16_t words , in hex
// if Stream == nullptr, PrintBuffer uses TRACE's TLOG
//-----------------------------------------------------------------------------    
    void         PrintBuffer     (const void* ptr, int nw, std::ostream* Stream = nullptr);

    void         PrintRatesSingleRoc(std::vector<uint16_t>* Rates, std::vector<int>* ChMask = nullptr, std::ostream& Stream = std::cout);
    void         PrintRatesAllRocs  (std::vector<uint16_t>* Rates, std::vector<int>* ChMask, std::ostream& Stream = std::cout);
    
//-----------------------------------------------------------------------------
// Format = 0 : for each register, print a register and its value
// Format = 1 : add short description of each register
// if Link = -1, print a line per register for each ROC
//-----------------------------------------------------------------------------
    void         PrintRocRegister (uint Reg, std::string& Desc, int Format = 1, int LinkMask = -1, std::ostream& Stream = std::cout);
    void         PrintRocRegister2(uint Reg, std::string& Desc, int Format = 1, int LinkMask = -1, std::ostream& Stream = std::cout);
    void         PrintRocStatus   (uint32_t Format = 1, int LinkMask = -1, std::ostream& Stream = std::cout);
    void         PrintSpiAll      (trkdaq::TrkSpiData_t* Spi, std::ostream& Stream = std::cout);

    void         ReadSubevents   (std::vector<std::unique_ptr<DTCLib::DTC_SubEvent>>& Vsev, 
                                  ulong       FirstTS,
                                  int         PrintData,
                                  int         Validate = 0      , 
                                  const char* OutputFn = nullptr);

    int          ReadRocDDR  (int Link, int Block, std::ostream& Stream = std::cout);
    int          RocBlockRead(int Link, int Reg, std::vector<uint16_t>& Res, int NExpected = -1);

    std::vector<DTCLib::roc_data_t> ReadROCBlockEnsured(const DTCLib::DTC_Link_ID& Link,
                                                        const DTCLib::roc_address_t& address);

    Alignment    FindAlignment (DTCLib::DTC_Link_ID Link);

    int          FindAlignments(int PrintLevel=1, int Link=-1, std::ostream& Stream = std::cout);

    void         SetRocLaneMask    (int Mask ) { fRocLaneMask     = Mask ; }
    void         SetRocNHitsPerLane(int NHits) { fRocNHitsPerLane = NHits; }

//-----------------------------------------------------------------------------
// return number of found errors
//-----------------------------------------------------------------------------
    int          ValidateDigiPatterns (ushort* Data, ulong EwTag, ulong* Offset, int PrintLevel, int* NErrRoc);
    int          ValidateFixedPatterns(ushort* Data, ulong EwTag, ulong* Offset, int PrintLevel, int* NErrRoc);
    int          ValidateVarPatterns  (ushort* Data, ulong EwTag, ulong* Offset, int PrintLevel, int* NErrRoc);
//-----------------------------------------------------------------------------
// reset digitizers .. to be called in the beginning of each event 
//-----------------------------------------------------------------------------
    int          MonicaDigiClear();
//-----------------------------------------------------------------------------
// ROC has 4 lanes: 2 CAL lanes (0x5) and 2 HV lanes (0xa)
//-----------------------------------------------------------------------------
    int          MonicaVarLinkConfig   (int LaneMask = 0xf);
//-----------------------------------------------------------------------------
// VarPatternConfig = RocConfigurePatternMode
//-----------------------------------------------------------------------------
    int          MonicaVarPatternConfig(int LaneMask = -1, int NHits = -1);
//-----------------------------------------------------------------------------
// overloaded functions of the base class
//-----------------------------------------------------------------------------
    virtual int   InitRocReadoutMode() override;
    virtual int   ResetLink         (int Link) override;

    
    roc_serial_t                    ReadSerialNumber(const DTCLib::DTC_Link_ID& Link);
    std::vector<DTCLib::roc_data_t> ReadDeviceID    (DTCLib::DTC_Link_ID Link,
                                                     int                 PrintLevel = 0,
                                                     std::ostream&       Stream     = std::cout);
  };

  
  struct RocData_t {                    // 8 16-byte words in total
    RocDataHeaderPacket_t header;
    uint16_t              data[1];
  };
};

#endif
