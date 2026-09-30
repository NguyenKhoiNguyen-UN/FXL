#include "lexer.hpp"
#include "parser.hpp"
#include "ast.hpp"
#include "semantic.hpp"
#include "auto_resolver.hpp"
#include "codegen.hpp"
#include "diagnostic.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <cstdlib>
#include <vector>
#include <string>
#include <array>
#include <memory>
#include <regex>

namespace fs = std::filesystem;

void print_usage(const char* program_name) {
    std::cerr << "FXL Compiler v1.0.0\n";
    std::cerr << "Usage: " << program_name << " <command> [options] <input.fxl>\n\n";
    std::cerr << "Commands:\n";
    std::cerr << "  compile [options] <input.fxl>     Compile FXL to C++ (default command)\n";
    std::cerr << "  build [options] <input.fxl>       Compile and build executable\n";
    std::cerr << "  preview [options] <input.fxl>     Build and run preview\n\n";
    std::cerr << "Compile options:\n";
    std::cerr << "  -o <file>    Output file (default: input.cpp)\n";
    std::cerr << "  --ast        Print AST for debugging\n";
    std::cerr << "  --tokens     Print tokens for debugging\n\n";
    std::cerr << "Build options:\n";
    std::cerr << "  -o <file>    Output executable (default: input.exe / input)\n";
    std::cerr << "  --debug      Build with debug symbols (default)\n";
    std::cerr << "  --release    Build with optimizations\n";
    std::cerr << "  --keep-cpp   Keep generated C++ file\n\n";
    std::cerr << "Preview options:\n";
    std::cerr << "  --debug      Run debug build (default)\n";
    std::cerr << "  --release    Run release build\n\n";
    std::cerr << "  -h, --help   Show this help\n";
}

std::string read_file(const fs::path& path) {
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("Cannot open file: " + path.string());
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void write_file(const fs::path& path, const std::string& content) {
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Cannot write to file: " + path.string());
    }
    file.write(content.data(), static_cast<std::streamsize>(content.size()));
    file.flush();
    file.close();
}

// Compile FXL source to C++
bool compile_fxl(const std::string& source, [[maybe_unused]] const fs::path& input_path, 
                 std::string& cpp_code, fxl::DiagnosticBag& diagnostics,
                 bool print_ast, bool print_tokens) {
    
    // Phase 1: Lexer
    fxl::Lexer lexer(source, &diagnostics);
    auto tokens = lexer.tokenize();
    
    if (print_tokens) {
        for (const auto& token : tokens) {
            std::cout << token << "\n";
        }
    }
    
    if (diagnostics.has_errors()) return false;
    
    // Phase 2: Parser
    fxl::Parser parser(tokens, &diagnostics);
    auto program = parser.parse();
    
    if (diagnostics.has_errors()) return false;
    
    if (print_ast) {
        std::cout << fxl::ast_to_string(*program) << "\n";
    }
    
    // Phase 3: Semantic Analysis
    fxl::SemanticAnalyzer semantic(&diagnostics);
    semantic.analyze(*program);
    
    if (diagnostics.has_errors()) return false;
    
    // Phase 4: Auto Resolution
    fxl::AutoSetResolver auto_resolver(&diagnostics);
    auto_resolver.resolve(*program);
    
    if (diagnostics.has_errors()) return false;
    
    if (print_ast) {
        std::cout << "\n=== After Auto Resolution ===\n";
        std::cout << fxl::ast_to_string(*program) << "\n";
    }
    
    // Phase 5: Code Generation
    fxl::CodeGenerator codegen(&diagnostics);
    cpp_code = codegen.generate(*program);
    
    if (diagnostics.has_errors()) return false;
    
    return true;
}

// Detect host C++ compiler
std::string detect_cpp_compiler() {
    // Try common compilers in order of preference
#if defined(_WIN32) || defined(_WIN64)
    const std::array<const char*, 4> compilers = {"cl.exe", "g++", "clang++", "clang-cl"};
#else
    const std::array<const char*, 3> compilers = {"g++", "clang++", "c++"};
#endif

    for (const char* compiler : compilers) {
        // Check if compiler exists in PATH
        std::string cmd = std::string(compiler) + " --version 2>&1";
        int result = std::system(cmd.c_str());
        if (result == 0) {
            return compiler;
        }
    }
    return "";
}

// Build executable from C++ code
bool build_executable(const std::string& cpp_code, const fs::path& output_path, 
                      bool debug, bool keep_cpp, [[maybe_unused]] const fs::path& source_path) {
    
    std::string compiler = detect_cpp_compiler();
    if (compiler.empty()) {
        std::cerr << "Error: No C++ compiler found in PATH (tried cl.exe, g++, clang++)\n";
        return false;
    }
    
    std::cout << "Using C++ compiler: " << compiler << "\n";
    
    // Write C++ source next to the output executable
    fs::path cpp_file = output_path;
    cpp_file.replace_extension(".cpp");
    write_file(cpp_file, cpp_code);
    
    // Determine include paths
    fs::path runtime_include = fs::absolute("runtime/include");
    fs::path compiler_include = fs::absolute("compiler/include");
    
    // Escape paths for command line
    auto escape_path = [](const fs::path& p) -> std::string {
        std::string s = p.string();
        if (s.find('\\') != std::string::npos) {
            std::replace(s.begin(), s.end(), '\\', '/');
        }
        return s;
    };
    
    std::string runtime_include_str = escape_path(runtime_include);
    std::string compiler_include_str = escape_path(compiler_include);
    std::string cpp_file_str = escape_path(cpp_file);
    std::string output_path_str = escape_path(output_path);
    
    // Build command
    std::ostringstream cmd;
    cmd << compiler << " ";
    
    if (compiler == "cl.exe") {
        // MSVC
        cmd << "/std:c++20 ";
        if (debug) {
            cmd << "/Zi /Od /MDd ";
        } else {
            cmd << "/O2 /MD ";
        }
        cmd << "/I\"" << runtime_include.string() << "\" ";
        cmd << "/I\"" << compiler_include.string() << "\" ";
        cmd << "/Fe\"" << output_path.string() << "\" ";
        cmd << "\"" << cpp_file.string() << "\" ";
    } else {
        // GCC/Clang
        cmd << "-std=c++20 ";
        if (debug) {
            cmd << "-g -O0 ";
        } else {
            cmd << "-O3 -DNDEBUG ";
        }
        cmd << "-I" << runtime_include_str << " ";
        cmd << "-I" << compiler_include_str << " ";
        // Source file first (for proper static library linking order)
        cmd << cpp_file_str << " ";
        // Link runtime backend library directly
        fs::path runtime_lib = fs::absolute("build/lib/libfxl_runtime_backend.a");
        std::string runtime_lib_str = escape_path(runtime_lib);
        cmd << runtime_lib_str << " ";
        cmd << "-lopengl32 -lgdi32 -luser32 ";
        cmd << "-o " << output_path_str << " ";
    }
    
    std::string build_cmd = cmd.str();
    std::cout << "Build command: " << build_cmd << "\n";
    
    int result = std::system(build_cmd.c_str());
    
    // Remove C++ file if not requested to keep
    if (!keep_cpp) {
        try {
            fs::remove(cpp_file);
        } catch (...) {}
    } else {
        std::cout << "Kept C++ source: " << cpp_file << "\n";
    }
    
    return result == 0;
}

// Run executable
int run_executable(const fs::path& exe_path) {
#if defined(_WIN32) || defined(_WIN64)
    std::string cmd = "\"" + exe_path.string() + "\"";
#else
    std::string cmd = "\"" + exe_path.string() + "\"";
#endif
    std::cout << "Running: " << cmd << "\n";
    return std::system(cmd.c_str());
}

enum class Command {
    Compile,
    Build,
    Preview,
    Help
};

struct Options {
    Command cmd = Command::Compile;
    fs::path input_path;
    fs::path output_path;
    bool print_ast = false;
    bool print_tokens = false;
    bool debug = true;
    bool release = false;
    bool keep_cpp = false;
};

Options parse_args(int argc, char* argv[]) {
    Options opts;
    
    if (argc < 2) {
        opts.cmd = Command::Help;
        return opts;
    }
    
    // First argument might be a command
    std::string first_arg = argv[1];
    int arg_start = 1;
    
    if (first_arg == "compile" || first_arg == "build" || first_arg == "preview" || first_arg == "help") {
        if (first_arg == "compile") opts.cmd = Command::Compile;
        else if (first_arg == "build") opts.cmd = Command::Build;
        else if (first_arg == "preview") opts.cmd = Command::Preview;
        else opts.cmd = Command::Help;
        arg_start = 2;
    }
    
    // If no explicit command, default to compile
    if (opts.cmd == Command::Compile && arg_start == 1) {
        // Check if first arg looks like an option
        if (first_arg[0] == '-') {
            opts.cmd = Command::Compile;
            arg_start = 1;
        } else {
            opts.cmd = Command::Compile;
            arg_start = 1;
        }
    }
    
    // Parse remaining arguments
    for (int i = arg_start; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            opts.cmd = Command::Help;
        } else if (arg == "-o") {
            if (i + 1 >= argc) {
                std::cerr << "Error: -o requires an argument\n";
                exit(1);
            }
            opts.output_path = argv[++i];
        } else if (arg == "--ast") {
            opts.print_ast = true;
        } else if (arg == "--tokens") {
            opts.print_tokens = true;
        } else if (arg == "--debug") {
            opts.debug = true;
            opts.release = false;
        } else if (arg == "--release") {
            opts.debug = false;
            opts.release = true;
        } else if (arg == "--keep-cpp") {
            opts.keep_cpp = true;
        } else if (arg[0] == '-') {
            std::cerr << "Error: Unknown option: " << arg << "\n";
            exit(1);
        } else {
            if (opts.input_path.empty()) {
                opts.input_path = arg;
            } else {
                std::cerr << "Error: Multiple input files specified\n";
                exit(1);
            }
        }
    }
    
    return opts;
}

int cmd_compile(const Options& opts) {
    if (opts.input_path.empty()) {
        std::cerr << "Error: No input file specified\n";
        return 1;
    }
    
    fs::path output_path = opts.output_path.empty() ? 
        (fs::path(opts.input_path).replace_extension(".cpp")) : opts.output_path;
    
    try {
        std::string source = read_file(opts.input_path);
        fxl::DiagnosticBag diagnostics;
        std::string cpp_code;
        
        if (!compile_fxl(source, opts.input_path, cpp_code, diagnostics, 
                         opts.print_ast, opts.print_tokens)) {
            std::cerr << diagnostics;
            return 1;
        }
        
        write_file(output_path, cpp_code);
        
        if (diagnostics.has_warnings()) {
            std::cerr << diagnostics;
        }
        
        std::cout << "Generated: " << output_path << "\n";
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}

int cmd_build(const Options& opts) {
    if (opts.input_path.empty()) {
        std::cerr << "Error: No input file specified\n";
        return 1;
    }
    
    fs::path output_path = opts.output_path.empty() ? 
        fs::path(opts.input_path).replace_extension("") : opts.output_path;
    
#if defined(_WIN32) || defined(_WIN64)
    if (output_path.extension().empty()) output_path += ".exe";
#endif
    
    try {
        std::string source = read_file(opts.input_path);
        fxl::DiagnosticBag diagnostics;
        std::string cpp_code;
        
        if (!compile_fxl(source, opts.input_path, cpp_code, diagnostics, 
                         false, false)) {
            std::cerr << diagnostics;
            return 1;
        }
        
        if (diagnostics.has_warnings()) {
            std::cerr << diagnostics;
        }
        
        std::cout << "Compiled to C++, building executable...\n";
        
        if (!build_executable(cpp_code, output_path, opts.debug, opts.keep_cpp, opts.input_path)) {
            std::cerr << "Build failed\n";
            return 1;
        }
        
        std::cout << "Built: " << output_path << " (" << (opts.debug ? "debug" : "release") << ")\n";
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}

int cmd_preview(const Options& opts) {
    if (opts.input_path.empty()) {
        std::cerr << "Error: No input file specified\n";
        return 1;
    }
    
    // Read source and compile to C++
    std::string source = read_file(opts.input_path);
    fxl::DiagnosticBag diagnostics;
    
    // Phase 1: Lexer
    fxl::Lexer lexer(source, &diagnostics);
    auto tokens = lexer.tokenize();
    if (diagnostics.has_errors()) {
        std::cerr << diagnostics;
        return 1;
    }
    
    // Phase 2: Parser
    fxl::Parser parser(tokens, &diagnostics);
    auto program = parser.parse();
    if (diagnostics.has_errors()) {
        std::cerr << diagnostics;
        return 1;
    }
    
    // Phase 3: Semantic Analysis
    fxl::SemanticAnalyzer semantic(&diagnostics);
    semantic.analyze(*program);
    if (diagnostics.has_errors()) {
        std::cerr << diagnostics;
        return 1;
    }
    
    // Phase 4: Auto Resolution
    fxl::AutoSetResolver auto_resolver(&diagnostics);
    auto_resolver.resolve(*program);
    if (diagnostics.has_errors()) {
        std::cerr << diagnostics;
        return 1;
    }
    
    // Phase 5: Code Generation
    fxl::CodeGenerator codegen(&diagnostics);
    std::string cpp_code = codegen.generate(*program);
    if (diagnostics.has_errors()) {
        std::cerr << diagnostics;
        return 1;
    }
    
    // For preview, inject run() call for the first window
    // Find the first window variable declaration and add a run() call after all statements
    std::string preview_cpp = cpp_code;
    // Find the first "auto <name> = fxl::runtime::Window();" pattern and add run() call at the end of main
    size_t main_end = preview_cpp.rfind("return 0;");
    if (main_end != std::string::npos) {
        // Find the first window variable name
        std::string window_var = "";
        std::regex window_regex(R"(auto\s+(\w+)\s*=\s*fxl::runtime::Window\(\)\s*;)");
        std::smatch match;
        if (std::regex_search(cpp_code, match, window_regex)) {
            window_var = match[1].str();
            std::string run_call = "    " + window_var + ".run();\n";
            preview_cpp.insert(main_end, run_call);
            std::cout << "[Preview] Added run() call for window '" << window_var << "'\n";
        } else {
            std::cerr << "[Preview] Warning: No window found to run\n";
        }
    }
    
    // Build with debug by default
    Options build_opts = opts;
    build_opts.cmd = Command::Build;
    build_opts.output_path = fs::temp_directory_path() / ("fxl_preview_" + std::to_string(std::time(nullptr)));
#if defined(_WIN32) || defined(_WIN64)
    if (build_opts.output_path.extension().empty()) build_opts.output_path += ".exe";
#endif
    build_opts.debug = !opts.release;
    build_opts.release = opts.release;
    build_opts.keep_cpp = false;
    
    // Write preview C++ to temp file and build
    fs::path temp_cpp = fs::temp_directory_path() / ("fxl_preview_" + std::to_string(std::time(nullptr)) + ".cpp");
    write_file(temp_cpp, preview_cpp);
    
    // Build the preview executable
    fs::path temp_exe = build_opts.output_path;
    if (!build_executable(preview_cpp, temp_exe, build_opts.debug, false, opts.input_path)) {
        fs::remove(temp_cpp);
        return 1;
    }
    
    fs::remove(temp_cpp);
    
    std::cout << "Launching preview...\n";
    return run_executable(build_opts.output_path);
}

int main(int argc, char* argv[]) {
    Options opts = parse_args(argc, argv);
    
    if (opts.cmd == Command::Help) {
        print_usage(argv[0]);
        return 0;
    }
    
    switch (opts.cmd) {
        case Command::Compile: return cmd_compile(opts);
        case Command::Build:   return cmd_build(opts);
        case Command::Preview: return cmd_preview(opts);
        default: return 1;
    }
}