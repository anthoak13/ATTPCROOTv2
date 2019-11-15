#include "ATTPCFissionGeneratorV3.h"

// Default constructor
ATTPCFissionGeneratorV3::ATTPCFissionGeneratorV3() :
  fMult(0)
{
  
}

// Generator that takes in a file that specifies the expected distribution of
// fission particles. 
ATTPCFissionGeneratorV3::ATTPCFissionGeneratorV3(const char *name, TString fissionDistro) :
  fMult(0)
{

  //Get the working directory
  TString workingDir = std::getenv("VCMWORKDIR");

  //Look for the file defining the fission mass distrobution
  TString fileLoc = workingDir + fissionDistro;
  std::ifstream fileIn(fileLoc.Data());

  if( !fileIn.is_open())
    Fatal("ATTPCFissionGeneratorV3", Form("Cannot open input file: %s", fileLoc.Data()));

  // Read the file until all of the ions have been generated. Ion section ends with line
  // "EndIons"
  int nIons = 0;
  while(!fileIn.eof())
  {
    int A = 0, Z = 0;
    fileIn >> Z >> A;

    // If this wasn't a valid ion, then look to make sure we've hit the end of the ion section
    if(A == 0 && Z == 0)
    {
      char strIn[100];
      fileIn.getline(strIn, 100);
      std::cout << "No valid ion looking at next line: "
		<< strIn << std::endl;
      break; 
    }

    std::cout << "Found ion: " << Z << A << std::endl;
  }// end loop through ion list

  
}

// Deep copy constructor
ATTPCFissionGeneratorV3::ATTPCFissionGeneratorV3(ATTPCFissionGeneratorV3 &rhs)
{
  
}

ATTPCFissionGeneratorV3::~ATTPCFissionGeneratorV3()
{

}

Bool_t ATTPCFissionGeneratorV3::ReadEvent(FairPrimaryGenerator* primGen)
{

}

ClassImp(ATTPCFissionGeneratorV3)
