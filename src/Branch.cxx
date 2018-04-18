// local includes
#include "Branch.h"
#include "Node.h"
#include "Event.h"
#include "Variables.h"
#include "Config.h"

// stl includes
#include <vector>
#include <string>


Branch::Branch(Node * input, const std::string & variableName, float cutValue, bool isGreater, float sumSource, float sumTarget) :
  m_input(input),
  m_output(0),
  m_cut(0),
  m_sumSource(sumSource),
  m_sumTarget(sumTarget),
  m_log("Branch")
{
  
  // set log's print level
  std::string str_level;
  Config::Instance().getif<std::string>("PrintLevel", str_level);
  if (str_level.length() > 0) {
    Log::LEVEL level = Log::StringToLEVEL(str_level);
    m_log.SetLevel(level);
  }

  // get variable
  const Variable * variable = Variables::Get(variableName);
  
  // initialize cut object
  if ( isGreater ) m_cut = new Greater(variable, cutValue);
  else             m_cut = new Smaller(variable, cutValue);

}


Branch::~Branch()
{

  //m_log << Log::INFO << "~Branch() : Called" << Log::endl();

  delete m_cut;
  m_cut = 0;
  
}


const Node * Branch::InputNode() const
{

  return m_input;

}


const Node * Branch::OutputNode() const
{

  return m_output;

}


void Branch::SetOutputNode(const Node * node)
{

  m_output = node;

}


const std::string & Branch::VariableName() const
{

  return m_cut->GetVariable()->Name();
  
}


bool Branch::Pass() const
{

  return m_cut->Pass();
  
}


const Branch::Cut * Branch::CutObject() const
{

  return m_cut;
  
}


float Branch::SumSource() const
{

  return m_sumSource;
  
}


float Branch::SumTarget() const
{

  return m_sumTarget;
  
}
