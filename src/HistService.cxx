// local includes
#include "HistService.h"
#include "Config.h"

// ROOT includes
#include "TH1F.h"

HistService::HistService() :
  m_log("HistService")
{

  std::string str_level;
  Config::Instance().getif<std::string>("PrintLevel", str_level);
  if (str_level.length() > 0) {
    Log::LEVEL level = Log::StringToLEVEL(str_level);
    m_log.SetLevel(level);
  }

}


void HistService::AddHist(std::string name, int nbins, float xmin, float xmax)
{

  // check if already exists
  std::map<std::string, TH1F *>::iterator itr = m_map.find( name );
  if (itr != m_map.end() ) {
    m_log << Log::ERROR << "Histogram with name = " << name << " already exists in map!" << Log::endl();
    throw(0);
  }

  // add histogram to map
  TH1F * hist = new TH1F(name.c_str(), name.c_str(), nbins, xmin, xmax);
  hist->SetDirectory(0);
  m_map[name] = hist;
  
}


TH1F * HistService::GetHist(const std::string & name)
{

  // check if already exists
  std::map<std::string, TH1F *>::iterator itr = m_map.find( name );
  if (itr == m_map.end() ) {
    m_log << Log::ERROR << "Couldn't find histogram with name = " << name << Log::endl();
    return 0;
  }

  // return hist
  return itr->second;
  
}


std::map<std::string, TH1F *> HistService::GetMap()
{

  // return map of histograms
  return m_map;
  
}
