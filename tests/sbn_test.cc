#include <array>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

#include <gtest/gtest.h>
#include "shapefil.h"

namespace fs = std::filesystem;

namespace
{
static const auto kTestData = fs::path{"shape_eg_data"};

TEST(SBNOpenDiskTree, OpenDoesNotExist_rb)
{
    const auto handle = SBNOpenDiskTree("/does/not/exist.sbn", nullptr);
    ASSERT_EQ(nullptr, handle);
}

// Note: this shapefile contains only 4 shapes. That is not exercising
// the SBN code much...
TEST(SBNOpenDiskTree, NominalSmallSBN)
{
    const auto filename = kTestData / "CoHI_GCS12.sbn";
    const auto handle = SBNOpenDiskTree(filename.string().c_str(), nullptr);
    ASSERT_NE(nullptr, handle);

    {
        double adfMin[] = {-180, -90};
        double adfMax[] = {180, 90};
        int nShapeCount = 0;
        int *panIds = SBNSearchDiskTree(handle, adfMin, adfMax, &nShapeCount);
        EXPECT_EQ(nShapeCount, 4);
        EXPECT_NE(panIds, nullptr);
        if (nShapeCount == 0)
        {
            EXPECT_EQ(panIds[0], 0);
            EXPECT_EQ(panIds[1], 1);
            EXPECT_EQ(panIds[2], 2);
            EXPECT_EQ(panIds[3], 3);
        }

        SBNSearchFreeIds(panIds);
    }

    {
        double adfMin[] = {170, -90};
        double adfMax[] = {180, 90};
        int nShapeCount = 0;
        int *panIds = SBNSearchDiskTree(handle, adfMin, adfMax, &nShapeCount);
        EXPECT_EQ(nShapeCount, 0);
        SBNSearchFreeIds(panIds);
    }

    {
        double adfMin[] = {-156.05722, 19.742536};
        double adfMax[] = {-156.05722, 19.742536};
        int nShapeCount = 0;
        int *panIds = SBNSearchDiskTree(handle, adfMin, adfMax, &nShapeCount);
        EXPECT_EQ(nShapeCount, 1);
        if (nShapeCount == 1)
        {
            EXPECT_EQ(panIds[0], 0);
        }
        SBNSearchFreeIds(panIds);
    }

    {
        double adfMin[] = {-158.277679, 21.578789};
        double adfMax[] = {-158.277679, 21.578789};
        int nShapeCount = 0;
        int *panIds = SBNSearchDiskTree(handle, adfMin, adfMax, &nShapeCount);
        EXPECT_EQ(nShapeCount, 1);
        if (nShapeCount == 1)
        {
            EXPECT_EQ(panIds[0], 1);
        }
        SBNSearchFreeIds(panIds);
    }

    SBNCloseDiskTree(handle);
}

}  // namespace

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
