// Macro for plotting distributions of weights
// Usage:
//
// root -l -b -q 'Plot5.C+("../files/data_1.root", "source", "BDTWeight", "Boosted Decision Trees", 50, 0, 2.2, 50, 0, 13)'
//

// ROOT includes
#include "TFile.h"
#include "TTree.h"
#include "TH2F.h"
#include "TCanvas.h"
#include "TLegend.h"
#include "TString.h"
#include "TPad.h"
#include "TLatex.h"

// STL includes
#include <iostream>
#include <sys/stat.h>



void Plot5(TString inFile, TString sourceName, TString weightName, TString label, int nBinsX, float xMin, float xMax, int nBinsY, float yMin, float yMax)
{

  TFile * f = new TFile(inFile.Data(), "read");
  TTree * t = (TTree*)f->Get( sourceName );

  TH2F * h = new TH2F("h", "", nBinsX, xMin, xMax, nBinsY, yMin, yMax);

  TCanvas * c = new TCanvas("c","", 1200, 1000);
  c->cd();

  TString cond = "";//"EffWeight>0";//TString::Format("(%s>0)*(%s-%s)/%s", weightName.Data(), weightName.Data(), "EffWeight", "EffWeight");
  
  //t->Draw( TString::Format("(%s-%s)/%s:%s>>h", weightName.Data(), "EffWeight", "EffWeight", "EffWeight"), cond.Data() );
  t->Draw( TString::Format("%s-%s:%s>>h", weightName.Data(), "EffWeight", "EffWeight"), cond.Data() );
  
  
  TPad * p0 = new TPad("p0", "p0", 0.0, 0.0, 1.0, 1.0);
  p0->Draw();
  p0->SetTicks(1,1);
  p0->SetTopMargin(0.07);
  p0->SetBottomMargin(0.12);
  p0->SetRightMargin(0.17);
  p0->SetLeftMargin(0.12);
  
  p0->cd();
  p0->SetLogz();
  
  TH1 * frame = p0->DrawFrame(h->GetXaxis()->GetXmin(), h->GetYaxis()->GetXmin(), h->GetXaxis()->GetXmax(), h->GetYaxis()->GetXmax());
  frame->GetXaxis()->SetTitle( "Efficiency Weight" );
  frame->GetXaxis()->CenterTitle();
  frame->GetXaxis()->SetTitleSize(0.05);
  frame->GetXaxis()->SetTitleOffset(0.975);
  frame->GetXaxis()->SetLabelSize(0.05);
  frame->GetYaxis()->SetTitle( "Pred. Weight - Eff. Weight" );
  frame->GetYaxis()->CenterTitle();
  frame->GetYaxis()->SetTitleSize(0.05);
  frame->GetYaxis()->SetTitleOffset(0.975);
  frame->GetYaxis()->SetLabelSize(0.05);
  
  h->SetStats(0);
  h->GetZaxis()->SetTitle("");
  h->GetZaxis()->CenterTitle();
  h->GetZaxis()->SetTitleSize(0.05);
  h->GetZaxis()->SetTitleOffset(1.1);
  h->GetZaxis()->SetLabelSize(0.05);
  h->Draw("colz same");

  TLatex tex;
  tex.SetNDC();
  tex.SetTextFont(42);
  tex.SetTextSize(0.035);
  tex.DrawLatex(0.45, 0.87, label);
  
  gPad->RedrawAxis();
  
  TString directory = "plot5";
  const char * dirname = directory.Data();
  struct stat dir;
  char command[500];
  if (stat(dirname,&dir) != 0) {
    std::cout << "Creating directories ./" << dirname << std::endl;
    sprintf(command,"mkdir -p %s",dirname);
    int result = system(command);
  }
  
  c->SaveAs( TString::Format("%s/%s_weightMap.pdf", directory.Data(), weightName.Data()) );
  c->SaveAs( TString::Format("%s/%s_weightMap.eps", directory.Data(), weightName.Data()) );

}
  
