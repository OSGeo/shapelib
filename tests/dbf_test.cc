#include <array>
#include <chrono>
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

static bool SetContents(const fs::path &filename, std::string_view content)
{
    std::ofstream file(filename);
    if (!file.is_open())
        return false;
    file << content;
    file.close();
    return true;
}

TEST(DBFOpenTest, OpenDoesNotExist_rb)
{
    const auto handle = DBFOpen("/does/not/exist.dbf", "rb");
    ASSERT_EQ(nullptr, handle);
}

TEST(DBFOpenTest, OpenDoesNotExist_rb_plus)
{
    const auto handle = DBFOpen("/does/not/exist2.dbf", "rb+");
    ASSERT_EQ(nullptr, handle);
}

TEST(DBFOpenTest, OpenUnexpectedFormat)
{
    const auto filename = kTestData / "README.md";
    const auto handle = DBFOpen(filename.string().c_str(), "rb");
    ASSERT_EQ(nullptr, handle);
}

TEST(DBFOpenTest, OpenExisting)
{
    const auto filename = kTestData / "anno.dbf";
    const auto handle = DBFOpen(filename.string().c_str(), "rb");
    ASSERT_NE(nullptr, handle);
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
    ASSERT_NE(nullptr, handle);
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

static auto GenerateUniqueFilename(std::string_view ext) -> auto
{
    const auto now = std::chrono::system_clock::now();
    const auto timestamp =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch())
            .count();
    std::ostringstream oss;
    oss << "test_" << timestamp << ext;
    return oss.str();
}

TEST(DBFCreateTest, CloneEmpty)
{
    const auto filename =
        fs::temp_directory_path() / GenerateUniqueFilename(".clone.dbf");
    const auto handle = [&]
    {
        const auto handle =
            DBFOpen(fs::path(kTestData / "anno.dbf").string().c_str(), "rb");
        EXPECT_NE(nullptr, handle);
        const auto clonedhandle =
            DBFCloneEmpty(handle, filename.string().c_str());
        DBFClose(handle);
        return clonedhandle;
    }();
    ASSERT_NE(nullptr, handle);
    const auto fieldcount = DBFGetFieldCount(handle);
    EXPECT_EQ(10, fieldcount);
    const auto recordcount = DBFGetRecordCount(handle);
    EXPECT_EQ(0, recordcount);
    DBFClose(handle);
    const auto size = fs::file_size(filename);
    EXPECT_EQ(354, size);
    fs::remove(filename);
}

static auto WriteDate(const fs::path &filename,
                      const std::unique_ptr<const SHPDate> &date)
{
    const auto handle = DBFCreate(filename.string().c_str());
    ASSERT_NE(nullptr, handle);
    const auto fid = DBFAddField(handle, "date", FTDate, 8, 0);
    EXPECT_GE(fid, 0);
    const auto success = DBFWriteDateAttribute(handle, 0, fid, date.get());
    EXPECT_TRUE(success);
    const auto recordcount = DBFGetRecordCount(handle);
    EXPECT_EQ(1, recordcount);
    DBFClose(handle);
}

static auto ReadDate(const fs::path &filename) -> auto
{
    const auto handle = DBFOpen(filename.string().c_str(), "rb");
    EXPECT_NE(nullptr, handle);
    const auto fieldcount = DBFGetFieldCount(handle);
    EXPECT_EQ(1, fieldcount);
    const auto fieldname =
        std::make_unique<std::array<char, XBASE_FLDNAME_LEN_READ + 1>>();
    int width, decimals;
    const auto fieldtype =
        DBFGetFieldInfo(handle, 0, fieldname->data(), &width, &decimals);
    EXPECT_EQ(FTDate, fieldtype);
    EXPECT_EQ(0, std::strcmp("date", fieldname->data()));
    EXPECT_EQ(8, width);
    EXPECT_EQ(0, decimals);
    const auto recordcount = DBFGetRecordCount(handle);
    EXPECT_EQ(1, recordcount);
    const auto date = DBFReadDateAttribute(handle, 0, 0);
    DBFClose(handle);
    return std::make_unique<const SHPDate>(date);
}

TEST(DBFFieldTest, SetAndGetDateToday)
{
    const auto filename =
        fs::temp_directory_path() / GenerateUniqueFilename(".dbf");
    const auto today = []
    {
        const auto calendarTime = std::chrono::system_clock::to_time_t(
            std::chrono::system_clock::now());
        const auto tlocal = std::localtime(&calendarTime);
        return std::make_unique<const SHPDate>(SHPDate{
            1900 + tlocal->tm_year, tlocal->tm_mon + 1, tlocal->tm_mday});
    }();
    WriteDate(filename, today);
    const auto size = fs::file_size(filename);
    EXPECT_EQ(75, size);
    const auto date = ReadDate(filename);
    EXPECT_EQ(today->year, date->year);
    EXPECT_EQ(today->month, date->month);
    EXPECT_EQ(today->day, date->day);
    fs::remove(filename);
}

TEST(DBFFieldTest, SetAndGetDateInvalid)
{
    const auto filename =
        fs::temp_directory_path() / GenerateUniqueFilename(".dbf");
    WriteDate(filename, std::make_unique<const SHPDate>());
    const auto size = fs::file_size(filename);
    EXPECT_EQ(75, size);
    const auto date = ReadDate(filename);
    EXPECT_EQ(0, date->year);
    EXPECT_EQ(0, date->month);
    EXPECT_EQ(0, date->day);
    fs::remove(filename);
}

}  // namespace

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
