#ifndef S800INVERSEMAP_H_
#define S800INVERSEMAP_H_

#include <Rtypes.h>
#include <TNamed.h>

#include <algorithm>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

class TBuffer;
class TClass;
class TMemberInspector;
class TSpline3;

class S800InverseMap : public TNamed {

public:
   S800InverseMap(const char *filename);
   // S800InverseMap(std::vector<std::string> &fileList);
   S800InverseMap();
   static S800InverseMap *Get(const char *filename = "");
   virtual ~S800InverseMap();

   virtual void Print(Option_t *opt = "") const; //{ ; }
   virtual void Clear(Option_t *opt = "") { ; }

   float Ata(int degree, double xfp, double afp, double yfp, double bfp) const;
   float Ata(int degree, double xfp, double afp, double yfp, double bfp, double z);
   // float Ata(int,const TS800*);
   float Bta(int degree, double xfp, double afp, double yfp, double bfp) const;
   float Bta(int degree, double xfp, double afp, double yfp, double bfp, double z);
   // float Bta(int,const TS800*);
   float Yta(int degree, double xfp, double afp, double yfp, double bfp) const;
   float Yta(int degree, double xfp, double afp, double yfp, double bfp, double z);
   // float Yta(int,const TS800*);
   float Dta(int degree, double xfp, double afp, double yfp, double bfp) const;
   float Dta(int degree, double xfp, double afp, double yfp, double bfp, double z);
   // float Dta(int,const TS800*);

   float MapCalc(int, int, float *) const;
   float MapCalc_s(int order, int par, float *input, double z);

   void SetDistPivotTarget(std::vector<Double_t> vec)
   {
      std::cout << "check setDistPivotTarget " << vec.size() << " " << vec.at(2) << std::endl;
      fMapDist_v = vec;
   };

   int Size() { return fMap.size(); }

   // bool ReadMultiMapFile(std::vector<std::string> &str);
   bool ReadMultiMapFile(std::vector<TString> &str);

private:
   // S800InverseMap(const char* filename);
   static std::unique_ptr<S800InverseMap> fInverseMap;

   bool ReadMapFile(const char *filename);
   // bool ReadMultiMapFile(std::vector<std::string> &str);

   struct InvMapRow {
      double coefficient;
      int order;
      int exp[6];
   };

   struct InvMapRowS {
      TSpline3 *coefficient;
      int order;
      int exp[6];
   };

   // data cleared on reset; i.e. Read new inverse map.
   std::map<int, std::vector<InvMapRow>> fMap;
   std::map<int, std::vector<InvMapRowS>> fMap_s;
   std::vector<std::map<int, std::vector<InvMapRow>>> fMap_v;
   std::vector<Double_t> fMapDist_v;
   float fBrho{};
   int fMass{};
   int fCharge{};
   Int_t fsize{};
   std::string info;

   ClassDef(S800InverseMap, 0)
};
#endif
