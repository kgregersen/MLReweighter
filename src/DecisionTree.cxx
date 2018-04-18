// local includes
#include "DecisionTree.h"
#include "Branch.h"
#include "Node.h"
#include "Config.h"
#include "Event.h"
#include "HistDefs.h"

// stl includes
#include <vector>
#include <algorithm>
#include <limits>

// ROOT includes
#include "TTree.h"
#include "TRandom3.h"



DecisionTree::DecisionTree(TTree * source, TTree * target, const std::vector<long> * indicesSource, const std::vector<long> * indicesTarget, const HistDefs * histDefs) :
  m_source(source),
  m_target(target),
  m_indicesSource(indicesSource),
  m_indicesTarget(indicesTarget),
  m_histDefs(histDefs),
  m_log("DecisionTree")
{
  
  std::string str_level;
  Config::Instance().getif<std::string>("PrintLevel", str_level);
  if (str_level.length() > 0) {
    Log::LEVEL level = Log::StringToLEVEL(str_level);
    m_log.SetLevel(level);
  }
  
}


DecisionTree::DecisionTree(const std::vector<std::pair<float, std::vector<const Branch::Cut *> > > & tree) :
  m_source(0),
  m_target(0),
  m_indicesSource(0),
  m_indicesTarget(0),
  m_histDefs(0),
  m_log("DecisionTree")
{

  std::string str_level;
  Config::Instance().getif<std::string>("PrintLevel", str_level);
  if (str_level.length() > 0) {
    Log::LEVEL level = Log::StringToLEVEL(str_level);
    m_log.SetLevel(level);
  }
  
  // declare first node
  Node * firstNode = new Node(0); 
  firstNode->SetStatus(Node::FIRST);   
  AddNodeToTree(firstNode);
  
  // loop over entries (weight, cuts) and add nodes and branches
  for (unsigned int inode = 0; inode < tree.size(); ++inode) {

    // get final node weight
    float weight = tree.at(inode).first;

    // get cuts leading to the final node
    const std::vector<const Branch::Cut *> & cuts = tree.at(inode).second;

    // now add nodes and branches as needed to reconstruct this part of the decision tree
    Node * node = firstNode;
    for (const Branch::Cut * cut : cuts) {

      // try to fetch output branch corresponding to the current cut
      bool isGreater = dynamic_cast<const Branch::Greater *>(cut);
      Branch * branch = const_cast<Branch *>(node->OutputBranch(isGreater));
      
      // if branch doesn't exist, then create it (and its output node), otherwise fetch its output node
      if ( ! branch ) {
	Branch * b = new Branch(node, cut->GetVariable()->Name(), cut->CutValue(), isGreater, 0, 0);
	node->SetOutputBranch(b, isGreater);
	node = new Node(b);
	node->SetStatus(Node::INTERMEDIATE);
	AddNodeToTree(node);
      }
      else {
	node = const_cast<Node *>(branch->OutputNode());
      }

    }

    // set weight and status of final node
    node->SetAndLockWeight(weight);
    node->SetStatus(Node::FINAL);
    
  }

  // print tree to screen
  m_log << Log::VERBOSE << "DecisionTree() : ----------------> VERBOSE <----------------" << Log::endl();
  Print("DecisionTree() : ", Log::VERBOSE);
  m_log << Log::VERBOSE << "DecisionTree() : ----------------------------------------" << Log::endl();
  
}


DecisionTree::~DecisionTree()
{

  for (unsigned int i = 0; i < m_nodes.size(); ++i) {
    delete m_nodes.at(i);
    m_nodes.at(i) = 0;
  }
  
}


void DecisionTree::GrowTree(std::vector<float> * MLWeights)
{

  // print info
  static int counter = 1;
  m_log << Log::INFO << "GrowTree() : Decision Tree " << counter++ << Log::endl();

  // keep track of time
  std::clock_t start = std::clock();

  // declare first node
  Node * node = new Node(0);

  // declare vector to hold nodes in a given layer
  std::vector<Node *> layer;
  layer.push_back(node);
  
  // grow tree layer-by-layer
  int nlayers = 0;
  while ( layer.size() > 0 ) {

    // check number of layers
    static int maxLayers = Config::Instance().get<int>("MaxTreeLayers");
    if ( nlayers >= maxLayers ) {

      // print verbose message
      m_log << Log::VERBOSE << "GrowTree() : Max layers reached - finalizing nodes!" << Log::endl();

      // set status of nodes to FINAL and add to decision tree
      for (Node * node : layer) {
	node->SetStatus( Node::FINAL );
	AddNodeToTree( node );
      }

      // break out of loop (no more layers to grow)
      break;
      
    }

    // initialise histograms for each variable on nodes
    for (Node * node : layer) {
      node->Initialize(m_histDefs);
    }
      
    // fill nodes (first target, then source)
    // (if MLWeights from previous trees are provided (BDT), they are used in conjunction with the intrinsic event weight)
    FillNodes(layer, TARGET, 0);
    FillNodes(layer, SOURCE, MLWeights); 
    
    // prepare vector for next layer of nodes
    std::vector<Node *> nextLayer;
        
    // build nodes
    for (Node * node : layer) {
      
      // declare the node's output branches
      Branch * b1 = 0;
      Branch * b2 = 0;
      
      // build node
      node->Build(b1, b2);

      // add to decision tree nodes
      AddNodeToTree(node);
      
      // create sub-nodes (if branches exist)
      if ( b1 ) CreateNode(b1, nextLayer);
      if ( b2 ) CreateNode(b2, nextLayer);
      
    }

    // set next layer
    layer = nextLayer;
    
    // increment layer counter
    ++nlayers;
    
  }

  // calculate and set weights on final nodes
  FinalizeWeights();
  
  // update ML weights
  if ( MLWeights ) UpdateWeights(MLWeights);
  
  // time spent on growing tree
  double duration = (std::clock() - start)/static_cast<double>(CLOCKS_PER_SEC);    
  
  // print tree to screen
  m_log << Log::INFO << "GrowTree() : ----------------> INFO <----------------" << Log::endl();
  m_log << Log::INFO << "GrowTree() : Time spent  : " << duration << " sec" << Log::endl();
  Print("GrowTree() : ", Log::INFO);
  m_log << Log::INFO << "GrowTree() : ----------------------------------------" << Log::endl();

  
}


void DecisionTree::FinalizeWeights()
{
  
  // get final nodes
  std::vector<const Node *> finalNodes = FinalNodes();

  // prepare vector of weights (one per final node)
  std::vector<double> weights(finalNodes.size());

  // loop over final nodes to get individual weights and normalization factor
  double sumSource = 0;
  double sumTarget = 0;
  for (unsigned int i = 0; i < finalNodes.size(); ++i) {
    const Node * node = finalNodes[i];
    double target = node->SumTarget();
    double source = node->SumSource();
    if ( ! (source > 0) ) {
      m_log << Log::ERROR << "FinalizeWeights() : source is not positive! (source = " << source << ")" << Log::endl();
      throw(0);
    }
    static float learningRate = Config::Instance().get<float>("LearningRate");
    double w = exp(learningRate*log(target/source));
    weights.at(i)  = w;
    sumSource     += w*source;
    sumTarget     += target;
  }

  // now set and lock the weights on the final nodes
  for (unsigned int i = 0; i < weights.size(); ++i) {
    finalNodes.at(i)->SetAndLockWeight( sumTarget*weights[i]/sumSource );
  }

}

  
void DecisionTree::Print(const std::string & prefix, Log::LEVEL level) const
{

  // get final nodes
  std::vector<const Node *> finalNodes = FinalNodes();
  m_log << level << prefix << "Final nodes : " << finalNodes.size() << " (out of " << m_nodes.size() << ")" << Log::endl();

  // get total number of events
  float totalTarget = 0;
  float totalSource = 0;
  for (const Node * node : finalNodes) {
    totalTarget += node->SumTarget();
    totalSource += node->SumSource();
  }
  m_log << level << prefix << "Total target : " << totalTarget << Log::endl();
  m_log << level << prefix << "Total source : " << totalSource << Log::endl();
  
  // loop over final nodes and print weights and cuts
  for (const Node * node : finalNodes) {
    m_log << level << prefix << " ---> Weight = " << std::setw(10) << std::left << node->GetWeight();
    if ( node->SumSource() > 0 ) {
      m_log << " SumTarget / SumSource = " << std::setw(6) << std::right << node->SumTarget() << " / " << std::setw(6) << std::right << node->SumSource() << " = " << std::setw(10) << std::left << node->SumTarget()/node->SumSource();
    }
    m_log << "  Cuts = ";
    const Branch * b = node->InputBranch();
    while ( b ) {     
      // get cut object
      const Branch::Cut * cut = b->CutObject();
      const Branch::Smaller * lt = dynamic_cast<const Branch::Smaller *>(cut);
      const Branch::Greater * gt = dynamic_cast<const Branch::Greater *>(cut);    
      // check if valid
      if ( (lt && gt) || (!lt && !gt) ) {
	m_log << Log::endl();
	m_log << Log::ERROR << prefix << "Couldn't determine if cut is greater or smaller!" << Log::endl();
	throw(0);
      }
      // print cut
      m_log << cut->GetVariable()->Name() << (lt ? "<" : ">") << cut->CutValue() << "|";
      // update branch
      b = b->InputNode()->InputBranch();
    }
    m_log << Log::endl();
  }
  
}


void DecisionTree::CreateNode(Branch * input, std::vector<Node *> & nextLayer) 
{

  // declare node
  Node * node = new Node(input);

  // check if it's a FINAL node or if we can grow it further
  if (node->Status() == Node::FINAL) {
    AddNodeToTree( node );
  }
  else {
    nextLayer.push_back( node );
  }

}


void DecisionTree::FillNodes(std::vector<Node *> layer, INPUT input, std::vector<float> * MLWeights) const
{

  // switch target/source
  TTree * tree = 0;
  const std::vector<long> * indices = 0;
  if ( input == SOURCE ) {
    tree    = m_source;
    indices = m_indicesSource;
  }
  else if ( input == TARGET ) {
    tree    = m_target;
    indices = m_indicesTarget;
  }
 
  // get intrinsic event weight name
  std::string eventWeightName = Config::Instance().get<std::string>("EventWeightVariableName");

  // Loop over TTree entries
  std::clock_t start = std::clock();
  long maxEvent = indices->size();
  long reportFrac = maxEvent/(maxEvent > 100000 ? 10 : 1) + 1;
  m_log << Log::VERBOSE << "FillNodes() : Looping over events (" << tree->GetName() << ") : "  << maxEvent << Log::endl();
  for (long ievent = 0; ievent < maxEvent; ++ievent) {

    // print progress
    if( ievent > 0 && ievent % reportFrac == 0 ) {
      double duration     = (std::clock() - start)/static_cast<double>(CLOCKS_PER_SEC);    
      double frequency    = static_cast<double>(ievent) / duration;
      double timeEstimate = static_cast<double>(maxEvent - ievent) / frequency;
      m_log << Log::VERBOSE << "FillNodes() : ---> processed : " << std::setw(4) << 100*ievent/maxEvent << "\%  ---  frequency : " << std::setw(7) << static_cast<int>(frequency) << " events/sec  ---  time : " << std::setw(4) << static_cast<int>(duration) << " sec  ---  remaining time : " << std::setw(4) << static_cast<int>(timeEstimate) << " sec"<< Log::endl(); 
    }

    // event index
    long index = indices->at(ievent);
    
    // get event
    tree->GetEntry( index );

    // loop over nodes
    for (Node * node : layer) {
    
      // apply cuts
      bool pass = true;
      const Branch * b = node->InputBranch();
      while ( b ) {
	if ( ! b->Pass() ) {
	  pass = false;
	  break;
	}
	b = b->InputNode()->InputBranch();
      }
      if ( ! pass ) continue;
      
      // get intrinsic event weight
      static const float & eventWeight = Event::Instance().get<float>(eventWeightName);
      
      // fill nodes
      float MLw = 1.;
      if ( MLWeights ) {
	MLw = MLWeights->at(index);
      }
      if ( input == SOURCE ) {
	node->FillSource( MLw*eventWeight );	  
      }
      else if ( input == TARGET ) {
	node->FillTarget( eventWeight );
      }
       
      // all nodes are orthogonal, so we can break the loop here
      break;

    }
    
  }

  // print out
  double duration  = (std::clock() - start)/static_cast<double>(CLOCKS_PER_SEC);    
  double frequency = static_cast<double>(maxEvent) / duration;
  m_log << Log::VERBOSE<< "FillNodes() : ---> processed :  100\%  ---  frequency : " << std::setw(7) << static_cast<int>(frequency) << " events/sec  ---  time : " << std::setw(4) << static_cast<int>(duration) << " sec  ---  remaining time :    0 sec"<< Log::endl(); 
  
}


void DecisionTree::UpdateWeights(std::vector<float> * MLWeights) const
{

  // get TTree
  TTree * tree = m_source;
  const std::vector<long> * indices = m_indicesSource;

  // Loop over TTree entries
  std::clock_t start = std::clock();
  long maxEvent = indices->size();
  long reportFrac = maxEvent/(maxEvent > 100000 ? 10 : 1) + 1;
  m_log << Log::VERBOSE << "UpdateWeights() : Looping over events (" << tree->GetName() << ") : "  << maxEvent << Log::endl();
  for (long ievent = 0; ievent < maxEvent; ++ievent) {

    // print progress
    if( ievent > 0 && ievent % reportFrac == 0 ) {
      double duration     = (std::clock() - start)/static_cast<double>(CLOCKS_PER_SEC);    
      double frequency    = static_cast<double>(ievent) / duration;
      double timeEstimate = static_cast<double>(maxEvent - ievent) / frequency;
      m_log << Log::VERBOSE << "FillNodes() : ---> processed : " << std::setw(4) << 100*ievent/maxEvent << "\%  ---  frequency : " << std::setw(7) << static_cast<int>(frequency) << " events/sec  ---  time : " << std::setw(4) << static_cast<int>(duration) << " sec  ---  remaining time : " << std::setw(4) << static_cast<int>(timeEstimate) << " sec"<< Log::endl(); 
    }
    
    // get event index
    long index = indices->at(ievent);

    // continue if this event was already updated (when using bagging 'with replacement')
    if ( ievent > 0 && index == indices->at(ievent - 1)) continue;
    
    // get event
    tree->GetEntry( index );
 
    // update weights vector
    MLWeights->at( index ) *= GetWeight();

  }

  // print out
  double duration  = (std::clock() - start)/static_cast<double>(CLOCKS_PER_SEC);    
  double frequency = static_cast<double>(maxEvent) / duration;
  m_log << Log::VERBOSE<< "FillNodes() : ---> processed :  100\%  ---  frequency : " << std::setw(7) << static_cast<int>(frequency) << " events/sec  ---  time : " << std::setw(4) << static_cast<int>(duration) << " sec  ---  remaining time :    0 sec"<< Log::endl(); 
  
}


float DecisionTree::GetWeight() const
{
  
  // to identify which node the current event belongs to, get the first node in the tree
  const Node * node = FirstNode();

  // and then propagate down the tree until reaching a final node
  while(node->Status() != Node::FINAL) {
    node = node->OutputBranch()->OutputNode(); 
  }

  // return the weight 
  return node->GetWeight();

}


void DecisionTree::AddNodeToTree(const Node * node)
{

  // check if this node is already in the tree
  for (const Node * n : m_nodes) {
    if (node == n) {
      m_log << Log::ERROR << "AddNodeToTree() : This node already exists in the decision tree!" << Log::endl();
      throw(0);
    }
  }

  // add node to decision tree
  m_nodes.push_back( node );
  
}


const Node * DecisionTree::FirstNode() const
{

  // check if there are any nodes
  if ( m_nodes.size() == 0 ) {
    m_log << Log::ERROR << "FirstNode() : Number of nodes = " << m_nodes.size() << Log::endl();
    throw(0);
  }

  // the first node should be the first entry
  const Node * node = *(m_nodes.begin());

  // but to make sure, we check
  const Branch * b = node->InputBranch();
  while ( b ) {
    b = b->InputNode()->InputBranch();
  }

  // check status
  if ( node->Status() != Node::FIRST ) {
    m_log << Log::ERROR << "FirstNode() : Couldn't find first node! (Number of nodes = " << m_nodes.size() << ")" << Log::endl();
    throw(0);
  }

  // return first node in tree
  return node;

}


const std::vector<const Node *> DecisionTree::FinalNodes() const
{

  // initialize set of nodes
  std::vector<const Node *> finalNodes;

  // loop through nodes and identify final nodes
  for (const Node * node : m_nodes) {
    if ( node->Status() == Node::FINAL ) {
      finalNodes.push_back(node);
    }
  }

  // check if we found any
  if ( finalNodes.size() == 0 ) {
    m_log << Log::ERROR << "FinalNodes() : Couldn't find any final nodes!" << Log::endl();
    throw(0);
  }

  // return nodes
  return finalNodes;

}


void DecisionTree::Write(std::ofstream & file) const
{

  // instantiate tree counter
  static int counter = 1;

  // initial print
  file << "# Decision Tree : " << counter << "\n"; 
  ++counter;

  // print weights and corresponding cuts
  const std::vector<const Node *> & finalNodes = FinalNodes();
  for (const Node * node : finalNodes) {

    // print weight
    file << "weight=" << node->GetWeight() << ":SumTarget/SumSource=" << node->SumTarget() << "/" << node->SumSource() << "=" << node->SumTarget()/node->SumSource() << ":";

    // print cuts for each final node
    const Branch * b = node->InputBranch();
    while ( b ) {

      // get cut object
      const Branch::Cut * cut = b->CutObject();
      const Branch::Smaller * lt = dynamic_cast<const Branch::Smaller *>(cut);
      const Branch::Greater * gt = dynamic_cast<const Branch::Greater *>(cut);    
      
      // check if valid
      if ( (lt && gt) || (!lt && !gt) ) {
	m_log << Log::ERROR << "Write() : Couldn't determine if greater or smaller!" << Log::endl();
	throw(0);
      }

      // print cut
      file << cut->GetVariable()->Name() << (lt ? "<" : ">") << cut->CutValue() << "|";
	
      // update branch
      b = b->InputNode()->InputBranch();

    }

    // end line for this final node
    file << "\n";
    
  }

  // add an extra empty line
  file << "\n";
  
}

