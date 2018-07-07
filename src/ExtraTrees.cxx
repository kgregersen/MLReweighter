// local includes
#include "ExtraTrees.h"
#include "Forest.h"
#include "DecisionTree.h"
#include "Config.h"
#include "HistDefs.h"

// stl includes
#include <vector>

// ROOT includes
#include "TTree.h"



ExtraTrees::ExtraTrees(TTree * source, TTree * target) :
  Algorithm(source, target),
  m_log("ExtraTrees")
{
  
  std::string str_level;
  Config::Instance().getif<std::string>("PrintLevel", str_level);
  if (str_level.length() > 0) {
    Log::LEVEL level = Log::StringToLEVEL(str_level);
    m_log.SetLevel(level);
  }
  
  m_log << Log::INFO << "ExtraTrees() : Instantiating algorithm = ExtraTrees" << Log::endl(); 

}


ExtraTrees::ExtraTrees(std::vector<const Forest *> forests) :
  Algorithm(),
  m_forests(forests),
  m_log("ExtraTrees")
{
  
  std::string str_level;
  Config::Instance().getif<std::string>("PrintLevel", str_level);
  if (str_level.length() > 0) {
    Log::LEVEL level = Log::StringToLEVEL(str_level);
    m_log.SetLevel(level);
  }
  
  m_log << Log::INFO << "ExtraTrees() : Instantiating algorithm = ExtraTrees" << Log::endl(); 

}


ExtraTrees::~ExtraTrees()
{
 
}


void ExtraTrees::Initialize()
{

  // check if bagging is enabled
  static bool bagging = false;
  Config::Instance().getif<bool>("Bagging", bagging); 
  if ( ! bagging ) {
    m_log << Log::ERROR << "Initialize() : Bagging needs to be enabled. In config file : 'bool bagging = true'" << Log::endl();
    throw(0);    
  }
  
  // get histogram definitions
  m_histDefs = new HistDefs;
  m_histDefs->Initialize();
  m_histDefs->UpdateVariableRanges(m_target);
  m_histDefs->UpdateVariableRanges(m_source);
  for (const HistDefs::Entry & entry : m_histDefs->GetEntries()) {
    m_log << Log::INFO << "Initialize() : Histogram name : " << entry.Name() << ", range = ( " << entry.Xmin() << " , " << entry.Xmax() << " )" << Log::endl();
  }

}


void ExtraTrees::Process()
{

  // declare forest
  Forest * forest = new Forest;
  
  // grow decision trees
  int ntree = Config::Instance().get<int>("NumberOfTrees");
  for (int itree = 0; itree < ntree; ++itree) {

    // bagging (prepare event indices)
    Algorithm::PrepareIndices();
    
    // create tree
    DecisionTree * dtree = new DecisionTree(m_source, m_target, m_indicesSource, m_indicesTarget, m_histDefs);
    dtree->GrowTree();
    
    // add tree to forest
    forest->AddTree( dtree );

  }
  
  // add forest to internal vector
  m_forests.clear();
  m_forests.push_back( forest );



}


void ExtraTrees::Write(std::ofstream & outfile) {

  // get first forest (in 'calculate' mode, there is only one forest)
  const std::vector<const DecisionTree *> & decisionTrees = m_forests.at(0)->GetTrees();

  // write trees to file
  float norm = GetNormalization();
  for (const DecisionTree * dtree : decisionTrees) {
    dtree->Write( outfile, norm );
  }

}


void ExtraTrees::GetWeight(float & weight, float & error) const
{

  // reset weight/error
  weight = 0;
  error  = 0;
  
  // get total number of trees
  static int nTreeTotal = 0;
  if (nTreeTotal == 0) {
    for (const Forest * forest : m_forests) {
      nTreeTotal += forest->GetTrees().size();
    }
  }
  
  // declare vector to hold error
  static std::vector<float> error_vec( nTreeTotal );
  
  // loop over forests
  int iTree = 0;
  for (const Forest * forest : m_forests) {

    // loop over trees
    for (const DecisionTree * tree : forest->GetTrees()) {     
      float w = tree->GetWeight();
      weight += w;
      error_vec.at( iTree ) = w;
    }

    // increment tree counter
    ++iTree;
    
  }
  
  // finalise weight
  weight /= static_cast<float>( nTreeTotal );

  // finalise error
  for (float e : error_vec) {
    error += pow(e - weight, 2);
  }
  error = sqrt( error/(nTreeTotal - 1) );
  
}

