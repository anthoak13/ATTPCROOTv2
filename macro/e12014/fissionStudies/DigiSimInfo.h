namespace digiSimInfo {

/// Copy the simulation truth histogram from the input tree to the output tree.
void Forward(TString inName, TString outName)
{
   FairRootManager *ioManager = FairRootManager::Instance();
   if (ioManager == nullptr) {
      LOG(error) << "Cannot find RootManager!" << std::endl;
      return;
   }

   auto infoBranch = dynamic_cast<TH1D *>(ioManager->GetObject(inName));
   if (infoBranch == nullptr) {
      cout << inName << " branch is nullptr!" << endl;
      return;
   }

   ioManager->Register(outName, "AtTPC", infoBranch, true);
}

} // namespace digiSimInfo
