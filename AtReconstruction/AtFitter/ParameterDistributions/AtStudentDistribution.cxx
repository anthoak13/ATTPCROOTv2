#include "AtStudentDistribution.h"

#include "AtRandom.h"

namespace MCFitter {

AtStudentDistribution::AtStudentDistribution(double mean, double spread) : AtParameterDistribution(mean, spread) {}

double AtStudentDistribution::SampleSpread()
{
   // N.B. We recreate the distribution (which is cheap) each call to avoid data races when multithreaded
   std::student_t_distribution<> distro{fDistro.param()};
   AtTools::Random::Engine engine(AtTools::Random::Get());
   return distro(engine);
}

} // namespace MCFitter
