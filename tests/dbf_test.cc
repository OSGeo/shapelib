#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>

#include <catch2/catch_all.hpp>
#include "shapefil.h"

namespace fs = std::filesystem;

namespace
{

static const auto kTestData = fs::path{"shape_eg_data"};

bool SetContents(const fs::path &file_name, std::string_view content)
{
    std::ofstream file(file_name);
    if (!file.is_open())
        return false;
    file << content;
    file.close();
    return true;
}

TEST_CASE("DBFOpen", "[dbfopen]")
{
    SECTION("Open does not exist - rb")
    {
        const auto handle = DBFOpen("/does/not/exist.dbf", "rb");
        REQUIRE(handle == nullptr);
    }

    SECTION("Open does not exist - rb+")
    {
        const auto handle = DBFOpen("/does/not/exist2.dbf", "rb+");
        REQUIRE(handle == nullptr);
    }

    SECTION("Open unexpected format")
    {
        const auto filename = kTestData / "README.md";
        const auto handle = DBFOpen(filename.string().c_str(), "rb");
        REQUIRE(handle == nullptr);
    }

    SECTION("Open existing")
    {
        const auto filename = kTestData / "anno.dbf";
        const auto handle = DBFOpen(filename.string().c_str(), "rb");
        REQUIRE(handle != nullptr);
        DBFClose(handle);
    }
}

TEST_CASE("DBFCreate", "[dbfcreate]")
{
    SECTION("Create does not exist")
    {
        const auto handle = DBFCreate("/does/not/exist");
        REQUIRE(nullptr == handle);
    }

    SECTION("Create already existing")
    {
        const auto filename = kTestData / "in-the-way.dbf";
        fs::copy(kTestData / "anno.dbf", filename);
        REQUIRE(SetContents(filename, "some content"));
        const auto handle = DBFCreate(filename.string().c_str());
        // TODO(schwehr): Seems like a bug to overwrite an existing.
        REQUIRE(nullptr != handle);
        DBFClose(handle);
        const auto size = fs::file_size(filename);
        REQUIRE(34 == size);
        fs::remove(filename);
    }

    SECTION("Create and close")
    {
        const auto filename = kTestData / "empty.dbf";
        const auto handle = DBFCreate(filename.string().c_str());
        DBFClose(handle);
        const auto size = fs::file_size(filename);
        REQUIRE(34 == size);
        fs::remove(filename);
    }
}

}  // namespace
