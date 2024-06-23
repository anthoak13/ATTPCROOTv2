#ifndef HDBSCANRUNNER_HPP
#define HDBSCANRUNNER_HPP

#include "hdbscanParameters.hpp"
#include "hdbscanResult.hpp"
class hdbscanRunner {
public:
   static hdbscanResult run(hdbscanParameters parameters);
};

#endif
