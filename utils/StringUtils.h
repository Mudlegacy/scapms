#ifndef STRINGUTILS_H
#define STRINGUTILS_H

#include <string>
#include <vector>

namespace scapms {
namespace utils {

std::string toLower(const std::string& text);
int compareIgnoreCase(const std::string& a, const std::string& b);   // <0, 0, >0
bool containsIgnoreCase(const std::string& text, const std::string& keyword);

// For the data file
std::string escapeField(const std::string& text);     // makes text safe to put in a CSV field
std::string unescapeField(const std::string& text);   // reverses escapeField
std::vector<std::string> splitFields(const std::string& line, char delimiter);
bool tryParseInt(const std::string& text, int& result);   // false if text is not a whole number

} // namespace utils
} // namespace scapms

#endif