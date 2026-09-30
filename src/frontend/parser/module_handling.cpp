#include <frontend/parser.hpp>
#include <io/logging.hpp>
#include <string>
#include <unordered_map>
#include <utils/resolution_status.hpp>

namespace Manganese::parser {

namespace {
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
        auto cycleStart = std::find(context.resolutionStack.begin(), context.resolutionStack.end(), moduleName);

        for (auto it = cycleStart; it != context.resolutionStack.end(); ++it) {
            loopString += std::format("'{}' imports ", *it);
        }
        loopString += std::format("'{}'", moduleName);

        logging::logCritical(0, 0, "Circular dependency detected: module '{}' eventually imports itself ({})",
                             moduleName, loopString);
    }

    if (state == ResolutionStatus::Success) { return; /* already handled */ }

    state = ResolutionStatus::InProgress;
    context.resolutionStack.push_back(moduleName);

    if (auto it = context.modules.find(moduleName); it != context.modules.end()) {
        // for each dependency, first sort its dependencies
        for (const auto& dependency : it->second.dependencies) { topologicalSort(dependency, context); }

        // this is hit once all a module's dependencies have been analyzed, or a module has no dependencies
        // the semantic analyzer will now have all the symbols from any and all imported modules so it's safe to analyze
        // this one
        state = ResolutionStatus::Success;
        context.orderedModules.push_back(context.modules.at(moduleName));
        context.resolutionStack.pop_back();
    }

    logging::logCritical(0, 0, "Imported module '{}' was not found", moduleName);
}
}  // namespace

std::vector<Module> sortModules(const std::vector<ParsedFile>& parsedFiles) {
    std::unordered_map<std::string, Module> modules;

    // Group files by their modules

    for (const auto& file : parsedFiles) {
        std::string name = (file.fileModule != nullptr) ? flattenPath(file.fileModule->path) : "anonymous";
        Module& currentModule = modules[name];

        if (currentModule.name.empty()) { currentModule.name = std::move(name); }

        for (const auto* importStatement : file.imports) {
            currentModule.dependencies.insert(flattenPath(importStatement->path));
        }

        currentModule.fullProgram.insert(currentModule.fullProgram.end(), file.program.begin(), file.program.end());
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

}  // namespace Manganese::parser