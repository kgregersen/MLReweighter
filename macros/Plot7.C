// Macro for plotting distributions of weights
// Usage:
//
// root -l -b -q 'Plot2.C+("../files/data_1.root", "source", 25, 2.5)'
//

// ROOT includes
#include "TFile.h"
#include "TTree.h"
#include "TH1F.h"
#include "TCanvas.h"
#include "TLegend.h"
#include "TString.h"
#include "TPad.h"
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



void Plot7(TString inFile, TString sourceName, TString eventWeightName, int nBins = 100, float xmax = 3)
{

  TString BDTw1 = "BDTWeight_10Trees";
  TString BDTw2 = "BDTWeight_20Trees";
  TString BDTw3 = "BDTWeight_30Trees";
  TString BDTw4 = "BDTWeight_50Trees";
  TString BDTw5 = "BDTWeight_100Trees";
  TString Effw = "EffWeight";
  
  float ymin = 5.e-4;
  
  TFile * f = new TFile(inFile.Data(), "read");
  TTree * t = (TTree*)f->Get( sourceName );

  TH1F * hBDT1 = new TH1F("hBDT1", "", nBins, 0.00001, xmax);
  TH1F * hBDT2 = new TH1F("hBDT2", "", nBins, 0.00001, xmax);
  TH1F * hBDT3 = new TH1F("hBDT3", "", nBins, 0.00001, xmax);
  TH1F * hBDT4 = new TH1F("hBDT4", "", nBins, 0.00001, xmax);
  TH1F * hBDT5 = new TH1F("hBDT5", "", nBins, 0.00001, xmax);
  TH1F * hEff  = new TH1F("hEff"      , "", nBins, 0.00001, xmax);
  PrepareTH1F(hBDT1, kGreen + 1, kSolid , 21, 0, "Weight");
  PrepareTH1F(hBDT2, kMagenta  , kSolid , 25, 0, "Weight");
  PrepareTH1F(hBDT3, kMagenta  , kSolid , 22, 0, "Weight");
  PrepareTH1F(hBDT4, kAzure + 1, kSolid , 25, 0, "Weight");
  PrepareTH1F(hBDT5, kRed      , kSolid , 20, 0, "Weight");
  PrepareTH1F(hEff , kBlack    , kSolid ,  0, 0, "Weight");
  
  TCanvas * c = new TCanvas("c","", 1200, 800);
  c->cd();
  c->SetLogy();

  t->Draw(TString::Format("%s*%s>>hBDT1", eventWeightName.Data(), BDTw1.Data()));
  t->Draw(TString::Format("%s*%s>>hBDT2", eventWeightName.Data(), BDTw2.Data()));
  t->Draw(TString::Format("%s*%s>>hBDT3", eventWeightName.Data(), BDTw3.Data()));
  t->Draw(TString::Format("%s*%s>>hBDT4", eventWeightName.Data(), BDTw4.Data()));
  t->Draw(TString::Format("%s*%s>>hBDT5", eventWeightName.Data(), BDTw5.Data()));

  t->Draw(TString::Format("%s*%s>>hEff", eventWeightName.Data(), Effw.Data()));

  TH1F * ratio_BDT1 = static_cast<TH1F *>(hBDT1->Clone());
  ratio_BDT1->Add(hEff, -1);
  ratio_BDT1->Divide(hEff);   

  TH1F * ratio_BDT2 = static_cast<TH1F *>(hBDT2->Clone());
  ratio_BDT2->Add(hEff, -1);
  ratio_BDT2->Divide(hEff);   

  TH1F * ratio_BDT3 = static_cast<TH1F *>(hBDT3->Clone());
  ratio_BDT3->Add(hEff, -1);
  ratio_BDT3->Divide(hEff);   

  TH1F * ratio_BDT4 = static_cast<TH1F *>(hBDT4->Clone());
  ratio_BDT4->Add(hEff, -1);
  ratio_BDT4->Divide(hEff);   

  TH1F * ratio_BDT5 = static_cast<TH1F *>(hBDT5->Clone());
  ratio_BDT5->Add(hEff, -1);
  ratio_BDT5->Divide(hEff);   

  hBDT1->Scale( 1./hBDT1->Integral() );
  hBDT2->Scale( 1./hBDT2->Integral() );
  hBDT3->Scale( 1./hBDT3->Integral() );
  hBDT4->Scale( 1./hBDT4->Integral() );
  hBDT5->Scale( 1./hBDT5->Integral() );
  hEff->Scale( 1./hEff->Integral() );

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
  
  TH1 * frame1 = p1->DrawFrame(hEff->GetXaxis()->GetXmin(), ymin, hEff->GetXaxis()->GetXmax(), hEff->GetMaximum()*2.7);
  frame1->GetYaxis()->SetTitle("Probaility Density");
  frame1->GetYaxis()->CenterTitle();
  frame1->GetYaxis()->SetTitleSize(0.05);
  frame1->GetYaxis()->SetTitleOffset(0.975);
  frame1->GetXaxis()->SetLabelSize(0.);
  frame1->GetYaxis()->SetLabelSize(0.05);
  
  hBDT1->DrawCopy("histsame");
  hBDT1->Draw("psame");
  // hBDT2->DrawCopy("histsame");
  // hBDT2->Draw("psame");
  //hBDT3->DrawCopy("histsame");
  //hBDT3->Draw("psame");
  hBDT4->DrawCopy("histsame");
  hBDT4->Draw("psame");
  hBDT5->DrawCopy("histsame");
  hBDT5->Draw("psame");
  hEff->Draw("histsame");
  
  TLegend l(0.55,0.6,0.85,0.87);
  l.SetLineColor(0);
  l.SetTextSize(0.04);
  l.AddEntry(hBDT1, "Boosted Decision Trees (10 trees)", "lp");
  //  l.AddEntry(hBDT2, "Boosted Decision Trees (20 trees)", "lp");
  //l.AddEntry(hBDT3, "Boosted Decision Trees (30 trees)", "lp");
  l.AddEntry(hBDT4, "Boosted Decision Trees (50 trees)", "lp");
  l.AddEntry(hBDT5, "Boosted Decision Trees (100 trees)", "lp");
  l.AddEntry(hEff , "Efficiency Function"              , "l");
  l.Draw();

  gPad->RedrawAxis();
  
  p2->cd();
  
  //TH1 * frame2 = p2->DrawFrame(hBDT->GetXaxis()->GetXmin(), -1.49, hBDT->GetXaxis()->GetXmax(), 01.49);
  //TH1 * frame2 = p2->DrawFrame(hBDT->GetXaxis()->GetXmin(), -0.99, hBDT->GetXaxis()->GetXmax(), 0.99);
  //TH1 * frame2 = p2->DrawFrame(hBDT->GetXaxis()->GetXmin(), -0.79, hBDT->GetXaxis()->GetXmax(), 0.79);
  TH1 * frame2 = p2->DrawFrame(hEff->GetXaxis()->GetXmin(), -0.69, hEff->GetXaxis()->GetXmax(), 0.69);
  //TH1 * frame2 = p2->DrawFrame(hEff->GetXaxis()->GetXmin(), -0.49, hEff->GetXaxis()->GetXmax(), 0.49);
  //TH1 * frame2 = p2->DrawFrame(hEff->GetXaxis()->GetXmin(), -0.39, hEff->GetXaxis()->GetXmax(), 0.39);
  //TH1 * frame2 = p2->DrawFrame(hBDT->GetXaxis()->GetXmin(), -0.049, hBDT->GetXaxis()->GetXmax(), 0.049);
  frame2->GetXaxis()->SetTitle( hBDT1->GetXaxis()->GetTitle() );
  frame2->GetXaxis()->CenterTitle();
  frame2->GetYaxis()->SetTitle("(pred - eff) / eff");
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
  
  ratio_BDT1->DrawCopy("histsame");
  ratio_BDT1->Draw("psame");
  //ratio_BDT2->DrawCopy("histsame");
  //ratio_BDT2->Draw("psame");
  //ratio_BDT3->DrawCopy("histsame");
  //ratio_BDT3->Draw("psame");
  ratio_BDT4->DrawCopy("histsame");
  ratio_BDT4->Draw("psame");
  ratio_BDT5->DrawCopy("histsame");
  ratio_BDT5->Draw("psame");

  TLine line(hEff->GetXaxis()->GetXmin(), 0.0, hEff->GetXaxis()->GetXmax(), 0.0);
  line.SetLineColor(kBlack);
  line.SetLineStyle(kDashed);
  line.Draw("same");

  // float chi2_train = 0;
  // float chi2_test  = 0;
  // int ndf_train    = 0;
  // int ndf_test     = 0;
  // for (int i = 1; i <= ratio_train->GetNbinsX(); ++i) {
  //   float r_train   = ratio_train->GetBinContent(i);
  //   float r_train_e = ratio_train->GetBinError(i);
  //   if ( r_train > 0. && r_train_e > 0.) {
  //     chi2_train += std::pow((r_train-0.)/r_train_e, 2.);
  //     ndf_train += 1;
  //   }
  //   float r_test   = ratio_test->GetBinContent(i);
  //   float r_test_e = ratio_test->GetBinError(i);
  //   if ( r_test > 0. && r_test_e > 0.) {
  //     chi2_test += std::pow((r_test-0.)/r_test_e, 2.);
  //     ndf_test += 1;
  //   }
  // }
  // tex.SetTextSize(0.1);
  // tex.DrawLatex(0.30,0.77, TString::Format("#chi^{2}/ndf_{train} = %0.1f/%d = %0.1f", chi2_train, ndf_train, chi2_train/ndf_train));
  // tex.DrawLatex(0.53,0.77, TString::Format("#chi^{2}/ndf_{test} = %0.1f/%d = %0.1f", chi2_test, ndf_test, chi2_test/ndf_test));
  
  gPad->RedrawAxis();
  
  TString directory = "plot7";
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
  
