#ifndef REPORTGENERATOR_H
#define REPORTGENERATOR_H

#include <string>
#include "model/ClinicDatabase.h"

namespace scapms {
namespace controller {

// Every report only READS the database (const), so it can never change data.
class ReportGenerator {
public:
    // Patients with the most completed appointments
    static std::string mostFrequentlyAttended(const model::ClinicDatabase& db, int topN = 5);

    // Patients who still have appointments that are not completed or cancelled
    static std::string patientsWithPending(const model::ClinicDatabase& db);

    // How many patients of each category
    static std::string categoryDistribution(const model::ClinicDatabase& db);

    // Totals for one day (date format YYYY-MM-DD)
    static std::string dailySummary(const model::ClinicDatabase& db, const std::string& date);
};

} // namespace controller
} // namespace scapms

#endif