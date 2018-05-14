#ifndef __HISTSERVICE__
#define __HISTSERVICE__

// local includes
#include "Log.h"

// stl includes
#include <string>
#include <map>

// forward declarations
class TH1F;


class HistService {

public:

  // singleton pattern
  static HistService & Instance()
  {
    static HistService instance;
    return instance;  
  }

  // disable copy-constructor and assignment operator
  HistService(const HistService & other) = delete;
  void operator=(const HistService & other) = delete;

  // add histogram
  void AddHist(std::string name, int nbins, float xmin, float xmax);

  // get histogram
  TH1F * GetHist(const std::string & name);

  // get map of histograms
  std::map<std::string, TH1F *> GetMap();
  
  
private:

  // constructor
  HistService();

  // internal map of histograms
  std::map<std::string, TH1F *> m_map;

    // logger
  mutable Log m_log;
  
};


#endif
