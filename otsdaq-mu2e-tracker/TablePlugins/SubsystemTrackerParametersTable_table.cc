// Ed Callaghan
// Dump ots configuration payload into Offline-compatible text and json
// October 2026

#include "otsdaq-mu2e-tracker/SubsystemTrackerParametersTable.h"

namespace ots{
  void init(ConfigurationManager* configManager){
    __SS__ << "TODO: implement init() for "
           << getTableName()
           << "'"
           << __E__;
    __SS_ONLY_THROW__;
  }

  std::string SubsystemTrackerParametersTable::getStructureAsJSON(const ConfigurationManager* configManager){
    __SS__ << "TODO: implement getStructureAsJSON() for "
           << getTableName()
           << "'"
           << __E__;
    __SS_ONLY_THROW__;
  }

  std::string SubsystemTrackerParametersTable::initPrereqsForARTDAQ(const ConfigurationManager* configManager){
    __SS__ << "TODO: implement initPrereqsForARTDAQ() for "
           << getTableName()
           << "'"
           << __E__;
    __SS_ONLY_THROW__;
  }

  std::string SubsystemTrackerParametersTable::getFclValueForARTDAQ(const ConfigurationManager* configManager, const std::string& field){
    __SS__ << "TODO: implement getFclValueForARTDAQ() for "
           << getTableName()
           << "'"
           << __E__;
    __SS_ONLY_THROW__;
  }
}
