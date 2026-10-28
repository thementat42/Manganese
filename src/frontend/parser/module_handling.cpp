#include <algorithm>
#include <execution>
#include <frontend/parser.hpp>
#include <io/logging.hpp>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utils/chunk_allocator.hpp>
#include <utils/resolution_status.hpp>

namespace Manganese::parser {

namespace {

std::string flattenPath(const utils::Slice<utils::StringID>& pathSlice, const utils::StringInterner& interner) {
    std::string result;
    for (std::size_t i = 0; i < pathSlice.size; ++i) {
        if (i > 0) { result += "::"; }
        result.append(interner.get_view(pathSlice[i]));
    }
    return result;
}

struct SortContext {
    const std::unordered_map<std::string, Module>& modules;
    std::unordered_map<std::string_view, ResolutionStatus>& states;
    std::vector<std::string_view>& resolutionStack;
    std::vector<Module>& orderedModules;
};

void topologicalSort(const std::string& moduleName, SortContext& context) {
    ResolutionStatus& state = context.states[moduleName];

    if (state == ResolutionStatus::InProgress) {
        std::string loopString;
        auto cycleStart = std::ranges::find(context.resolutionStack, moduleName);

        for (auto it = cycleStart; it != context.resolutionStack.end(); ++it) {
            loopString += std::format("'{}' imports ", *it);
        }
        loopString += std::format("'{}'", moduleName);

        logging::logCritical(0, 0, "Circular dependency detected: module '{}' eventually imports itself ({})",
                             moduleName, loopString);
    }

    if (state == ResolutionStatus::Success) { return; }

    state = ResolutionStatus::InProgress;
    context.resolutionStack.push_back(moduleName);

    if (auto it = context.modules.find(moduleName); it != context.modules.end()) {
        for (const auto& dependency : it->second.dependencies) { topologicalSort(dependency, context); }

        state = ResolutionStatus::Success;
        context.orderedModules.push_back(context.modules.at(moduleName));
        context.resolutionStack.pop_back();
        return;
    }

    logging::logCritical(0, 0, "Imported module '{}' was not found", moduleName);
}

}  // namespace

std::vector<Module> sortModules(const std::vector<ParsedFile>& parsedFiles, const utils::StringInterner& interner) {
    std::unordered_map<std::string, Module> modules;

    // Group files by their modules
    for (const auto& file : parsedFiles) {
        std::string name = "anonymous";
        if (file.fileModule != nullptr) { name = interner.get_copy(file.fileModule->name); }

        Module& currentModule = modules[name];
        if (currentModule.name.empty()) { currentModule.name = name; }

        // Extract import path strings via interner from ImportStatement
        for (const auto* importStatement : file.imports) {
            currentModule.dependencies.insert(flattenPath(importStatement->path, interner));
        }

        // Collect statements into the temporary vector buffer
        currentModule.statementsBuffer.insert(currentModule.statementsBuffer.end(), file.program.begin(),
                                              file.program.end());
    }

    std::unordered_map<std::string_view, ResolutionStatus> states;
    std::vector<std::string_view> resolutionStack;
    std::vector<Module> orderedModules;
    orderedModules.reserve(modules.size());

    for (const auto& [name, _] : modules) { states[name] = ResolutionStatus::NotStarted; }

    SortContext c{
        .modules = modules, .states = states, .resolutionStack = resolutionStack, .orderedModules = orderedModules};

    for (const auto& [name, _] : modules) {
        if (states[name] == ResolutionStatus::NotStarted) { topologicalSort(name, c); }
    }

    return orderedModules;
}

std::vector<ParsedFile> parseInParallel(const std::vector<std::string>& filenames, utils::chunk_allocator& globalArena,
                                        utils::StringInterner& interner) {
    std::vector<ParsedFile> parsedFiles;
    parsedFiles.resize(filenames.size());

    std::mutex resultsMutex;
    std::vector<logging::LogRecord> aggregatedLogs;
    std::mutex logsMutex;

    std::for_each(std::execution::par, filenames.begin(), filenames.end(), [&](const std::string& path) {
        logging::ThreadLogBufferGuard logGuard;

        utils::chunk_allocator localArena;
        Parser p{path, lexer::Mode::File, localArena, interner};
        ParsedFile file = p.parse();

        {
            std::lock_guard<std::mutex> lock{resultsMutex};

            globalArena.merge_with(std::move(localArena));

            // Store the result at the correct index to preserve input file ordering
            auto idx = static_cast<std::size_t>(&path - filenames.data());
            parsedFiles[idx] = file;
        }

        if (!logGuard.records.empty()) {
            std::lock_guard<std::mutex> lock{logsMutex};
            for (auto& rec : logGuard.records) { aggregatedLogs.push_back(std::move(rec)); }
        }
    });

    // flush logs on the main thread
    if (!aggregatedLogs.empty()) {
        std::ranges::sort(aggregatedLogs, [](const auto& a, const auto& b) {
            if (a.line != b.line) { return a.line < b.line; }
            return a.col < b.col;
        });

        for (const auto& rec : aggregatedLogs) {
            if (rec.level == logging::LogLevel::Error) {
                logging::logError(rec.line, rec.col, "{}", rec.message);
            } else if (rec.level == logging::LogLevel::Warning) {
                logging::logWarning(rec.line, rec.col, "{}", rec.message);
            }
        }
    }

    return parsedFiles;
}

}  // namespace Manganese::parser