#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>

#include <gtest/gtest.h>
#include "shapefil.h"

namespace fs = std::filesystem;

namespace
{

static const auto kTestData = fs::path{"shape_eg_data"};

static bool SetContents(const fs::path &file_name, std::string_view content)
{
    std::ofstream file(file_name);
    if (!file.is_open())
        return false;
    file << content;
    file.close();
    return true;
}

TEST(DBFOpenTest, OpenDoesNotExist_rb)
{
    const auto handle = DBFOpen("/does/not/exist.dbf", "rb");
    EXPECT_EQ(nullptr, handle);
}

TEST(DBFOpenTest, OpenDoesNotExist_rb_plus)
{
    const auto handle = DBFOpen("/does/not/exist2.dbf", "rb+");
    EXPECT_EQ(nullptr, handle);
}

TEST(DBFOpenTest, OpenUnexpectedFormat)
{
    const auto filename = kTestData / "README.md";
    const auto handle = DBFOpen(filename.string().c_str(), "rb");
    EXPECT_EQ(nullptr, handle);
}

TEST(DBFOpenTest, OpenExisting)
{
    const auto filename = kTestData / "anno.dbf";
    const auto handle = DBFOpen(filename.string().c_str(), "rb");
    EXPECT_NE(nullptr, handle);
    DBFClose(handle);
}

TEST(DBFCreateTest, CreateDoesNotExist)
{
    const auto handle = DBFCreate("/does/not/exist");
    EXPECT_EQ(nullptr, handle);
}

TEST(DBFCreateTest, CreateAlreadyExisting)
{
    const auto filename = kTestData / "in-the-way.dbf";
    fs::copy(kTestData / "anno.dbf", filename);
    EXPECT_TRUE(SetContents(filename, "some content"));
    const auto handle = DBFCreate(filename.string().c_str());
    // TODO(schwehr): Seems like a bug to overwrite an existing.
    EXPECT_NE(nullptr, handle);
    DBFClose(handle);
    const auto size = fs::file_size(filename);
    EXPECT_EQ(34, size);
    fs::remove(filename);
}

TEST(DBFCreateTest, CreateAndClose)
{
    const auto filename = kTestData / "empty.dbf";
    const auto handle = DBFCreate(filename.string().c_str());
    DBFClose(handle);
    const auto size = fs::file_size(filename);
    EXPECT_EQ(34, size);
    fs::remove(filename);
}

}  // namespace

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
