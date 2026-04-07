#include "AtTrackTransformer.h"

#include "AtTrackClusterBuilder.h"
#include "AtHit.h"
#include "AtHitCluster.h"
#include "AtTrack.h"

#include <TMath.h>
#include <TVector3.h>

#include <memory>
#include <vector>

AtTools::AtTrackTransformer::AtTrackTransformer() = default;
AtTools::AtTrackTransformer::~AtTrackTransformer() = default;

void AtTools::AtTrackTransformer::SetDiffusionParams(double coefT, double coefL, double driftVel, double tbTime,
                                                     double padResXY)
{
   fCoefT = coefT;
   fCoefL = coefL;
   fDriftVel = driftVel;
   fTBTime = tbTime;
   if (padResXY > 0)
      fPadResXY = padResXY;
}
using XYZPoint = ROOT::Math::XYZPoint;

void AtTools::AtTrackTransformer::ClusterizeSmooth3D(AtTrack &track, Float_t radius, Float_t distance)
{
   AtTrackClusterBuilderConfig cfg;
   cfg.coefT = fCoefT;
   cfg.coefL = fCoefL;
   cfg.driftVel = fDriftVel;
   cfg.samplingRate = fTBTime;
   cfg.padResXY = fPadResXY;
   cfg.padResZ = fPadResXY * 1.5;
   cfg.covarianceMode = fCovarianceMode;
   AtTools::ClusterizeSmooth3D(track, radius, distance, cfg);
}

void AtTools::AtTrackTransformer::ClusterizeByGroup(AtTrack &track, int hitsPerCluster)
{
   auto hitArray = track.GetHitArrayObject(); // copy as value (vector<AtHit>)
   if (hitArray.empty())
      return;

   AtTrackClusterBuilderConfig builderConfig;
   builderConfig.coefT = fCoefT;
   builderConfig.coefL = fCoefL;
   builderConfig.driftVel = fDriftVel;
   builderConfig.samplingRate = fTBTime;
   builderConfig.padResXY = fPadResXY;
   builderConfig.padResZ = fPadResXY * 1.5;
   builderConfig.covarianceMode = fCovarianceMode;
   AtTrackClusterBuilder clusterBuilder(builderConfig);

   int nHits = hitArray.size();
   int clusterID = 0;

   for (int start = 0; start < nHits; start += hitsPerCluster) {
      int end = std::min(start + hitsPerCluster, nHits);
      int groupSize = end - start;
      if (groupSize < 2)
         continue;

      std::vector<AtHit> clusterHits;
      clusterHits.reserve(groupSize);

      for (int i = start; i < end; i++) {
         clusterHits.push_back(hitArray[i]);
      }

      auto hitCluster = clusterBuilder.BuildCluster(clusterHits, clusterID);
      if (!hitCluster)
         continue;

      clusterID++;
      track.AddClusterHit(hitCluster);
   }
}

Bool_t AtTools::AtTrackTransformer::FindVertexTrack(AtTrack *trA, AtTrack *trB)
{
   // Determination of first hit distance. NB: Assuming both tracks have the same angle sign
   Double_t vertexA = 0.0;
   Double_t vertexB = 0.0;
   if (trA->GetGeoTheta() * TMath::RadToDeg() < 90) {
      auto iniClusterA = trA->GetHitClusterArray()->back();
      auto iniClusterB = trB->GetHitClusterArray()->back();
      vertexA = 1000.0 - iniClusterA.GetPosition().Z();
      vertexB = 1000.0 - iniClusterB.GetPosition().Z();
   } else if (trA->GetGeoTheta() * TMath::RadToDeg() > 90) {
      auto iniClusterA = trA->GetHitClusterArray()->front();
      auto iniClusterB = trB->GetHitClusterArray()->front();
      vertexA = iniClusterA.GetPosition().Z();
      vertexB = iniClusterB.GetPosition().Z();
   }

   return vertexA < vertexB;
}

const std::tuple<Double_t, Double_t> AtTools::AtTrackTransformer::GetPIDFromHits(AtTrack &track, Double_t theta)
{

   Double_t dedx = 0.0;
   Double_t eloss = 0.0;

   auto hitArray = &track.GetHitArray();
   std::size_t cnt = 0;

   if (theta < 90) {

      auto it = hitArray->rbegin();
      while (it != hitArray->rend()) {

         if (((Float_t)cnt / (Float_t)hitArray->size()) > 0.8)
            break;

         eloss += (*it).get()->GetCharge();
         dedx += (*it).get()->GetCharge();
         // std::cout<<(*it).GetCharge()<<"\n";
         it++;
         ++cnt;
      }
   } else if (theta > 90) {

      eloss += hitArray->at(0).get()->GetCharge();

      cnt = 1;
      for (auto iHitClus = 1; iHitClus < hitArray->size(); ++iHitClus) {

         if (((Float_t)cnt / (Float_t)hitArray->size()) > 0.8)
            break;

         eloss += hitArray->at(iHitClus).get()->GetCharge();
         dedx += hitArray->at(iHitClus).get()->GetCharge();
         // std::cout<<len<<" - "<<eloss<<" - "<<hitClusterArray->at(iHitClus).GetCharge()<<"\n";
         ++cnt;
      }
   }

   eloss /= cnt;

   return std::forward_as_tuple(dedx, eloss);
}

Bool_t AtTools::AtTrackTransformer::MergeTracks(std::vector<AtTrack *> *trackCandSource,
                                                std::vector<AtTrack> *trackDest, Bool_t enableSingleVertexTrack,
                                                Double_t clusterRadius, Double_t clusterDistance)
{

   Bool_t toMerge = kFALSE;

   Int_t addHitCnt = 0;
   // Find the track closer to vertex
   std::sort(trackCandSource->begin(), trackCandSource->end(),
             [this](AtTrack *trA, AtTrack *trB) { return FindVertexTrack(trA, trB); });

   // Track stitching from vertex
   AtTrack *vertexTrack = *trackCandSource->begin();

   if (enableSingleVertexTrack) {

      // Mark all tracks as merged
      for (auto track : *trackCandSource)
         track->SetIsMerged(kTRUE);

      trackDest->push_back(*vertexTrack);
      return true;
   }

   // Check if the candidate vertex track was merged
   if (vertexTrack->GetIsMerged())
      return kFALSE;
   else
      vertexTrack->SetIsMerged(kTRUE);

   // If enabled, choose only the track closest to vertex (i.e. first one of the collection of candidates)
   // TODO: Select by number of points

   for (auto it = trackCandSource->begin() + 1; it != trackCandSource->end(); ++it) {
      // NB: These tracks were previously marked to merge. If merging fails they should be discarded.
      AtTrack *trackToMerge = *(it);
      toMerge = kFALSE;

      // Skip trackes flagged as merged
      if (!trackToMerge->GetIsMerged()) {
         trackToMerge->SetIsMerged(kTRUE);
      } else
         continue;

      Double_t endVertexZ = 0.0;
      Double_t iniMergeZ = 0.0;
      std::cout << " Trying to merge ... "
                << "\n";
      std::cout << " Vertex track " << vertexTrack->GetTrackID() << " - Track to Merge " << trackToMerge->GetTrackID()
                << "\n";
      // Check relative position between end and begin of each track using Hit Clusters
      std::cout << " Vertex angle " << vertexTrack->GetGeoTheta() * TMath::RadToDeg() << "\n";
      if (vertexTrack->GetGeoTheta() * TMath::RadToDeg() < 90) {
         auto endClusterVertex = vertexTrack->GetHitClusterArray()->front();
         auto iniClusterMerge = trackToMerge->GetHitClusterArray()->back();
         // Check separation and relative distance
         endVertexZ = 1000.0 - endClusterVertex.GetPosition().Z();
         iniMergeZ = 1000.0 - iniClusterMerge.GetPosition().Z();

         Double_t distance = std::sqrt((iniClusterMerge.GetPosition() - endClusterVertex.GetPosition()).Mag2());
         // std::cout << " Distance between tracks " << distance << "\n";
         // std::cout << " Ini Merge " << iniMergeZ << " - endVertexZ " << endVertexZ << "\n";
         if (((iniMergeZ + 10.0) > endVertexZ) && distance < 200) {
            toMerge = kTRUE;
         }

      } else if (vertexTrack->GetGeoTheta() * TMath::RadToDeg() > 90) {
         auto endClusterVertex = vertexTrack->GetHitClusterArray()->back();
         auto iniClusterMerge = trackToMerge->GetHitClusterArray()->front();
         // Check separation and relative distance
         endVertexZ = endClusterVertex.GetPosition().Z();
         iniMergeZ = iniClusterMerge.GetPosition().Z();
         Double_t distance = std::sqrt((iniClusterMerge.GetPosition() - endClusterVertex.GetPosition()).Mag2());
         // std::cout<<" Distance between tracks "<<distance<<"\n";
         // std::cout<<" Ini Merge "<<iniMergeZ<<" - endVertexZ "<<endVertexZ<<"\n";
         if (((iniMergeZ + 10.0) > endVertexZ) &&
             distance < 100) { // NB: Distance between parts of the backward tracks is more critical
            toMerge = kTRUE;
         }
      }

      if (toMerge) {

         std::cout << " --- Merging Succeeded! Vertex track " << vertexTrack->GetTrackID() << " - Track to Merge "
                   << trackToMerge->GetTrackID() << "\n";
         for (const auto &hit : trackToMerge->GetHitArray()) {

            vertexTrack->AddHit(hit->Clone()); // TODO: Look at code and see if this can be a move instead of a copy
            ++addHitCnt;
         }

         // Reclusterize after merging
         vertexTrack->SortHitArrayTime();
         vertexTrack->ResetHitClusterArray();
         ClusterizeSmooth3D(
            *vertexTrack, clusterRadius,
            clusterDistance); // NB: It can be removed if we force reclusterization for any track in the mina program

         // TODO: Check if phi recalculatio is needed

      } else {
         std::cout << " --- Merging Failed ! Vertex track " << vertexTrack->GetTrackID() << " - Track to Merge "
                   << trackToMerge->GetTrackID() << "\n";
      }
   }

   trackDest->push_back(*vertexTrack);

   return toMerge;
}
