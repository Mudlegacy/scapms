#ifndef FILEHANDLER_H
#define FILEHANDLER_H

#include <string>
#include "model/ClinicDatabase.h"

namespace scapms {
namespace utils {

class FileHandler {
public:
    // Writes every patient and appointment to a text file.
    static void save(const model::ClinicDatabase& db, const std::string& path);

    // Replaces the contents of db with the file's data.
    // If anything in the file is wrong, the file is rejected and db is left exactly as it was.
    static void load(model::ClinicDatabase& db, const std::string& path);
};

} // namespace utils
} // namespace scapms

#endif