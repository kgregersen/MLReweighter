// local includes
#include "Forest.h"
#include "DecisionTree.h"
#include "Branch.h"
#include "Variables.h"
#include "Config.h"

// stl includes
#include <string>
#include <fstream>
#include <ctime>
#include <algorithm>
#include <utility>
#include <sstream>


Log Forest::m_log("Forest");


Forest::Forest() :
  m_trees()
{

  std::string str_level;
  Config::Instance().getif<std::string>("PrintLevel", str_level);
  if (str_level.length() > 0) {
    Log::LEVEL level = Log::StringToLEVEL(str_level);
    m_log.SetLevel(level);
  }

}


Forest::Forest(const std::vector<const DecisionTree *> trees) :
  m_trees(trees)
{

  std::string str_level;
  Config::Instance().getif<std::string>("PrintLevel", str_level);
  if (str_level.length() > 0) {
    Log::LEVEL level = Log::StringToLEVEL(str_level);
    m_log.SetLevel(level);
  }

}


Forest::~Forest()
{

}


void Forest::AddTree(const DecisionTree * tree)
{

  // add decision tree to the forest
  m_trees.push_back( tree );
  
}


const std::vector<const DecisionTree *> & Forest::GetTrees() const
{

  return m_trees;
  
}


const std::vector<const Forest *> Forest::ReadForests(const std::string & weightsFileName)
{

  // set log level
  std::string str_level;
  Config::Instance().getif<std::string>("PrintLevel", str_level);
  if (str_level.length() > 0) {
    Log::LEVEL level = Log::StringToLEVEL(str_level);
    m_log.SetLevel(level);
  }

  // open weights file
  std::ifstream weightsFile;
  m_log << Log::INFO << "ReadForests() : Opening file " << weightsFileName << Log::endl();
  weightsFile.open(weightsFileName.c_str());
 
  // vector of forests
  std::vector<const Forest *> forests;

  // forest of decision trees
  std::vector<const DecisionTree *> trees;

  // single tree (collection of final node weights and corresponding cuts)
  std::vector<std::pair<float, std::vector<const Branch::Cut *> > > treeReadIn;
 
  // read lines
  m_log << Log::INFO << "ReadForests() : Reading file " << weightsFileName << Log::endl();
  std::string line;
  int lineNumber = 0;
  while ( ! weightsFile.eof() ) {

    // increment line counter
    ++lineNumber;

    // get line 
    std::getline( weightsFile , line );
    m_log << Log::DEBUG << "ReadForests() : line ---> " << line << Log::endl();

    // check if we are at new forest
    if ( line.size() >= 10 && line.substr(0,10) == "Time stamp" ) {
      if (trees.size()) {
	forests.push_back( new Forest(trees) );
	trees.clear();
      }
    }

    // check if we are at new tree
    if ( (line.size() >= 14 && line.substr(2,13) == "Decision Tree") || (line.size() >= 5 && line.substr(2,3) == "End") ) {
      if (treeReadIn.size()) {
	trees.push_back( new DecisionTree(treeReadIn) );
      }
      for (unsigned int i = 0; i < treeReadIn.size(); ++i) {
	std::vector<const Branch::Cut *> cuts = treeReadIn.at(i).second;
	for (unsigned int j = 0; j < cuts.size(); ++j) {
	  delete cuts.at(j);
	}
      }
      treeReadIn.clear();
      continue;
    }
    
    // only consider lines that starts with 'weight='
    if ( ! (line.size() >= 7 && line.substr(0,7) == "weight=") ) continue;

    // declare position markers
    size_t pos1, pos2;

    // retrieve weight
    float weight;
    pos1 = line.find(':') + 1;
    std::istringstream( line.substr(7, pos1 - 7) ) >> weight;
    m_log << Log::DEBUG << "ReadForests() : weight = " << weight << Log::endl();
    
    // get target/source event counts
    float sumTarget;
    float sumSource; 
    pos1 = line.find('=', pos1 + 1) + 1;
    pos2 = line.find("/", pos1);
    std::istringstream( line.substr(pos1, pos2 - pos1) ) >> sumTarget;
    pos1 = pos2 + 1;
    pos2 = line.find("=", pos1);
    std::istringstream( line.substr(pos1, pos2 - pos1) ) >> sumSource;
    m_log << Log::DEBUG << "ReadForests() : SumTarget = " << sumTarget << Log::endl();
    m_log << Log::DEBUG << "ReadForests() : SumSource = " << sumSource << Log::endl();

    // move pos1 to cuts (passed 'SumTarget/SumSource')
    pos1 = line.find(':', pos2) + 1;
    
    // branches for this weight
    std::vector<const Branch::Cut *> cuts;

    // retrieve cuts from this line and convert them to branches
    while (pos1 != std::string::npos) {

      size_t next = line.find_first_of('|',pos1);
      std::string buffer = line.substr(pos1, next - pos1);
      
      size_t posLT = buffer.find('<');
      size_t posGT = buffer.find('>');
      float value;
      if (posLT != std::string::npos && posGT != std::string::npos) {
	m_log << Log::ERROR << "ReadForests() : There is both a '<' and a '>' in line " << lineNumber << Log::endl();
	throw(0);;
      }
      else if (posLT != std::string::npos) {
	std::string name  = buffer.substr(0, posLT);
	std::istringstream( buffer.substr(posLT + 1) ) >> value;
	m_log << Log::DEBUG << "ReadForests() : " << name << " < " << value << Log::endl();
	cuts.push_back( new Branch::Smaller(Variables::Get(name), value) );
      }
      else if (posGT != std::string::npos) {
	std::string name  = buffer.substr(0, posGT);
	std::istringstream( buffer.substr(posGT + 1) ) >> value;
	m_log << Log::DEBUG << "ReadForests() : " << name << " > " << value << Log::endl();
	cuts.push_back( new Branch::Greater(Variables::Get(name), value) );
      }
      
      pos1 = (next == std::string::npos ? next : next + 1);

    }

    // reverse vector of branches so input branch is first
    std::reverse(cuts.begin(), cuts.end());

    // add final node to tree (weight, branches)
    m_log <<Log::DEBUG << "ReadForests() : Adding decision tree to forest" << Log::endl();
    treeReadIn.push_back( std::make_pair(weight, cuts) );
        
  }
   
  // remember to add last forest
  forests.push_back( new Forest(trees) );

  // return the forests
  return forests;

}
