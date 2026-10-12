// Ed Callaghan
// Dump ots configuration payload into Offline-compatible text and json
// October 2026

#ifndef _ots_SubsystemTrackerParamteresTable_h_
#define _ots_SubsystemTrackerParamteresTable_h_

// stl
#include <fstream>
#include <string>

// offline
#include "Offline/DataProducts/inc/StrawId.hh"

// ots
#include "otsdaq/ConfigurationInterface/ConfigurationManager.h"
#include "otsdaq/Macros/TablePluginMacros.h"  //for DEFINE_OTS_TABLE
#include "otsdaq/TableCore/TableBase.h"

namespace ots{
  class SubsystemTrackerParametersTable: public TableBase{
    public:
      using Node = std::pair<std::string,ConfigurationTree>;
      using NodeList = std::vector<Node>;

      SubsystemTrackerParametersTable();
     ~SubsystemTrackerParametersTable(){ /**/ };

      // TableBase interface
      virtual void init(ConfigurationManager*) override;
//    virtual std::string getStructureAsJSON(const ConfigurationManager*) override;
//    virtual std::string initPrereqsForARTDAQ(const ConfigurationManager*) override;
//    virtual std::string getFclValueForARTDAQ(const ConfigurationManager*, const std::string&) override;

      // supporting implementations
      std::string generateTrkStrawStatusShort(ConfigurationManager*);

    protected:
      /**/

    private:
      /**/
  };
}// namespace ots

#endif
