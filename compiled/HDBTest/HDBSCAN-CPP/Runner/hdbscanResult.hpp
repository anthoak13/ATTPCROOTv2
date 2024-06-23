#ifndef HDBSCAN_RESULT_HPP
#define HDBSCAN_RESULT_HPP

#include <vector>

#include "../HdbscanStar/outlierScore.hpp"
using namespace std;
class hdbscanResult {
public:
   vector<int> labels;
   vector<outlierScore> outliersScores;
   vector<double> membershipProbabilities;
   bool hasInfiniteStability;
   hdbscanResult();
   hdbscanResult(vector<int> pLables, vector<outlierScore> pOutlierScores, vector<double> pmembershipProbabilities,
                 bool pHsInfiniteStability);
};
#endif
