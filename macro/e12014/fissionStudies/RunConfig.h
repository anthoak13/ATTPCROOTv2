#ifndef FISSION_RUNCONFIG_H
#define FISSION_RUNCONFIG_H
/**
 * Settings for one stage of one chunk of a study section, read from a key=value file (written by
 * fission.py, or by hand).
 *
 * With no file every Get() returns the default passed to it, so the macros still run when called
 * with no arguments. Finish() records the settings in the output file (TNamed "RunConfig") and next
 * to it (<output>.cfg). The TNamed holds every setting the stage ran with, including macro defaults.
 * The .cfg is a copy of the config file (or the same record for manual runs without one). fission.py
 * compares it with what it would pass now to decide whether a stage needs to be rerun, so it is only
 * written after the stage finished.
 *
 * Paths come from the environment:
 *   FISSION_DATA     directory for all run files (default ./data). A relative path is relative to the
 *                    directory ROOT was started in.
 *   TPC_SHARED_INFO  directory holding eLoss/, respAvg.root and e12014_zap.csv
 */

class RunConfig {
   std::map<std::string, std::string> fValues; // From the config file
   std::map<std::string, std::string> fUsed;   // What the macro asked for, including defaults
   std::string fText;                          // Config file contents, copied to the .cfg

   static std::string Trim(const std::string &s)
   {
      auto first = s.find_first_not_of(" \t\r");
      auto last = s.find_last_not_of(" \t\r");
      return first == std::string::npos ? "" : s.substr(first, last - first + 1);
   }

public:
   explicit RunConfig(TString path = "")
   {
      if (path.IsNull())
         return;
      std::ifstream file(path.Data());
      if (!file)
         throw std::runtime_error(std::string("Cannot open config file ") + path.Data());
      std::stringstream ss;
      ss << file.rdbuf();
      Parse(ss.str());
   }

   /// The settings recorded in a stage's output file by Finish(). Empty if the file has none.
   static RunConfig FromOutput(const TString &rootFile)
   {
      RunConfig cfg;
      std::unique_ptr<TFile> file(TFile::Open(rootFile));
      auto record = file ? file->Get<TNamed>("RunConfig") : nullptr;
      if (record)
         cfg.Parse(record->GetTitle());
      return cfg;
   }

   void Parse(const std::string &text)
   {
      fText = text;
      std::istringstream lines(fText);
      std::string line;
      while (std::getline(lines, line)) {
         auto eq = line.find('=');
         if (line.empty() || line[0] == '#' || eq == std::string::npos)
            continue;
         fValues[Trim(line.substr(0, eq))] = Trim(line.substr(eq + 1));
      }
   }

   bool Empty() const { return fValues.empty(); }

   std::string GetStr(const std::string &key, const std::string &def)
   {
      auto it = fValues.find(key);
      auto val = it == fValues.end() ? def : it->second;
      fUsed[key] = val;
      return val;
   }
   double Get(const std::string &key, double def)
   {
      std::ostringstream ss;
      ss << def;
      return std::stod(GetStr(key, ss.str()));
   }
   int GetInt(const std::string &key, int def) { return std::stoi(GetStr(key, std::to_string(def))); }

   static TString DataDir()
   {
      auto env = gSystem->Getenv("FISSION_DATA");
      TString dir = env ? env : "./data";
      if (!gSystem->IsAbsoluteFileName(dir))
         gSystem->PrependPathName(gSystem->WorkingDirectory(), dir); // So messages show where it looked
      gSystem->mkdir(dir, true);
      return dir;
   }
   static TString SharedInfo()
   {
      auto env = gSystem->Getenv("TPC_SHARED_INFO");
      if (!env) {
         std::cerr << "TPC_SHARED_INFO is not set, using ./tpcSharedInfo" << std::endl;
         return "./tpcSharedInfo";
      }
      return env;
   }

   /// File path stored under key ("input" or "output"), or DataDir()/manual.<stage>.root for manual runs.
   TString File(const std::string &key, const TString &stage)
   {
      return GetStr(key, (DataDir() + "/manual." + stage + ".root").Data());
   }

   /**
    * Throw if the config file has a setting the macro never read, which is usually a typo. Settings
    * starting with one of `ignored` are allowed: each stage receives every upstream stage's settings
    * (e.g. the fit stage is passed all sim.* keys). Call after the last Get().
    */
   void CheckUnused(const std::vector<std::string> &ignored = {}) const
   {
      std::string unused;
      for (auto &entry : fValues) {
         auto &key = entry.first;
         bool isIgnored =
            std::any_of(ignored.begin(), ignored.end(), [&key](const std::string &p) { return key.rfind(p, 0) == 0; });
         if (!fUsed.count(key) && !isIgnored)
            unused += " " + key;
      }
      if (!unused.empty())
         throw std::runtime_error("Settings never read by this stage (typo?):" + unused);
   }

   /// Call after fRun->Run(). Closes the output file, then records the settings in and next to it.
   void Finish(const TString &outFile)
   {
      // Everything passed in (upstream settings describe the input), then the defaults the macro used
      std::string record;
      for (auto &[key, val] : fValues)
         record += key + " = " + val + "\n";
      std::string defaults;
      for (auto &[key, val] : fUsed)
         if (!fValues.count(key))
            defaults += key + " = " + val + "\n";
      if (!defaults.empty())
         record += "# Macro defaults\n" + defaults;

      FairRootManager::Instance()->CloseSink();
      TFile file(outFile, "UPDATE");
      TNamed("RunConfig", record.c_str()).Write("RunConfig", TObject::kOverwrite);
      file.Close();

      TString cfgFile = outFile;
      cfgFile.ReplaceAll(".root", ".cfg");
      std::ofstream(cfgFile.Data()) << (fText.empty() ? record : fText);
   }
};

/**** The sim's ion settings (sim.ions.*), passed on to the fit so both use the same nucleus ****/

/// [Z, A] of the compound nucleus that fissions.
std::pair<int, int> CompoundNucleus(RunConfig &cfg)
{
   return {cfg.GetInt("sim.ions.zcn", 85), cfg.GetInt("sim.ions.acn", 204)};
}

/**
 * Fission-fragment species [Z, A] that can be simulated or fit. A follows Z/Zcn of the compound nucleus.
 * Both fragments of a split need a table, so the partner Zcn - Z of every Z must be in the range too,
 * which holds only if zmin + zmax = Zcn.
 */
std::vector<std::pair<int, int>> IonList(RunConfig &cfg)
{
   auto [Zcn, Acn] = CompoundNucleus(cfg);
   int zMin = cfg.GetInt("sim.ions.zmin", 26);
   int zMax = cfg.GetInt("sim.ions.zmax", 59);
   if (zMin + zMax != Zcn || zMin > zMax)
      throw std::invalid_argument("sim.ions.zmin (" + std::to_string(zMin) + ") + sim.ions.zmax (" +
                                  std::to_string(zMax) + ") must equal sim.ions.zcn (" + std::to_string(Zcn) +
                                  ") so both fragments of every split have a table");
   std::vector<std::pair<int, int>> ions;
   for (int Z = zMin; Z <= zMax; Z++)
      ions.emplace_back(Z, std::round((double)Z / Zcn * Acn));
   return ions;
}

/**
 * Load an energy-loss table for [Z, A] from TPC_SHARED_INFO/eLoss/, with every dE/dx multiplied by scale (LISE
 * files only). type is
 *   LISE:    the LISE file's ATIMA 1.2 column (column 2), the baseline's model
 *   ZIEGLER: the same file's Ziegler column (column 1, "H-base", the stopping powers of older SRIM versions)
 *   SRIM:    the SRIM file
 * A LISE file gives dE/dx against energy per nucleon, so massA != 0 loads it for the same Z with mass massA.
 * SetDensity scales a table by the ratio of the new to the old density, so the LISE tables (in MeV/um) are loaded
 * with a nominal density of 1; a negative density keeps those units.
 */
std::shared_ptr<AtTools::AtELossTable> LoadELoss(const std::string &type, int Z, int A, double scale = 1, int massA = 0)
{
   bool liseFile = type == "LISE" || type == "ZIEGLER";
   auto eloss = std::make_shared<AtTools::AtELossTable>();
   auto file =
      TString::Format("%s/eLoss/%s/%d_%d.txt", RunConfig::SharedInfo().Data(), liseFile ? "LISE" : type.c_str(), Z, A);
   if (massA != 0 && massA != A && !liseFile)
      throw std::invalid_argument("Only a LISE table can be used for another mass (sim.massShift)");
   if (liseFile) // Table in MeV/u: the same Z at another mass
      eloss->LoadLiseTable(file.Data(), massA ? massA : A, -1, type == "LISE" ? 2 : 1);
   else if (type == "SRIM")
      eloss->LoadSrimTable(file.Data());
   else
      throw std::invalid_argument("Unknown energy-loss table type " + type + " (use LISE, ZIEGLER or SRIM)");
   if (scale != 1) {
      if (!liseFile)
         throw std::invalid_argument("A dE/dx scale (sim.elossScale) is only supported for LISE tables");
      eloss->SetDensity(scale);
   }
   return eloss;
}

/**** Space charge ****/

/// Nominal line charge of the beam (C/m), the experimental data's range is about 5e-9 to 2e-8
constexpr double kNominalLambda = 1.54e-8;
/// Radius (mm) inside which the line-charge models leave charge where it is
constexpr double kBeamRadius = 20;

/**
 * The beam line the space-charge models are centered on, from "<x> <y> at the pad plane, <x> <y> at the window"
 * (mm). The models measure z from the pad plane, so their first point (SetBeamLocation's "window") is at the pad
 * plane. The default is the line the experimental-data analysis assumes.
 */
void SetScBeam(AtSpaceChargeModel &model, const std::string &line)
{
   std::istringstream ss(line);
   double xPad, yPad, xWin, yWin;
   if (!(ss >> xPad >> yPad >> xWin >> yWin))
      throw std::invalid_argument("A space-charge beam line needs four numbers, got: " + line);
   model.SetBeamLocation({xPad, yPad, 0}, {xWin, yWin, 1000});
}

/**
 * Radial field (V/cm) at rho and z (cm, z from the pad plane) of a beam line charge that grows linearly from 0 at
 * the pad plane to lambda (C/m) at the window, 1 m away (the positive ions drift toward the window).
 *   kLine:     all the charge on the line, no field inside kBeamRadius (as AtLineChargeZDep)
 *   kUniform:  the charge spread uniformly over kBeamRadius, so the field grows linearly inside it
 *   kConstant: as kLine, with the same total charge spread evenly in z (lambda / 2 everywhere)
 */
struct ScField {
   enum Shape { kLine, kUniform, kConstant };
   double lambda;
   Shape shape;

   double operator()(double rho, double z) const
   {
      constexpr double eps2pi = 2 * 3.14159265358979 * 8.85418782E-12; // SI
      double rBeam = kBeamRadius / 1000;                               // m
      rho /= 100.;                                                     // cm to m
      z /= 100.;
      double lambdaZ = shape == kConstant ? lambda / 2 : lambda * z / 1.;
      double field = 0;
      if (rho > rBeam)
         field = lambdaZ / eps2pi / rho;
      else if (shape == kUniform)
         field = lambdaZ / eps2pi * rho / rBeam / rBeam;
      return field / 100.; // V/m to V/cm
   }
};

/**
 * The space-charge model named by type, with magnitude lambda (C/m) about the beam line `beam` (see SetScBeam):
 *   none:        nullptr
 *   line:        AtLineChargeModel, the model the experimental data are corrected with
 *   lineNumeric: the same charge, the electrons' drift integrated step by step (AtRadialChargeModel)
 *   uniform, constant: AtRadialChargeModel with the ScField of that shape
 */
std::shared_ptr<AtSpaceChargeModel> MakeSpaceCharge(const std::string &type, double lambda, const std::string &beam)
{
   std::shared_ptr<AtSpaceChargeModel> model;
   if (type == "none")
      return nullptr;
   if (type == "line") {
      auto line = std::make_shared<AtLineChargeModel>();
      line->SetLambda(lambda);
      line->SetBeamRadius(kBeamRadius / 1000); // m
      model = line;
   } else {
      ScField::Shape shape;
      if (type == "lineNumeric")
         shape = ScField::kLine;
      else if (type == "uniform")
         shape = ScField::kUniform;
      else if (type == "constant")
         shape = ScField::kConstant;
      else
         throw std::invalid_argument("Unknown space-charge model " + type +
                                     " (use none, line, lineNumeric, uniform or constant)");
      auto radial = std::make_shared<AtRadialChargeModel>(ScField{lambda, shape});
      radial->SetStepSize(0.1); // us
      model = radial;
   }
   SetScBeam(*model, beam);
   return model;
}

/**
 * Mark the pads in TPC_SHARED_INFO/e12014_zap.csv low-gain. Digi turns them down by digi.lowGain, and
 * the fit's charge objective skips them (as it does on real data), so both stages must use the same list.
 */
void InhibitZapPads(AtMap &map)
{
   auto path = RunConfig::SharedInfo() + "/e12014_zap.csv";
   std::ifstream file(path.Data());
   if (!file)
      throw std::runtime_error(std::string("Cannot open smart zap file ") + path.Data());

   // Some copies of the file end lines with a bare \r, which getline would read as one line
   std::stringstream ss;
   ss << file.rdbuf();
   TString text = ss.str();
   text.ReplaceAll("\r\n", "\n").ReplaceAll("\r", "\n");
   std::istringstream lines(text.Data());

   // Skip the two header lines
   std::string temp;
   std::getline(lines, temp);
   std::getline(lines, temp);

   int nPads = 0;
   for (auto &row : CSVRange<int>(lines)) {
      if (row.size() < 5)
         continue; // Blank line
      map.InhibitPad(row[4], AtMap::InhibitType::kLowGain);
      ++nPads;
   }
   if (nPads == 0)
      throw std::runtime_error(std::string("No pads in smart zap file ") + path.Data());
   std::cout << "Marked " << nPads << " pads low-gain from " << path << std::endl;
}

/**** Pads and electronics of the digitization ****/

/**
 * Pads to make dead (no charge) in the digitization, in order:
 *   deadAsAd: "<cobo> <asad>", every channel of that AsAd (the experimental data lack CoBo 2, AsAd 3), or ""
 *   nRandom:  further pads, drawn from the small pads that are not already inhibited (the pads the fit can use)
 *   patch:    the further pads come in patches of this many neighbors: each patch is a random pad and the
 *             patch - 1 candidate pads nearest it. 1 scatters them.
 * The draw uses its own generator (seed), so the pattern is fixed for a sample and leaves the events' draws alone.
 */
std::vector<int> DeadPads(AtMap &map, const std::string &deadAsAd, int nRandom, int patch, int seed)
{
   std::vector<int> dead;
   if (!deadAsAd.empty()) {
      std::istringstream ss(deadAsAd);
      int cobo, asad;
      if (!(ss >> cobo >> asad))
         throw std::invalid_argument("digi.deadAsAd needs \"<cobo> <asad>\", got: " + deadAsAd);
      for (int aget = 0; aget < 4; ++aget)
         for (int ch = 0; ch < 68; ++ch) {
            int pad = map.GetPadNum({cobo, asad, aget, ch});
            if (pad >= 0)
               dead.push_back(pad);
         }
   }
   std::set<int> deadSet(dead.begin(), dead.end());

   std::vector<int> candidates;
   for (int pad = 0; pad < (int)map.GetNumPads(); ++pad)
      if (map.GetPadSize(pad) == 0 && map.IsInhibited(pad) == AtMap::InhibitType::kNone && !deadSet.count(pad))
         candidates.push_back(pad);
   if (nRandom > (int)candidates.size())
      throw std::invalid_argument("digi.deadPads (" + std::to_string(nRandom) + ") is more than the " +
                                  std::to_string(candidates.size()) + " pads it is drawn from");

   std::vector<ROOT::Math::XYPoint> centers;
   for (int pad : candidates)
      centers.push_back(map.CalcPadCenter(pad));

   TRandom3 rand(seed);
   int nAdded = 0;
   while (nAdded < nRandom) {
      int iCenter = rand.Integer(candidates.size());
      if (deadSet.count(candidates[iCenter]))
         continue;
      std::vector<std::pair<double, int>> byDist;
      for (size_t i = 0; i < candidates.size(); ++i)
         if (!deadSet.count(candidates[i]))
            byDist.emplace_back((centers[i] - centers[iCenter]).Mag2(), candidates[i]);
      int n = std::min({patch, nRandom - nAdded, (int)byDist.size()});
      std::partial_sort(byDist.begin(), byDist.begin() + n, byDist.end());
      for (int i = 0; i < n; ++i) {
         dead.push_back(byDist[i].second);
         deadSet.insert(byDist[i].second);
      }
      nAdded += n;
   }
   return dead;
}

/**
 * Gain factor of every pad: 1 + Gaus(0, padSigma) for the pad, times the same for its AGET (agetSigma) and its
 * AsAd (asadSigma). Fixed for a sample: drawn from their own generator (seed). Factors are kept above 0.
 */
std::unordered_map<int, double> PadGains(AtMap &map, double padSigma, double agetSigma, double asadSigma, int seed)
{
   TRandom3 rand(seed);
   auto draw = [&rand](double sigma) { return sigma > 0 ? std::max(0., 1 + rand.Gaus(0, sigma)) : 1.; };
   std::map<std::tuple<int, int, int>, double> aget;
   std::map<std::pair<int, int>, double> asad;
   std::unordered_map<int, double> gains;
   for (int pad = 0; pad < (int)map.GetNumPads(); ++pad) {
      auto ref = map.GetPadRef(pad);
      auto agetKey = std::make_tuple(ref.cobo, ref.asad, ref.aget);
      auto asadKey = std::make_pair(ref.cobo, ref.asad);
      if (!aget.count(agetKey))
         aget[agetKey] = draw(agetSigma);
      if (!asad.count(asadKey))
         asad[asadKey] = draw(asadSigma);
      gains[pad] = draw(padSigma) * aget[agetKey] * asad[asadKey];
   }
   return gains;
}

#endif
