#include "AtTpc.h"

#include "FakeTVirtualMC.h"
#include "FakeTVirtualMCApplication.h"

#include <gtest/gtest.h>

class AtTpcTest : public ::testing::Test {
protected:
   AtTpcTest()
   {
      fFakeApp = new FakeTVirtualMCApplication();
      fFakeMC = new FakeTVirtualMC();
   }
   ~AtTpcTest() override
   {
      delete fFakeApp;
      delete fFakeMC;
   }

   // These should be created and destoryed for each test (get around their "singleton" nature)
   FakeTVirtualMCApplication *fFakeApp;
   FakeTVirtualMC *fFakeMC;
   AtTpc tpc; // This is the unit under test
};

TEST_F(AtTpcTest, InitializeTest)
{
   EXPECT_TRUE(true);
}

TEST_F(AtTpcTest, ProcessHitsTest)
{

   EXPECT_TRUE(true);
}
