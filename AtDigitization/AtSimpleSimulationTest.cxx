#include "AtSimpleSimulation.h"

#include "AtLineChargeModel.h"
#include "AtMCPoint.h"
#include "AtSpaceChargeModel.h"

#include <Math/Point3D.h>
#include <Math/Vector4D.h>
#include <TGeoManager.h>

#include <cmath>
#include <gtest/gtest.h>
#include <memory>

namespace {
using XYZPoint = ROOT::Math::XYZPoint;

/// Moves every point 5 mm in x and leaves y and z alone (in the model's frame, z = 0 at the pad plane).
class ShiftXModel : public AtSpaceChargeModel {
public:
   XYZPoint CorrectSpaceCharge(const XYZPoint &pos) override { return {pos.X() - 5, pos.Y(), pos.Z()}; }
   XYZPoint ApplySpaceCharge(const XYZPoint &pos) override { return {pos.X() + 5, pos.Y(), pos.Z()}; }
   void LoadParameters(const AtDigiPar *) override {}
};

/// Exposes AddHit, which places a point of the current track
class TestSimulation : public AtSimpleSimulation {
public:
   using AtSimpleSimulation::AddHit;
};

class AtSimpleSimulationTest : public ::testing::Test {
protected:
   void SetUp() override
   {
      if (gGeoManager == nullptr)
         new TGeoManager("test", "empty geometry"); // AtSimpleSimulation needs one, AddHit doesn't use it
   }
};
} // namespace

// Simulation z (mm) runs from the window (0) to the pad plane (1000); the space-charge models measure z from
// the pad plane. A point must keep its z through the round trip.
TEST_F(AtSimpleSimulationTest, SpaceChargeKeepsZ)
{
   TestSimulation sim;
   sim.SetSpaceChargeModel(std::make_shared<ShiftXModel>());
   sim.NewEvent();
   sim.AddHit(1, {30, 40, 200}, {0, 0, 0, 0}, 0);

   auto &point = sim.GetMcPoint(0); // Positions in cm
   EXPECT_NEAR(point.GetX() * 10, 35, 1e-9);
   EXPECT_NEAR(point.GetY() * 10, 40, 1e-9);
   EXPECT_NEAR(point.GetZ() * 10, 200, 1e-9);
}

TEST_F(AtSimpleSimulationTest, LineChargePullsTowardBeam)
{
   TestSimulation sim;
   auto model = std::make_shared<AtLineChargeModel>(); // Beam along the z axis
   sim.SetSpaceChargeModel(model);
   sim.NewEvent();
   sim.AddHit(1, {100, 0, 200}, {0, 0, 0, 0}, 0); // 800 mm from the pad plane
   sim.AddHit(1, {25, 0, 200}, {0, 0, 0, 0}, 0);  // Closer than the distortion reaches

   double dist2 = model->getDist2(800);
   auto &far = sim.GetMcPoint(0);
   EXPECT_NEAR(far.GetX() * 10, std::sqrt(100 * 100 - dist2), 1e-9);
   EXPECT_NEAR(far.GetZ() * 10, 200, 1e-9);

   auto &near = sim.GetMcPoint(1);
   EXPECT_NEAR(near.GetX() * 10, 0, 1e-9);
   EXPECT_NEAR(near.GetZ() * 10, 200, 1e-9);
}

// rho^2 of charge drifting dZ changes by lambda / (pi eps0 E) * dZ for a constant line charge, and by half that
// times dZ / L when the charge grows linearly from the pad plane to the window
TEST(AtLineChargeModelTest, DistortionUnits)
{
   AtLineChargeModel model;                                          // lambda 1.54e-8 C/m, E 70000 V/m, L 1 m
   double prefactor = 1.54e-8 / M_PI / 8.85418782e-12 / 70000 * 1e3; // mm
   EXPECT_NEAR(model.getDist2(800), prefactor * 800 * 0.8 / 2, 1e-6);
   model.SetConstantCharge();
   EXPECT_NEAR(model.getDist2(800), prefactor * 800, 1e-6);
}
