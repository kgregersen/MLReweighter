#ifndef __FOREST__
#define __FOREST__

// local includes
#include "Log.h"

// std includes
#include <vector>
#include <string>

// forward declarations
class DecisionTree;


class Forest {

public:
  
  // constructors
  Forest();
  Forest(const std::vector<const DecisionTree *> trees);

  // destructor
  ~Forest();

  // add decision tree to forest
  void AddTree(const DecisionTree * tree);  

  // get decision trees
  const std::vector<const DecisionTree *> & GetTrees() const;
  
  // read in forest(s) from file
  static const std::vector<const Forest *> ReadForests(const std::string & weightsFileName);
  

private:

  // trees
  std::vector<const DecisionTree *> m_trees;
 
  // log
  static Log m_log;
  
};




#endif
