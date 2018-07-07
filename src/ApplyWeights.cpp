//local includes
#include "Algorithm.h"
#include "Forest.h"
#include "BDT.h"
#include "RandomForest.h"
#include "ExtraTrees.h"
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



int main(int argc, char * argv[]) {

  // check number of arguments
  if ( argc != 2 ) {
    std::cout << "Provide 1 argument: ./bin/ApplyWeights <config-path>" << std::endl;
    return 0;
  }

  // get confiuration file
  std::string configpath = argv[1];
  Config::Instance(configpath.c_str());

  // initialize log
  Log log("ApplyWeights");
  std::string str_level;
  Config::Instance().getif<std::string>("PrintLevel", str_level);
  if (str_level.length() > 0) {
    Log::LEVEL level = Log::StringToLEVEL(str_level);
    log.SetLevel(level);
  }

  // initialize variables (needs to be done before declaring the algorithm)
  Variables::Initialize();

  // initialize algorithm
  std::string str_method;
  Config::Instance().getif<std::string>("Method", str_method);
  if (str_level.length() == 0) {
    log << Log::ERROR << "Method not specified! Syntax : 'string Method = <method-name>'. Available methods: BDT, RF, ET (see ./inc/Methods.h)." << Log::endl();
    return 0;    
  }
  Method::TYPE method = Method::Type(str_method);

  // declare/run algorithm
  std::string weightsFileName = Config::Instance().get<std::string>("WeightsFileName");
  Algorithm * algorithm = 0;
  if ( method == Method::BDT ) {
    algorithm = new BDT( Forest::ReadForests(weightsFileName) );
  }
  else if ( method == Method::RF ) {
    algorithm = new RandomForest( Forest::ReadForests(weightsFileName) );
  }
  else if ( method == Method::ET ) {
    algorithm = new ExtraTrees( Forest::ReadForests(weightsFileName) );
  }
  else {
    log << Log::ERROR << "Couldn't recognize method!" << Log::endl();
    return 0;
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

  // create event object and connect TTrees
  Event & event = Event::Instance();
  event.ConnectAllVariables(source, false);

  // ML event weight
  float weight;
  std::string weightName = "Weight";
  Config::Instance().getif<std::string>("WeightName", weightName);
  TBranch * b_weight = source->Branch(weightName.c_str(), &weight);

  // ML event weight error
  float weight_err;
  std::string weightErrName = weightName + "_err";
  TBranch * b_weight_err = 0;
  b_weight_err = source->Branch(weightErrName.c_str(), &weight_err);
  
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

    // get weight/error
    algorithm->GetWeight(weight, weight_err);
    
    // fill output branches
    b_weight->Fill();
    b_weight_err->Fill();
    
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
