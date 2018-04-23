// This macro can be used to split the data into training and testing samples.
// Usage :
//
// > root -l -b -q 'SplitTree.C+("../files/data_orig.root", "source", "target")'
// > root -l -b -q 'SplitTree.C+("../files/data.root", "source", "target")'
//

// ROOT includes
#include "TFile.h"
#include "TTree.h"
#include "TString.h"

// stl includes
#include <vector>
#include <string>
#include <iostream>


void SplitSource(TString inFile, TString sourceName, TString targetName) 
{
  
  TFile * input = new TFile(inFile, "read");
  
  TTree * tree_S = static_cast<TTree *>(input->Get( sourceName ));
  long nEvents_S = tree_S->GetEntries();
  long subEvents_S = nEvents_S/2.;

  TTree * tree_T = static_cast<TTree *>(input->Get( targetName ));
  long nEvents_T = tree_T->GetEntries();

  std::vector<TFile *> outFiles(2, 0);
  std::vector<TTree *> outTrees_S(2, 0);
  std::vector<TTree *> outTrees_T(2, 0);

  std::string outName = inFile.Data();
  outName.erase(outName.size() - 5);
  
  for (int i = 0; i < 2; ++i) {
    outFiles.at(i) = new TFile(TString::Format("./%s_%d.root", outName.c_str(), i + 1), "recreate");
    outTrees_S.at(i) = tree_S->CloneTree(0);
    outTrees_T.at(i) = tree_T->CloneTree();
  }
  
  for (long iEvent = 0; iEvent < nEvents_S; ++iEvent) {
    int index = 0;
    tree_S->GetEntry(iEvent);   
    if (iEvent % 2 == 0) index = 0;
    else index = 1;
    outTrees_S.at(index)->Fill();
  }

  for (int i = 0; i < 2; ++i) {
    std::cout << "Writing file : " << outFiles.at(i)->GetName() << "   events :  target = " << outTrees_T.at(i)->GetEntries() << "  source = " << outTrees_S.at(i)->GetEntries() << " / " << tree_S->GetEntries() << std::endl;
    outFiles.at(i)->Write();
  }

}
