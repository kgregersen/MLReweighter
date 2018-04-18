// local includes
#include "Config.h"
#include "Store.h"

// STL includes
//#include <algorithm>
#include <vector>
#include <string>
#include <fstream>


Config::Config(const char * filename) :
  m_store(0),
  m_log("Config")
{

  if ( ! m_store && filename ) {
    
    m_store = Store::createStore(filename);

    if ( ! m_store ) {
      m_log << Log::ERROR << "Config() : Couldn't create store! filename = " << filename << Log::endl();
      throw(0);
    }

  }

  std::string str_level;
  getif<std::string>("PrintLevel", str_level);
  if (str_level.length() > 0) {
    Log::LEVEL level = Log::StringToLEVEL(str_level);
    m_log.SetLevel(level);
  }
  
}


void Config::write(std::ofstream & file) const
{

  const std::vector<std::string> & lines = get<std::vector<std::string> >("ConfigFile");
  
  for (const std::string & line : lines) {
    file << line << "\n";
  }
  file << "\n";
  
}
