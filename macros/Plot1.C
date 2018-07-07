// Plot training/testing against the target distribution for one algorithm
// Usage :
//
// root -l -b -q 'Plot1.C("../files/data_1.root", "../files/data_2.root", "source", "target", "BDTWeight", "Boosted Decision Trees", "X1", 50, 0.0, 1.0)'
//

// ROOT includes
#include "TFile.h"
#include "TTree.h"
#include "TH1F.h"
#include "TCanvas.h"
#include "TPad.h"
#include "TLegend.h"
#include "TLatex.h"
#include "TLine.h"

// STL includes
#include <vector>
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



void Plot1(TString inFileTrain, TString inFileTest, TString sourceName, TString targetName, TString eventWeightName, TString MLWeightName, TString label, TString variable, int nBins, float xMin, float xMax)
{

  TString errName = MLWeightName + "_err"; 

  TH1F * hS_train = 0;
  TH1F * hR_train = 0;
  TH1F * hS_test  = 0;
  TH1F * hR_test  = 0;
  TH1F * hT       = 0;
  
  TFile * fS_train = new TFile(inFileTrain.Data(), "read");
  TFile * fS_test  = new TFile(inFileTest.Data(), "read");
  TFile * fT       = new TFile(inFileTrain.Data(), "read");
  TTree * tS_train = static_cast<TTree *>(fS_train->Get( sourceName ));
  TTree * tS_test  = static_cast<TTree *>(fS_test->Get( sourceName ));
  TTree * tT       = static_cast<TTree *>(fT->Get( targetName ));

  TCanvas * c = new TCanvas("c", "c", 1200, 800);
  c->cd();

  hS_train = new TH1F("hS_train_integral", "", 1, 0,1);
  hR_train = new TH1F("hR_train_integral", "", 1, 0,1);
  tS_train->Draw( TString::Format("%s>>hS_train_integral", variable.Data()), eventWeightName.Data() );
  tS_train->Draw( TString::Format("%s>>hR_train_integral", variable.Data()), TString::Format("%s*%s", eventWeightName.Data(), MLWeightName.Data()));

  hS_test = new TH1F("hS_test_integral", "", 1, 0,1);
  hR_test = new TH1F("hR_test_integral", "", 1, 0,1);
  tS_test->Draw( TString::Format("%s>>hS_test_integral", variable.Data()), eventWeightName.Data() );
  tS_test->Draw( TString::Format("%s>>hR_test_integral", variable.Data()), TString::Format("%s*%s", eventWeightName.Data(), MLWeightName.Data()));

  hT = new TH1F("hT_integral", "", 1, 0,1);
  tT->Draw( TString::Format("%s>>hT_integral", variable.Data()), eventWeightName.Data() );

  float hS_train_integral = hS_train->Integral(0,-1);
  float hR_train_integral = hR_train->Integral(0,-1);
  float hS_test_integral  = hS_test->Integral(0,-1);
  float hR_test_integral  = hR_test->Integral(0,-1);
  float hT_integral       = hT->Integral(0,-1);
  
  hS_train = new TH1F("hS_train" + variable, "", nBins, xMin, xMax);
  hR_train = new TH1F("hR_train" + variable, "", nBins, xMin, xMax);
  hS_test  = new TH1F("hS_test"  + variable, "", nBins, xMin, xMax);
  hR_test  = new TH1F("hR_test"  + variable, "", nBins, xMin, xMax);
  hT       = new TH1F("hT_"      + variable, "", nBins, xMin, xMax);
  PrepareTH1F(hR_train , kYellow - 4 , kSolid  ,  1, 1, variable);
  PrepareTH1F(hS_train , kRed        , kDashed ,  1, 0, variable);
  PrepareTH1F(hR_test  , kAzure + 1  , kSolid  , 22, 0, variable);
  PrepareTH1F(hS_test  , kGreen + 1  , kDashed , 22, 0, variable);
  PrepareTH1F(hT       , kBlack      , kSolid  , 20, 0, variable);

  tS_train->Draw( TString::Format("%s>>%s", variable.Data(), hS_train->GetName()), eventWeightName.Data() );
  tS_test ->Draw( TString::Format("%s>>%s", variable.Data(), hS_test ->GetName()), eventWeightName.Data() );

  tS_train->Draw( TString::Format("%s>>%s", variable.Data(), hR_train->GetName()), TString::Format("%s*%s", eventWeightName.Data(), MLWeightName.Data()));
  tS_test ->Draw( TString::Format("%s>>%s", variable.Data(), hR_test ->GetName()), TString::Format("%s*%s", eventWeightName.Data(), MLWeightName.Data()));

  tT->Draw( TString::Format("%s>>%s", variable.Data(), hT->GetName()), eventWeightName.Data() );
  
  hR_train->Scale( hT_integral/hR_train_integral );
  hS_train->Scale( hT_integral/hS_train_integral );
  hR_test ->Scale( hT_integral/hR_test_integral  );
  hS_test ->Scale( hT_integral/hS_test_integral  );

  
  TH1F * ratio_train = static_cast<TH1F *>(hT->Clone());
  TH1F * ratio_test  = static_cast<TH1F *>(hT->Clone());
  // for (int xbin = 0; xbin <= ratio->GetNbinsX(); ++xbin) {
  //   ratio->SetBinError(xbin, 0);
  // }
  ratio_train->Add(hR_train, -1);
  ratio_train->Divide(hR_train);   
  ratio_test->Add(hR_test, -1);
  ratio_test->Divide(hR_test);   
  
  TH1F * err_train = new TH1F( TString::Format("%s_Err", hR_train->GetName()),"", hR_train->GetNbinsX(), hR_train->GetXaxis()->GetXmin(), hR_train->GetXaxis()->GetXmax());
  tS_train->Draw( TString::Format("%s>>%s", variable.Data(), err_train->GetName()), TString::Format("pow(%s,2)", errName.Data()) );

  TH1F * err_test = new TH1F( TString::Format("%s_Err", hR_test->GetName()),"", hR_test->GetNbinsX(), hR_test->GetXaxis()->GetXmin(), hR_test->GetXaxis()->GetXmax());
  tS_test->Draw( TString::Format("%s>>%s", variable.Data(), err_test->GetName()), TString::Format("pow(%s,2)", errName.Data()) );
  
  for (int xbin = 0; xbin <= err_train->GetNbinsX(); ++xbin) {
    hR_train->SetBinError(xbin, sqrt(pow(hR_train->GetBinError(xbin), 2) + err_train->GetBinContent(xbin)) );
    hR_test ->SetBinError(xbin, sqrt(pow(hR_test ->GetBinError(xbin), 2) + err_test ->GetBinContent(xbin)) );
  }

  // now take square root of sum of squares 
  // and do the error prop. for 
  // ratio = (d - b)/b 
  for (int xbin = 0; xbin <= hT->GetNbinsX(); ++xbin) {
    float d         = hT->GetBinContent(xbin);
    float d_e       = hT->GetBinError(xbin);
    float b_train   = hR_train->GetBinContent(xbin); 
    float b_e_train = hR_train->GetBinError(xbin);
    float b_test    = hR_test ->GetBinContent(xbin); 
    float b_e_test  = hR_test ->GetBinError(xbin);
    if (hT->GetBinContent(xbin) == 0) {
      ratio_train->SetBinContent(xbin, -999);
      ratio_test ->SetBinContent(xbin, -999);
    }
    ratio_train->SetBinError(xbin, b_train > 0 ? sqrt( pow(d_e/b_train, 2) + pow(b_e_train*d/(b_train*b_train), 2) ) : 0);
    ratio_test ->SetBinError(xbin, b_test  > 0 ? sqrt( pow(d_e/b_test, 2)  + pow(b_e_test*d/(b_test*b_test)   , 2) ) : 0);
  }

  
  TPad * p1 = new TPad("p1", "p1", 0.0, 0.3, 1.0, 1.0);
  TPad * p2 = new TPad("p2", "p2", 0.0, 0.0, 1.0, 0.3);
  p1->Draw();
  p2->Draw();
  
  p1->SetTicks(1,1);
  p1->SetTopMargin(0.05);
  p1->SetBottomMargin(0.02);
  p1->SetRightMargin(0.08);
  
  p2->SetTicks(1,1);
  p2->SetBottomMargin(0.3);
  p2->SetTopMargin(0.05);
  p2->SetRightMargin(0.08);
  
  p1->cd();
  
  TH1 * frame1 = p1->DrawFrame(hT->GetXaxis()->GetXmin(), 0.05, hT->GetXaxis()->GetXmax(), hT->GetMaximum()*2.);
  frame1->GetYaxis()->SetTitle("Number of events");
  frame1->GetYaxis()->CenterTitle();
  frame1->GetYaxis()->SetTitleSize(0.05);
  frame1->GetYaxis()->SetTitleOffset(0.975);
  frame1->GetXaxis()->SetLabelSize(0.);
  frame1->GetYaxis()->SetLabelSize(0.05);
  
  hT->SetMarkerStyle(20);
  hT->SetMarkerColor(kBlack);
  hT->SetMarkerSize();
  hT->SetMarkerStyle(20);
  
  hR_train->Draw("histsame");
  hR_test ->DrawCopy("histsame");
  hR_test ->Draw("hist p same");
  hS_test ->DrawCopy("histsame");
  hS_test ->Draw("hist p same");
  hS_train->Draw("histsame");
  hT->Draw("psame");
    
  TLegend leg(0.6, 0.62, 0.8, 0.89);
  leg.SetFillColor(0);
  leg.SetLineColor(0);
  leg.SetTextSize(0.045);
  leg.AddEntry(hT      , "Target"                    , "pe");
  leg.AddEntry(hR_train, "Prediction (train)"        , "f" );
  leg.AddEntry(hR_test , "Prediction (test)"         , "lp");
  leg.AddEntry(hS_train, "Source (train, normalised)", "l");
  leg.AddEntry(hS_test , "Source (test, normalised)" , "lp");
  leg.Draw("same");
    
  TLatex tex;
  tex.SetNDC();
  tex.SetTextFont(42);
  tex.SetTextSize(0.04);
  tex.DrawLatex(0.16, 0.80, label);
  
  gPad->RedrawAxis();
  
  p2->cd();
  
  //TH1 * frame2 = p2->DrawFrame(hT->GetXaxis()->GetXmin(), -1.49, hT->GetXaxis()->GetXmax(), 01.49);
  //TH1 * frame2 = p2->DrawFrame(hT->GetXaxis()->GetXmin(), -0.99, hT->GetXaxis()->GetXmax(), 0.99);
  //TH1 * frame2 = p2->DrawFrame(hT->GetXaxis()->GetXmin(), -0.79, hT->GetXaxis()->GetXmax(), 0.79);
  //TH1 * frame2 = p2->DrawFrame(hT->GetXaxis()->GetXmin(), -0.69, hT->GetXaxis()->GetXmax(), 0.69);
  //TH1 * frame2 = p2->DrawFrame(hT->GetXaxis()->GetXmin(), -0.49, hT->GetXaxis()->GetXmax(), 0.49);
  TH1 * frame2 = p2->DrawFrame(hT->GetXaxis()->GetXmin(), -0.19, hT->GetXaxis()->GetXmax(), 0.19);
  //TH1 * frame2 = p2->DrawFrame(hT->GetXaxis()->GetXmin(), -0.049, hT->GetXaxis()->GetXmax(), 0.049);
  frame2->GetXaxis()->SetTitle( hT->GetXaxis()->GetTitle() );
  frame2->GetXaxis()->CenterTitle();
  frame2->GetYaxis()->SetTitle("(target - pred)/pred");
  frame2->GetYaxis()->CenterTitle();
  frame2->GetXaxis()->SetTitleSize(0.11);
  frame2->GetXaxis()->SetTitleOffset(1.15);
  frame2->GetXaxis()->SetLabelSize(0.12);
  frame2->GetXaxis()->SetTickLength(0.08);
  frame2->GetYaxis()->SetTitleSize(0.09);
  frame2->GetYaxis()->SetTitleOffset(0.5);
  frame2->GetYaxis()->SetLabelSize(0.11);
  frame2->GetYaxis()->SetTickLength(0.02);
  frame2->GetYaxis()->SetNdivisions(505);
  
  ratio_train->SetLineWidth(1);
  ratio_train->SetLineColor(kBlack);
  ratio_train->SetMarkerColor(kBlack);
  ratio_train->SetMarkerStyle(20);
  ratio_train->Draw("samepe");

  ratio_test->SetLineWidth(1);
  ratio_test->SetLineColor( hR_test->GetLineColor() );
  ratio_test->SetMarkerColor( hR_test->GetMarkerColor() );
  ratio_test->SetMarkerStyle( hR_test->GetMarkerStyle() );
  ratio_test->Draw("samepe");

  TLine line(hT->GetXaxis()->GetXmin(), 0.0, hT->GetXaxis()->GetXmax(), 0.0);
  line.SetLineColor(kBlack);
  line.SetLineStyle(kDashed);
  line.Draw("same");

  float chi2_train = 0;
  float chi2_test  = 0;
  int ndf_train    = 0;
  int ndf_test     = 0;
  for (int i = 1; i <= ratio_train->GetNbinsX(); ++i) {
    float r_train   = ratio_train->GetBinContent(i);
    float r_train_e = ratio_train->GetBinError(i);
    if ( r_train > 0. && r_train_e > 0.) {
      chi2_train += std::pow((r_train-0.)/r_train_e, 2.);
      ndf_train += 1;
    }
    float r_test   = ratio_test->GetBinContent(i);
    float r_test_e = ratio_test->GetBinError(i);
    if ( r_test > 0. && r_test_e > 0.) {
      chi2_test += std::pow((r_test-0.)/r_test_e, 2.);
      ndf_test += 1;
    }
  }
  tex.SetTextSize(0.1);
  tex.DrawLatex(0.30,0.77, TString::Format("#chi^{2}/ndf_{train} = %0.1f/%d = %0.1f", chi2_train, ndf_train, chi2_train/ndf_train));
  tex.DrawLatex(0.53,0.77, TString::Format("#chi^{2}/ndf_{test} = %0.1f/%d = %0.1f", chi2_test, ndf_test, chi2_test/ndf_test));
  
  gPad->RedrawAxis();
  
  TString directory = "plot1";
  const char * dirname = directory.Data();
  struct stat dir;
  char command[500];
  if (stat(dirname,&dir) != 0) {
    std::cout << "Creating directories ./" << dirname << std::endl;
    sprintf(command,"mkdir -p %s",dirname);
    int result = system(command);
  }
  
  c->SaveAs( TString::Format("%s/%s_%s__LinScale.pdf", directory.Data(), MLWeightName.Data(), variable.Data()) );
  c->SaveAs( TString::Format("%s/%s_%s__LinScale.eps", directory.Data(), MLWeightName.Data(), variable.Data()) );
  
  frame1->SetMaximum(hT->GetMaximum()*5000.);
  p1->SetLogy();
  c->SaveAs( TString::Format("%s/%s_%s__LogScale.pdf", directory.Data(), MLWeightName.Data(), variable.Data()) );
  c->SaveAs( TString::Format("%s/%s_%s__LogScale.eps", directory.Data(), MLWeightName.Data(), variable.Data()) );
    
}
  
