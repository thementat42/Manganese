#include <filesystem>
#include <frontend/parser.hpp>
#include <iostream>
#include <string>
#include <utils/chunk_allocator.hpp>
#include <utils/string_interner.hpp>
#include <vector>

#include "tests.hpp"

namespace Manganese::tests {
namespace multifile_tests {

namespace {
bool testParallelParsingAndModuleSorting() {
    const std::filesystem::path fixtureDir = std::filesystem::current_path() / "tests" / "fixtures";

    std::vector<std::string> filenames = {
        (fixtureDir / "multifile_b.mn").string(),  // graphics::rendering (depends on math::vector)
        (fixtureDir / "multifile_a.mn").string()  // math::vecto
    };

    utils::chunk_allocator globalArena;
    utils::StringInterner interner;

    // Make sure parallel parsing and thread-safe arena/log merging works
    std::vector<parser::ParsedFile> parsedFiles = parser::parseInParallel(filenames, globalArena, interner);
    if (parsedFiles.size() != 2) {
        std::cout << std::format("ERROR: Expected 2 parsed files, got {}\n", parsedFiles.size());
        return false;
    }

    // Check topological module sorting & dependency graph resolution
    std::vector<parser::Module> sortedModules = parser::sortModules(parsedFiles, interner);
    if (sortedModules.size() < 2) {
        std::cout << std::format("ERROR: Expected at least 2 sorted modules, got {}\n", sortedModules.size());
        return false;
    }

    // math::vector must be sorted before graphics::rendering because rendering imports vector
    const std::string firstModule = sortedModules[0].name;
    if (firstModule != "math::vector") {
        std::cout << std::format("ERROR: Expected first sorted module to be 'math::vector', got '{}'\n", firstModule);
        return false;
    }

    return true;
}

bool testCircularDependencyDetection() {
    const std::filesystem::path fixtureDir = std::filesystem::current_path() / "tests" / "fixtures";

    std::vector<std::string> filenames
        = {(fixtureDir / "multifile_cycle_a.mn").string(), (fixtureDir / "multifile_cycle_b.mn").string()};

    utils::chunk_allocator globalArena;
    utils::StringInterner interner;

    // Parse files in parallel
    std::vector<parser::ParsedFile> parsedFiles = parser::parseInParallel(filenames, globalArena, interner);

    // sortModules should encounter the cycle and throw due to logCritical
    try {
        std::vector<parser::Module> sortedModules = parser::sortModules(parsedFiles, interner);
        // If it didn't throw, the circular dependency wasn't caught
        std::cout
            << "ERROR: Expected circular dependency to trigger a critical error exception, but it completed successfully.\n";
        return false;
    } catch (const std::exception& e) { return true; } catch (...) {
        return true;
    }
}
}  // namespace

}  // namespace multifile_tests

void TestRunner::runMultiFileTests() {
    runTest("Multi-file: Parallel Parsing & Topological Module Sorting",
            multifile_tests::testParallelParsingAndModuleSorting);
    runTest("Multi-file: Circular Dependency Detection", multifile_tests::testCircularDependencyDetection);
}

}  // namespace Manganese::tests