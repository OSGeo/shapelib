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
                      const std::unique_ptr<const SHPDate> &date) -> auto
{
    const auto handle = DBFCreate(filename.string().c_str());
    EXPECT_NE(nullptr, handle);
    const auto fid = DBFAddField(handle, "date", FTDate, 8, 0);
    EXPECT_GE(fid, 0);
    const auto success = DBFWriteDateAttribute(handle, 0, fid, date.get());
    DBFClose(handle);
    return success;
}

static auto ReadDate(const fs::path &filename, int expectIsNull,
                     int expectWidth) -> auto
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
    EXPECT_EQ(expectWidth, width);
    EXPECT_EQ(0, decimals);
    const auto isNull = DBFIsAttributeNULL(handle, 0, 0);
    EXPECT_EQ(expectIsNull, isNull);
    const auto recordcount = DBFGetRecordCount(handle);
    EXPECT_EQ(1, recordcount);
    const auto date = DBFReadDateAttribute(handle, 0, 0);
    DBFClose(handle);
    return std::make_unique<const SHPDate>(date);
}

static auto WriteLogical(const fs::path &filename, const char logical) -> auto
{
    const auto handle = DBFCreate(filename.string().c_str());
    EXPECT_NE(nullptr, handle);
    const auto fid = DBFAddField(handle, "logical", FTLogical, 1, 0);
    EXPECT_GE(fid, 0);
    const auto success = DBFWriteLogicalAttribute(handle, 0, fid, logical);
    DBFClose(handle);
    return success;
}

static auto ReadLogical(const fs::path &filename) -> auto
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
    EXPECT_EQ(FTLogical, fieldtype);
    EXPECT_EQ(0, std::strcmp("logical", fieldname->data()));
    EXPECT_EQ(1, width);
    EXPECT_EQ(0, decimals);
    const auto recordcount = DBFGetRecordCount(handle);
    EXPECT_EQ(1, recordcount);
    const auto logical = std::string(DBFReadLogicalAttribute(handle, 0, 0));
    DBFClose(handle);
    return logical;
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
    const auto success = WriteDate(filename, today);
    EXPECT_TRUE(success);
    const auto size = fs::file_size(filename);
    EXPECT_EQ(75, size);
    const auto date = ReadDate(filename, 0, 8);
    EXPECT_EQ(today->year, date->year);
    EXPECT_EQ(today->month, date->month);
    EXPECT_EQ(today->day, date->day);
    fs::remove(filename);
}

TEST(DBFFieldTest, SetAndGetDateEmpty)
{
    const auto filename =
        fs::temp_directory_path() / GenerateUniqueFilename(".dbf");
    const auto success = WriteDate(filename, std::make_unique<const SHPDate>());
    EXPECT_TRUE(success);
    const auto size = fs::file_size(filename);
    EXPECT_EQ(75, size);
    const auto date = ReadDate(filename, 1, 8);
    EXPECT_EQ(0, date->year);
    EXPECT_EQ(0, date->month);
    EXPECT_EQ(0, date->day);
    fs::remove(filename);
}

TEST(DBFFieldTest, SetAndGetDateNull)
{
    const auto filename =
        fs::temp_directory_path() / GenerateUniqueFilename(".dbf");
    for (int width = 1; width <= XBASE_FLD_MAX_WIDTH; ++width)
    {
        const auto success = [&filename, &width]
        {
            const auto handle = DBFCreate(filename.string().c_str());
            EXPECT_NE(nullptr, handle);
            const auto fid = DBFAddField(handle, "date", FTDate, width, 0);
            EXPECT_GE(fid, 0);
            const auto success = DBFWriteNULLAttribute(handle, 0, 0);
            DBFClose(handle);
            return success;
        }();
        EXPECT_TRUE(success);
        const auto size = fs::file_size(filename);
        EXPECT_EQ(67 + width, size);
        const auto date = ReadDate(filename, 1, width);
        EXPECT_EQ(0, date->year);
        EXPECT_EQ(0, date->month);
        EXPECT_EQ(0, date->day);
        fs::remove(filename);
    }
}

TEST(DBFFieldTest, SetAndGetDateEmptyString)
{
    const auto filename =
        fs::temp_directory_path() / GenerateUniqueFilename(".dbf");
    const auto success = [&filename]
    {
        const auto handle = DBFCreate(filename.string().c_str());
        EXPECT_NE(nullptr, handle);
        const auto fid = DBFAddField(handle, "date", FTDate, 8, 0);
        EXPECT_GE(fid, 0);
        constexpr const auto emptyString = "";
        const auto success =
            DBFWriteAttributeDirectly(handle, 0, 0, emptyString);
        DBFClose(handle);
        return success;
    }();
    EXPECT_TRUE(success);
    const auto size = fs::file_size(filename);
    EXPECT_EQ(75, size);
    const auto date = ReadDate(filename, 1, 8);
    EXPECT_EQ(0, date->year);
    EXPECT_EQ(0, date->month);
    EXPECT_EQ(0, date->day);
    fs::remove(filename);
}

TEST(DBFFieldTest, SetAndGetLogicalTrue)
{
    const auto filename =
        fs::temp_directory_path() / GenerateUniqueFilename(".dbf");
    const auto success = WriteLogical(filename, 'T');
    EXPECT_TRUE(success);
    const auto size = fs::file_size(filename);
    EXPECT_EQ(68, size);
    const auto logical = ReadLogical(filename);
    EXPECT_EQ(std::string("T"), logical);
    fs::remove(filename);
}

TEST(DBFFieldTest, SetAndGetLogicalFalse)
{
    const auto filename =
        fs::temp_directory_path() / GenerateUniqueFilename(".dbf");
    const auto success = WriteLogical(filename, 'F');
    EXPECT_TRUE(success);
    const auto size = fs::file_size(filename);
    EXPECT_EQ(68, size);
    const auto logical = ReadLogical(filename);
    EXPECT_EQ(std::string("F"), logical);
    fs::remove(filename);
}

TEST(DBFFieldTest, SetLogicalInvalid)
{
    const auto filename =
        fs::temp_directory_path() / GenerateUniqueFilename(".dbf");
    const auto success = WriteLogical(filename, '0');
    EXPECT_FALSE(success);
    fs::remove(filename);
}

TEST(DBFFieldTest, SetAndGetDouble)
{
    const auto filename =
        fs::temp_directory_path() / GenerateUniqueFilename(".dbf");
    constexpr const double value = 1623819823.809;
    {
        const auto handle = DBFCreate(filename.string().c_str());
        EXPECT_NE(nullptr, handle);
        const auto fid = DBFAddField(handle, "double", FTDouble, 14, 4);
        EXPECT_GE(fid, 0);
        const auto success = DBFWriteDoubleAttribute(handle, 0, 0, value);
        EXPECT_TRUE(success);
        DBFClose(handle);
    }
    {
        const auto handle = DBFOpen(filename.string().c_str(), "r");
        EXPECT_NE(nullptr, handle);
        EXPECT_EQ(value, DBFReadDoubleAttribute(handle, 0, 0));
        DBFClose(handle);
    }
    fs::remove(filename);
}

static auto WriteDuplicateFields(const fs::path &filename) -> auto
{
    const auto handle = DBFCreate(filename.string().c_str());
    EXPECT_NE(nullptr, handle);
    const auto fid1 = DBFAddField(handle, "field", FTDate, 8, 0);
    EXPECT_GE(fid1, 0);
    const auto fid2 = DBFAddField(handle, "field", FTDate, 8, 0);
    EXPECT_GE(fid2, 0);
    const auto fid3 = DBFAddField(handle, "field", FTDouble, 8, 5);
    EXPECT_GE(fid3, 0);
    const auto fid4 = DBFAddField(handle, "field", FTDouble, 8, 10);
    EXPECT_GE(fid4, 0);
    const auto fid5 = DBFAddField(handle, "field", FTInteger, 8, 0);
    EXPECT_GE(fid5, 0);
    const auto fid6 = DBFAddField(handle, "field", FTInteger, 8, 0);
    EXPECT_GE(fid6, 0);
    const auto fid7 = DBFAddField(handle, "field", FTLogical, 1, 0);
    EXPECT_GE(fid7, 0);
    const auto fid8 = DBFAddField(handle, "field", FTLogical, 1, 0);
    EXPECT_GE(fid8, 0);
    const auto fid9 = DBFAddField(handle, "field", FTString, 5, 0);
    EXPECT_GE(fid9, 0);
    const auto fid0 = DBFAddField(handle, "field", FTString, 10, 0);
    EXPECT_GE(fid0, 0);
    const auto nFields = DBFGetFieldCount(handle);
    EXPECT_EQ(nFields, 10);
    DBFClose(handle);
}

static auto ReadDuplicateFields(const fs::path &filename) -> auto
{
    const auto handle = DBFOpen(filename.string().c_str(), "r");
    EXPECT_NE(nullptr, handle);
    const auto nFields = DBFGetFieldCount(handle);
    EXPECT_EQ(nFields, 10);
    const auto fid = DBFGetFieldIndex(handle, "field");
    EXPECT_EQ(fid, 0);
    DBFClose(handle);
}

TEST(DBFFieldTest, AddDuplicateField)
{
    const auto filename =
        fs::temp_directory_path() / GenerateUniqueFilename(".dbf");
    WriteDuplicateFields(filename);
    const auto size = fs::file_size(filename);
    EXPECT_EQ(354, size);
    ReadDuplicateFields(filename);
    fs::remove(filename);
}

}  // namespace

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
