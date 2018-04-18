#ifndef __NODE__
#define __NODE__

//stl includes
#include <vector>
#include <string>
#include <iostream>

// local includes
#include "Log.h"
#include "HistDefs.h"

// Rootv includes
#include "TH1F.h"

// forward declarations
class Branch;
class Event;
class DecisionTree;
class TTree;


class Node {

public:


  // -----------------------
  // class to hold histogram
  // -----------------------
  class Hist {

  public:

    // constructor
    Hist(const HistDefs::Entry & histDef) : m_hist(0), m_variable(histDef.GetVariable())
    {
      m_hist = new TH1F(histDef.Name().c_str(), histDef.Name().c_str(), histDef.Nbins(), histDef.Xmin(), histDef.Xmax());
      m_hist->SetDirectory(0);
    }

    // destructor
    ~Hist()
    {
      delete m_hist;
      m_hist = 0;
    }

    // fill histogram
    void Fill(float weight) { m_hist->Fill(m_variable->Value(), weight); }

    // get name
    const std::string & Name() const { return m_variable->Name(); }
    
    // get ROOT hist
    const TH1F * ROOTHist() const { return m_hist; }

    
  private:

    // histogram and variable
    TH1F * m_hist;
    const Variable * m_variable;

  };

  // --------------------------
  // class to hold hist summary
  // --------------------------
  class Summary {

  public:

    // constructor
    Summary(Hist * source, Hist * target, float cutValue, float chisquare, float sumInitLow, float sumTargLow, float sumInitHigh, float sumTargHigh) :
      m_source(source), m_target(target), m_cutValue(cutValue), m_chisquare(chisquare), m_sumSourceLow(sumInitLow), m_sumTargetLow(sumTargLow), m_sumSourceHigh(sumInitHigh), m_sumTargetHigh(sumTargHigh) {}

    ~Summary()
    {
      // delete m_source;
      // delete m_target;
      // m_source = 0;
      // m_target = 0;
    }
    
    // get information
    const Hist * SourceHist() const { return m_source; }
    const Hist * TargetHist() const { return m_target; }
    float CutValue     () const { return m_cutValue;      }
    float Chisquare    () const { return m_chisquare;     }
    float SumSourceLow () const { return m_sumSourceLow;  }
    float SumTargetLow () const { return m_sumTargetLow;  }
    float SumSourceHigh() const { return m_sumSourceHigh; }
    float SumTargetHigh() const { return m_sumTargetHigh; }
    const std::string & Name() const { return m_source->Name(); }

    
  private:

    // summary info
    Hist * m_source;
    Hist * m_target;
    float m_cutValue;
    float m_chisquare;
    float m_sumSourceLow;
    float m_sumTargetLow;
    float m_sumSourceHigh;
    float m_sumTargetHigh;
    
  };
  
  
  // Node type
  enum STATUS {
    NEW,
    FIRST,
    INTERMEDIATE,
    FINAL
  };

  // split mode
  enum SPLITMODE {
    NONE,
    CHISQUARE,
    RANDOM
  };
  
  
  // constructors
  Node(Branch * input = 0);

  // destructor
  ~Node();

  // get status
  STATUS Status() const;

  // string version of status
  std::string StatusStr() const;
   
  // set status
  void SetStatus(STATUS status);

  // get input branch
  const Branch * InputBranch() const;

  // get output branch
  const Branch * OutputBranch() const;

  // check if output branch exist
  const Branch * OutputBranch(bool isGreater) const;

  // set output branch (for reconstructing decision tree)
  void SetOutputBranch(const Branch * branch, bool isGreater);
  
  // intialize histograms
  void Initialize(const HistDefs * histDefs);
  
  // fill histograms
  void FillSource(float weight);
  void FillTarget(float weight);

  // build node
  void Build(Branch *& b1, Branch *& b2);

  // node splitting functions
  Summary * SplitChisquare();
  Summary * SplitRandom();
  
  // get #source
  float SumSource() const;
  
  // get #target
  float SumTarget() const;

  // get weight
  float GetWeight() const;

  // set weight (and lock it)
  void SetAndLockWeight(float weight) const;

  // print
  void Print(const std::string & prefix, Log::LEVEL level) const;

    
private:

  // status
  STATUS m_status;
  
  // I/O branches
  const Branch * m_input;
  const Branch * m_output1;
  const Branch * m_output2;

  // weight
  mutable float m_weight;
  mutable bool  m_weightIsSet;

  // histograms
  std::vector<Hist *> m_histSetSource;
  std::vector<Hist *> m_histSetTarget;
  
  // sum of events
  float m_sumSource;
  float m_sumTarget;

  // switch for doing feature sampling
  bool m_doFeatSampling;

  // switch for split mode
  SPLITMODE m_splitMode;
  
  // logger
  mutable Log m_log;

};

#endif
