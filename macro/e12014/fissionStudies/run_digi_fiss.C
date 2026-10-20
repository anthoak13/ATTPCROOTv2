// Code to take MC tracks and digitize
// #include "eventCombine.h"
#include "DigiSimInfo.h"
#include "RunConfig.h"

#include <random>

bool reduceFunc(AtRawEvent *evt);

void run_digi_fiss(TString cfgFile = "", bool saveRawEvent = true)
{
   RunConfig cfg(cfgFile);
   // 0: don't write the traces (about 12 MB per event); no later stage reads them
   saveRawEvent = cfg.GetInt("digi.saveTraces", saveRawEvent);

   delete gRandom;
   gRandom = new TRandom3;
   gRandom->SetSeed(cfg.GetInt("digi.seed", std::random_device{}() & 0x7fffffff));

   auto verbSpec =
      fair::VerbositySpec::Make(fair::VerbositySpec::Info::severity, fair::VerbositySpec::Info::file_line_function);
   fair::Logger::DefineVerbosity("user1", verbSpec);
   fair::Logger::SetVerbosity("user1");

   TString mcFile = cfg.File("input", "sim");
   TString outputFile = cfg.File("output", "digi");
   TString scriptfile = "e12014_pad_map_size.xml";
   TString paramFile = "ATTPC.e12014.par";

   TString dir = getenv("VMCWORKDIR");
   TString sharedInfoDir = RunConfig::SharedInfo() + "/";

   // Create the full parameter file paths
   TString digiParFile = dir + "/parameters/" + paramFile;
   TString mapParFile = dir + "/scripts/" + scriptfile;

   // -----   Timer   --------------------------------------------------------
   TStopwatch timer;

   // ------------------------------------------------------------------------

   // __ Run ____________________________________________
   FairRunAna *fRun = new FairRunAna();
   FairFileSource *source = new FairFileSource(mcFile);
   fRun->SetSource(source);
   fRun->SetOutputFile(outputFile);

   FairRuntimeDb *rtdb = fRun->GetRuntimeDb();
   FairParAsciiFileIo *parIo1 = new FairParAsciiFileIo();
   parIo1->open(digiParFile.Data(), "in");
   rtdb->setFirstInput(parIo1);

   // Create the detector map to pass to the simulation
   auto mapping = std::make_shared<AtTpcMap>();
   mapping->ParseXMLMap(mapParFile.Data());
   mapping->GeneratePadPlane();

   InhibitZapPads(*mapping); // Same pads the fit skips

   // Dead pads: no charge. The fit is not told; it compares charge only on pads with a hit.
   auto deadPads = DeadPads(*mapping, cfg.GetStr("digi.deadAsAd", ""), cfg.GetInt("digi.deadPads", 0),
                            cfg.GetInt("digi.deadPatch", 1), cfg.GetInt("digi.deadSeed", 1));
   for (int pad : deadPads)
      mapping->InhibitPad(pad, AtMap::InhibitType::kTotal);
   std::cout << "Made " << deadPads.size() << " pads dead" << std::endl;

   // __ AT digi tasks___________________________________
   // AtClusterizeTask *clusterizer = new AtClusterizeTask(std::make_shared<AtClusterize>());
   AtClusterizeTask *clusterizer = new AtClusterizeTask(std::make_shared<AtClusterizeLine>());
   clusterizer->SetPersistence(false);

   // AtPulseLineTask *pulse = new AtPulseLineTask();

   // AtPulseTask *pulse = new AtPulseTask(std::make_shared<AtPulse>(mapping));
   AtRawEvent *respAvgEvent;
   TFile *f2 = new TFile(sharedInfoDir + "respAvg.root");
   f2->GetObject("avgResp", respAvgEvent);
   f2->Close();
   auto respPad = respAvgEvent->GetPad(0);
   // The measured response, one sample per 320 ns time bucket
   ElectronicResponse::AtVectorResponse response(0.32, {respPad->GetADC().begin(), respPad->GetADC().end()}); // us
   response.SetScale(cfg.Get("digi.respScale", 0.01 * 0.75));
   auto pulse = std::make_shared<AtPulseLine>(mapping, response);
   pulse->SetSaveCharge(true);
   pulse->SetLowGain(cfg.Get("digi.lowGain", 0.19));
   pulse->SetNumIntegrationPoints(cfg.GetInt("digi.intPoints", 1000)); // Points sampling each line's diffusion
   pulse->SetNoiseSigma(cfg.Get("digi.noise", 0)); // Gaussian noise added to every sample of a trace (ADC)
   AtPulseTask *pulseTask = new AtPulseTask(pulse);
   pulseTask->SetPersistence(saveRawEvent);

   /*AtMacroTask *combTask = new AtMacroTask();
   combTask->AddInitFunction(eventComb::init);
   combTask->AddFunction(eventComb::combine);*/

   /**** PSA Task ****/
   auto threshold = 50;
   auto maxThreshold = 1500;

   auto psa = std::make_unique<AtPSADeconvFit>();
   psa->SetResponse(*respAvgEvent);
   psa->SetThreshold(15); // Threshold in charge units
   psa->SetFilterOrder(6);
   psa->SetCutoffFreq(75);
   psa->SetThreshold(10);
   auto psaBeam = std::make_unique<AtPSATBAvg>();
   psaBeam->SetThreshold(45);
   auto psaComp = std::make_unique<AtPSAComposite>(std::move(psaBeam), std::move(psa), 20);
   AtPSAtask *psaTask = new AtPSAtask(std::move(psaComp));
   psaTask->SetInputBranch("AtRawEvent");
   psaTask->SetOutputBranch("AtEvent");
   psaTask->SetPersistence(true);

   /**** Space charge correction, as on the experimental data ****/
   // The hits are corrected with the line-charge model; the Y-pattern is found in the corrected hits, and the fit
   // is told lambda through the AtFissionEvent. Without the correction, lambda is 0.
   bool scCorrection = cfg.GetInt("digi.scCorrection", 0);
   double scLambda = scCorrection ? cfg.Get("digi.lambda", kNominalLambda) : 0;
   AtSpaceChargeCorrectionTask *scTask = nullptr;
   if (scCorrection) {
      auto scModel = std::make_unique<AtLineChargeModel>();
      scModel->SetLambda(scLambda);
      scModel->SetBeamRadius(kBeamRadius / 1000); // m
      SetScBeam(*scModel, cfg.GetStr("digi.scBeam", "0 -6 10 0"));
      scTask = new AtSpaceChargeCorrectionTask(std::move(scModel));
      scTask->SetInputBranch("AtEvent");
      scTask->SetOutputBranch("AtEventCorr");
      scTask->SetPersistence(false);
   }

   /**** Y-pattern fit ****/
   auto method = std::make_unique<SampleConsensus::AtSampleConsensus>(
      SampleConsensus::Estimators::kYRANSAC, AtPatterns::PatternType::kFission, RandomSample::SampleMethod::kY);
   method->SetDistanceThreshold(20);
   method->SetNumIterations(500);
   method->SetMinHitsPattern(50);
   method->SetChargeThreshold(10); //-1 implies no charge-weighted fitting
   method->SetFitPattern(true);
   auto sacTask = new AtSampleConsensusTask(std::move(method));
   sacTask->SetPersistence(false);
   sacTask->SetInputBranch(scCorrection ? "AtEventCorr" : "AtEvent");
   sacTask->SetOutputBranch("AtPatternEvent");

   /******** Create fission task ********/
   AtFissionTask *fissionTask = new AtFissionTask(scLambda);
   fissionTask->SetUncorrectedEventBranch("AtEvent");
   fissionTask->SetPatternBranch("AtPatternEvent");
   fissionTask->SetOutBranch("AtFissionEvent");
   fissionTask->SetPersistance(true);

   /**** Gas of the pseudo-data ****/
   // The electrons drift and diffuse with these, but the PSA (and the fit) reconstruct with the parameter file's.
   // So the parameters are changed only while the clusterizer and the pulse initialize.
   double vdScale = cfg.Get("digi.driftVelocityScale", 1);
   double coefTScale = cfg.Get("digi.coefTScale", 1);
   double coefLScale = cfg.Get("digi.coefLScale", 1);
   auto gasPar = [] {
      return dynamic_cast<AtDigiPar *>(FairRunAna::Instance()->GetRuntimeDb()->getContainer("AtDigiPar"));
   };
   AtMacroTask *gasTask = new AtMacroTask();
   gasTask->AddInitFunction([=] {
      auto par = gasPar();
      par->SetDriftVelocity(par->GetDriftVelocity() * vdScale);
      par->SetCoefDiffusionTrans(par->GetCoefDiffusionTrans() * coefTScale);
      par->SetCoefDiffusionLong(par->GetCoefDiffusionLong() * coefLScale);
   });
   AtMacroTask *gasRestoreTask = new AtMacroTask();
   gasRestoreTask->AddInitFunction([=] {
      auto par = gasPar();
      par->SetDriftVelocity(par->GetDriftVelocity() / vdScale);
      par->SetCoefDiffusionTrans(par->GetCoefDiffusionTrans() / coefTScale);
      par->SetCoefDiffusionLong(par->GetCoefDiffusionLong() / coefLScale);
   });

   /**** Pad gains ****/
   // Each pad's trace is multiplied by a fixed factor (pad, AGET and AsAd spreads), before the PSA.
   double padGainSigma = cfg.Get("digi.padGainSigma", 0);
   double agetGainSigma = cfg.Get("digi.agetGainSigma", 0);
   double asadGainSigma = cfg.Get("digi.asadGainSigma", 0);
   int gainSeed = cfg.GetInt("digi.gainSeed", 1);
   AtMacroTask *gainTask = nullptr;
   if (padGainSigma > 0 || agetGainSigma > 0 || asadGainSigma > 0) {
      auto gains = std::make_shared<std::unordered_map<int, double>>(
         PadGains(*mapping, padGainSigma, agetGainSigma, asadGainSigma, gainSeed));
      auto rawArray = std::make_shared<TClonesArray *>(nullptr);
      gainTask = new AtMacroTask();
      gainTask->AddInitFunction([rawArray] {
         *rawArray = dynamic_cast<TClonesArray *>(FairRootManager::Instance()->GetObject("AtRawEvent"));
      });
      gainTask->AddFunction([rawArray, gains] {
         auto event = dynamic_cast<AtRawEvent *>((*rawArray)->At(0));
         for (auto &pad : event->GetPads()) {
            double gain = gains->at(pad->GetPadNum());
            for (int i = 0; i < (int)pad->GetADC().size(); ++i)
               pad->SetADC(i, pad->GetADC(i) * gain);
         }
      });
   }

   AtMacroTask *infoTask = new AtMacroTask();
   infoTask->AddInitFunction([] { digiSimInfo::Forward("Info", "SimInfo"); });

   fRun->AddTask(gasTask);     // The pseudo-data's gas, for the clusterizer and the pulse
   fRun->AddTask(clusterizer); // This task turns energy into electons in the gas
   fRun->AddTask(pulseTask);   // This task turns electorns in gas to traces on the pad plabe
   fRun->AddTask(gasRestoreTask);
   if (gainTask)
      fRun->AddTask(gainTask); // Pad gains
   fRun->AddTask(psaTask);     // This turns waveforms on pads into hits in space
   if (scTask)
      fRun->AddTask(scTask);   // Space charge correction
   fRun->AddTask(sacTask);     // This task finds the fisison Y-pattern
   fRun->AddTask(fissionTask); // This task assembles the fisison events
   fRun->AddTask(infoTask);    //

   cfg.CheckUnused({"sim."}); // The sim's settings, passed on to describe the input
   fRun->Init();

   timer.Start();
   fRun->Run(); // All events in the input file
   cfg.Finish(outputFile);
   timer.Stop();

   std::cout << std::endl << std::endl;
   std::cout << "Macro finished succesfully." << std::endl << std::endl;
   // -----   Finish   -------------------------------------------------------

   Double_t rtime = timer.RealTime();
   Double_t ctime = timer.CpuTime();
   cout << endl;
   cout << "Real time " << rtime << " s, CPU time " << ctime << " s" << endl;
   cout << endl;
   // ------------------------------------------------------------------------
}

bool reduceFunc(AtRawEvent *evt)
{
   /*if(evt->GetEventID() % 2 == 0)
      return false;
   return (evt->GetNumPads() > 0) && evt->IsGood();*/
   return true;
}
