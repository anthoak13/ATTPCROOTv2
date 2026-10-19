namespace Response {

double response[512] = {};
double scaling = 1.0;
// AtPulse passes the time since the charge arrived in us, and response[] is sampled once per time bucket.
// false (the old behaviour) indexes response[] by the time in us, which stretches the response by
// 1 / (TB width in us), 3.1 times for 320 ns buckets. true indexes it by time bucket, as AtVectorResponse does.
bool timeInTB = false;
double tbTime = 0.32; // Width of a time bucket in us, used when timeInTB

void Init(TString sharedInfoDir)
{
   AtRawEvent *respAvgEvent;
   TFile *f1 = new TFile(sharedInfoDir + "respAvg.root");
   f1->GetObject("avgResp", respAvgEvent);
   f1->Close();

   auto pad = respAvgEvent->GetPad(0);
   for (int i = 0; i < 512; i++) {
      response[i] = pad->GetADC(i);
   }
}

double GetResponse(int padNum, double time)
{
   int idx = timeInTB ? (int)(time / tbTime) : (int)time;
   if (idx < 0 || idx >= 512)
      return 0;
   return scaling * response[idx];
}

} // namespace Response