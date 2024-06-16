#include <TVirtualMCApplication.h>

class FakeTVirtualMCApplication : public TVirtualMCApplication {
public:
   FakeTVirtualMCApplication() = default;

   /// Construct user geometry
   virtual void ConstructGeometry() {}

   /// Misalign user geometry (optional)
   virtual Bool_t MisalignGeometry() { return kFALSE; }

   /// Define parameters for optical processes (optional)
   virtual void ConstructOpGeometry() {}

   /// Define sensitive detectors (optional)
   virtual void ConstructSensitiveDetectors() {}

   /// Initialize geometry
   /// (Usually used to define sensitive volumes IDs)
   virtual void InitGeometry() {}

   /// Add user defined particles (optional)
   virtual void AddParticles() {}

   /// Add user defined ions (optional)
   virtual void AddIons() {}

   /// Generate primary particles
   virtual void GeneratePrimaries() {}

   /// Define actions at the beginning of the event
   virtual void BeginEvent() {}

   /// Define actions at the beginning of the primary track
   virtual void BeginPrimary() {}

   /// Define actions at the beginning of each track
   virtual void PreTrack() {}

   /// Define action at each step
   virtual void Stepping() {}

   /// Define actions at the end of each track
   virtual void PostTrack() {}

   /// Define actions at the end of the primary track
   virtual void FinishPrimary() {}

   /// Define actions at the end of the event before calling SD's end of the event
   virtual void EndOfEvent() {}

   /// Define actions at the end of the event
   virtual void FinishEvent() {}
};