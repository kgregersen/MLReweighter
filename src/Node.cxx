// local includes
#include "Node.h"
#include "Branch.h"
#include "Event.h"
#include "Config.h"
#include "DecisionTree.h"
#include "Method.h"

// stl includes
#include <map>
#include <iomanip>
#include <string>
#include <limits>
#include <cmath>
#include <algorithm>

// ROOT includes
#include "TTree.h"
#include "TRandom3.h"



Node::Node(Branch * input) :
  m_status(NEW),
  m_input(input),
  m_output1(0),
  m_output2(0),
  m_weight(0.),
  m_weightIsSet(false),
  m_sumSource(-1),
  m_sumTarget(-1),
  m_doFeatSampling(false),
  m_splitMode(NONE),
  m_log("Node")
{

  // set this node as output node of input branch, and get sum of events from input branch
  if ( input ) {

    input->SetOutputNode(this);
    m_sumSource = input->SumSource();
    m_sumTarget = input->SumTarget();

    // set to FINAL if there are fewer than twice the min number of events on the node since then it can't be split
    // (there still exist other cases where it can't be split, but we have to fill the histograms to identify these...)
    int minEvents = 0;
    Config::Instance().getif<int>("MinEventsNode", minEvents);
    if ( m_sumTarget < 2.*minEvents || m_sumSource < 2.*minEvents) {
      m_status = FINAL;
    }

  }

  // set method depending switches 
  static const std::string & methodStr = Config::Instance().get<std::string>("Method");
  Method::TYPE method = Method::Type( methodStr );
  // feature sampling
  if (method == Method::RF || method == Method::ET) {
    m_doFeatSampling = true;
  }
  // split mode
  if (method == Method::ET) {
    m_splitMode = RANDOM;
  }
  else if (method == Method::BDT || method == Method::RF) {
    m_splitMode = CHISQUARE;
  }

  // set log level
  std::string str_level;
  Config::Instance().getif<std::string>("PrintLevel", str_level);
  if (str_level.length() > 0) {
    Log::LEVEL level = Log::StringToLEVEL(str_level);
    m_log.SetLevel(level);
  }
  
}


Node::~Node()
{

  delete m_input;
  m_input = 0;

  for (unsigned int i = 0; i < m_histSetSource.size(); ++i) {
    delete m_histSetSource.at(i);
    m_histSetSource.at(i) = 0;
  }

  for (unsigned int i = 0; i < m_histSetTarget.size(); ++i) {
    delete m_histSetTarget.at(i);
    m_histSetTarget.at(i) = 0;
  }

}


void Node::Initialize(const HistDefs * histDefs)
{

  // check if this node was already initialized
  if ( m_histSetSource.size() || m_histSetTarget.size() ) {
    m_log << Log::ERROR << "Initialize() : Histograms already initialized!" << Log::endl();
    throw(0);
  }
  
  // get variables used for splitting the tree
  const std::vector<HistDefs::Entry> & histDefEntries = histDefs->GetEntries();
  std::vector<unsigned int> indices;
  if ( m_doFeatSampling ) {
    
    // Random Forest and ExtraTrees use "feature sampling", only using random subset of the variables to grow the decision tree
    static int samplingFractionSeed = Config::Instance().get<float>("SamplingFractionSeed");
    static float featSamplingFraction = Config::Instance().get<float>("FeatureSamplingFraction");
    static TRandom3 ran( samplingFractionSeed );
    for (unsigned int index = 0; index < histDefEntries.size(); ++index) indices.push_back( index );
    for (unsigned int index = 0; index < histDefEntries.size(); ++index) std::swap(indices[ index ], indices[static_cast<int>(ran.Rndm()*(static_cast<float>(indices.size()) - std::numeric_limits<float>::epsilon()))] );
    indices.resize(featSamplingFraction*histDefEntries.size());
    
  }
  else {
    
    // use all variables
    for (unsigned int index = 0; index < histDefEntries.size(); ++index) {
      indices.push_back( index );
    }
    
  }

  // we need at least one variable
  if ( indices.size() == 0 ) {
    m_log << Log::ERROR << "Initialize() : No variables chosen for splitting the node! Check that 'FeatSamplingFraction' is not set to low compared to the total number of variables defined in 'inc/VARIABLES'" << Log::endl();
    throw(0);
  }
  
  // declare target and initial histograms for each variable
  for (unsigned int index : indices) {
    const HistDefs::Entry & histDef = histDefEntries.at(index);
    m_histSetSource.push_back( new Hist(histDef) );
    m_histSetTarget.push_back( new Hist(histDef) ); 
  }
  
  
}


void Node::Build(Branch *& b1, Branch *& b2)
{

  // get node split
  Summary * nodeSummary = 0;
  if ( m_splitMode == RANDOM ) {
    nodeSummary = SplitRandom();
  }
  else if ( m_splitMode == CHISQUARE ) {
    nodeSummary = SplitChisquare();
  }
  else {
    m_log << Log::ERROR << "Couldn't optimize node splitting - no split function was chosen!" << Log::endl();
    throw(0);
  }
				   
  // sanity check
  if ( m_input == 0 && nodeSummary == 0 ) {
    m_log << Log::ERROR << "Build() : This is the first node in the tree (input branch is null), but there is no node summary - we can't build the friggin tree?!?!" << Log::endl();
    throw(0);
  }
  
  // set node status 
  if ( nodeSummary == 0 ) {
    m_status = FINAL;
  }
  else if ( m_input == 0 ) {
    m_status = FIRST;
    m_sumTarget = nodeSummary->TargetHist()->ROOTHist()->Integral(0, -1);
    m_sumSource = nodeSummary->SourceHist()->ROOTHist()->Integral(0, -1);
  } 
  else {
    m_status = INTERMEDIATE;
  }
  
  // set outgoing branches
  if ( m_status == FINAL ) {
    b1 = 0;
    b2 = 0;
  }
  else {
    b1 = new Branch(this, nodeSummary->Name(), nodeSummary->CutValue(), false, nodeSummary->SumSourceLow() , nodeSummary->SumTargetLow() );
    b2 = new Branch(this, nodeSummary->Name(), nodeSummary->CutValue(), true , nodeSummary->SumSourceHigh(), nodeSummary->SumTargetHigh());
  }

  // set output branches for this node
  m_output1 = b1;
  m_output2 = b2;

  // print info
  Print("Build() : ", Log::VERBOSE);
  
  // clean up
  delete nodeSummary;
  for (Hist * hist : m_histSetSource) {
    delete hist;
    hist = 0;
  }
  for (Hist * hist : m_histSetTarget) {
    delete hist;
    hist = 0;
  }
  
}


Node::Summary * Node::SplitChisquare()
{
  
  // declare NodeSummary
  Summary * nodeSummary = 0;
  
  // get number of histograms
  int nhist = m_histSetSource.size();
  if ( nhist == 0 ) {
    m_log << Log::ERROR << "SplitChisquare() : No histograms!" << Log::endl();
    throw(0);
  }
  
  // get min events required to form a node
  static int minEvents = Config::Instance().get<int>("MinEventsNode");
  
  // calculate cut-values and chisquares
  for (int i = 0; i < nhist; ++i) {
    
    Hist * histTarg = m_histSetTarget.at(i);
    Hist * histSour = m_histSetSource.at(i);
    
    const TH1F * rootHistTarg = histTarg->ROOTHist();
    const TH1F * rootHistSour = histSour->ROOTHist();
    
    m_log << Log::DEBUG << "SplitChisquare() : targ integral = " << rootHistTarg->Integral(0,-1) << "  sour integral = " << rootHistSour->Integral(0,-1) << Log::endl();
    
    float maxChisquare = 0;
    float cutValue = std::numeric_limits<float>::max();
    float sumTargetLow  = 0;
    float sumTargetHigh = 0;
    float sumSourceLow  = 0;
    float sumSourceHigh = 0;
    
    // loop over bins in histogram
    for (int xbin = 1; xbin < rootHistTarg->GetNbinsX(); ++xbin) {
      
      // get integrals above and below
      Double_t sumSourLowErr  = 0;
      Double_t sumTargLowErr  = 0;
      Double_t sumSourHighErr = 0;
      Double_t sumTargHighErr = 0;
      Double_t sumSourLow  = rootHistSour->IntegralAndError(0       , xbin, sumSourLowErr );
      Double_t sumTargLow  = rootHistTarg->IntegralAndError(0       , xbin, sumTargLowErr );
      Double_t sumSourHigh = rootHistSour->IntegralAndError(xbin + 1, -1  , sumSourHighErr);
      Double_t sumTargHigh = rootHistTarg->IntegralAndError(xbin + 1, -1  , sumTargHighErr);
      
      m_log << Log::DEBUG << "SplitChisquare() : sumSourLow = " << sumSourLow << "  sumTargLow = " << sumTargLow << "  sumSourHigh = " << sumSourHigh << "  sumTargHigh = " << sumTargHigh << Log::endl();
      
      // check min events on potential sub-nodes
      if (sumSourLow < minEvents || sumSourHigh < minEvents || sumTargLow < minEvents || sumTargHigh < minEvents ) continue;
      
      // calculate chisquare and update best candidate
      float chisquare = pow(sumSourLow - sumTargLow, 2)/(pow(sumSourLowErr, 2) + pow(sumTargLowErr, 2)) + pow(sumSourHigh - sumTargHigh, 2)/(pow(sumSourHighErr, 2) + pow(sumTargHighErr, 2));
      if (chisquare > maxChisquare) {
	maxChisquare  = chisquare;
	cutValue      = rootHistTarg->GetBinLowEdge(xbin + 1);
	sumSourceLow  = sumSourLow;
	sumSourceHigh = sumSourHigh;
	sumTargetLow  = sumTargLow;
	sumTargetHigh = sumTargHigh;
      }
      
    }
    
    m_log << Log::DEBUG << "SplitChisquare() : sumSourceLow = " << sumSourceLow << "  sumTargetLow = " << sumTargetLow << "  sumSourceHigh = " << sumSourceHigh << "  sumTargetHigh = " << sumTargetHigh << Log::endl();
    
    // store info for this variable
    if (maxChisquare > 0) {
      if ( ! nodeSummary ) {
	nodeSummary = new Summary(histSour, histTarg, cutValue, maxChisquare, sumSourceLow, sumTargetLow, sumSourceHigh, sumTargetHigh);
      }
      else if ( maxChisquare > nodeSummary->Chisquare() ) {
	delete nodeSummary;
	nodeSummary = new Summary(histSour, histTarg, cutValue, maxChisquare, sumSourceLow, sumTargetLow, sumSourceHigh, sumTargetHigh);
      }
    }
    
  }

  // return result
  return nodeSummary;
  
}
 

Node::Summary * Node::SplitRandom()
{

  // declare NodeSummary
  Summary * nodeSummary = 0;

  // get number of histograms
  int nhist = m_histSetSource.size();
  if ( nhist == 0 ) {
    m_log << Log::ERROR << "SplitRandom() : No histograms!" << Log::endl();
    throw(0);
  }
  
  // get min events required to form a node
  static int minEvents = Config::Instance().get<int>("MinEventsNode");

  // randomly chose variable
  static int samplingFractionSeed = Config::Instance().get<float>("SamplingFractionSeed");
  static TRandom3 ran( samplingFractionSeed );
  unsigned int ranIndex = static_cast<unsigned int>(ran.Rndm()*(static_cast<float>(nhist) - std::numeric_limits<float>::epsilon()));

  // get ROOT histograms
  Hist * histTarg = m_histSetTarget.at( ranIndex );
  Hist * histSour = m_histSetSource.at( ranIndex );
  const TH1F * rootHistTarg = histTarg->ROOTHist();
  const TH1F * rootHistSour = histSour->ROOTHist();

  // identify valid cut values
  std::vector<int> xbins;
  for (int xbin = 1; xbin <= rootHistSour->GetNbinsX(); ++xbin) {

    // get event counts below/above potential cut
    Double_t sumSourLowErr  = 0;
    Double_t sumTargLowErr  = 0;
    Double_t sumSourHighErr = 0;
    Double_t sumTargHighErr = 0;
    Double_t sumSourLow  = rootHistSour->IntegralAndError(0       , xbin, sumSourLowErr );
    Double_t sumTargLow  = rootHistTarg->IntegralAndError(0       , xbin, sumTargLowErr );
    Double_t sumSourHigh = rootHistSour->IntegralAndError(xbin + 1, -1  , sumSourHighErr);
    Double_t sumTargHigh = rootHistTarg->IntegralAndError(xbin + 1, -1  , sumTargHighErr);

    // check if both source and target distributions have enough events below/above the cuts
    if (sumSourLow >= minEvents && sumSourHigh >= minEvents && sumTargLow >= minEvents && sumTargHigh >= minEvents ) {
      xbins.push_back( xbin );
    }
    
  }

  // if any valid cuts, then randomly pick one
  if ( xbins.size() > 0 ) {

    // get random cut (among valid cuts) 
    int index = static_cast<int>(ran.Rndm()*(static_cast<float>(xbins.size()) - std::numeric_limits<float>::epsilon()));
    int xbin = xbins.at(index);

    // get event counts below/above cut
    Double_t sumSourLowErr  = 0;
    Double_t sumTargLowErr  = 0;
    Double_t sumSourHighErr = 0;
    Double_t sumTargHighErr = 0;
    Double_t sumSourLow  = rootHistSour->IntegralAndError(0       , xbin, sumSourLowErr );
    Double_t sumTargLow  = rootHistTarg->IntegralAndError(0       , xbin, sumTargLowErr );
    Double_t sumSourHigh = rootHistSour->IntegralAndError(xbin + 1, -1  , sumSourHighErr);
    Double_t sumTargHigh = rootHistTarg->IntegralAndError(xbin + 1, -1  , sumTargHighErr);

    // calculate chisquare and get cut value
    float chisquare = pow(sumSourLow - sumTargLow, 2)/(pow(sumSourLowErr, 2) + pow(sumTargLowErr, 2)) + pow(sumSourHigh - sumTargHigh, 2)/(pow(sumSourHighErr, 2) + pow(sumTargHighErr, 2));
    float cutValue  = rootHistTarg->GetBinLowEdge(xbin + 1);
    
    // set node summary
    nodeSummary = new Summary(histSour, histTarg, cutValue, chisquare, sumSourLow, sumTargLow, sumSourHigh, sumTargHigh);
    
  } 
 
  if ( nodeSummary ) m_log << Log::VERBOSE << "SplitRandom() : Node::Summary details:  Chisquare = " << nodeSummary->Chisquare() << "  SumTargetLow = " << nodeSummary->SumTargetLow() << "  SumSourceLow = " << nodeSummary->SumSourceLow() << "  SumTargetHigh = " << nodeSummary->SumTargetHigh() << "  SumSourceHigh = " << nodeSummary->SumSourceHigh() << Log::endl();
  else m_log << Log::VERBOSE << "SplitRandom() : No Summary!" << Log::endl();
  
  // return result
  return nodeSummary;
  
}


void Node::Print(const std::string & prefix, Log::LEVEL level) const
{

  m_log << level << prefix << "-----------> INFO <-----------" << Log::endl();
  m_log << level << prefix << "Status      : " << StatusStr() << Log::endl(); 
  m_log << level << prefix << "Sum Target  : " << m_sumTarget << Log::endl(); 
  m_log << level << prefix << "Sum Source  : " << m_sumSource << Log::endl(); 
  m_log << level << prefix << "Cuts        : ";
  const Branch * b = InputBranch();
  while ( b ) {
    // get cut object
    const Branch::Cut * cut = b->CutObject();
    const Branch::Smaller * lt = dynamic_cast<const Branch::Smaller *>(cut);
    const Branch::Greater * gt = dynamic_cast<const Branch::Greater *>(cut);    
    // check if valid
    if ( (lt && gt) || (!lt && !gt) ) {
      m_log << Log::endl();
      m_log << Log::ERROR << "Couldn't determine if greater or smaller!" << Log::endl();
      throw(0);
    }
    // print cut
    m_log << cut->GetVariable()->Name() << (lt ? "<" : ">") << cut->CutValue() << "|";
    // update branch
    b = b->InputNode()->InputBranch();
  }
  m_log << Log::endl();
  m_log << level << prefix << "------------------------------" << Log::endl();

}


Node::STATUS Node::Status() const
{

  return m_status;

}
  

void Node::SetStatus(Node::STATUS status)
{

  m_status = status;

}
  

const Branch * Node::InputBranch() const
{

  return m_input;

}


const Branch * Node::OutputBranch() const
{

  if      ( m_output1 && m_output1->Pass() ) return m_output1;
  else if ( m_output2 && m_output2->Pass() ) return m_output2;
  else if ( m_output1 && m_output2 ) {
    m_log << Log::ERROR << "OutputBranch() : Output branches are not both null, but the event doesn't pass one of them!" << Log::endl();
    throw(0);
  }
  
  return nullptr;

}


const Branch * Node::OutputBranch(bool isGreater) const
{

  if (isGreater) return m_output2;
  return m_output1;
  
}


void Node::SetOutputBranch(const Branch * branch, bool isGreater)
{
  
  if (isGreater) m_output2 = branch;
  else m_output1 = branch;
  
}


void Node::FillSource(float weight)
{

  // fill histograms
  for (Hist * hist : m_histSetSource) {
    hist->Fill(weight);
  }

}


void Node::FillTarget(float weight)
{

  // fill histograms
  for (Hist * hist : m_histSetTarget) {
    hist->Fill(weight);
  }

}


float Node::SumSource() const
{

  return m_sumSource;

}


float Node::SumTarget() const
{

  return m_sumTarget;

}


void Node::SetAndLockWeight(float weight) const
{
  
  if (m_weightIsSet == false) {
    m_weight = weight;
    m_weightIsSet = true;
  }

}

float Node::GetWeight() const
{

  return m_weight;

}

 
std::string Node::StatusStr() const
{

  if (m_status == NEW         ) return "NEW";
  if (m_status == FIRST       ) return "FIRST";
  if (m_status == INTERMEDIATE) return "INTERMEDIATE";
  if (m_status == FINAL       ) return "FINAL";
  return "NOSTATUS";
  
}
