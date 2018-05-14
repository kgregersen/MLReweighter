// analysis inlcudes
#include "Algorithm.h"
#include "Config.h"
#include "Event.h"

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
 
  // set log level
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

  // set log level
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
  long maxEventSource = m_source->GetEntries();
  long maxEventTarget = m_target->GetEntries();
  static float samplingFraction   = Config::Instance().get<float>("SamplingFraction");
  static int samplingFractionSeed = Config::Instance().get<float>("SamplingFractionSeed");
  static TRandom3 ran( samplingFractionSeed );
  static bool bagging = false;
  Config::Instance().getif<bool>("Bagging", bagging); 
  
  if ( bagging ) {

    // random subset (sampling with replacement)

    // ---> source
    m_log << Log::INFO << "PrepareIndices() : ---> source" << Log::endl();
    m_indicesSource = new std::vector<long>();
    m_indicesSource->reserve(maxEventSource*samplingFraction);
    static std::vector<float> cumulativeSource(maxEventSource);
    for (long ievent = 0; ievent < maxEventSource; ++ievent) {
      m_source->GetEntry( ievent );
      static const std::string & eventWeightName = Config::Instance().get<std::string>("EventWeightVariableName");
      static const float & eventWeight = Event::Instance().get<float>(eventWeightName);
      cumulativeSource.at(ievent) = eventWeight;
      if (ievent > 0) cumulativeSource.at(ievent) += cumulativeSource.at(ievent-1);
    }
    while (m_indicesSource->size() < maxEventSource*samplingFraction) m_indicesSource->push_back( BinarySearchIndex(cumulativeSource, ran.Rndm()*cumulativeSource.back(), 0, cumulativeSource.size() - 1) );

    // ---> target
    m_log << Log::INFO << "PrepareIndices() : ---> target" << Log::endl();
    m_indicesTarget = new std::vector<long>();
    m_indicesTarget->reserve(maxEventTarget*samplingFraction);
    static std::vector<float> cumulativeTarget(maxEventTarget);
    for (long ievent = 0; ievent < maxEventTarget; ++ievent) {
      m_target->GetEntry( ievent );
      static const std::string & eventWeightName = Config::Instance().get<std::string>("EventWeightVariableName");
      static const float & eventWeight = Event::Instance().get<float>(eventWeightName);
      cumulativeTarget.at(ievent) = eventWeight;
      if (ievent > 0) cumulativeTarget.at(ievent) += cumulativeTarget.at(ievent-1);
    }
    while (m_indicesTarget->size() < maxEventTarget*samplingFraction) m_indicesTarget->push_back( BinarySearchIndex(cumulativeTarget, ran.Rndm()*cumulativeTarget.back(), 0, cumulativeTarget.size() - 1) );
    
  }
  else {

    // use all events
    // ---> source
    m_log << Log::INFO << "PrepareIndices() : ---> source" << Log::endl();
    m_indicesSource = new std::vector<long>();
    m_indicesSource->reserve(maxEventSource);
    for (long ievent = 0; ievent < maxEventSource; ++ievent) m_indicesSource->push_back(ievent);

    // ---> target
    m_log << Log::INFO << "PrepareIndices() : ---> target" << Log::endl();
    m_indicesTarget = new std::vector<long>();
    m_indicesTarget->reserve(maxEventTarget);
    for (long ievent = 0; ievent < maxEventTarget; ++ievent) m_indicesTarget->push_back(ievent);

  }

  // need to sort to optimise reading of TTree (TTree::GetEntry(index) reads in chunks of sequential data, so we don't want to jump around in indices...)
  std::sort(m_indicesSource->begin(), m_indicesSource->end());
  std::sort(m_indicesTarget->begin(), m_indicesTarget->end());

  //for (unsigned int i = 0; i < 101; ++i) std::cout << i << " " << m_indicesSource->at(i) << " " << m_indicesTarget->at(i) << std::endl;
  
  m_log << Log::INFO << "PrepareIndices() : Sorted vector of indices created!" << Log::endl();

}


int Algorithm::BinarySearchIndex(const std::vector<float> & cDist , float cVal, int l, int r) const
{

  // check if left/right are valid
  if (l < 0 || r > (int)cDist.size() - 1 || r < l) {
    m_log << Log::ERROR << "BinarySearchIndex() : Something went wrong:  l = " << l << ", r = " << r << "cDist.size() = " << cDist.size() << Log::endl();
    throw(0);
  }

  // print debug
  //m_log << Log::DEBUG << "BinarySearchIndex() : Searching --> cVal = " << cVal << ", cDist[" << l << "] = " << cDist[l] << ", cDist[" << r << "] = " << cDist[r] <<Log::endl();
  
  // continue search if not done
  if (r - l  > 1) {

    // get index for mid-point
    int mid = l + (r - l)/2;

    // check if cVal is at 'mid' element (a somewhat pathological case)
    if (cDist[mid] == cVal) {
      //m_log << Log::DEBUG << "BinarySearchIndex() : Search done (index = " << mid <<") --> cVal = " << cVal << ", cDist[" << l << "] = " << cDist[l] << ", cDist[" << r << "] = " << cDist[r] << ", cDist[" << mid << "] = " << cDist[mid] <<Log::endl();
      return mid;
    }
    
    // check if cVal is in left subarray
    if (cDist[mid] > cVal) return BinarySearchIndex(cDist, cVal, l, mid);
    
    // otherwise, cVal is in right subarray
    return BinarySearchIndex(cDist, cVal, mid, r);

  }

  // r - l <= 1 : we're done - return upper index
  //m_log << Log::DEBUG << "BinarySearchIndex() : Search done (index = " << r <<") --> cVal = " << cVal << ", cDist[" << l << "] = " << cDist[l] << ", cDist[" << r << "] = " << cDist[r] <<Log::endl();
  return r;

}
