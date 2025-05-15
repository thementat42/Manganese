/**
 * @file filereader.cpp
 * @brief This file contains the implementation of the FileReader class
 */

#include "include/filereader.h"

#include <stdio.h>

#include <fstream>
#include <memory>
#include <string>

#include "../global_macros.h"
#include "include/filereader.h"

MANG_BEGIN
namespace io {
FileReader::FileReader(const std::string& filename, size_t _bufferCapacity) : position(0), line(1), column(1), bufferCapacity(_bufferCapacity) {
    fileStream.open(filename, std::ios::in);
    if (!fileStream.is_open()) {
        fprintf(stderr, "Error: Could not open file %s\n", filename.c_str());
        exit(EXIT_FAILURE);
    }
    buffer = std::make_unique<char[]>(_bufferCapacity + 1);                              // +1 for null terminator
    fileStream.get(buffer.get(), static_cast<std::streamsize>(_bufferCapacity));         // Read the first chunk of data into the buffer
    bufferSize = static_cast<size_t>(std::max<std::streamsize>(0, fileStream.gcount())); // Ensure non-negative before casting

    if (bufferSize == 0) {
        fprintf(stderr, "Error: File %s is empty or could not be read\n", filename.c_str());
        exit(EXIT_FAILURE);
    }
    buffer[bufferSize] = '\0'; // Null-terminate the buffer
}

FileReader::~FileReader() {
    if (fileStream.is_open()) {
        fileStream.close();
    }
}

bool isLexemeEnd(char c) {
    return c == ' ' || c == '\n' || c == '\t' ||
           c == ';' || c == ',' || c == '{' ||
           c == '}' || c == '(' || c == ')';
}

void FileReader::refillBuffer() {
    // Save any remaining data that hasn't been processed yet
    size_t remainingSize = 0;
    if (position < bufferSize) {
        remainingSize = bufferSize - position;
        // Move remaining data to beginning of buffer
        memmove(buffer.get(), buffer.get() + position, remainingSize);
    }

    // Reset position since we moved remaining data to front
    position = 0;

    // Read new data after the remaining data
    if (!fileStream.eof()) {
        fileStream.read(buffer.get() + remainingSize, static_cast<std::streamsize>(bufferCapacity - remainingSize));
        bufferSize = remainingSize + static_cast<size_t>(std::max<std::streamsize>(0, fileStream.gcount()));  // ensure non-negative before casting
    } else {
        bufferSize = remainingSize;
    }

    // Always null-terminate
    buffer[bufferSize] = '\0';

    // If we're at a token boundary but not at EOF, try to read until a good stopping point
    if (bufferSize > 0 && !fileStream.eof()) {
        // Keep reading until we hit a good stopping point or reach buffer capacity
        while (bufferSize < bufferCapacity && !fileStream.eof()) {
            char nextChar;
            fileStream.get(nextChar);
            if (fileStream.eof()) break;

            // Add the character to our buffer
            buffer[bufferSize++] = nextChar;
            buffer[bufferSize] = '\0';

            // Check if this is a good stopping point
            if (isLexemeEnd(nextChar)) {
                break;
            }
        }
    }
}
char FileReader::peekChar(size_t offset) {
    // Check if we need more data
    if (position + offset >= bufferSize) {
        if (fileStream.eof()) {
            return EOF_CHAR;
        }
        refillBuffer();

        // If still out of bounds after refill, return EOF
        if (position + offset >= bufferSize) {
            return EOF_CHAR;
        }
    }
    return buffer[position + offset];
}

char FileReader::getChar() {
    // Check if we need more data
    if (position >= bufferSize) {
        if (fileStream.eof() && position >= bufferSize) {
            return EOF_CHAR;
        }
        refillBuffer();

        // If still out of bounds after refill, return EOF
        if (position >= bufferSize) {
            return EOF_CHAR;
        }
    }

    char c = buffer[position++];
    if (c == '\n') {
        ++line;
        column = 1;
    } else {
        ++column;
    }
    return c;
}

void FileReader::setPosition(size_t newPosition) {
    if (newPosition >= bufferSize) {
        position = bufferSize;
    } else {
        position = newPosition;
    }
}

inline size_t FileReader::getPosition() const {
    return position;
}

inline size_t FileReader::getLine() const {
    return line;
}

inline size_t FileReader::getColumn() const {
    return column;
}

inline bool FileReader::done() const {
    return position >= bufferSize && fileStream.eof();
}

} // namespace io
MANG_END