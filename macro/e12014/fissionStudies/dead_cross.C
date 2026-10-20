/**
 * Which events' fragments cross a set of dead pads, for the dead-pad samples of the systematics study.
 *
 * One CSV row per event of the results files matching `glob` (relative to FISSION_DATA, e.g. a nominal fit's
 * fetched results): the chunk, entry, and for each fragment of the Y-pattern the number of pads with a hit and
 * how many of them are in the dead set. The dead set is made as run_digi_fiss.C makes it (DeadPads) from the
 * same settings, so the hits of the nominal digitization show where each fragment's track lies.
 *
 *    root -l -b -q 'dead_cross.C("remote/hpc-syst/syst_base-rec.fit.c*.results.root", "2 3", 111, 1, 1, "x.csv")'
 */
#include "RunConfig.h"

void dead_cross(TString glob, std::string deadAsAd, int nRandom, int patch, int seed, TString csv)
{
   auto map = std::make_shared<AtTpcMap>();
   map->ParseXMLMap((TString(getenv("VMCWORKDIR")) + "/scripts/e12014_pad_map_size.xml").Data());
   map->GeneratePadPlane();
   InhibitZapPads(*map); // As in the digitization: the further dead pads are drawn among the others
   auto deadList = DeadPads(*map, deadAsAd, nRandom, patch, seed);
   std::set<int> dead(deadList.begin(), deadList.end());

   TString dir = RunConfig::DataDir();
   TString globDir = gSystem->GetDirName(dir + "/" + glob);
   TString globName = gSystem->BaseName(glob);
   std::unique_ptr<TSystemDirectory> sysDir(new TSystemDirectory(globDir, globDir));
   std::vector<TString> files;
   TRegexp re(globName, true);
   for (auto *obj : *sysDir->GetListOfFiles()) {
      TString name = obj->GetName();
      if (name.Index(re) == 0 && name.Length() == name(re).Length())
         files.push_back(name);
   }
   std::sort(files.begin(), files.end());
   if (files.empty())
      throw std::runtime_error(std::string("No files match ") + (globDir + "/" + globName).Data());

   std::ofstream out(csv.Data());
   out << "chunk,entry,pads0,dead0,pads1,dead1\n";
   for (auto &name : files) {
      TString chunk = "";
      auto at = name.Index(".c");
      if (at >= 0 && name.Length() > at + 4 && isdigit(name[at + 2]) && isdigit(name[at + 3]))
         chunk = name(at + 1, 3);
      TFile file(globDir + "/" + name);
      auto tree = file.Get<TTree>("cbmsim");
      TTreeReader reader(tree);
      TTreeReaderValue<TClonesArray> fission(reader, "AtFissionEvent");
      while (reader.Next()) {
         out << chunk << "," << reader.GetCurrentEntry();
         auto event = dynamic_cast<AtFissionEvent *>(fission->At(0));
         for (int i = 0; i < 2; ++i) {
            std::set<int> pads;
            if (event && event->GetYPattern())
               for (auto hit : event->GetFragHits(i))
                  pads.insert(hit->GetPadNum());
            int nDead = std::count_if(pads.begin(), pads.end(), [&dead](int p) { return dead.count(p); });
            out << "," << pads.size() << "," << nDead;
         }
         out << "\n";
      }
   }
   std::cout << "Dead pads: " << dead.size() << ", wrote " << csv << std::endl;
}
