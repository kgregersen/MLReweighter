#ifndef __EVENT__
#define __EVENT__

// STL includes
#include <string>

// local includes
#include "Log.h"

// forward declarations
class TTree;
class Store;


class Event {

public:

  // singleton pattern
  static Event & Instance() {
    static Event instance;
    return instance;
  }
  
  // disable copy-constructor and assignment operator
  Event(const Event & other) = delete;
  void operator=(const Event & other)  = delete;

  // destructor
  ~Event() {}
  
  // connect all
  void ConnectAllVariables(TTree * tree, bool disableOtherBranches = true);

  // get variable (non-const)
  template <typename T>
  T & get(const std::string & key);

  // get variable (const)
  template <typename T>
  const T & get(const std::string & key) const;

  
private:

  // constructor
  Event(); 

  // map of variables
  Store * m_store;

  // logger
  mutable Log m_log;

  // store variable from tree in internal map
  template<typename T>
  void ConnectVariable(const std::string & key, TTree * tree);
       
};

#include "Event.icc"

#endif
