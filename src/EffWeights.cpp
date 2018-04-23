//local includes
#include "Variable.h"
#include "Variables.h"
#include "Event.h"
#include "Config.h"
#include "Log.h"

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
#include "TFormula.h"



int main(int argc, char * argv[]) {

  // check number of arguments
  if ( argc != 2 ) {
    std::cout << "Provide 1 argument: ./bin/EffWeights <config-path>" << std::endl;
    return 0;
  }

  // get confiuration file
  std::string configpath = argv[1];
  Config::Instance(configpath.c_str());

  // initialize log
  Log log("EffWeights");
  std::string str_level;
  Config::Instance().getif<std::string>("PrintLevel", str_level);
  if (str_level.length() > 0) {
    Log::LEVEL level = Log::StringToLEVEL(str_level);
    log.SetLevel(level);
  }

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

  // initialize variables (needs to be done before declaring the algorithm)
  Variables::Initialize();

  // read efficiency function from config and set variables
  const std::string & formula = Config::Instance().get<std::string>("EfficiencyFunction");  
  TFormula effFunc;
  effFunc.SetName("effFunc"); 
  for (const Variable * var : Variables::Get()) {
    effFunc.AddVariable( (var->Name()).c_str() );
  }
  effFunc.Compile( formula.c_str() );
  
  // create event object and connect TTrees
  Event & event = Event::Instance();
  event.ConnectAllVariables(source, false);

  // ML event weight
  float weight;
  std::string weightName = "Weight";
  Config::Instance().getif<std::string>("WeightName", weightName);
  TBranch * b_weight = source->Branch(weightName.c_str(), &weight);

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

    // get weight
    for (const Variable * var : Variables::Get()) {
      effFunc.SetVariable((var->Name()).c_str(), var->Value());
    }
    weight = effFunc.EvalPar(0);
    
    // fill output branches
    b_weight->Fill();
    
  }

  // print out
  double duration  = (std::clock() - start)/static_cast<double>(CLOCKS_PER_SEC);    
  double frequency = static_cast<double>(maxEvent) / duration;
  log << Log::INFO << "---> processed : " << std::setw(8) << 100 << "\%  ---  frequency : " << std::setw(7) << static_cast<int>(frequency) << " events/sec  ---  time : " << std::setw(4) << static_cast<int>(duration) << " sec  ---  remaining time :    0 sec"<< Log::endl(); 

  // write tree
  source->Write();  
  
  // and we're done!
  return 0;
  
}
