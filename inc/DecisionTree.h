#ifndef __DECISIONTREE__
#define __DECISIONTREE__

// stl includes
#include <vector>
#include <utility>
#include <fstream>

// local includes
#include "Log.h"
#include "Branch.h"

// forward declarations
class Node;
class HistDefs;
class TTree;


class DecisionTree {

public:

  // target/source
  enum INPUT {
    SOURCE,
    TARGET    
  };

  // constructor (calculate weights)
  DecisionTree(TTree * source, TTree * target, const std::vector<long> * indicesSource, const std::vector<long> * indicesTarget, const HistDefs * histDefs);

  // constructor (apply weights)
  DecisionTree(const std::vector<std::pair<float, std::vector<const Branch::Cut *> > > & tree);

  // destructor
  ~DecisionTree();

  // grow tree
  void GrowTree(std::vector<float> * weights = 0);

  // finalize weights on final nodes
  void FinalizeWeights();

  // get weight
  float GetWeight() const;

  // print tree
  void Print(const std::string & prefix, Log::LEVEL level) const;

  // write to file
  void Write(std::ofstream & file) const;

  
private:
  
  // fill nodes
  void FillNodes(std::vector<Node *> layer, INPUT input,  std::vector<float> * MLWeights = 0) const;

  // update ML weights
  void UpdateWeights(std::vector<float> * MLWeights) const;

  // create new node
  void CreateNode(Branch * input, std::vector<Node *> & nextLayer);

  // add node to decision tree
  void AddNodeToTree(const Node * node);

  // helper functions
  const Node * FirstNode() const; 
  const std::vector<const Node *> FinalNodes() const;

  // TTrees for initial and target samples, and event indices for Random Forest
  TTree * m_source;
  TTree * m_target;
  const std::vector<long> * m_indicesSource;
  const std::vector<long> * m_indicesTarget;

  // histogram definitions
  const HistDefs * m_histDefs;
  
  // nodes
  std::vector<const Node *> m_nodes;

  // logger
  mutable Log m_log;

};

#endif
