#pragma once

#include <string>
#include <vector>
#include <ostream>

namespace fxl {

enum class DiagnosticSeverity {
    Error,
    Warning,
    Info
};

struct SourceLocation {
    int line = 1;
    int column = 1;

    SourceLocation() = default;
    SourceLocation(int line_, int column_) : line(line_), column(column_) {}

    bool operator==(const SourceLocation& other) const {
        return line == other.line && column == other.column;
    }
    bool operator!=(const SourceLocation& other) const {
        return !(*this == other);
    }
    bool operator<(const SourceLocation& other) const {
        if (line != other.line) return line < other.line;
        return column < other.column;
    }
};

struct SourceRange {
    SourceLocation start;
    SourceLocation end;

    SourceRange() = default;
    SourceRange(SourceLocation start_, SourceLocation end_) : start(start_), end(end_) {}
};

struct Diagnostic {
    DiagnosticSeverity severity;
    std::string message;
    SourceLocation location;
    std::string code;  // Optional error code

    Diagnostic() = default;
    Diagnostic(DiagnosticSeverity severity_, std::string message_, SourceLocation location_, std::string code_ = "")
        : severity(severity_), message(std::move(message_)), location(location_), code(std::move(code_)) {}

    static Diagnostic error(std::string message, SourceLocation location, std::string code = "") {
        return Diagnostic(DiagnosticSeverity::Error, std::move(message), location, std::move(code));
    }

    static Diagnostic warning(std::string message, SourceLocation location, std::string code = "") {
        return Diagnostic(DiagnosticSeverity::Warning, std::move(message), location, std::move(code));
    }

    static Diagnostic info(std::string message, SourceLocation location, std::string code = "") {
        return Diagnostic(DiagnosticSeverity::Info, std::move(message), location, std::move(code));
    }
};

class DiagnosticBag {
public:
    void add(const Diagnostic& diagnostic) {
        diagnostics_.push_back(diagnostic);
    }

    void add(Diagnostic&& diagnostic) {
        diagnostics_.push_back(std::move(diagnostic));
    }

    void add_error(std::string message, SourceLocation location, std::string code = "") {
        add(Diagnostic::error(std::move(message), location, std::move(code)));
    }

    void add_warning(std::string message, SourceLocation location, std::string code = "") {
        add(Diagnostic::warning(std::move(message), location, std::move(code)));
    }

    void add_info(std::string message, SourceLocation location, std::string code = "") {
        add(Diagnostic::info(std::move(message), location, std::move(code)));
    }

    const std::vector<Diagnostic>& diagnostics() const { return diagnostics_; }
    std::vector<Diagnostic>& diagnostics() { return diagnostics_; }

    bool has_errors() const {
        for (const auto& d : diagnostics_) {
            if (d.severity == DiagnosticSeverity::Error) return true;
        }
        return false;
    }

    bool has_warnings() const {
        for (const auto& d : diagnostics_) {
            if (d.severity == DiagnosticSeverity::Warning) return true;
        }
        return false;
    }

    size_t error_count() const {
        size_t count = 0;
        for (const auto& d : diagnostics_) {
            if (d.severity == DiagnosticSeverity::Error) ++count;
        }
        return count;
    }

    size_t warning_count() const {
        size_t count = 0;
        for (const auto& d : diagnostics_) {
            if (d.severity == DiagnosticSeverity::Warning) ++count;
        }
        return count;
    }

    void clear() { diagnostics_.clear(); }
    size_t size() const { return diagnostics_.size(); }
    bool empty() const { return diagnostics_.empty(); }

    // Iterator support
    auto begin() const { return diagnostics_.begin(); }
    auto end() const { return diagnostics_.end(); }
    auto begin() { return diagnostics_.begin(); }
    auto end() { return diagnostics_.end(); }

private:
    std::vector<Diagnostic> diagnostics_;
};

// Formatting helpers
std::string to_string(DiagnosticSeverity severity);
std::ostream& operator<<(std::ostream& os, const SourceLocation& loc);
std::ostream& operator<<(std::ostream& os, const Diagnostic& diag);
std::ostream& operator<<(std::ostream& os, const DiagnosticBag& bag);

} // namespace fxl