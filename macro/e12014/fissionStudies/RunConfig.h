#ifndef FISSION_RUNCONFIG_H
#define FISSION_RUNCONFIG_H
/**
 * Settings for one stage of one run, read from a key=value file (written by fission.py, or by hand).
 *
 * With no file every Get() returns the default passed to it, so the macros still run when called
 * with no arguments. Finish() records the settings in the output file (TNamed "RunConfig") and next
 * to it (<output>.cfg). fission.py reads the .cfg to decide whether a stage needs to be rerun, so it
 * is only written after the stage finished.
 *
 * Paths come from the environment:
 *   FISSION_DATA     directory for all run files (default ./data)
 *   TPC_SHARED_INFO  directory holding eLoss/, respAvg.root and e12014_zap.csv
 */
#include <FairRootManager.h>

#include <TFile.h>
#include <TNamed.h>
#include <TString.h>
#include <TSystem.h>

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
   std::map<std::string, std::string> fUsed;   // What the macro asked for (recorded for manual runs)
   std::string fText;                          // Config file contents, recorded verbatim

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
      fText = ss.str();

      std::istringstream lines(fText);
      std::string line;
      while (std::getline(lines, line)) {
         auto eq = line.find('=');
         if (line.empty() || line[0] == '#' || eq == std::string::npos)
            continue;
         fValues[Trim(line.substr(0, eq))] = Trim(line.substr(eq + 1));
      }
   }

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

   /// Call after fRun->Run(). Closes the output file, then records the settings in and next to it.
   void Finish(const TString &outFile)
   {
      std::string text = fText;
      if (text.empty())
         for (auto &[key, val] : fUsed)
            text += key + " = " + val + "\n";

      FairRootManager::Instance()->CloseSink();
      TFile file(outFile, "UPDATE");
      TNamed("RunConfig", text.c_str()).Write("RunConfig", TObject::kOverwrite);
      file.Close();

      TString cfgFile = outFile;
      cfgFile.ReplaceAll(".root", ".cfg");
      std::ofstream(cfgFile.Data()) << text;
   }
};

/**** Settings shared by the sim and fit stages, so the two can't disagree ****/

/// Fission-fragment species [Z, A] that can be simulated or fit. A follows Z/Zcn of the compound nucleus.
std::vector<std::pair<int, int>> IonList(RunConfig &cfg)
{
   int Zcn = cfg.GetInt("ions.zcn", 85);
   int Acn = cfg.GetInt("ions.acn", 204);
   std::vector<std::pair<int, int>> ions;
   for (int Z = cfg.GetInt("ions.zmin", 26); Z <= cfg.GetInt("ions.zmax", 59); Z++)
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

#endif
