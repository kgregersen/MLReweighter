// analysis inlcudes
#include "Algorithm.h"
#include "Config.h"

// ROOT includes
#include "TTree.h"
#include "TRandom3.h"


Algorithm::Algorithm() :
  m_source(0),
  m_target(0),
  m_indicesSource(0),
  m_indicesTarget(0),
  m_weights(),
  m_log("Algorithm")
{
 
  std::string str_level;
  Config::Instance().getif<std::string>("PrintLevel", str_level);
  if (str_level.length() > 0) {
    Log::LEVEL level = Log::StringToLEVEL(str_level);
    m_log.SetLevel(level);
  }

}


Algorithm::Algorithm(TTree * source, TTree * target) :
  m_source(source),
  m_target(target),
  m_indicesSource(0),
  m_indicesTarget(0),
  m_weights(),
  m_log("Algorithm")
{
 
  std::string str_level;
  Config::Instance().getif<std::string>("PrintLevel", str_level);
  if (str_level.length() > 0) {
    Log::LEVEL level = Log::StringToLEVEL(str_level);
    m_log.SetLevel(level);
  }

}


void Algorithm::PrepareIndices()
{

  m_log << Log::INFO << "PrepareIndices() : Preparing sorted vector of event indices" << Log::endl();

  // delete previous indices (if any)
  delete m_indicesSource;
  delete m_indicesTarget;
  m_indicesSource = 0;
  m_indicesTarget = 0;
  
  // get info needed to create lists indices
  long maxEventSour = m_source->GetEntries();
  long maxEventTarg = m_target->GetEntries();
  static float samplingFraction   = Config::Instance().get<float>("SamplingFraction");
  static int samplingFractionSeed = Config::Instance().get<float>("SamplingFractionSeed");
  static TRandom3 ran( samplingFractionSeed );
  static bool bagging = false;
  Config::Instance().getif<bool>("Bagging", bagging); 
  
  if ( ! bagging ) {

    std::vector<long> indices;
    indices.reserve(maxEventSour);
    // get unique vector of indices to subset of events
    // ---> source
    for (long ievent = 0; ievent < maxEventSour; ++ievent) indices.push_back(ievent);
    for (long ievent = 0; ievent < maxEventSour; ++ievent) std::swap(indices[ievent], indices[static_cast<int>(ran.Rndm()*(static_cast<float>(indices.size()) - std::numeric_limits<float>::epsilon()))] );
    m_indicesSource = new std::vector<long>(indices.begin(), indices.begin() + samplingFraction*maxEventSour);
    // ---> target
    indices.clear();
    indices.reserve(maxEventTarg);
    for (long ievent = 0; ievent < maxEventTarg; ++ievent) indices.push_back(ievent);
    for (long ievent = 0; ievent < maxEventTarg; ++ievent) std::swap(indices[ievent], indices[static_cast<int>(ran.Rndm()*(static_cast<float>(indices.size()) - std::numeric_limits<float>::epsilon()))] );
    m_indicesTarget = new std::vector<long>(indices.begin(), indices.begin() + samplingFraction*maxEventTarg);

  }
  else {
    
    // get non-unique vector of indices to subset of events (Random Forest, Extremely Randomised Trees)
    // ---> source
    m_indicesSource = new std::vector<long>();
    m_indicesSource->reserve(samplingFraction*maxEventSour);
    while ( m_indicesSource->size() < samplingFraction*maxEventSour ) m_indicesSource->push_back( static_cast<long>(ran.Rndm()*maxEventSour) );
    // ---> target
    m_indicesTarget = new std::vector<long>();
    m_indicesTarget->reserve(samplingFraction*maxEventSour);
    while ( m_indicesTarget->size() < samplingFraction*maxEventTarg ) m_indicesTarget->push_back( static_cast<long>(ran.Rndm()*maxEventTarg) );

  }

  // need to sort to optimise reading of TTree (TTree::GetEntry(index) reads in chunks of sequential data, so we don't want to jump around in indices...)
  std::sort(m_indicesSource->begin(), m_indicesSource->end());
  std::sort(m_indicesTarget->begin(), m_indicesTarget->end());

  //for (unsigned int i = 0; i < 101; ++i) std::cout << i << " " << m_indicesSource->at(i) << " " << m_indicesTarget->at(i) << std::endl;
  
  m_log << Log::INFO << "PrepareIndices() : Sorted vector of indices created!" << Log::endl();

}


