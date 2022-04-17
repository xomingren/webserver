#pragma once

#include <sys/types.h>

//#include <concepts>
#include <string>

class Timestamp //: public std::equality_comparable<Timestamp>//public std::less_than_comparable<Timestamp>               
{
public:
    Timestamp()
        : microsecondssinceepoch_(0)
    {
    }
    explicit Timestamp(int64_t microSecondsSinceEpochArg)
        : microsecondssinceepoch_(microSecondsSinceEpochArg)
    {
    }
    // default copy/assignment/dtor are Okay
    ~Timestamp() = default;
    
    std::string ToString(bool showmicroseconds = true) const;
    bool Valid() const
    { return microsecondssinceepoch_ > 0; }

    int64_t get_microsecondssinceepoch() const
    { return microsecondssinceepoch_; }

    static Timestamp Now();
    static Timestamp Invalid()//0
    {  return Timestamp();  }

    static int64_t NowMicroSeconds();
    static const int kMicroSecondsPerSecond = 1000 * 1000;
    
private:
    int64_t microsecondssinceepoch_;
};
inline bool operator<(Timestamp lhs, Timestamp rhs)
{
    return lhs.get_microsecondssinceepoch() < rhs.get_microsecondssinceepoch();
}

inline bool operator==(Timestamp lhs, Timestamp rhs)
{
    return lhs.get_microsecondssinceepoch() == rhs.get_microsecondssinceepoch();
}

inline double timeDifference(Timestamp high, Timestamp low)//seconds
{
    int64_t diff = high.get_microsecondssinceepoch() - low.get_microsecondssinceepoch();
    return static_cast<double>(diff) / Timestamp::kMicroSecondsPerSecond;
}

inline Timestamp addTime(Timestamp timestamp, double seconds)
{
    int64_t delta = static_cast<int64_t>(seconds * Timestamp::kMicroSecondsPerSecond);
    return Timestamp(timestamp.get_microsecondssinceepoch() + delta);
}

