/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <fstream>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "ecflow/core/Filesystem.hpp"
#include "ecflow/core/WhiteListFile.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

namespace {

class Directory {
public:
    explicit Directory(fs::path location)
        : location_{std::move(location)} {
        ECF_TEST_DBG("Created working directory at: " << location_.string());
    }

    Directory(const Directory&)            = delete;
    Directory& operator=(const Directory&) = delete;
    Directory(Directory&&)                 = delete;
    Directory& operator=(Directory&&)      = delete;

    ~Directory() {
        fs::remove_all(location_);
        ECF_TEST_DBG("Removed working directory at: " << location_.string());
    }

    [[nodiscard]] const fs::path& path() const { return location_; }

private:
    fs::path location_;
};

class File {
public:
    explicit File(fs::path location)
        : location_{std::move(location)} {
        assert(fs::exists(location_));
        assert(fs::is_regular_file(location_));
        ECF_TEST_DBG("Created file: " << location_);
    }

    File(const File&)                = delete;
    File& operator=(const File&)     = delete;
    File(File&&) noexcept            = default;
    File& operator=(File&&) noexcept = default;

    ~File() {
        fs::remove(location_);
        ECF_TEST_DBG("Removed file: " << location_);
    }

    [[nodiscard]] const fs::path& path() const { return location_; }
    [[nodiscard]] fs::path filename() const { return location_.filename(); }

private:
    fs::path location_;
};

class SpecificFileLocation {
public:
    explicit SpecificFileLocation(const std::string& name, const Directory& cwd)
        : SpecificFileLocation(name, cwd.path()) {}

    explicit SpecificFileLocation(const std::string& name, const fs::path& location)
        : path_{location / name} {}

    constexpr const fs::path& path() const { return path_; }

private:
    fs::path path_;
};

class MakeTestFile {
public:
    explicit MakeTestFile()
        : location_{},
          content_{} {}

    MakeTestFile(const MakeTestFile&)                = delete;
    MakeTestFile& operator=(const MakeTestFile&)     = delete;
    MakeTestFile(MakeTestFile&&) noexcept            = delete;
    MakeTestFile& operator=(MakeTestFile&&) noexcept = delete;

    ~MakeTestFile() = default;

    MakeTestFile& with(const SpecificFileLocation& location) {
        location_ = location.path();
        return *this;
    }

    MakeTestFile& with(const char* content) {
        content_ = content;
        return *this;
    }

    MakeTestFile& with(const std::string& content) {
        content_ = content;
        return *this;
    }

    template <typename Content>
    MakeTestFile& with(const Content& content) {
        content_ = content.data();
        return *this;
    }

    [[nodiscard]] File create() const {
        { // Caution: We assume that existing test files can be overwritten
            std::ofstream os(location_.string(), std::ios::out | std::ios::trunc);
            os << content_;
        }

        // Now that the file actually exists, we update the location to the canonical path
        auto actual_location = fs::canonical(location_);

        return File{actual_location};
    }

    const fs::path& path() const { return location_; }

private:
    fs::path location_;
    std::string content_;
};

} // namespace

BOOST_AUTO_TEST_SUITE(U_Core)

BOOST_AUTO_TEST_SUITE(T_WhiteListIncludes)

BOOST_AUTO_TEST_CASE(test_parse_whitelist_file_with_single_include) {
    ECF_NAME_THIS_TEST();

    auto included = MakeTestFile()
                        .with(SpecificFileLocation("whitelist.included.txt", fs::current_path()))
                        .with("# This is the included whitelist file\n"
                              "\n"
                              "i # comment\n"
                              "j /j\n"
                              "-k\n"
                              "\n")
                        .create();

    auto main = MakeTestFile()
                    .with(SpecificFileLocation("whitelist.main.txt", fs::current_path()))
                    .with("5.0.0 # comment"
                          "# This is the main whitelist file\n"
                          "\n"
                          "a\n"
                          "b /b\n"
                          "\n"
                          "#include <whitelist.included.txt>\n"
                          "\n"
                          "-z /z\n")
                    .create();

    WhiteListFile wl;

    std::string error;
    auto loaded = wl.load(main.path().c_str(), error);

    BOOST_CHECK(loaded);
    BOOST_CHECK(error.empty());

    BOOST_CHECK(wl.verify_read_access("a"));
    BOOST_CHECK(wl.verify_write_access("a"));
    BOOST_CHECK(wl.verify_read_access("a", "/any/path"));
    BOOST_CHECK(wl.verify_write_access("a", "/any/path"));

    BOOST_CHECK(wl.verify_read_access("b"));
    BOOST_CHECK(!wl.verify_write_access("b"));
    BOOST_CHECK(wl.verify_read_access("b", "/b"));
    BOOST_CHECK(wl.verify_write_access("b", "/b"));
    BOOST_CHECK(!wl.verify_read_access("b", "/any/path"));
    BOOST_CHECK(!wl.verify_write_access("b", "/any/path"));

    BOOST_CHECK(wl.verify_read_access("i"));
    BOOST_CHECK(wl.verify_write_access("i"));
    BOOST_CHECK(wl.verify_read_access("i", "/any/path"));
    BOOST_CHECK(wl.verify_write_access("i", "/any/path"));

    BOOST_CHECK(wl.verify_read_access("j"));
    BOOST_CHECK(!wl.verify_write_access("j"));
    BOOST_CHECK(wl.verify_read_access("j", "/j"));
    BOOST_CHECK(wl.verify_write_access("j", "/j"));
    BOOST_CHECK(!wl.verify_read_access("j", "/any/path"));
    BOOST_CHECK(!wl.verify_write_access("j", "/any/path"));

    BOOST_CHECK(wl.verify_read_access("k"));
    BOOST_CHECK(!wl.verify_write_access("k"));
    BOOST_CHECK(wl.verify_read_access("k", "/any/path"));
    BOOST_CHECK(!wl.verify_write_access("k", "/any/path"));

    BOOST_CHECK(wl.verify_read_access("z"));
    BOOST_CHECK(!wl.verify_write_access("z"));
    BOOST_CHECK(wl.verify_read_access("z", "/z"));
    BOOST_CHECK(!wl.verify_write_access("z", "/z"));
    BOOST_CHECK(!wl.verify_read_access("z", "/any/path"));
    BOOST_CHECK(!wl.verify_write_access("z", "/any/path"));
}

BOOST_AUTO_TEST_CASE(test_parse_whitelist_file_with_multiple_include) {
    ECF_NAME_THIS_TEST();

    auto i = MakeTestFile()
                 .with(SpecificFileLocation("whitelist.i.txt", fs::current_path()))
                 .with("# This is an included whitelist file\n"
                       "\n"
                       "i # comment\n")
                 .create();

    auto j = MakeTestFile()
                 .with(SpecificFileLocation("whitelist.j.txt", fs::current_path()))
                 .with("# This is an included whitelist file\n"
                       "\n"
                       "j /j\n")
                 .create();

    auto k = MakeTestFile()
                 .with(SpecificFileLocation("whitelist.k.txt", fs::current_path()))
                 .with("# This is an included whitelist file\n"
                       "\n"
                       "-k\n")
                 .create();

    auto main = MakeTestFile()
                    .with(SpecificFileLocation("whitelist.main.txt", fs::current_path()))
                    .with("5.0.0 # comment"
                          "# This is the main whitelist file\n"
                          "\n"
                          "a\n"
                          "b /b\n"
                          "\n"
                          "#include <whitelist.i.txt>\n"
                          "#include <whitelist.j.txt>\n"
                          "#include <whitelist.k.txt>\n"
                          "\n"
                          "-z /z\n")
                    .create();

    WhiteListFile wl;

    std::string error;
    auto loaded = wl.load(main.path().c_str(), error);

    BOOST_CHECK(loaded);
    BOOST_CHECK(error.empty());

    BOOST_CHECK(wl.verify_read_access("a"));
    BOOST_CHECK(wl.verify_write_access("a"));
    BOOST_CHECK(wl.verify_read_access("a", "/any/path"));
    BOOST_CHECK(wl.verify_write_access("a", "/any/path"));

    BOOST_CHECK(wl.verify_read_access("b"));
    BOOST_CHECK(!wl.verify_write_access("b"));
    BOOST_CHECK(wl.verify_read_access("b", "/b"));
    BOOST_CHECK(wl.verify_write_access("b", "/b"));
    BOOST_CHECK(!wl.verify_read_access("b", "/any/path"));
    BOOST_CHECK(!wl.verify_write_access("b", "/any/path"));

    BOOST_CHECK(wl.verify_read_access("i"));
    BOOST_CHECK(wl.verify_write_access("i"));
    BOOST_CHECK(wl.verify_read_access("i", "/any/path"));
    BOOST_CHECK(wl.verify_write_access("i", "/any/path"));

    BOOST_CHECK(wl.verify_read_access("j"));
    BOOST_CHECK(!wl.verify_write_access("j"));
    BOOST_CHECK(wl.verify_read_access("j", "/j"));
    BOOST_CHECK(wl.verify_write_access("j", "/j"));
    BOOST_CHECK(!wl.verify_read_access("j", "/any/path"));
    BOOST_CHECK(!wl.verify_write_access("j", "/any/path"));

    BOOST_CHECK(wl.verify_read_access("k"));
    BOOST_CHECK(!wl.verify_write_access("k"));
    BOOST_CHECK(wl.verify_read_access("k", "/any/path"));
    BOOST_CHECK(!wl.verify_write_access("k", "/any/path"));

    BOOST_CHECK(wl.verify_read_access("z"));
    BOOST_CHECK(!wl.verify_write_access("z"));
    BOOST_CHECK(wl.verify_read_access("z", "/z"));
    BOOST_CHECK(!wl.verify_write_access("z", "/z"));
    BOOST_CHECK(!wl.verify_read_access("z", "/any/path"));
    BOOST_CHECK(!wl.verify_write_access("z", "/any/path"));
}

BOOST_AUTO_TEST_CASE(test_parse_whitelist_file_with_nonexistent_include) {
    ECF_NAME_THIS_TEST();

    auto main = MakeTestFile()
                    .with(SpecificFileLocation("whitelist.main.txt", fs::current_path()))
                    .with("5.0.0 # comment"
                          "# This is the main whitelist file\n"
                          "\n"
                          "a\n"
                          "b /b\n"
                          "\n"
                          "#include <nonexistent.txt>\n"
                          "\n"
                          "-z /z\n")
                    .create();

    WhiteListFile wl;

    std::string error;
    auto loaded = wl.load(main.path().c_str(), error);

    std::cout << " =========> : " << error << std::endl;

    BOOST_CHECK(!loaded);
    BOOST_CHECK(error.find("Could not open invalid file specified by ECF_LISTS") != std::string::npos);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
