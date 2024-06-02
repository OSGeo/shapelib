#include <cstring>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>

#include <gtest/gtest.h>
#include "shapefil.h"

namespace fs = std::filesystem;

namespace
{

static const auto kTestData = fs::path{"shape_eg_data"};

TEST(SHPOpenTest, OpenDoesNotExist_rb)
{
    const auto handle = SHPOpen("/does/not/exist.shp", "rb");
    ASSERT_EQ(nullptr, handle);
}

TEST(SHPOpenTest, OpenDoesNotExist_rb_plus)
{
    const auto handle = SHPOpen("/does/not/exist2.shp", "rb+");
    ASSERT_EQ(nullptr, handle);
}

TEST(SHPOpenTest, OpenUnexpectedFormat)
{
    const auto filename = kTestData / "README.md";
    const auto handle = SHPOpen(filename.string().c_str(), "rb");
    ASSERT_EQ(nullptr, handle);
}

TEST(SHPOpenTest, OpenExisting)
{
    const auto filename = kTestData / "anno.shp";
    const auto handle = SHPOpen(filename.string().c_str(), "rb");
    ASSERT_NE(nullptr, handle);
    SHPClose(handle);
}

TEST(SHPOpenTest, OpenExistingSHX)
{
    const auto filename = kTestData / "anno.shx";
    const auto handle = SHPOpen(filename.string().c_str(), "rb");
    ASSERT_NE(nullptr, handle);
    SHPClose(handle);
}

TEST(SHPOpenTest, OpenExistingWithRestoreSHX)
{
    const auto filename = kTestData / "anno.shx";
    fs::remove(filename);
    const auto sHooks = std::make_unique<SAHooks>();
    SASetupDefaultHooks(sHooks.get());
    auto handle =
        SHPOpenLLEx(filename.string().c_str(), "rb", sHooks.get(), false);
    ASSERT_EQ(nullptr, handle);
    handle = SHPOpenLLEx(filename.string().c_str(), "rb", sHooks.get(), true);
    ASSERT_NE(nullptr, handle);
    SHPClose(handle);
    EXPECT_TRUE(fs::exists(filename));
}

TEST(SHPCreateTest, CreateDoesNotExist)
{
    const auto handle = SHPCreate("/does/not/exist", 42);
    EXPECT_EQ(nullptr, handle);
}

TEST(SHPCreateTest, CreateAndClose)
{
    const auto filename = kTestData / "empty.shp";
    const auto handle = SHPCreate(filename.string().c_str(), 1234);
    SHPClose(handle);
    for (const auto &_filename : {filename, kTestData / "empty.shx"})
    {
        const auto size = fs::file_size(_filename);
        EXPECT_EQ(100, size);
        fs::remove(_filename);
    }
}

}  // namespace

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
