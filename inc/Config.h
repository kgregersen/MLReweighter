#ifndef __CONFIG__
#define __CONFIG__

// STL includes
#include <string>

// local includes
#include "Log.h"

// forward declarations
class Store;


class Config {

public:

  // singleton pattern
  static Config & Instance(const char * filename = 0) {
    static Config instance(filename);
    return instance;
  }
  
  // disable copy-constructor and assignment operator
  Config(const Config & other) = delete;
  void operator=(const Config & other)  = delete;

  // destructor
  ~Config() {}
  
  // get variable
  template <typename T>
  const T & get(const std::string & name) const;

  // get variable if exists
  template <typename T>
  bool getif(const std::string & name, T & value) const;

  // write to file
  void write(std::ofstream & file) const;

  
private:

  // constructor
  Config(const char * filename); 

  // map of variables
  Store * m_store;

  // logger
  mutable Log m_log;
       
};

#include "Config.icc"

#endif
