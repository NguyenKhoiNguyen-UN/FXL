#include "diagnostic.hpp"
#include <sstream>
#include <algorithm>

namespace fxl {

std::string to_string(DiagnosticSeverity severity) {
    switch (severity) {
        case DiagnosticSeverity::Error:   return "error";
        case DiagnosticSeverity::Warning: return "warning";
        case DiagnosticSeverity::Info:    return "info";
    }
    return "unknown";
}

std::ostream& operator<<(std::ostream& os, const SourceLocation& loc) {
    os << "line " << loc.line << ", column " << loc.column;
    return os;
}

std::ostream& operator<<(std::ostream& os, const Diagnostic& diag) {
    os << to_string(diag.severity);
    if (!diag.code.empty()) {
        os << " [" << diag.code << "]";
    }
    os << ": " << diag.message << " at " << diag.location;
    return os;
}

std::ostream& operator<<(std::ostream& os, const DiagnosticBag& bag) {
    for (const auto& diag : bag.diagnostics()) {
        os << diag << "\n";
    }
    return os;
}

} // namespace fxl