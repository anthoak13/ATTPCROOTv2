#include "AtCircleSeeder.h"

#include "AtHit.h"
#include "AtPatternCircle2D.h"
#include "AtPatternEvent.h"
#include "AtPatternLine.h"
#include "AtPatternTypes.h"
#include "AtSampleConsensus.h"
#include "AtTrack.h"

#include <FairLogger.h>

#include <Math/Point3D.h>
#include <Math/Vector2D.h>
#include <TMath.h>

#include <algorithm>
#include <cmath>
#include <exception>
#include <iostream>
#include <memory>
#include <vector>

void AtPATTERN::AtCircleSeeder::Transform(AtPatternEvent &event)
{
   for (auto &track : event.GetTrackCand()) {
      if (!track.GetHitArray().empty())
         SeedTrack(track);
   }
}

void AtPATTERN::AtCircleSeeder::SeedTrack(AtTrack &track) const
{
   auto &allHits = track.GetHitArray();
   int nTotal = static_cast<int>(allHits.size());
   int nFromFraction = std::max(3, static_cast<int>(nTotal * fRadiusFitFraction));
   int nHitsForFit = std::max(std::min(fMinHitsRadius, nTotal),
                               std::min(nFromFraction, std::min(fMaxHitsRadius, nTotal)));

   std::vector<const AtHit *> hitsForFit;
   int startIdx = std::max(0, nTotal - nHitsForFit);
   for (int i = startIdx; i < nTotal; i++)
      hitsForFit.push_back(allHits[i].get());

   LOG(debug) << "AtCircleSeeder: circle fit using " << hitsForFit.size() << "/" << nTotal << " hits";

   SampleConsensus::AtSampleConsensus ransacSmoothRadius;
   ransacSmoothRadius.SetPatternType(AtPatterns::PatternType::kCircle2D);
   ransacSmoothRadius.SetMinHitsPattern(0.1 * hitsForFit.size());
   ransacSmoothRadius.SetDistanceThreshold(6.0);
   ransacSmoothRadius.SetNumIterations(1000);
   auto circularTracks = ransacSmoothRadius.Solve(hitsForFit).GetTrackCand();

   if (circularTracks.empty())
      return;

   auto &hits = circularTracks.at(0).GetHitArray();

   auto circle = std::make_unique<AtPatterns::AtPatternCircle2D>();
   circle->AtPattern::FitPattern(hitsForFit);

   auto center = circle->GetCenter();
   auto radius = circle->GetRadius();

   track.SetGeoCenter({center.X(), center.Y()});
   track.SetGeoRadius(radius);
   track.SetPattern(circle->Clone());

   circularTracks.at(0).SetPattern(std::move(circle));

   std::vector<double> whit;
   std::vector<double> arclength;

   auto posPCA = hits.at(0)->GetPosition();
   auto refPosOnCircle = posPCA - center;
   auto refAng = refPosOnCircle.Phi();

   std::vector<AtHit> thetaHits;

   int numYCross = 0;
   int lastYSign = (0 < refPosOnCircle.Y()) - (refPosOnCircle.Y() < 0);

   for (size_t i = 0; i < hits.size(); ++i) {
      auto pos = hits.at(i)->GetPosition();
      auto posOnCircle = pos - center;
      auto angleHit = posOnCircle.Phi();

      int currYSign = (0 < posOnCircle.Y()) - (posOnCircle.Y() < 0);
      if (posOnCircle.X() < 0 && lastYSign != currYSign)
         numYCross -= currYSign;
      lastYSign = currYSign;
      angleHit += 2 * M_PI * numYCross;

      whit.push_back(angleHit);
      arclength.push_back(radius * (refAng - whit.at(i)));

      thetaHits.emplace_back(i, hits.at(i)->GetPadNum(),
                             ROOT::Math::XYZPoint(arclength.at(i), pos.Z(), i * 1E-19),
                             hits.at(i)->GetCharge());
   }

   double angle = 0.0;
   double phi0 = 0.0;

   try {
      if (!thetaHits.empty()) {
         SampleConsensus::AtSampleConsensus ransacTheta;
         ransacTheta.SetPatternType(AtPatterns::PatternType::kLine);
         ransacTheta.SetMinHitsPattern(0.1 * thetaHits.size());
         ransacTheta.SetDistanceThreshold(6.0);
         ransacTheta.SetFitPattern(true);
         auto thetaTracks = ransacTheta.Solve(thetaHits).GetTrackCand();

         if (!thetaTracks.empty()) {
            auto line = dynamic_cast<const AtPatterns::AtPatternLine *>(thetaTracks.at(0).GetPattern());
            LOG(info) << "AtCircleSeeder track " << track.GetTrackID() << ": " << track.GetHitArray().size()
                      << " hits, fit " << thetaTracks[0].GetHitArray().size() << "/" << thetaHits.size();

            auto dirTheta = line->GetDirection();
            auto dir2D = ROOT::Math::XYVector(dirTheta.X(), dirTheta.Y()).Unit();

            int sign = dir2D.X() * dir2D.Y() < 0 ? -1 : 1;
            if (dir2D.X() != 0)
               angle = std::acos(sign * std::abs(dir2D.Y())) * TMath::RadToDeg();

            auto temp2 = posPCA - center;
            phi0 = TMath::ATan2(temp2.Y(), temp2.X());
         }
         track.SetGeoTheta(angle * TMath::Pi() / 180.0);
         track.SetGeoPhi(phi0);
      }
   } catch (std::exception &e) {
      std::cout << "AtCircleSeeder: exception: " << e.what() << "\n";
   }
}
