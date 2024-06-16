#include "AtTpc.h"

#include "AtStack.h"

#include "FairVolume.h"
#include "FakeTVirtualMC.h"
#include "FakeTVirtualMCApplication.h"

#include <gtest/gtest.h>

class AtTpcTest : public ::testing::Test {
protected:
   AtTpcTest()
   {
      fFakeApp = new FakeTVirtualMCApplication();
      fFakeMC = new FakeTVirtualMC();
      stack = new AtStack();
      fFakeMC->SetStack(stack);
   }
   ~AtTpcTest() override
   {
      delete fFakeApp;
      delete fFakeMC;
      delete stack;
   }

   // These should be created and destoryed for each test (get around their "singleton" nature)
   FakeTVirtualMCApplication *fFakeApp;
   FakeTVirtualMC *fFakeMC;
   AtStack *stack;
   AtTpc tpc; // This is the unit under test
};

TEST_F(AtTpcTest, ProcessHits_NoVolume)
{
   EXPECT_FALSE(tpc.ProcessHits());
}

TEST_F(AtTpcTest, ProcessHits_WithVolume)
{
   FairVolume v;
   EXPECT_TRUE(tpc.ProcessHits(&v));
}
