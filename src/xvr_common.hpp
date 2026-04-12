#ifndef XVR_COMMON_HPP
#define XVR_COMMON_HPP

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "xvr_memory.h"

namespace xvr {

constexpr int VERSION_MAJOR = 0;
constexpr int VERSION_MINOR = 6;
constexpr int VERSION_PATCH = 16;
constexpr std::string_view VERSION_BUILD = __DATE__ " " __TIME__;

enum class Bitness {
    Bits32 = 32,
    Bits64 = 64,
    Unknown = -1
};

#if defined(__linux__) && defined(__LP64__)
constexpr Bitness CURRENT_BITNESS = Bitness::Bits64;
#elif defined(__linux__) && !defined(__LP64__)
constexpr Bitness CURRENT_BITNESS = Bitness::Bits32;
#elif defined(_WIN64)
constexpr Bitness CURRENT_BITNESS = Bitness::Bits64;
#elif defined(__APPLE__) && defined(__LP64__)
constexpr Bitness CURRENT_BITNESS = Bitness::Bits64;
#elif defined(__APPLE__) && !defined(__LP64__)
constexpr Bitness CURRENT_BITNESS = Bitness::Bits32;
#else
constexpr Bitness CURRENT_BITNESS = Bitness::Unknown;
#endif

class CommandLine {
public:
    bool error{false};
    bool help{false};
    bool version{false};
    std::optional<std::string> sourceFile;
    std::optional<std::string> compileFile;
    std::optional<std::string> outFile;
    std::optional<std::string> source;
    std::optional<std::string> initialfile;
    bool enablePrintNewline{false};
    bool verbose{false};
    bool dumpTokens{false};
    bool dumpAST{false};
    bool dumpLLVM{false};
    bool compileOnly{false};
    bool compileAndRun{false};
    bool showTiming{false};
    std::optional<std::string> emitType;
    std::optional<std::string> asmSyntax;
    int optimizationLevel{0};
    
    static CommandLine& instance();
    void parse(int argc, const char* argv[]);
    void printUsage() const;
    void printHelp() const;
    void printCopyright() const;
};

CommandLine& commandLine();

std::string strdup(std::string_view str);

template<typename... Args>
std::string format(std::string_view fmt, Args... args) {
    return std::string(fmt);
}

template<typename T>
constexpr T min(T a, T b) { return a < b ? a : b; }

template<typename T>
constexpr T max(T a, T b) { return a > b ? a : b; }

template<typename T>
constexpr T clamp(T value, T minVal, T maxVal) {
    return value < minVal ? minVal : value > maxVal ? maxVal : value;
}

}  // namespace xvr

#endif  // XVR_COMMON_HPP