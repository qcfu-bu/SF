#include <fstream>
#include <print>
#include <sstream>

#include "elaborate/table.hpp"
#include "parsing/parser.hpp"
#include "llvm/Support/CommandLine.h"

std::string read_file_to_string(const std::string& filename) {
    std::ifstream ifs(filename, std::ios::binary);
    if (!ifs) {
        throw std::runtime_error("Could not open file: " + filename);
    }
    std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
    return content;
}

int main(int argc, char** argv) {
    llvm::cl::OptionCategory options("implang options");
    llvm::cl::opt<std::string> input(
        "i",
        llvm::cl::desc("Input file"),
        llvm::cl::value_desc("filename"),
        llvm::cl::cat(options)
    );
    llvm::cl::opt<std::string> output(
        "o",
        llvm::cl::desc("Output filename"),
        llvm::cl::value_desc("filename"),
        llvm::cl::cat(options),
        llvm::cl::init("output.o")
    );

    llvm::cl::HideUnrelatedOptions(options);
    llvm::cl::ParseCommandLineOptions(argc, argv);

    // read input file
    const char* filename = input.c_str();
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error(std::format("Could not open file: {}", filename));
    }
    std::stringstream buffer;
    buffer << file.rdbuf();

    parsing::Parser parser(filename, buffer.str());
    parsing::Package pkg = parser.parse_package();

    std::println("// Parsed successfully.");
    std::println("/* Initial AST:");
    std::println("{}", pkg);
    std::println("*/");

    elaborate::TableBuilder table_builder(pkg);
    elaborate::Table table = table_builder.build();

    std::println("{}", pkg);

    return 0;
}
