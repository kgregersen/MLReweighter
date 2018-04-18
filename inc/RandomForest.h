#ifndef __RandomForest__
#define __RandomForest__

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


class RandomForest : public Algorithm {

public:

  // constructor
  RandomForest(TTree * source, TTree * target);
  RandomForest(std::vector<const Forest *> forests);

  // destructor
  virtual ~RandomForest();

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
