#ifndef FAKE_TVIRTUAL_MC_H
#define FAKE_TVIRTUAL_MC_H

#include <TVirtualMC.h>

/**
 * @class FakeTVirtualMC
 * @brief A fake implementation of the TVirtualMC class.
 *
 * This class provides a fake implementation of the TVirtualMC class, which is used for Monte Carlo simulations in ROOT.
 * It inherits from the TVirtualMC class and overrides all its virtual functions.
 * The purpose of this class is to serve as a placeholder for testing simulation-related classes that need to
 * communicate with the VMC interface.
 */
class FakeTVirtualMC : public TVirtualMC {
public:
   FakeTVirtualMC() : TVirtualMC("", "") {}

   virtual Bool_t IsRootGeometrySupported() const { return false; }

   virtual void Material(Int_t &kmat, const char *name, Double_t a, Double_t z, Double_t dens, Double_t radl,
                         Double_t absl, Float_t *buf, Int_t nwbuf)
   {
   }
   virtual void Material(Int_t &kmat, const char *name, Double_t a, Double_t z, Double_t dens, Double_t radl,
                         Double_t absl, Double_t *buf, Int_t nwbuf)
   {
   }
   virtual void
   Mixture(Int_t &kmat, const char *name, Float_t *a, Float_t *z, Double_t dens, Int_t nlmat, Float_t *wmat)
   {
   }
   virtual void
   Mixture(Int_t &kmat, const char *name, Double_t *a, Double_t *z, Double_t dens, Int_t nlmat, Double_t *wmat)
   {
   }

   virtual void Medium(Int_t &kmed, const char *name, Int_t nmat, Int_t isvol, Int_t ifield, Double_t fieldm,
                       Double_t tmaxfd, Double_t stemax, Double_t deemax, Double_t epsil, Double_t stmin, Float_t *ubuf,
                       Int_t nbuf)
   {
   }
   virtual void Medium(Int_t &kmed, const char *name, Int_t nmat, Int_t isvol, Int_t ifield, Double_t fieldm,
                       Double_t tmaxfd, Double_t stemax, Double_t deemax, Double_t epsil, Double_t stmin,
                       Double_t *ubuf, Int_t nbuf)
   {
   }
   virtual void
   Matrix(Int_t &krot, Double_t thetaX, Double_t phiX, Double_t thetaY, Double_t phiY, Double_t thetaZ, Double_t phiZ)
   {
   }
   virtual void Gstpar(Int_t itmed, const char *param, Double_t parval) {}
   virtual Int_t Gsvolu(const char *name, const char *shape, Int_t nmed, Float_t *upar, Int_t np) { return 0; }
   virtual Int_t Gsvolu(const char *name, const char *shape, Int_t nmed, Double_t *upar, Int_t np) { return 0; }
   virtual void Gsdvn(const char *name, const char *mother, Int_t ndiv, Int_t iaxis) {}
   virtual void Gsdvn2(const char *name, const char *mother, Int_t ndiv, Int_t iaxis, Double_t c0i, Int_t numed) {}
   virtual void Gsdvt(const char *name, const char *mother, Double_t step, Int_t iaxis, Int_t numed, Int_t ndvmx) {}
   virtual void
   Gsdvt2(const char *name, const char *mother, Double_t step, Int_t iaxis, Double_t c0, Int_t numed, Int_t ndvmx)
   {
   }
   virtual void Gsord(const char *name, Int_t iax) {}
   virtual void Gspos(const char *name, Int_t nr, const char *mother, Double_t x, Double_t y, Double_t z, Int_t irot,
                      const char *konly = "ONLY")
   {
   }
   virtual void Gsposp(const char *name, Int_t nr, const char *mother, Double_t x, Double_t y, Double_t z, Int_t irot,
                       const char *konly, Float_t *upar, Int_t np)
   {
   }
   virtual void Gsposp(const char *name, Int_t nr, const char *mother, Double_t x, Double_t y, Double_t z, Int_t irot,
                       const char *konly, Double_t *upar, Int_t np)
   {
   }
   virtual void Gsbool(const char *onlyVolName, const char *manyVolName) {}
   virtual void SetCerenkov(Int_t itmed, Int_t npckov, Float_t *ppckov, Float_t *absco, Float_t *effic, Float_t *rindex,
                            Bool_t aspline = false, Bool_t rspline = false)
   {
   }
   virtual void SetCerenkov(Int_t itmed, Int_t npckov, Double_t *ppckov, Double_t *absco, Double_t *effic,
                            Double_t *rindex, Bool_t aspline = false, Bool_t rspline = false)
   {
   }
   virtual void DefineOpSurface(const char *name, EMCOpSurfaceModel model, EMCOpSurfaceType surfaceType,
                                EMCOpSurfaceFinish surfaceFinish, Double_t sigmaAlpha)
   {
   }
   virtual void SetBorderSurface(const char *name, const char *vol1Name, int vol1CopyNo, const char *vol2Name,
                                 int vol2CopyNo, const char *opSurfaceName)
   {
   }
   virtual void SetSkinSurface(const char *name, const char *volName, const char *opSurfaceName) {}
   virtual void SetMaterialProperty(Int_t itmed, const char *propertyName, Int_t np, Double_t *pp, Double_t *values,
                                    Bool_t createNewKey = false, Bool_t spline = false)
   {
   }
   virtual void SetMaterialProperty(Int_t itmed, const char *propertyName, Double_t value) {}
   virtual void SetMaterialProperty(const char *surfaceName, const char *propertyName, Int_t np, Double_t *pp,
                                    Double_t *values, Bool_t createNewKey = false, Bool_t spline = false)
   {
   }

   virtual Bool_t GetTransformation(const TString &volumePath, TGeoHMatrix &matrix) { return true; }
   virtual Bool_t GetShape(const TString &volumePath, TString &shapeType, TArrayD &par) { return true; }
   virtual Bool_t GetMaterial(Int_t imat, TString &name, Double_t &a, Double_t &z, Double_t &density, Double_t &radl,
                              Double_t &inter, TArrayD &par)
   {
      return true;
   }
   virtual Bool_t GetMaterial(const TString &volumeName, TString &name, Int_t &imat, Double_t &a, Double_t &z,
                              Double_t &density, Double_t &radl, Double_t &inter, TArrayD &par)
   {
      return true;
   }
   virtual Bool_t GetMedium(const TString &volumeName, TString &name, Int_t &imed, Int_t &nmat, Int_t &isvol,
                            Int_t &ifield, Double_t &fieldm, Double_t &tmaxfd, Double_t &stemax, Double_t &deemax,
                            Double_t &epsil, Double_t &stmin, TArrayD &par)
   {
      return true;
   }

   virtual void WriteEuclid(const char *filnam, const char *topvol, Int_t number, Int_t nlevel) {}
   virtual void SetRootGeometry() {}
   virtual void SetUserParameters(Bool_t isUserParameters) {}

   //
   // get methods
   // ------------------------------------------------
   //

   /// Return the unique numeric identifier for volume name volName
   virtual Int_t VolId(const char *volName) const { return 0; }

   /// Return the volume name for a given volume identifier id
   virtual const char *VolName(Int_t id) const { return ""; }

   /// Return the unique numeric identifier for medium name mediumName
   virtual Int_t MediumId(const char *mediumName) const { return 0; }

   /// Return total number of volumes in the geometry
   virtual Int_t NofVolumes() const { return 0; }

   /// Return material number for a given volume id
   virtual Int_t VolId2Mate(Int_t id) const { return 0; }

   /// Return number of daughters of the volume specified by volName
   virtual Int_t NofVolDaughters(const char *volName) const { return 0; }

   /// Return the name of i-th daughter of the volume specified by volName
   virtual const char *VolDaughterName(const char *volName, Int_t i) const { return ""; }

   /// Return the copyNo of i-th daughter of the volume specified by volName
   virtual Int_t VolDaughterCopyNo(const char *volName, Int_t i) const { return 0; }

   //
   // ------------------------------------------------
   // methods for sensitive detectors
   // ------------------------------------------------
   //

   /// Set a sensitive detector to a volume
   /// - volName - the volume name
   /// - sd - the user sensitive detector
   virtual void SetSensitiveDetector(const TString &volName, TVirtualMCSensitiveDetector *sd) {}

   /// Get a sensitive detector of a volume
   /// - volName - the volume name
   virtual TVirtualMCSensitiveDetector *GetSensitiveDetector(const TString &volName) const { return nullptr; }

   /// The scoring option:
   /// if true, scoring is performed only via user defined sensitive detectors and
   /// MCApplication::Stepping is not called
   virtual void SetExclusiveSDScoring(Bool_t exclusiveSDScoring) {}

   //
   // ------------------------------------------------
   // methods for physics management
   // ------------------------------------------------
   //

   //
   // set methods
   // ------------------------------------------------
   //

   /// Set transport cuts for particles
   virtual Bool_t SetCut(const char *cutName, Double_t cutValue) { return true; }

   /// Set process control
   virtual Bool_t SetProcess(const char *flagName, Int_t flagValue) { return true; }

   /// Set a user defined particle
   /// Function is ignored if particle with specified pdg
   /// already exists and error report is printed.
   /// - pdg           PDG encoding
   /// - name          particle name
   /// - mcType        VMC Particle type
   /// - mass          mass [GeV]
   /// - charge        charge [eplus]
   /// - lifetime      time of life [s]
   /// - pType         particle type as in Geant4
   /// - width         width [GeV]
   /// - iSpin         spin
   /// - iParity       parity
   /// - iConjugation  conjugation
   /// - iIsospin      isospin
   /// - iIsospinZ     isospin - #rd component
   /// - gParity       gParity
   /// - lepton        lepton number
   /// - baryon        baryon number
   /// - stable        stability
   /// - shortlived    is shorlived?
   /// - subType       particle subType as in Geant4
   /// - antiEncoding  anti encoding
   /// - magMoment     magnetic moment
   /// - excitation    excitation energy [GeV]
   virtual Bool_t DefineParticle(Int_t pdg, const char *name, TMCParticleType mcType, Double_t mass, Double_t charge,
                                 Double_t lifetime)
   {
      return true;
   }

   /// Set a user defined particle
   /// Function is ignored if particle with specified pdg
   /// already exists and error report is printed.
   /// - pdg           PDG encoding
   /// - name          particle name
   /// - mcType        VMC Particle type
   /// - mass          mass [GeV]
   /// - charge        charge [eplus]
   /// - lifetime      time of life [s]
   /// - pType         particle type as in Geant4
   /// - width         width [GeV]
   /// - iSpin         spin
   /// - iParity       parity
   /// - iConjugation  conjugation
   /// - iIsospin      isospin
   /// - iIsospinZ     isospin - #rd component
   /// - gParity       gParity
   /// - lepton        lepton number
   /// - baryon        baryon number
   /// - stable        stability
   /// - shortlived    is shorlived?
   /// - subType       particle subType as in Geant4
   /// - antiEncoding  anti encoding
   /// - magMoment     magnetic moment
   /// - excitation    excitation energy [GeV]
   virtual Bool_t DefineParticle(Int_t pdg, const char *name, TMCParticleType mcType, Double_t mass, Double_t charge,
                                 Double_t lifetime, const TString &pType, Double_t width, Int_t iSpin, Int_t iParity,
                                 Int_t iConjugation, Int_t iIsospin, Int_t iIsospinZ, Int_t gParity, Int_t lepton,
                                 Int_t baryon, Bool_t stable, Bool_t shortlived = kFALSE, const TString &subType = "",
                                 Int_t antiEncoding = 0, Double_t magMoment = 0.0, Double_t excitation = 0.0)
   {
      return true;
   }

   /// Set a user defined ion.
   /// - name          ion name
   /// - Z             atomic number
   /// - A             atomic mass
   /// - Q             charge [eplus}
   /// - excitation    excitation energy [GeV]
   /// - mass          mass  [GeV] (if not specified by user, approximative
   ///                 mass is calculated)
   virtual Bool_t DefineIon(const char *name, Int_t Z, Int_t A, Int_t Q, Double_t excEnergy, Double_t mass = 0.)
   {
      return true;
   }

   /// Set a user phase space decay for a particle
   /// -  pdg           particle PDG encoding
   /// -  bratios       the array with branching ratios (in %)
   /// -  mode[6][3]    the array with daughters particles PDG codes  for each
   ///                 decay channel
   virtual Bool_t SetDecayMode(Int_t pdg, Float_t bratio[6], Int_t mode[6][3]) { return true; }

   /// Calculate X-sections
   /// (Geant3 only)
   /// Deprecated
   virtual Double_t Xsec(char *, Double_t, Int_t, Int_t) { return 0; }

   //
   // particle table usage
   // ------------------------------------------------
   //

   /// Return MC specific code from a PDG and pseudo ENDF code (pdg)
   virtual Int_t IdFromPDG(Int_t pdg) const { return 0; }

   /// Return PDG code and pseudo ENDF code from MC specific code (id)
   virtual Int_t PDGFromId(Int_t id) const { return 0; }

   //
   // get methods
   // ------------------------------------------------
   //

   /// Return name of the particle specified by pdg.
   virtual TString ParticleName(Int_t pdg) const { return ""; }

   /// Return mass of the particle specified by pdg.
   virtual Double_t ParticleMass(Int_t pdg) const { return 0; }

   /// Return charge (in e units) of the particle specified by pdg.
   virtual Double_t ParticleCharge(Int_t pdg) const { return 0; }

   /// Return life time of the particle specified by pdg.
   virtual Double_t ParticleLifeTime(Int_t pdg) const { return 0; }

   /// Return VMC type of the particle specified by pdg.
   virtual TMCParticleType ParticleMCType(Int_t pdg) const { return TMCParticleType::kPTUndefined; }
   //
   // ------------------------------------------------
   // methods for step management
   // ------------------------------------------------
   //

   //
   // action methods
   // ------------------------------------------------
   //

   /// Stop the transport of the current particle and skip to the next
   virtual void StopTrack() {}

   /// Stop simulation of the current event and skip to the next
   virtual void StopEvent() {}

   /// Stop simulation of the current event and set the abort run flag to true
   virtual void StopRun() {}

   //
   // set methods
   // ------------------------------------------------
   //

   /// Set the maximum step allowed till the particle is in the current medium
   virtual void SetMaxStep(Double_t) {}

   /// Set the maximum number of steps till the particle is in the current medium
   virtual void SetMaxNStep(Int_t) {}

   /// Force the decays of particles to be done with Pythia
   /// and not with the Geant routines.
   virtual void SetUserDecay(Int_t pdg) {}

   /// Force the decay time of the current particle
   virtual void ForceDecayTime(Float_t) {}

   //
   // tracking volume(s)
   // ------------------------------------------------
   //

   /// Return the current volume ID and copy number
   virtual Int_t CurrentVolID(Int_t &copyNo) const { return 0; }

   /// Return the current volume off upward in the geometrical tree
   /// ID and copy number
   virtual Int_t CurrentVolOffID(Int_t off, Int_t &copyNo) const { return 0; }

   /// Return the current volume name
   virtual const char *CurrentVolName() const { return ""; }

   /// Return the current volume off upward in the geometrical tree
   /// name and copy number'
   /// if name=0 no name is returned
   virtual const char *CurrentVolOffName(Int_t off) const { return ""; }

   /// Return the path in geometry tree for the current volume
   virtual const char *CurrentVolPath() { return ""; }

   /// If track is on a geometry boundary, fill the normal vector of the crossing
   /// volume surface and return true, return false otherwise
   virtual Bool_t CurrentBoundaryNormal(Double_t &x, Double_t &y, Double_t &z) const { return true; }

   /// Return the parameters of the current material during transport
   virtual Int_t CurrentMaterial(Float_t &a, Float_t &z, Float_t &dens, Float_t &radl, Float_t &absl) const
   {
      return 0;
   }

   //// Return the number of the current medium
   virtual Int_t CurrentMedium() const { return 0; }
   // new function (to replace GetMedium() const)

   /// Return the number of the current event
   virtual Int_t CurrentEvent() const { return 0; }

   /// Computes coordinates xd in daughter reference system
   /// from known coordinates xm in mother reference system.
   /// - xm    coordinates in mother reference system (input)
   /// - xd    coordinates in daughter reference system (output)
   /// - iflag
   ///   - IFLAG = 1  convert coordinates
   ///   - IFLAG = 2  convert direction cosines
   virtual void Gmtod(Float_t *xm, Float_t *xd, Int_t iflag) {}

   /// The same as previous but in double precision
   virtual void Gmtod(Double_t *xm, Double_t *xd, Int_t iflag) {}

   /// Computes coordinates xm in mother reference system
   /// from known coordinates xd in daughter reference system.
   /// - xd    coordinates in daughter reference system (input)
   /// - xm    coordinates in mother reference system (output)
   /// - iflag
   ///   - IFLAG = 1  convert coordinates
   ///   - IFLAG = 2  convert direction cosines
   virtual void Gdtom(Float_t *xd, Float_t *xm, Int_t iflag) {}

   /// The same as previous but in double precision
   virtual void Gdtom(Double_t *xd, Double_t *xm, Int_t iflag) {}

   /// Return the maximum step length in the current medium
   virtual Double_t MaxStep() const { return 0; }

   /// Return the maximum number of steps allowed in the current medium
   virtual Int_t GetMaxNStep() const { return 0; }

   //
   // get methods
   // tracking particle
   // dynamic properties
   // ------------------------------------------------
   //

   /// Return the current position in the master reference frame of the
   /// track being transported
   virtual void TrackPosition(TLorentzVector &position) const {}

   /// Only return spatial coordinates (as double)
   virtual void TrackPosition(Double_t &x, Double_t &y, Double_t &z) const {}

   /// Only return spatial coordinates (as float)
   virtual void TrackPosition(Float_t &x, Float_t &y, Float_t &z) const {}

   /// Return the direction and the momentum (GeV/c) of the track
   /// currently being transported
   virtual void TrackMomentum(TLorentzVector &momentum) const {}

   /// Return the direction and the momentum (GeV/c) of the track
   /// currently being transported (as double)
   virtual void TrackMomentum(Double_t &px, Double_t &py, Double_t &pz, Double_t &etot) const {}

   /// Return the direction and the momentum (GeV/c) of the track
   /// currently being transported (as float)
   virtual void TrackMomentum(Float_t &px, Float_t &py, Float_t &pz, Float_t &etot) const {}

   /// Return the length in centimeters of the current step (in cm)
   virtual Double_t TrackStep() const { return 0; }

   /// Return the length of the current track from its origin (in cm)
   virtual Double_t TrackLength() const { return 0; }

   /// Return the current time of flight of the track being transported
   virtual Double_t TrackTime() const { return 0; }

   /// Return the energy lost in the current step
   virtual Double_t Edep() const { return 0; }

   /// Return the non-ionising energy lost (NIEL) in the current step
   virtual Double_t NIELEdep() const { return 0; }

   /// Return the current step number
   virtual Int_t StepNumber() const { return 0; }

   /// Get the current weight
   virtual Double_t TrackWeight() const { return 0; }

   /// Get the current polarization
   virtual void TrackPolarization(Double_t &polX, Double_t &polY, Double_t &polZ) const {}

   /// Get the current polarization
   virtual void TrackPolarization(TVector3 &pol) const {}

   //
   // get methods
   // tracking particle
   // static properties
   // ------------------------------------------------
   //

   /// Return the PDG of the particle transported
   virtual Int_t TrackPid() const { return 0; }

   /// Return the charge of the track currently transported
   virtual Double_t TrackCharge() const { return 0; }

   /// Return the mass of the track currently transported
   virtual Double_t TrackMass() const { return 0; }

   /// Return the total energy of the current track
   virtual Double_t Etot() const { return 0; }

   //
   // get methods - track status
   // ------------------------------------------------
   //

   /// Return true when the track performs the first step
   virtual Bool_t IsNewTrack() const { return true; }

   /// Return true if the track is not at the boundary of the current volume
   virtual Bool_t IsTrackInside() const { return true; }

   /// Return true if this is the first step of the track in the current volume
   virtual Bool_t IsTrackEntering() const { return true; }

   /// Return true if this is the last step of the track in the current volume
   virtual Bool_t IsTrackExiting() const { return true; }

   /// Return true if the track is out of the setup
   virtual Bool_t IsTrackOut() const { return true; }

   /// Return true if the current particle has disappeared
   /// either because it decayed or because it underwent
   /// an inelastic collision
   virtual Bool_t IsTrackDisappeared() const { return true; }

   /// Return true if the track energy has fallen below the threshold
   virtual Bool_t IsTrackStop() const { return true; }

   /// Return true if the current particle is alive and will continue to be
   /// transported
   virtual Bool_t IsTrackAlive() const { return true; }

   //
   // get methods - secondaries
   // ------------------------------------------------
   //

   /// Return the number of secondary particles generated in the current step
   virtual Int_t NSecondaries() const { return 0; }

   /// Return the parameters of the secondary track number isec produced
   /// in the current step
   virtual void GetSecondary(Int_t isec, Int_t &particleId, TLorentzVector &position, TLorentzVector &momentum) {}

   /// Return the VMC code of the process that has produced the secondary
   /// particles in the current step
   virtual TMCProcess ProdProcess(Int_t isec) const { return TMCProcess::kPUserDefined; }

   /// Return the array of the VMC code of the processes active in the current
   /// step
   virtual Int_t StepProcesses(TArrayI &proc) const { return 0; }

   /// Return the information about the transport order needed by the stack
   virtual Bool_t SecondariesAreOrdered() const { return true; }

   //
   // ------------------------------------------------
   // Control methods
   // ------------------------------------------------
   //

   /// Initialize MC
   virtual void Init() {}

   /// Initialize MC physics
   virtual void BuildPhysics() {}

   /// Process one event
   virtual void ProcessEvent() {}

   /// Process one event with given eventIs
   virtual void ProcessEvent(Int_t eventId) {}

   /// Process one  run and return true if run has finished successfully,
   /// return false in other cases (run aborted by user)
   virtual Bool_t ProcessRun(Int_t nevent) { return true; }

   /// Additional cleanup after a run can be done here (optional)
   virtual void TerminateRun() {}

   /// Set switches for lego transport
   virtual void InitLego() {}

   /// (In)Activate collecting TGeo tracks
   virtual void SetCollectTracks(Bool_t collectTracks) {}

   /// Return the info if collecting tracks is activated
   virtual Bool_t IsCollectTracks() const { return true; }

private:
   /// An interruptible event can be paused and resumed at any time. It must not
   /// call TVirtualMCApplication::BeginEvent() and ::FinishEvent()
   /// Further, when tracks are popped from the TVirtualMCStack it must be
   /// checked whether these are new tracks or whether they have been
   /// transported up to their current point.
   virtual void ProcessEvent(Int_t eventId, Bool_t isInterruptible) {}

   /// That triggers stopping the transport of the current track without dispatching
   /// to common routines like TVirtualMCApplication::PostTrack() etc.
   virtual void InterruptTrack() {}

   ClassDef(FakeTVirtualMC, 1);
};

#endif // FAKE_TVIRTUAL_MC_H