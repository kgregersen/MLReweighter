
// ROOT includes
#include "TFile.h"
#include "TH2F.h"
#include "TH1F.h"
#include "TCanvas.h"
#include "TPad.h"
#include "TLine.h"
#include "TLatex.h"
#include "TGaxis.h"

// STL includes
#include <iostream>
#include <sys/stat.h>


void Plot6(TString inFile, TString histName, TString label, float minMean = 0.5, float maxMean = 1.5, float minStdDev = 0, float maxStdDev = 1)
{

  TFile * f = new TFile(inFile.Data(), "read");
  TH2F * h = static_cast<TH2F*>(f->Get(histName.Data()));

  int nbinsX = h->GetNbinsX();
  TH1F * hMean   = new TH1F("hMean"  , "",nbinsX, 0, nbinsX);
  TH1F * hStdDev = new TH1F("hStdDev", "",nbinsX, 0, nbinsX);

  for (int i = 0; i <= nbinsX; ++i) {
    hMean  ->SetBinContent( i , h->ProjectionY("proj",i,i)->GetMean()   );
    hStdDev->SetBinContent( i , h->ProjectionY("proj",i,i)->GetStdDev() );
  }

  TCanvas * c = new TCanvas("c","", 1200, 800);
  c->cd();
   
  TPad * p0 = new TPad("p0", "p0", 0.0, 0.0, 1.0, 1.0);
  p0->Draw();
  p0->SetTicks(1,0);
  p0->SetTopMargin(0.07);
  p0->SetBottomMargin(0.12);
  p0->SetRightMargin(0.12);
  p0->SetLeftMargin(0.12);
  //p0->SetLogy();
  
  p0->cd();
  
  TH1 * frame1 = p0->DrawFrame(0, minMean, nbinsX, maxMean);

  frame1->GetYaxis()->SetTitle("Mean Event Weight");
  frame1->GetYaxis()->CenterTitle();
  frame1->GetYaxis()->SetTitleSize(0.05);
  frame1->GetYaxis()->SetTitleOffset(1.05);
  frame1->GetYaxis()->SetLabelSize(0.05);

  frame1->GetXaxis()->SetTitle("Tree Number");
  frame1->GetXaxis()->CenterTitle();
  frame1->GetXaxis()->SetTitleSize(0.05);
  frame1->GetXaxis()->SetTitleOffset(0.975);
  frame1->GetXaxis()->SetLabelSize(0.05);

  hMean->SetLineColor(kBlack);
  hMean->Draw("histsame");
  
  
  for (int i = 1; i <= nbinsX; ++i) {
    hStdDev->AddBinContent(i, -minStdDev);
  }
  hStdDev->Scale( (maxMean - minMean)/(maxStdDev - minStdDev) );
  for (int i = 1; i <= nbinsX; ++i) {
    hStdDev->AddBinContent(i, minMean);
  }
  hStdDev->SetLineColor(kRed);
  hStdDev->Draw("histsame");

  TLine line(hMean->GetXaxis()->GetXmin(), 1.0, hMean->GetXaxis()->GetXmax(), 1.0);
  line.SetLineColor(kBlack);
  line.SetLineStyle(kDashed);
  line.Draw("same");

  TLatex tex;
  tex.SetNDC();
  tex.SetTextFont(42);
  tex.SetTextSize(0.035);
  tex.DrawLatex(0.18, 0.85, label);

  gPad->RedrawAxis();

  //draw an axis on the right side
  TGaxis *axis = new TGaxis(nbinsX, minMean, nbinsX, maxMean, minStdDev, maxStdDev, 510, "+L");
  axis->SetLineColor(kRed);
  axis->SetLabelColor(kRed);
  axis->SetTitleColor(kRed);
  axis->SetTitleFont( frame1->GetYaxis()->GetTitleFont() );
  axis->SetTitleSize( frame1->GetYaxis()->GetTitleSize() );
  axis->CenterTitle();
  axis->SetTitle("Standard Deviation of Event Weights");
  axis->Draw();
  
  
  TString directory = "plot6";
  const char * dirname = directory.Data();
  struct stat dir;
  char command[500];
  if (stat(dirname,&dir) != 0) {
    std::cout << "Creating directories ./" << dirname << std::endl;
    sprintf(command,"mkdir -p %s",dirname);
    int result = system(command);
  }

  c->SaveAs( TString::Format("%s/%s.pdf", directory.Data(), histName.Data()) );
  c->SaveAs( TString::Format("%s/%s.eps", directory.Data(), histName.Data()) );

}

 
