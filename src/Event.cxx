// local includes
#include "Event.h"
#include "Config.h"
#include "Store.h"

// preprocessor macro inserting code which connects TTree and Event
#define VARIABLE(name, type) ConnectVariable<type>(#name, tree);



Event::Event() :
  m_store(0),
  m_log("Event")
{

  std::string str_level;
  Config::Instance().getif<std::string>("PrintLevel", str_level);
  if (str_level.length() > 0) {
    Log::LEVEL level = Log::StringToLEVEL(str_level);
    m_log.SetLevel(level);
  }

  m_store = new Store;
  
}


void Event::ConnectAllVariables(TTree * tree, bool disableOtherBranches, bool connectEventWeight)
{

  // disable all branches
  if (disableOtherBranches) tree->SetBranchStatus("*",0);
  else tree->SetBranchStatus("*",1);
  
  // connect reweighting variables
  #include "VARIABLES"

  // connect event weight
  if (connectEventWeight) {
    const std::string & eventWeightName = Config::Instance().get<std::string>("EventWeightVariableName");
    ConnectVariable<float>(eventWeightName.c_str(), tree);
  }
  
}


