#pragma once

#include <captainlog/expected.hpp>
#include <captainlog/common.hpp>

using tl::expected;

namespace cl {

class Db;

class Importer
{
public:
    Importer(cl::Db& db):m_db(db) {}

    expected<unsigned int, std::string> import_legacy_csv(std::istream& is) CL_MUST_USE_RESULT;

    expected<unsigned int, std::string> import_legacy_csv(const std::string& filename) CL_MUST_USE_RESULT;

private:
    cl::Db& m_db;
};

class Exporter
{
public:
    Exporter(cl::Db& db):m_db(db) {}

    expected<unsigned int, std::string> export_legacy_csv(const std::string& filename) CL_MUST_USE_RESULT;

private:
    cl::Db& m_db;
};

}
