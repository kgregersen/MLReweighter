// local includes
#include "BDT.h"
#include "Forest.h"
#include "DecisionTree.h"
#include "Config.h"
#include "HistDefs.h"

// stl includes
#include <vector>

// ROOT includes
#include "TTree.h"



BDT::BDT(TTree * source, TTree * target) :
  Algorithm(source, target),
  m_log("BDT")
{
  
  std::string str_level;
  Config::Instance().getif<std::string>("PrintLevel", str_level);
  if (str_level.length() > 0) {
    Log::LEVEL level = Log::StringToLEVEL(str_level);
    m_log.SetLevel(level);
  }

  m_log << Log::INFO << "BDT() : Instantiating algorithm = BDT" << Log::endl(); 
 
}


BDT::BDT(std::vector<const Forest *> forests) :
  Algorithm(),
  m_forests(forests),
  m_log("BDT")
{
  
  std::string str_level;
  Config::Instance().getif<std::string>("PrintLevel", str_level);
  if (str_level.length() > 0) {
    Log::LEVEL level = Log::StringToLEVEL(str_level);
    m_log.SetLevel(level);
  }

  m_log << Log::INFO << "BDT() : Instantiating algorithm = BDT" << Log::endl(); 
 
}


BDT::~BDT()
{
 
}


void BDT::Initialize()
{

  // prepare event indices
  static bool bagging = false;
  Config::Instance().getif<bool>("Bagging", bagging); 
  if ( ! bagging ) Algorithm::PrepareIndices();
  
  // get histogram definitions
  m_histDefs = new HistDefs;
  m_histDefs->Initialize();
  m_histDefs->UpdateVariableRanges(m_target);
  m_histDefs->UpdateVariableRanges(m_source);
  for (const HistDefs::Entry & entry : m_histDefs->GetEntries()) {
    m_log << Log::INFO << "BDT() : Histogram name : " << entry.Name() << ", range = ( " << entry.Xmin() << " , " << entry.Xmax() << " )" << Log::endl();
  }

  // initialize weights
  m_weights.resize( m_source->GetEntries() );
  for (unsigned int i = 0; i < m_weights.size(); ++i) {
    m_weights.at(i) = 1.0;
  }

}


void BDT::Process()
{

  // declare forest
  Forest * forest = new Forest;
  
  // grow decision trees
  int ntree = Config::Instance().get<int>("NumberOfTrees");
  for (int itree = 0; itree < ntree; ++itree) {

    // bagging
    static bool bagging = false;
    Config::Instance().getif<bool>("Bagging", bagging); 
    if ( bagging ) Algorithm::PrepareIndices();

    // create tree
    DecisionTree * dtree = new DecisionTree(m_source, m_target, m_indicesSource, m_indicesTarget, m_histDefs);
    dtree->GrowTree( &m_weights );
    
    // add tree to forest
    forest->AddTree( dtree );

  }
  
  // add forest to internal vector
  m_forests.clear();
  m_forests.push_back( forest );

}


void BDT::Write(std::ofstream & outfile) {

  // get first forest (in 'calculate' mode, there is only one forest)
  const std::vector<const DecisionTree *> & decisionTrees = m_forests.at(0)->GetTrees();

  // write trees to file
  for (const DecisionTree * dtree : decisionTrees) {
    dtree->Write( outfile );
  }

}


void BDT::GetWeight(float & weight, float & error)
{

  // reset weight/error
  weight = 0;
  error  = 0;

  // get number of forests
  int nForest = m_forests.size();
  
  // declare vector to hold error from each forest
  std::vector<float> error_vec( nForest );

  // loop over forests
  int iForest = 0;
  for (const Forest * forest : m_forests) {

    // loop over trees
    const std::vector<const DecisionTree *> & trees = forest->GetTrees();
    
    // multiply weights from the trees in the forest
    float w = 1;
    for (const DecisionTree * t : trees) {
      w *= t->GetWeight();
    }
    weight += w;
    error_vec.at( iForest ) = w;
    
    // increment iForest
    ++iForest;

  }
  
  // finalise weight
  weight /= static_cast<float>( nForest );

  // finalise error
  for (float e : error_vec) {
    error += pow(e - weight, 2);
  }
  error = sqrt( error/( nForest > 1 ? nForest - 1 : 1 ) );
  
}
