#ifndef __BDT__
#define __BDT__

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


class BDT : public Algorithm {

public:

  // constructors
  BDT(TTree * source, TTree * target);
  BDT(std::vector<const Forest *> forests);

  // destructor
  virtual ~BDT();

  // initialize algorithm
  virtual void Initialize();

  // process algorithm
  virtual void Process();

  // write weights to outout file
  virtual void Write(std::ofstream & outfile);

  // get weight
  virtual void GetWeight(float & weight, float & error);

    
private:

  // histogram definitions
  HistDefs * m_histDefs;

  // forest(s)
  std::vector<const Forest *> m_forests;

  // Log
  mutable Log m_log;

};


#endif
