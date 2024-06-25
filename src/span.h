#ifndef MINI_SPAN_H_
#define MINI_SPAN_H_

#include <algorithm>
#include <cassert>
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
    size_t row_;
    size_t offset_;
};

class Span {
public:
    Span(size_t id, Position start, Position end)
        : id_(id), start_(start), end_(end) {}
    Span operator+(const Span& rhs) const {
        assert(id_ == rhs.id_);
        return Span(id_, std::min(start_, rhs.start_),
                    std::max(end_, rhs.end_));
    }
    inline size_t id() const { return id_; }
    inline Position start() const { return start_; }
    inline Position end() const { return end_; }

private:
    size_t id_;
    Position start_;
    Position end_;
};

#endif  // MINI_SPAN_H_
