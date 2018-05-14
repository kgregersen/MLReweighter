//local includes
#include "Algorithm.h"
#include "BDT.h"
#include "RandomForest.h"
#include "ExtraTrees.h"
#include "Variable.h"
#include "Variables.h"
#include "Event.h"
#include "Config.h"
#include "Log.h"
#include "Method.h"
#include "HistService.h"

// stl includes
#include <vector>
#include <string>
#include <fstream>
#include <ctime>
#include <map>

// ROOT includes
#include "TFile.h"
#include "TTree.h"
#include "TH1F.h"



int main(int argc, char * argv[]) {

  // check number of arguments
  if ( argc != 2 ) {
    std::cout << "Provide 1 argument: ./bin/CalculateWeights <config-path>" << std::endl;
    return 0;
  }

  // keep track of time
  std::clock_t start = std::clock();

  // get configuration file and instantiate static config object
  std::string configpath = argv[1];
  Config::Instance(configpath.c_str());
  
  // initialize log
  Log log("CalculateWeights");
  std::string str_level;
  Config::Instance().getif<std::string>("PrintLevel", str_level);
  if (str_level.length() > 0) {
    Log::LEVEL level = Log::StringToLEVEL(str_level);
    log.SetLevel(level);
  }

  // set method
  std::string str_method;
  Config::Instance().getif<std::string>("Method", str_method);
  if (str_level.length() == 0) {
    log << Log::ERROR << "Method not specified! Syntax : 'string Method = <method-name>'. Available methods: BDT, RF, ET (see ./inc/Methods.h)." << Log::endl();
    return 0;    
  }
  Method::TYPE method = Method::Type(str_method);
  if ( method == Method::NONE ) {
    log << Log::ERROR << "Method not recognized! Syntax : 'string Method = <method-name>'. Available methods: BDT, RF, ET (see ./inc/Methods.h)." << Log::endl();
    return 0;    
  }
  
  // check learning rate if RF/ET
  if (method == Method::RF || method == Method::ET) {
    float learningRate = Config::Instance().get<float>("LearningRate");
    if (learningRate < 0.99999) {
      log << Log::ERROR << "For Random Forest and ExtraTrees, the learning rate ('float LearningRate') needs to be 1, but in the config file it is " << learningRate << Log::endl();
      return 0;
    }
  }
  
  // get input file
  const std::string & inputFileName = Config::Instance().get<std::string>("InputFileName");
  TFile * f = new TFile(inputFileName.c_str(), "read");
  if ( ! f->IsOpen() ) {
    log << Log::ERROR << "Couldn't open file : " << inputFileName << Log::endl();
    return 0;
  }
  
  // get initial tree
  const std::string & treeNameSource = Config::Instance().get<std::string>("InputTreeNameSource");
  TTree * source = static_cast<TTree *>(f->Get(treeNameSource.c_str()));
  if ( ! source ) {
    log << Log::ERROR << "Couldn't get source TTree : " << treeNameSource << Log::endl();
    return 0;
  }

  // get target tree
  const std::string & treeNameTarget = Config::Instance().get<std::string>("InputTreeNameTarget");
  TTree * target = static_cast<TTree *>(f->Get(treeNameTarget.c_str()));
  if ( ! target ) {
    log << Log::ERROR << "Couldn't get target TTree : " << treeNameTarget << Log::endl();
    return 0;
  }

  // create event object and connect TTrees
  Event::Instance().ConnectAllVariables(source);
  Event::Instance().ConnectAllVariables(target);

  // initialize variables
  Variables::Initialize();
  
  // declare/run algorithm
  Algorithm * algorithm = 0;
  if ( method == Method::BDT ) {
    algorithm = new BDT(source, target);
  }
  else if ( method == Method::RF ) {
    algorithm = new RandomForest(source, target);
  }
  else if ( method == Method::ET ) {
    algorithm = new ExtraTrees(source, target);
  }
  else {
    log << Log::ERROR << "Couldn't recognize method!" << Log::endl();
    return 0;
  }
  algorithm->Initialize();
  algorithm->Process();

  // open ouput file
  std::string outfilename = "Weights.txt";
  Config::Instance().getif<std::string>("OutputFileName", outfilename);
  std::ofstream outfile;
  outfile.open(outfilename.c_str());

  // print timestamp to file
  std::time_t now= std::time(0);
  std::tm * now_tm= std::gmtime(&now);
  char buf[200];
  std::strftime(buf, 200, "%a, %d %b %y %T %z", now_tm);
  outfile << "Time stamp : " << buf << "\n\n";

  // print variables to file
  outfile << "Variables  : ";
  const std::vector<const Variable *> & variables = Variables::Get();
  for (const Variable * var : variables) {
    outfile << var->Name() << ",";
  }
  outfile << "\n\n"; 

  // print config to file
  outfile << "ConfigFile : \n";
  Config::Instance().write(outfile);
  
  // print ML algorithm to file
  algorithm->Write(outfile);
  outfile << "\n\n# End"; 

  // close file
  outfile.close();
  
  // Save histograms in HistService (if any)
  if (HistService::Instance().GetMap().size() > 0) {
    TFile * histOut = new TFile("histOut.root", "recreate");
    std::map<std::string, TH1F *> histMap = HistService::Instance().GetMap();
    for (std::pair<const std::string, TH1F *> & histEntry : histMap) {
      TH1F * hist = histEntry.second;
      hist->SetDirectory( histOut );
    }
    histOut->Write();
  }
  
  // time spent on growing tree
  double duration = (std::clock() - start)/static_cast<double>(CLOCKS_PER_SEC);    
  
  // done!
  log << Log::INFO << "And we're done !!" << Log::endl();
  log << Log::INFO << "Total time spent  : " << duration << " sec" << Log::endl();
  return 0;
  
}
