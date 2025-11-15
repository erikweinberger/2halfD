#include <chrono>

namespace TwoHalfD
{

class TimeDelta
{
  private:
    double m_timeDelta;
    double m_lastDeltaDurationMilli;
    double m_isUncapped;
    std::chrono::time_point<std::chrono::system_clock> m_lastTimeDeltaIntervalStart;
    std::chrono::time_point<std::chrono::system_clock> m_lastTime;

  public:
    TimeDelta(double timeDelta, bool isUncapped = false)
        : m_timeDelta(timeDelta), m_lastDeltaDurationMilli(0.01), m_isUncapped(isUncapped),
          m_lastTimeDeltaIntervalStart(std::chrono::system_clock::now()), m_lastTime((std::chrono::system_clock::now()))
    {
    }

    bool timeDeltaPassed();
    double getLastDeltaDuration();
};

} // namespace TwoHalfD