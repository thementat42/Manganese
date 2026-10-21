#include <fstream>
#include <io/logging.hpp>
#include <io/reader.hpp>
#include <vector>

namespace Manganese::io {
class HeapFileReader final : public Reader {
    std::size_t _position = 0, _line = 1, _column = 1;
    std::vector<char> _data;

   public:
    explicit HeapFileReader(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary | std::ios::ate);
        if (!file.is_open()) { logging::logCritical(0, 0, "Could not open file '{}'", filename); }

        std::streamsize fileSize = file.tellg();
        if (fileSize <= 0) { logging::logCritical(0, 0, "File '{}' is empty", filename); }

        _data.resize(static_cast<std::size_t>(fileSize));

        file.seekg(0, std::ios::beg);

        if (!file.read(_data.data(), fileSize)) { logging::logCritical(0, 0, "Failed to read file '{}' ", filename); }
    }

    char peekChar(std::size_t offset = 0) noexcept override {
        if (_position + offset >= _data.size()) { return '\0'; }
        return _data[_position + offset];
    }
    [[nodiscard]] char consumeChar() noexcept override {
        if (_position >= _data.size()) { return '\0'; }
        return _data[_position++];
    }

    void setPosition(std::size_t newPosition) noexcept override {
        newPosition = std::min(newPosition, _data.size());
        if (newPosition < _position) {
            _position = 0;
            _line = 1;
            _column = 1;
        }

        while (_position < newPosition) {
            const char c = _data[_position++];
            if (c == '\n') {
                ++_line;
                _column = 1;
            } else {
                ++_column;
            }
        }
    };

    [[nodiscard]] std::size_t getPosition() const noexcept override { return _position; }
    [[nodiscard]] std::size_t getLine() const noexcept override { return _line; }
    [[nodiscard]] std::size_t getColumn() const noexcept override { return _column; }
    [[nodiscard]] bool done() const noexcept override { return _position >= _data.size(); }

    [[nodiscard]] std::string_view getView(std::size_t start, std::size_t length) const noexcept override {
        if (start + length > _data.size()) { return {}; }
        return {_data.data() + start, length};
    }
};
}  // namespace Manganese::io