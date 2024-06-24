#ifndef MINI_SPAN_H_
#define MINI_SPAN_H_

#include <algorithm>
#include <cstddef>

class Position {
public:
    Position(size_t row, size_t offset) : row_(row), offset_(offset) {}
    inline size_t row() const { return row_; }
    inline size_t offset() const { return offset_; }
    bool operator==(const Position& rhs) const {
        return row_ == rhs.row_ && offset_ == rhs.offset_;
    }
    bool operator<(const Position& rhs) const {
        if (row_ < rhs.row_) {
            return true;
        } else if (row_ == rhs.row_) {
            return offset_ < rhs.offset_;
        } else {
            return false;
        }
    }
    bool operator!=(const Position& rhs) const { return !(*this == rhs); }
    bool operator<=(const Position& rhs) const {
        return *this < rhs || *this == rhs;
    }
    bool operator>(const Position& rhs) const { return !(*this <= rhs); }
    bool operator>=(const Position& rhs) const { return !(*this < rhs); }

private:
    const size_t row_;
    const size_t offset_;
};

class Span {
public:
    Span(Position start, Position end) : start_(start), end_(end) {}
    Span operator+(const Span& rhs) const {
        return Span(std::min(start_, rhs.start_), std::max(end_, rhs.end_));
    }
    inline Position start() const { return start_; }
    inline Position end() const { return end_; }

private:
    const Position start_;
    const Position end_;
};

#endif  // MINI_SPAN_H_
