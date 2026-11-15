#include "utils/StringUtils.h"

namespace scapms {
namespace utils {

std::string toLower(const std::string& text) {
    std::string result = text;
    for (size_t i = 0; i < result.size(); i++) {
        if (result[i] >= 'A' && result[i] <= 'Z') {
            result[i] = result[i] + ('a' - 'A');
        }
    }
    return result;
}

int compareIgnoreCase(const std::string& a, const std::string& b) {
    std::string x = toLower(a);
    std::string y = toLower(b);
    if (x < y) return -1;
    if (x > y) return 1;
    return 0;
}

bool containsIgnoreCase(const std::string& text, const std::string& keyword) {
    std::string t = toLower(text);
    std::string k = toLower(keyword);
    if (k.size() > t.size()) return false;

    for (size_t start = 0; start + k.size() <= t.size(); start++) {
        size_t j = 0;
        while (j < k.size() && t[start + j] == k[j]) j++;
        if (j == k.size()) return true;
    }
    return false;
}

// A comma inside a field would be mistaken for a field separator,
// so we write it as \c. A backslash becomes \\ and a new line becomes \n.
std::string escapeField(const std::string& text) {
    std::string out;
    for (size_t i = 0; i < text.size(); i++) {
        char c = text[i];
        if (c == '\\')      out += "\\\\";
        else if (c == ',')  out += "\\c";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') { /* dropped */ }
        else                out += c;
    }
    return out;
}

std::string unescapeField(const std::string& text) {
    std::string out;
    for (size_t i = 0; i < text.size(); i++) {
        if (text[i] == '\\' && i + 1 < text.size()) {
            char next = text[i + 1];
            if (next == '\\')      out += '\\';
            else if (next == 'c')  out += ',';
            else if (next == 'n')  out += '\n';
            else { out += text[i]; out += next; }   // unknown code: keep as written
            i++;                                     // we used two characters
        } else {
            out += text[i];
        }
    }
    return out;
}

// Splits at every delimiter and keeps empty fields.
std::vector<std::string> splitFields(const std::string& line, char delimiter) {
    std::vector<std::string> parts;
    std::string current;
    for (size_t i = 0; i < line.size(); i++) {
        if (line[i] == delimiter) {
            parts.push_back(current);
            current.clear();
        } else {
            current += line[i];
        }
    }
    parts.push_back(current);
    return parts;
}

bool tryParseInt(const std::string& text, int& result) {
    if (text.empty() || text.size() > 9) return false;   // 9 characters always fits in an int

    size_t start = 0;
    bool negative = false;
    if (text[0] == '-') {
        negative = true;
        start = 1;
        if (text.size() == 1) return false;
    }

    int value = 0;
    for (size_t i = start; i < text.size(); i++) {
        if (text[i] < '0' || text[i] > '9') return false;
        value = value * 10 + (text[i] - '0');
    }
    result = negative ? -value : value;
    return true;
}

} // namespace utils
} // namespace scapms