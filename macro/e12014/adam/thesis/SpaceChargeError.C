#ifndef __CLING__

#include "AtArrayAug.h"
#include "AtEvent.h"
#include "AtPSADeconv.h"
#include "AtPadArray.h"
#include "AtPadFFT.h"
#include "AtPadReference.h"
#include "AtPatternEvent.h"
#include "AtRadialChargeModel.h"
#include "AtRawEvent.h"
#include "AtTpcMap.h"

#include <TCanvas.h>
#include <TChain.h>
#include <TCutG.h>
#include <TF1.h>
#include <TFile.h>
#include <TGraph.h>
#include <TGraph2D.h>
#include <TGraph2DErrors.h>
#include <TH1.h>
#include <THStack.h>
#include <TRandom.h>
#include <TRootEmbeddedCanvas.h>
#include <TString.h>
#include <TSystem.h>
#include <TTreeReader.h>

#include <fstream>
#include <string>
#include <vector>
#endif

double lineField(double r, double z)
{
   double lambda = 5.28e-8;               // SI
   constexpr double eps = 8.85418782E-12; // SI
   constexpr double pi = 3.14159265358979;
   constexpr double eps2pi = 2 * pi * eps;
   r /= 100.;                        // Convert units from cm to m
   auto field = lambda / eps2pi / r; // v/m
   return field / 100.;              // V/cm
}

void SpaceChargeError()
{
   auto SCModel = std::make_unique<AtRadialChargeModel>(&lineField);
}
