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
 *   FISSION_DATA     directory for all run files (default ./data)
 *   TPC_SHARED_INFO  directory holding eLoss/, respAvg.root and e12014_zap.csv
 */
#include "AtCSVReader.h"

#include <FairRootManager.h>

#include <TFile.h>
#include <TNamed.h>
#include <TString.h>
#include <TSystem.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

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

/// Load a LISE or SRIM energy-loss table for [Z, A] from TPC_SHARED_INFO/eLoss/.
std::shared_ptr<AtTools::AtELossTable> LoadELoss(const std::string &type, int Z, int A)
{
   auto eloss = std::make_shared<AtTools::AtELossTable>();
   auto file = TString::Format("%s/eLoss/%s/%d_%d.txt", RunConfig::SharedInfo().Data(), type.c_str(), Z, A);
   if (type == "LISE")
      eloss->LoadLiseTable(file.Data(), A, 0);
   else if (type == "SRIM")
      eloss->LoadSrimTable(file.Data());
   else
      throw std::invalid_argument("Unknown energy-loss table type " + type + " (use LISE or SRIM)");
   return eloss;
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

#endif
