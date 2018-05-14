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
#include "TBranch.h"
#include "TFormula.h"



int main(int argc, char * argv[]) {

  // check number of arguments
  if ( argc != 2 ) {
    std::cout << "Provide 1 argument: ./bin/CreateTrueTarget <config-path>" << std::endl;
    return 0;
  }

  // get confiuration file
  std::string configpath = argv[1];
  Config::Instance(configpath.c_str());

  // initialize log
  Log log("CreateTrueTarget");
  std::string str_level;
  Config::Instance().getif<std::string>("PrintLevel", str_level);
  if (str_level.length() > 0) {
    Log::LEVEL level = Log::StringToLEVEL(str_level);
    log.SetLevel(level);
  }

  // open input file in 'update' mode
  const std::string & inputfilename = Config::Instance().get<std::string>("InputFileName");
  TFile * f = new TFile(inputfilename.c_str(), "read");
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

  // save new file with source and true target
  //  - make full clone of source into new file
  //  - disable weight variable in clone of source into target_true (we'll add this branch later where it will also contain the efficiency weight)
  log << Log::INFO << "Cloning source tree and writing to new file" << Log::endl();
  const std::string & outputfilename = Config::Instance().get<std::string>("OutputFileName");
  TFile * f_out = new TFile(outputfilename.c_str(), "recreate");
  TTree * source_copy = source->CloneTree();
  const std::string & eventWeightName = Config::Instance().get<std::string>("EventWeightVariableName");
  source->SetBranchStatus(eventWeightName.c_str(), 0);
  TTree * target_true = source->CloneTree();
  target_true->SetName("target_true");
  f_out->Write();

  // close everything
  log << Log::INFO << "Cleaning up" << Log::endl();
  delete source_copy;
  delete target_true;
  delete f_out;
  delete source;
  delete f;

  // Now open the file which contains both the source and target_true TTrees, and add new branch to target_true containing the eff. weight
  log << Log::INFO << "Opening new file with cloned source and adding branch with efficiency weight" << Log::endl();
  f = new TFile(outputfilename.c_str(), "update");
  source = static_cast<TTree *>(f->Get(treenamesource.c_str()));
  target_true = static_cast<TTree *>(f->Get("target_true"));
  float weight;
  TBranch * b_weight = target_true->Branch(eventWeightName.c_str(), &weight);

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
  event.ConnectAllVariables(target_true, false, false);

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

    // get event weight
    static float & evtWeight = Event::Instance().get<float>(eventWeightName);
    weight = evtWeight;
    
    // multiply with efficiency weight
    for (const Variable * var : Variables::Get()) {
      effFunc.SetVariable((var->Name()).c_str(), var->Value());
    }
    weight *= effFunc.EvalPar(0);
    
    // fill output branches
    target_true->GetEntry( ievent );
    b_weight->Fill();
    
  }

  // print out
  double duration  = (std::clock() - start)/static_cast<double>(CLOCKS_PER_SEC);    
  double frequency = static_cast<double>(maxEvent) / duration;
  log << Log::INFO << "---> processed : " << std::setw(8) << 100 << "\%  ---  frequency : " << std::setw(7) << static_cast<int>(frequency) << " events/sec  ---  time : " << std::setw(4) << static_cast<int>(duration) << " sec  ---  remaining time :    0 sec"<< Log::endl(); 

  // write tree
  target_true->Write();  
  
  // and we're done!
  return 0;
  
}
