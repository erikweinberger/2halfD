#include <chrono>

namespace TwoHalfD
{

class TimeDelta
{
  private:
    double m_timeDelta;
    std::chrono::time_point<std::chrono::system_clock> m_lastTimeDeltaIntervalStart;
    std::chrono::time_point<std::chrono::system_clock> m_lastTime;

  public:
    TimeDelta(double timeDelta)
        : m_timeDelta(timeDelta), m_lastTimeDeltaIntervalStart(std::chrono::system_clock::now()), m_lastTime((std::chrono::system_clock::now()))
    {
    }

    bool timeDeltaPassed();
};

} // namespace TwoHalfD