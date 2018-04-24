//local includes
#include "Forest.h"
#include "DecisionTree.h"
#include "Variable.h"
#include "Variables.h"
#include "Event.h"
#include "Config.h"
#include "Log.h"
#include "Method.h"

// stl includes
#include <vector>
#include <string>
#include <fstream>
#include <ctime>
#include <algorithm>
#include <utility>

// ROOT includes
#include "TFile.h"
#include "TTree.h"
#include "TH2F.h"



int main(int argc, char * argv[]) {

  // check number of arguments
  if ( argc != 2 ) {
    std::cout << "Provide 1 argument: ./bin/WeightDiagnostics <config-path>" << std::endl;
    return 0;
  }

  // get confiuration file
  std::string configpath = argv[1];
  Config::Instance(configpath.c_str());

  // initialize log
  Log log("WeightDiagnostics");
  std::string str_level;
  Config::Instance().getif<std::string>("PrintLevel", str_level);
  if (str_level.length() > 0) {
    Log::LEVEL level = Log::StringToLEVEL(str_level);
    log.SetLevel(level);
  }

  // initialize variables (needs to be done before declaring the algorithm)
  Variables::Initialize();

  // open weights file
  std::string weightsfilename = Config::Instance().get<std::string>("WeightsFileName");
  std::ifstream weightsfile;
  log << Log::INFO << "Opening file " << weightsfilename << Log::endl();
  weightsfile.open(weightsfilename.c_str());
 
  // declare/run algorithm
  std::string weightsFileName = Config::Instance().get<std::string>("WeightsFileName");
  const std::vector<const Forest *> forests = Forest::ReadForests(weightsFileName);
  
  // open input file in 'update' mode
  const std::string & inputfilename = Config::Instance().get<std::string>("InputFileName");
  TFile * f = new TFile(inputfilename.c_str(), "update");
  if ( ! f->IsOpen() ) {
    log << Log::ERROR << "Couldn't open file : " << inputfilename << Log::endl();
    return 0;
  }
  
  // get source tree
  const std::string & treenamesource = Config::Instance().get<std::string>("InputTreeNameSource");
  TTree * source = static_cast<TTree *>(f->Get(treenamesource.c_str()));
  if ( ! source ) {
    log << Log::ERROR << "Couldn't get TTree : " << treenamesource << Log::endl();
    return 0;
  }

  // create event object and connect TTrees
  Event & event = Event::Instance();
  event.ConnectAllVariables(source, false);

  // histograms for weight diagnostics
  int nMaxTrees = 0;
  for (unsigned int i = 0; i < forests.size(); ++i) {
    int nTrees = (forests.at(i)->GetTrees()).size();
    if (nTrees > nMaxTrees) {
      nMaxTrees = nTrees;
    }
  }

  // prepare output
  std::string str_method;
  Config::Instance().getif<std::string>("Method", str_method);
  if (str_level.length() == 0) {
    log << Log::ERROR << "Method not specified! Syntax : 'string Method = <method-name>'. Available methods: BDT, RF, ET (see ./inc/Methods.h)." << Log::endl();
    return 0;    
  }
  TFile * outFile = new TFile(TString::Format("WeightDiagnostics_%s.root", str_method.c_str()), "recreate");
  outFile->cd();
  TH2F * WeightDiagnostics = new TH2F("WeightDiagnostics", "WeightDiagnostics", nMaxTrees, 0, nMaxTrees, 200, 0.5, 1.5);

  // prepare for loop over tree entries
  long maxEvent = source->GetEntries();
  long reportFrac = maxEvent/(maxEvent > 100000 ? 100 : 1) + 1;
  log << Log::INFO << "Looping over events (" << source->GetName() << ") : "  << maxEvent << Log::endl();
  std::clock_t start = std::clock();

  // Loop over ree entries
  for (long ievent = 0; ievent < maxEvent; ++ievent) {

    // print progress
    if( ievent > 0 && ievent % reportFrac == 0 ) {
      double duration     = (std::clock() - start)/static_cast<double>(CLOCKS_PER_SEC);    
      double frequency    = static_cast<double>(ievent) / duration;
      double timeEstimate = static_cast<double>(maxEvent - ievent) / frequency;
      log << Log::INFO << "---> processed : " << std::setw(8) << 100*ievent/maxEvent << "\%  ---  frequency : " << std::setw(7) << static_cast<int>(frequency) << " events/sec  ---  time : " << std::setw(4) << static_cast<int>(duration) << " sec  ---  remaining time : " << std::setw(4) << static_cast<int>(timeEstimate) << " sec"<< Log::endl(); 
    }
    
    // get event
    source->GetEntry( ievent );

    // weight diagnostics  
    for (int index = 0; index < nMaxTrees; ++index) {
      for (unsigned int f = 0; f < forests.size(); ++f) {
	const std::vector<const DecisionTree *> trees = forests.at(f)->GetTrees();
	int nTrees = trees.size();
	if (index >= nTrees) continue;
	WeightDiagnostics->Fill(index, trees.at(index)->GetWeight());
      }
    }
    
  }

  // print out
  double duration  = (std::clock() - start)/static_cast<double>(CLOCKS_PER_SEC);    
  double frequency = static_cast<double>(maxEvent) / duration;
  log << Log::INFO << "---> processed : " << std::setw(8) << 100 << "\%  ---  frequency : " << std::setw(7) << static_cast<int>(frequency) << " events/sec  ---  time : " << std::setw(4) << static_cast<int>(duration) << " sec  ---  remaining time :    0 sec"<< Log::endl(); 

  // write to file
  outFile->Write();

  
  // and we're done!
  return 0;
  
}
