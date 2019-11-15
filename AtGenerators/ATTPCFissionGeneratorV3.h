#ifndef ATTPCFISSIONGENERATORV3_H
#define ATTPCFISSIONGENERATORV3_H

/* Fission generator for beam induced fission events
 * Takes a csv file that defines the distribution of fission fragments
 * availible. An example can be found in ______.
 * 
 * Adam Anthony 8/23/2019
 *
 */

#include "FairGenerator.h"
#include "FairIon.h"
#include "FairParticle.h"

#include <cstdlib>
#include <fstream>
#include <iostream>


class ATTPCFissionGeneratorV3 : public FairGenerator
{

public:

  //Default constructor
  ATTPCFissionGeneratorV3();

  // Generator that takes in a file that specifies the expected distribution of
  // fission particles. This file name is passed as the title for TNamed
  ATTPCFissionGeneratorV3(const char *name, TString fissionDistro);

  // Deep copy constructor
  ATTPCFissionGeneratorV3(ATTPCFissionGeneratorV3 &copy);

  // The main physics is here
  Bool_t ReadEvent(FairPrimaryGenerator* primGen) override;

  /** Destructor **/
  virtual ~ATTPCFissionGeneratorV3();

  // Internal variables for tracking the physics 
private:
  //Variables read from the file for each event
  Int_t fMult;
  std::vector<FairIon*> fIons;
  std::vector<FairParticle*> fParticles;
  
  
  ClassDef(ATTPCFissionGeneratorV3, 1)
    
};


#endif //#ifndef ATTPCFISSIONGENERATORV3_H
