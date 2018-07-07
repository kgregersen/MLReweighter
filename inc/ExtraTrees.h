#ifndef __ExtraTrees__
#define __ExtraTrees__

// analysis includes
#include "Algorithm.h"
#include "Log.h"

// stl includes
#include <fstream>
#include <vector>

// forward declarations
class Forest;
class HistDefs;
class TTree;


class ExtraTrees : public Algorithm {

public:

  // constructor
  ExtraTrees(TTree * source, TTree * target);
  ExtraTrees(std::vector<const Forest *> forests);

  // destructor
  virtual ~ExtraTrees();

  // initialize algorithm
  virtual void Initialize();

  // process algorithm
  virtual void Process();

  // write weights to outout file
  virtual void Write(std::ofstream & outfile);

  // get weight
  virtual void GetWeight(float & weight, float & error) const;


private:

  // histogram definitions
  HistDefs * m_histDefs;

  // forest(s)
  std::vector<const Forest *> m_forests;

  // Log
  mutable Log m_log;

};


#endif
