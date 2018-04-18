#ifndef __BRANCH__
#define __BRANCH__

// stl includes
#include <string>

// local includes
#include "Log.h"
#include "Variable.h"

// forward declarations
class Node;


class Branch {

public:


  //
  // base class to hold cut
  //
  class Cut {

  public:

    // constructor
    Cut(const Variable * variable, float cutValue) : m_variable(variable), m_cutValue(cutValue) {}

    // destructor
    virtual ~Cut() {}

    // pass cut
    virtual bool Pass() const = 0;

    // get variable
    const Variable * GetVariable() const { return m_variable; }

    // get cutValue
    float CutValue() const { return m_cutValue; }
    
    
  protected:
    
    // variable info
    const Variable * m_variable;
    const float m_cutValue;
    
  };

  
  //
  // greater-than cut
  //
  class Greater : public Cut {

  public:

    // constructor
    Greater(const Variable * variable, float cutValue) : Cut(variable, cutValue) {}

    // pass cut implementation
    bool Pass() const { return m_variable->Value() >= m_cutValue; }
    
  };


  //
  // smaller-than cut
  //
  class Smaller : public Cut {
    
  public:

    // constructor
    Smaller(const Variable * variable, float cutValue) : Cut(variable, cutValue) {}

    // pass cut implementation
    bool Pass() const { return m_variable->Value() < m_cutValue; }
    
  };

  
  // constructor
  Branch(Node * input, const std::string & variableName, float cutValue, bool isGreater, float sumSource, float sumTarget);

  // destructor
  ~Branch();

  // get input node
  const Node * InputNode () const;
  
  // get output node
  const Node * OutputNode () const;

  // set output node
  void SetOutputNode(const Node * node);

  // get variable name
  const std::string & VariableName() const ;

  // selection
  bool Pass() const;

  // get cut
  const Cut * CutObject() const;

  // get sum of events source
  float SumSource() const;

  // get sum of events target
  float SumTarget() const;
  
  
private:
  
  // input node
  const Node * m_input;
  const Node * m_output;
  
  // cut object
  const Cut * m_cut;

  // sum of events
  const float m_sumSource;
  const float m_sumTarget;
  
  // logger
  mutable Log m_log;

};

#endif
