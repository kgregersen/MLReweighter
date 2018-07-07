#ifndef __ALGORITHM__
#define __ALGORITHM__

// stl includes
#include <fstream>
#include <vector>

// local includes
#include "Log.h"

// forward declarations
class TTree;


class Algorithm {

public:

  // constructors
  Algorithm();
  Algorithm(TTree * source, TTree * target);

  // destructor
  virtual ~Algorithm() {}

  // initialize algorithm
  virtual void Initialize() = 0;

  // process algorithm
  virtual void Process() = 0;

  // write weights to outout file
  virtual void Write(std::ofstream & outfile) = 0;
  
  // get weight
  virtual void GetWeight(float & weight, float & error) const = 0;

  
protected:

  // trees
  TTree * m_source;
  TTree * m_target;
  
  // indices to events to be used
  void PrepareIndices();
  std::vector<long> * m_indicesSource;
  std::vector<long> * m_indicesTarget;
  
  // helper functions
  int BinarySearchIndex(const std::vector<float> & cDist , float cVal, int l, int r) const;
  float GetNormalization() const;
  
  // event weights
  std::vector<float> m_weights;

  // log
  mutable Log m_log;
  
};


#endif
