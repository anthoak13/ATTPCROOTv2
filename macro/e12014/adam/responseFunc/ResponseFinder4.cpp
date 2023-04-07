#ifndef __CLING__

#include <TGClient.h>
#include <TCanvas.h>
#include <TFile.h>
#include <TF1.h>
#include <TH1.h>
#include <TGraph.h>
#include <TGraph2D.h>
#include <TGraph2DErrors.h>
#include <TChain.h>
#include <TRandom.h>
#include <TGButton.h>
#include <TGFrame.h>
#include <TGTab.h>
#include <TGNumberEntry.h>
#include <TGLabel.h>
#include <TChain.h>
#include <TCutG.h>
#include <TRootEmbeddedCanvas.h>
#include <TString.h>
#include <TTreeReader.h>
#include <TSystem.h>
#include <RQ_OBJECT.h>

#include "AtRawEvent.h"
#include "AtEvent.h"
#include "AtPatternEvent.h"
#include "AtTpcMap.h"
#include "AtPadReference.h"
#include "AtPSADeconv.h"
#include "AtPadArray.h"
#include "AtPadFFT.h"

#include <fstream>
#include <string>
#include <vector>
#endif

using namespace std;

void ResponseFinder4()
{
   cout << "starting macro" << endl;
   TChain *runChain = new TChain("cbmsim");

   string chainFileName = "/mnt/analysis/e12014/huntc/newFork/ATTPCROOTv2/code/unpacking/data/pulser/run_0036.root";
   cout << "Opening file " << chainFileName << endl;
   runChain->Add(chainFileName.c_str());

   TTreeReader *reader = new TTreeReader(runChain);
   TTreeReaderValue<TClonesArray> *rawEventReader = new TTreeReaderValue<TClonesArray>(*reader, "FilledData");

   AtTpcMap *tpcMap = new AtTpcMap();
   tpcMap->ParseXMLMap(TString(gSystem->Getenv("VMCWORKDIR")) + "/scripts/e12014_pad_map_size.xml");
   tpcMap->AddAuxPad({10, 0, 0, 0}, "MCP_US");
   tpcMap->AddAuxPad({10, 0, 0, 34}, "TPC_Mesh");
   tpcMap->AddAuxPad({10, 0, 1, 0}, "MCP_DS");
   tpcMap->AddAuxPad({10, 0, 2, 34}, "IC");

   int counts[10] = {};

   double Cin = 100e-15;
   double Vin = 600e-3;

   AtPadReference padRef;

   int fpnChan[4] = {11, 22, 45, 56};

   cout << "making RawEvent" << endl;

   AtRawEvent *response = new AtRawEvent();
   AtRawEvent *response2 = new AtRawEvent();
   AtRawEvent *response3 = new AtRawEvent();
   AtRawEvent *response4 = new AtRawEvent();
   for (int cobo = 0; cobo < 10; cobo++) {
      padRef.cobo = cobo;
      for (int asad = 0; asad < 4; asad++) {
         padRef.asad = asad;
         for (int aget = 0; aget < 4; aget++) {
            padRef.aget = aget;
            for (int ch = 0; ch < 68; ch++) {
               padRef.ch = ch;
               if (ch != 11 && ch != 22 && ch != 45 && ch != 56) {
                  int padNum = tpcMap->GetPadNum(padRef);
                  AtPad *pad = new AtPad(padNum);
                  pad->AddAugment("variance", make_unique<AtPadArray>());
                  response->AddPad(*pad);
                  AtPad *pad2 = new AtPad(padNum);
                  pad2->AddAugment("variance", make_unique<AtPadArray>());
                  response2->AddPad(*pad2);
                  AtPad *pad3 = new AtPad(padNum);
                  pad3->AddAugment("variance", make_unique<AtPadArray>());
                  response3->AddPad(*pad3);
                  AtPad *pad4 = new AtPad(padNum);
                  pad4->AddAugment("variance", make_unique<AtPadArray>());
                  response4->AddPad(*pad4);
               }
            }
         }
      }
   }

   cout << "RawEvent made" << endl;

   int events = runChain->GetEntries();
   events = 10;

   cout << "entries gotten" << endl;

   TH1D *hist = new TH1D("hist", "hist", 512, 0, 512);
   TH1D *hist2 = new TH1D("hist2", "hist2", 512, 0, 512);

   for (int event = 0; event < events; event++) {
      cout << "processing event number " << event << endl;

      reader->SetEntry(event);
      AtRawEvent *rawEventPtr = dynamic_cast<AtRawEvent *>((*rawEventReader)->At(0));

      // cout << "rawEventPtr gotten" << endl;

      for (int cobo = 0; cobo < 1; cobo++) {
         padRef.cobo = cobo;
         int fpnCount = 0;
         for (int asad = 0; asad < 4; asad++) {
            padRef.asad = asad;
            for (int aget = 0; aget < 4; aget++) {
               padRef.aget = aget;
               double FPN_avg[512] = {};
               double FPN_max = 0;
               double Q_avg[512] = {};

               // cout << cobo << " " << asad << " " << aget << endl;

               for (int i = 0; i < 4; i++) {
                  padRef.ch = fpnChan[i];
                  auto *fpnPad = rawEventPtr->GetFpn(padRef);
                  if (fpnPad != nullptr) {
                     for (int r = 0; r < 512; r++) {
                        FPN_avg[r] += fpnPad->GetRawADC(r) / 4.;
                        // cout << fpnPad->GetRawADC(r);
                        if (i == 3 && FPN_avg[r] > FPN_max) {
                           FPN_max = FPN_avg[r];
                        }
                     }
                  }
               }

               // cout << "FPN_max: " << FPN_max << endl;

               double FPN_PeakVal = 0;
               int FPN_PeakChan = 0;
               double FPN_MinVal = 0;
               int FPN_MinChan = 0;

               for (int i = 5; i < 512; i++) {
                  double FPN_deriv = (FPN_avg[i] - FPN_avg[i - 1]);
                  Q_avg[i] = Cin * Vin / FPN_max * FPN_deriv;
                  if (Q_avg[i] > FPN_PeakVal) {
                     FPN_PeakVal = Q_avg[i];
                     FPN_PeakChan = i;
                  }
                  if (Q_avg[i] < FPN_MinVal) {
                     FPN_MinVal = Q_avg[i];
                     FPN_MinChan = i;
                  }
               }
               // cout << "FPN_PeakChan: " << FPN_PeakChan << endl;
               if (FPN_PeakChan > 0 && fpnCount == 0) {
                  counts[cobo]++;
                  fpnCount++;
               }

               // cout << "done with FPNS!" << endl;

               if (FPN_PeakChan < 10) {
                  // cout << "PeakChan too low!" << endl;
               } else {

                  for (int ch = 0; ch < 68; ch++) {
                     hist->Clear();
                     hist2->Clear();
                     padRef.ch = ch;
                     auto pad = rawEventPtr->GetPad(tpcMap->GetPadNum(padRef));
                     auto acPad = response->GetPad(tpcMap->GetPadNum(padRef));
                     auto acPad2 = response2->GetPad(tpcMap->GetPadNum(padRef));
                     auto acPad3 = response3->GetPad(tpcMap->GetPadNum(padRef));
                     auto acPad4 = response4->GetPad(tpcMap->GetPadNum(padRef));
                     double corrResp[512] = {};
                     double corrResp0[512] = {};
                     vector<double> value;
                     vector<double> timeBuk;
                     // cout << "pad " << cobo * 1000000 + asad * 10000 + aget * 100 + ch << endl;
                     // cout << "pad num " << tpcMap->GetPadNum(padRef) << endl;
                     if (pad == nullptr) {
                        // cout << "null pointer at " << cobo * 1000000 + asad * 10000 + aget * 100 + ch << endl;
                     } else {
                        for (int i = FPN_PeakChan - 10; i < 512; i++) {
                           double curVal0;
                           if (i < FPN_MinChan - 10) {
                              curVal0 = pad->GetADC(i);
                           } else {
                              curVal0 = pad->GetADC(i) + corrResp[i - FPN_MinChan + 10];
                           }
                           corrResp0[i - FPN_PeakChan + 10] = curVal0;
                           if (i < FPN_MinChan - 10 - 15) {
                              value.push_back(pad->GetADC(i));
                              timeBuk.push_back(i);
                           } else if (i > FPN_MinChan - 10 + 15 &&
                                      !(i > FPN_MinChan - 10 + (FPN_MinChan - FPN_PeakChan) - 15 &&
                                        i < FPN_MinChan - 10 + (FPN_MinChan - FPN_PeakChan) + 15)) {
                              value.push_back(pad->GetADC(i) + corrResp0[i - FPN_MinChan + 10]);
                              timeBuk.push_back(i);
                           }
                        }
                        for (int i = 0; i < 5; i++) {
                           value.pop_back();
                           timeBuk.pop_back();
                        }
                        value.push_back(value[value.size() - 1]);
                        timeBuk.push_back(511);
                        TGraph *grph = new TGraph(value.size(), &timeBuk[0], &value[0]);
                        TF1 *fit = new TF1("fit", "pol4", 30, 512);
                        grph->Fit("fit", "RQ");
                        for (int i = 0; i < value.size() - 1; i++) {
                           hist->SetBinContent(timeBuk[i] + 1, value[i]);
                           hist2->SetBinContent(timeBuk[i] + 1, value[i]);
                        }
                        for (int i = 0; i < 512; i++) {
                           if (hist->GetBinContent(i + 1) == 0) {
                              hist->SetBinContent(i + 1, fit->Eval(i));
                           }
                           if (i > 30) {
                              hist2->SetBinContent(i + 1, fit->Eval(i));
                           }
                        }
                        fit->Delete();
                        grph->Delete();
                        // cout << "i: " << i << endl;
                        // cout << "i - FPN_PeakChan + 10: " << i - FPN_PeakChan + 10 << endl;
                        // if (pad->GetADC(i) > 0) {
                        for (int i = FPN_PeakChan - 10; i < 512; i++) {
                           if (1) {
                              double curVal = 0;
                              double curVal2 = 0;
                              double curVal3 = hist->GetBinContent(i + 1);
                              double curVal4 = hist2->GetBinContent(i + 1);
                              curVal = pad->GetADC(i) / FPN_PeakVal;
                              if (i < FPN_MinChan - 10) {
                                 curVal2 = pad->GetADC(i) / FPN_PeakVal;
                              } else {
                                 curVal2 = pad->GetADC(i) / FPN_PeakVal + corrResp[i - FPN_MinChan + 10];
                              }
                              corrResp[i - FPN_PeakChan + 10] = curVal2;

                              auto prevVal = acPad->GetADC(i - FPN_PeakChan + 10);
                              auto delta = curVal - prevVal / (counts[cobo] - 1);
                              auto delta2 = curVal - (prevVal + curVal) / counts[cobo];

                              auto prevVal2 = acPad2->GetADC(i - FPN_PeakChan + 10);
                              auto prevVal3 = acPad3->GetADC(i - FPN_PeakChan + 10);
                              auto prevVal4 = acPad4->GetADC(i - FPN_PeakChan + 10);

                              auto delta_2 = curVal2 - prevVal2 / (counts[cobo] - 1);
                              auto delta_3 = curVal3 - prevVal3 / (counts[cobo] - 1);
                              auto delta_4 = curVal4 - prevVal4 / (counts[cobo] - 1);

                              auto delta2_2 = curVal2 - (prevVal2 + curVal2) / counts[cobo];
                              auto delta2_3 = curVal3 - (prevVal3 + curVal3) / counts[cobo];
                              auto delta2_4 = curVal4 - (prevVal4 + curVal4) / counts[cobo];

                              auto varia = dynamic_cast<AtPadArray *>(acPad->GetAugment("variance"));
                              if (varia == nullptr) {
                                 cout << "varia is null at " << cobo * 1000000 + asad * 10000 + aget * 100 + ch << endl;
                              } else {
                                 varia->SetArray(i - FPN_PeakChan + 10,
                                                 varia->GetArray(i - FPN_PeakChan + 10) + delta * delta2);
                              }
                              auto varia2 = dynamic_cast<AtPadArray *>(acPad2->GetAugment("variance"));
                              if (varia2 == nullptr) {
                                 cout << "varia2 is null at " << cobo * 1000000 + asad * 10000 + aget * 100 + ch
                                      << endl;
                              } else {
                                 varia2->SetArray(i - FPN_PeakChan + 10,
                                                  varia2->GetArray(i - FPN_PeakChan + 10) + delta_2 * delta2_2);
                              }
                              auto varia3 = dynamic_cast<AtPadArray *>(acPad3->GetAugment("variance"));
                              if (varia3 == nullptr) {
                                 cout << "varia3 is null at " << cobo * 1000000 + asad * 10000 + aget * 100 + ch
                                      << endl;
                              } else {
                                 varia3->SetArray(i - FPN_PeakChan + 10,
                                                  varia3->GetArray(i - FPN_PeakChan + 10) + delta_3 * delta2_3);
                              }
                              auto varia4 = dynamic_cast<AtPadArray *>(acPad4->GetAugment("variance"));
                              if (varia4 == nullptr) {
                                 cout << "varia4 is null at " << cobo * 1000000 + asad * 10000 + aget * 100 + ch
                                      << endl;
                              } else {
                                 varia4->SetArray(i - FPN_PeakChan + 10,
                                                  varia4->GetArray(i - FPN_PeakChan + 10) + delta_4 * delta2_4);
                              }

                              if (acPad == nullptr) {
                                 cout << "acPad is null at " << cobo * 1000000 + asad * 10000 + aget * 100 + ch << endl;
                              } else {
                                 acPad->SetADC(i - FPN_PeakChan + 10, prevVal + curVal);
                              }
                              if (acPad2 == nullptr) {
                                 cout << "acPad2 is null at " << cobo * 1000000 + asad * 10000 + aget * 100 + ch
                                      << endl;
                              } else {
                                 acPad2->SetADC(i - FPN_PeakChan + 10, prevVal2 + curVal2);
                              }
                              if (acPad3 == nullptr) {
                                 cout << "acPad3 is null at " << cobo * 1000000 + asad * 10000 + aget * 100 + ch
                                      << endl;
                              } else {
                                 acPad3->SetADC(i - FPN_PeakChan + 10, prevVal3 + curVal3);
                              }
                              if (acPad4 == nullptr) {
                                 cout << "acPad4 is null at " << cobo * 1000000 + asad * 10000 + aget * 100 + ch
                                      << endl;
                              } else {
                                 acPad4->SetADC(i - FPN_PeakChan + 10, prevVal4 + curVal4);
                              }
                           }
                        }
                     }
                  }
               }
            }
         }
      }
   }
   TH1D *respHist = new TH1D("respHist", "respHist", 512, 0, 512);
   TH1D *respHist2 = new TH1D("respHist2", "respHist2", 512, 0, 512);
   TH1D *respHist3 = new TH1D("respHist3", "respHist3", 512, 0, 512);
   TH1D *respHist4 = new TH1D("respHist4", "respHist4", 512, 0, 512);
   cout << "making pads" << endl;
   for (int cobo = 0; cobo < 1; cobo++) {
      padRef.cobo = cobo;
      cout << "cobo " << cobo << " counts: " << counts[cobo] << endl;
      for (int asad = 0; asad < 4; asad++) {
         padRef.asad = asad;
         for (int aget = 0; aget < 4; aget++) {
            padRef.aget = aget;
            for (int ch = 0; ch < 68; ch++) {
               padRef.ch = ch;
               if (ch != 11 && ch != 22 && ch != 45 && ch != 56) {
                  if (counts[cobo] > 0) {
                     // cout << "making pad" << cobo * 1000000 + asad * 10000 + aget * 100 + ch << endl;
                     // cout << "making pad " << tpcMap->GetPadNum(padRef) << endl;
                     // cout << "pad made" << endl;
                     auto pad = response->GetPad(tpcMap->GetPadNum(padRef));
                     auto pad2 = response2->GetPad(tpcMap->GetPadNum(padRef));
                     auto pad3 = response3->GetPad(tpcMap->GetPadNum(padRef));
                     auto pad4 = response4->GetPad(tpcMap->GetPadNum(padRef));
                     for (int i = 0; i < 512; i++) {
                        pad->SetADC(i, pad->GetADC(i) / counts[cobo]);
                        pad2->SetADC(i, pad2->GetADC(i) / counts[cobo]);
                        pad3->SetADC(i, pad3->GetADC(i) / counts[cobo]);
                        pad4->SetADC(i, pad4->GetADC(i) / counts[cobo]);
                        if (cobo == 0 && asad == 0 && aget == 0 && ch == 0) {
                           respHist->SetBinContent(i + 1, pad->GetADC(i));
                           respHist2->SetBinContent(i + 1, pad2->GetADC(i));
                           respHist3->SetBinContent(i + 1, pad3->GetADC(i));
                           respHist4->SetBinContent(i + 1, pad4->GetADC(i));
                        }
                        auto varia = dynamic_cast<AtPadArray *>(pad->GetAugment("variance"));
                        if (varia == nullptr) {
                           cout << "varia is null at " << cobo * 1000000 + asad * 10000 + aget * 100 + ch << endl;
                        } else {
                           varia->SetArray(i, varia->GetArray(i) / counts[cobo]);
                        }
                        auto varia2 = dynamic_cast<AtPadArray *>(pad2->GetAugment("variance"));
                        if (varia2 == nullptr) {
                           cout << "varia2 is null at " << cobo * 1000000 + asad * 10000 + aget * 100 + ch << endl;
                        } else {
                           varia2->SetArray(i, varia2->GetArray(i) / counts[cobo]);
                        }
                        auto varia3 = dynamic_cast<AtPadArray *>(pad3->GetAugment("variance"));
                        if (varia3 == nullptr) {
                           cout << "varia3 is null at " << cobo * 1000000 + asad * 10000 + aget * 100 + ch << endl;
                        } else {
                           varia3->SetArray(i, varia3->GetArray(i) / counts[cobo]);
                        }
                        auto varia4 = dynamic_cast<AtPadArray *>(pad4->GetAugment("variance"));
                        if (varia4 == nullptr) {
                           cout << "varia4 is null at " << cobo * 1000000 + asad * 10000 + aget * 100 + ch << endl;
                        } else {
                           varia4->SetArray(i, varia4->GetArray(i) / counts[cobo]);
                        }
                     }
                     // cout << "adding pad" << cobo * 1000000 + asad * 10000 + aget * 100 + ch << "  " <<
                     // pad->GetPadNum() << endl; cout << "pad added" << endl;
                  }
               }
            }
         }
      }
   }

   respHist->Draw();
   respHist2->SetLineColor(2);
   respHist2->Draw("SAME");

   response->SetName("response");
   response2->SetName("response2");
   response3->SetName("response3");
   response4->SetName("response4");
   cout << "making root file" << endl;
   TFile *respFile = new TFile("response.root", "recreate");
   cout << "writing file" << endl;
   response->Write();
   response2->Write();
   response3->Write();
   response4->Write();
   cout << "closing file" << endl;
   respFile->Close();
}
