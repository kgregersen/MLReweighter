// Macro for plotting distributions of weights
// Usage:
//
// root -l -b -q 'Plot3.C+("../files/data_1.root", "source", "X1", "X2", "BDTWeight", "Boosted Decision Trees", 50, 0, 1, 50, 0, 1)'
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



void Plot3(TString inFile, TString sourceName, TString var1, TString var2, TString weightName, TString label, int nBinsX, float xMin, float xMax, int nBinsY, float yMin, float yMax, float zMin = -999, float zMax = -999)
{

  TFile * f = new TFile(inFile.Data(), "read");
  TTree * t = (TTree*)f->Get( sourceName );

  TH2F * hAlg_2D = new TH2F("hAlg_2D", "", nBinsX, xMin, xMax, nBinsY, yMin, yMax);
  TH2F * hEff_2D = new TH2F("hEff_2D", "", nBinsX, xMin, xMax, nBinsY, yMin, yMax);

  TCanvas * c = new TCanvas("c","", 1200, 1000);
  c->cd();

  TString cond = "";// TString::Format("*(%s>1.5)", weightName.Data());
  
  t->Draw( TString::Format("%s:%s>>hAlg_2D", var1.Data(), var2.Data()), (weightName+cond).Data() );
  t->Draw( TString::Format("%s:%s>>hEff_2D", var1.Data(), var2.Data()), "EffWeight" );
  
  TH2F * ratio_2D = new TH2F("ratio_2D", "", nBinsX, xMin, xMax, nBinsY, yMin, yMax);
  float zMinTmp = 10000;
  for (int xbin = 1; xbin <= ratio_2D->GetNbinsX(); ++xbin) {
    for (int ybin = 1; ybin <= ratio_2D->GetNbinsY(); ++ybin) {
      float eff = hEff_2D->GetBinContent(xbin, ybin); 
      if ( eff > 0 ) {
	float alg = hAlg_2D->GetBinContent(xbin, ybin);
	//float val = alg/eff;
	float val = (alg-eff)/eff;
	ratio_2D->SetBinContent(xbin, ybin, val);
	if (val < zMinTmp) zMinTmp = val;
      }
      else {
	ratio_2D->SetBinContent(xbin, ybin, -1000);
	//ratio_2D->SetBinContent(xbin, ybin, 0.00000000000000000001);
      }
    }
  }
  if (zMin == -999) ratio_2D->SetMinimum(zMinTmp);
  else ratio_2D->SetMinimum(zMin);
  if (zMax > -999) ratio_2D->SetMaximum(zMax);
  
  TPad * p0 = new TPad("p0", "p0", 0.0, 0.0, 1.0, 1.0);
  p0->Draw();
  p0->SetTicks(1,1);
  p0->SetTopMargin(0.07);
  p0->SetBottomMargin(0.12);
  p0->SetRightMargin(0.17);
  p0->SetLeftMargin(0.12);
  
  p0->cd();
  //p0->SetLogz();
  
  TH1 * frame = p0->DrawFrame(hAlg_2D->GetXaxis()->GetXmin(), hAlg_2D->GetYaxis()->GetXmin(), hAlg_2D->GetXaxis()->GetXmax(), hAlg_2D->GetYaxis()->GetXmax());
  frame->GetXaxis()->SetTitle( var1 );
  frame->GetXaxis()->CenterTitle();
  frame->GetXaxis()->SetTitleSize(0.05);
  frame->GetXaxis()->SetTitleOffset(0.975);
  frame->GetXaxis()->SetLabelSize(0.05);
  frame->GetYaxis()->SetTitle( var2 );
  frame->GetYaxis()->CenterTitle();
  frame->GetYaxis()->SetTitleSize(0.05);
  frame->GetYaxis()->SetTitleOffset(0.975);
  frame->GetYaxis()->SetLabelSize(0.05);
  
  ratio_2D->GetZaxis()->SetTitle("(pred - true) / true");
  ratio_2D->GetZaxis()->CenterTitle();
  ratio_2D->GetZaxis()->SetTitleSize(0.05);
  ratio_2D->GetZaxis()->SetTitleOffset(1.1);
  ratio_2D->GetZaxis()->SetLabelSize(0.05);
  ratio_2D->Draw("colz same");

  TLatex tex;
  tex.SetNDC();
  tex.SetTextFont(42);
  tex.SetTextSize(0.035);
  tex.DrawLatex(0.45, 0.87, label);
  
  gPad->RedrawAxis();
  
  TString directory = "plot3";
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
  
