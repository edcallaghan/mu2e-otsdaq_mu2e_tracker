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

    // locally lock filesystem access
	  isFirstAppInContext_ = configManager->isOwnerFirstAppInContext();
    if (isFirstAppInContext_){
      std::string path = getenv("DBSERVICE_ONLINE_PATH");
      if (0 < path.size()){
        path += "/" + "TrkStrawStatusShort.txt";
        std::ofstream out(path);
        // TODO should this be a hard throw? or just log and move on?
        if (!out){
          __SS__ << "Failed to open file: " << offlineTableFileName << __E__;
          __SS_THROW__;
        }
        else{
          try{
            auto content = generateTrkStrawStatusShort(configManager);
            out << content;
            out.close();
          }
          catch(const std::exception& e) {
            __SS__ << "Failed to write offline table "
                   << "TrkStrawStatusShort"
                   << " to file: "
                   << e.what()
                   << __E__;
            __SS_THROW__;
          }
        }
      }
    }
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

  std::string SubsystemTrackerParametersTable:;generateTrkStrawStatusShort(ConfigurationManager* configManager){
    std::stringstream stream;
    stream << "# TrkStrawStatusShort"; // TODO append run numbers...?
    stream << std::endl;

    Node tln = configManager->getNode("SubsystemTrackerChannelsTable");
    NodeList nodes = tln.getChildren();
    for (const auto& node: nodes){
      // auto label = node.first;
      auto config = node.second;
      auto label = config.get<std::string>("UID"); // TODO is this the StrawId?
      auto numeric = static_cast<uint16_t>(label);
      auto strawid = mu2e::StrawId(numeric);
      auto enabled = config.getValue<bool>("Enabled");

      if (!enabled){
        std::string id = std::to_string(strawid.plane())
                       + "_"
                       + std::to_string(strawid.panel())
                       + "_"
                       + std::to_string(strawid.straw());
        stream << id << "," << "Disabled";
      }

      rv = stream.str();
      return rv;
    }
  }
}

DEFINE_OTS_TABLE(SubsystemCalorimeterParametersTable)
