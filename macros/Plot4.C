// Macro for plotting distributions of weights
// Usage:
//
// root -l -b -q 'Plot4.C+("../files/data_1.root", "source", 35, -1, 2.5)'
//

// ROOT includes
#include "TFile.h"
#include "TTree.h"
#include "TH1F.h"
#include "TCanvas.h"
#include "TLegend.h"
#include "TString.h"
#include "TPad.h"
#include "TLatex.h"
#include "TLine.h"

// STL includes
#include <iostream>
#include <sys/stat.h>



void PrepareTH1F(TH1F * h,  int color, int linestyle, int markerstyle, int fill, TString xaxisTitle)
{

  if ( ! h ) return;

  h->Sumw2();
  
  h->SetStats(0);
  h->GetYaxis()->SetTitleOffset(1.15);
  h->GetYaxis()->CenterTitle();
  h->GetXaxis()->SetTitle(xaxisTitle);
  h->GetXaxis()->CenterTitle();
  h->SetLineColor(color);
  h->SetLineStyle(linestyle);
  h->SetLineWidth(1);
  h->SetMarkerStyle(markerstyle);
  h->SetMarkerColor(color);
  if ( fill ) {
    h->SetLineColor(kBlack);
    h->SetFillColor(color);
  }

}



void Plot4(TString inFile, TString sourceName, TString eventWeightName, int nBins = 100, float xMin = -1, float xMax = 1)
{

  TString BDTw = "BDTWeight";
  TString RFw  = "RFWeight";
  TString ETw  = "ETWeight";
  TString Effw = "EffWeight";
  
  TFile * f = new TFile(inFile.Data(), "read");
  TTree * t = (TTree*)f->Get( sourceName );

  TH1F * hBDT = new TH1F("hBDT", "", nBins, xMin, xMax);
  TH1F * hRF  = new TH1F("hRF" , "", nBins, xMin, xMax);
  TH1F * hET  = new TH1F("hET" , "", nBins, xMin, xMax);
  PrepareTH1F(hBDT, kRed      , kSolid , 20, 0, "Weight - True weight");
  PrepareTH1F(hRF , kAzure + 1, kSolid , 22, 0, "Weight - True weight");
  PrepareTH1F(hET , kGreen + 1, kSolid , 21, 0, "Weight - True weight");
  
  TCanvas * c = new TCanvas("c","", 1200, 800);
  c->cd();

  t->Draw(BDTw + "-" + Effw + ">>hBDT", eventWeightName.Data());
  t->Draw(RFw + "-" + Effw  + ">>hRF" , eventWeightName.Data());
  t->Draw(ETw + "-" + Effw  + ">>hET" , eventWeightName.Data());

  hBDT->Scale( 1./hBDT->Integral() );
  hRF ->Scale( 1./hRF ->Integral() );
  hET ->Scale( 1./hET ->Integral() );

  float yMin = 100000;
  for (int i = 1; i <= nBins; ++i) {
    if (hBDT->GetBinContent(i) > 0 && hBDT->GetBinContent(i) < yMin) yMin = hBDT->GetBinContent(i);
    if (hRF->GetBinContent(i)  > 0 && hRF->GetBinContent(i)  < yMin) yMin = hRF->GetBinContent(i);
    if (hET->GetBinContent(i)  > 0 && hET->GetBinContent(i)  < yMin) yMin = hET->GetBinContent(i);
  }
   
  TPad * p0 = new TPad("p0", "p0", 0.0, 0.0, 1.0, 1.0);
  p0->Draw();
  p0->SetTicks(1,1);
  p0->SetTopMargin(0.07);
  p0->SetBottomMargin(0.12);
  p0->SetRightMargin(0.07);
  p0->SetLeftMargin(0.12);
  p0->SetLogy();
  
  p0->cd();
  
  TH1 * frame1 = p0->DrawFrame(hET->GetXaxis()->GetXmin(), yMin, hET->GetXaxis()->GetXmax(), hET->GetMaximum()*500);

  frame1->GetYaxis()->SetTitle("Probability Density Function");
  frame1->GetYaxis()->CenterTitle();
  frame1->GetYaxis()->SetTitleSize(0.05);
  frame1->GetYaxis()->SetTitleOffset(1.05);
  frame1->GetYaxis()->SetLabelSize(0.05);

  frame1->GetXaxis()->SetTitle("Predicted Weight - True Weight");
  frame1->GetXaxis()->CenterTitle();
  frame1->GetXaxis()->SetTitleSize(0.05);
  frame1->GetXaxis()->SetTitleOffset(0.975);
  frame1->GetXaxis()->SetLabelSize(0.05);
  
  hBDT->DrawCopy("histsame");
  hBDT->Draw("psame");
  hRF->DrawCopy("histsame");
  hRF->Draw("psame");
  hET->DrawCopy("histsame");
  hET->Draw("psame");
  
  TLegend l(0.15,0.70,0.45,0.89);
  l.SetLineColor(0);
  l.SetTextSize(0.04);
  l.AddEntry(hBDT, "Boosted Decision Trees "    , "lp");
  l.AddEntry(hRF , "Random Forest"              , "lp");
  l.AddEntry(hET , "Extremely Randomised Trees" , "lp");
  l.Draw();

  TLatex tex;
  tex.SetNDC();
  tex.SetTextFont(42);
  tex.SetTextSize(0.035);
  tex.DrawLatex(0.59, 0.845, TString::Format("#mu = %0.3f", hBDT->GetMean()));
  tex.DrawLatex(0.71, 0.845, TString::Format("#sigma = %0.3f", hBDT->GetStdDev()));
  tex.DrawLatex(0.59, 0.780, TString::Format("#mu = %0.3f", hRF->GetMean()));
  tex.DrawLatex(0.71, 0.780, TString::Format("#sigma = %0.3f", hRF->GetStdDev()));
  tex.DrawLatex(0.59, 0.717, TString::Format("#mu = %0.3f", hET->GetMean()));
  tex.DrawLatex(0.71, 0.717, TString::Format("#sigma = %0.3f", hET->GetStdDev()));

  TLine line(hBDT->GetXaxis()->GetXmin(), 0.0, hBDT->GetXaxis()->GetXmax(), 0.0);
  line.SetLineColor(kBlack);
  line.SetLineStyle(kDashed);
  line.Draw("same");
  
  gPad->RedrawAxis();
  
  TString directory = "plot4";
  const char * dirname = directory.Data();
  struct stat dir;
  char command[500];
  if (stat(dirname,&dir) != 0) {
    std::cout << "Creating directories ./" << dirname << std::endl;
    sprintf(command,"mkdir -p %s",dirname);
    int result = system(command);
  }
  
  c->SaveAs( TString::Format("%s/weights.pdf", directory.Data()) );
  c->SaveAs( TString::Format("%s/weights.eps", directory.Data()) );

}
  
