#pragma once

namespace carepilot {

class IDGenerator {
public:
    explicit IDGenerator(int start = 1) : next_(start) {}
    int next() { return next_++; }
    int peek() const { return next_; }
    void reserve(int usedId) { if (usedId >= next_) next_ = usedId + 1; }   // after loading data

private:
    int next_;
};

} // namespace carepilot