#pragma once

#include <sys/types.h>

//#include <concepts>
#include <string>


class Timestamp //:  public std::equality_comparable<>
                  //public std::less_than_comparable<Timestamp>               
{
public:
    Timestamp()
        : microsecondssinceepoch_(0)
    {
    }
    explicit Timestamp(uint64_t microSecondsSinceEpochArg)
        : microsecondssinceepoch_(microSecondsSinceEpochArg)
    {
    }
    ~Timestamp() = default;
    
    std::string ToString(bool showmicroseconds = true) const;
    bool Valid() const
    { return microsecondssinceepoch_ > 0; }

    uint64_t get_microsecondssinceepoch() const
    { return microsecondssinceepoch_; }

    static Timestamp Now();
    static Timestamp Invalid()//0
    {  return Timestamp();  }

    static uint64_t NowMicroSeconds();
    static constexpr const int kMicroSecondsPerSecond = 1000 * 1000;
    
private:
    uint64_t microsecondssinceepoch_;
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
    uint64_t diff = high.get_microsecondssinceepoch() - low.get_microsecondssinceepoch();
    return static_cast<double>(diff) / Timestamp::kMicroSecondsPerSecond;
}

inline Timestamp addTime(Timestamp timestamp, double seconds)
{
    uint64_t delta = static_cast<uint64_t>(seconds * Timestamp::kMicroSecondsPerSecond);
    return Timestamp(timestamp.get_microsecondssinceepoch() + delta);
}

