#include "time_delta.h"

namespace TwoHalfD
{
class EngineClocks
{
  private:
    double m_graphicsFpsCap;
    double m_gameFpsCap;
    bool m_graphicsFpsUncapped;
    bool m_gamesFpsUncapped;
    TimeDelta m_graphicsTimeDelta{1 / m_graphicsFpsCap};
    TimeDelta m_gameTimeDelta{1 / m_gameFpsCap};

  public:
    EngineClocks(double graphicsFpsCap = 100.0, double gameFpsCap = 60.0, bool graphicsFpsUncapped = false, bool gamesFpsUncapped = false)
        : m_graphicsFpsCap(graphicsFpsCap), m_gameFpsCap(gameFpsCap), m_graphicsFpsUncapped(graphicsFpsUncapped), m_gamesFpsUncapped(gamesFpsUncapped)
    {
    }

    bool graphicsTimeDeltaPassed();
    bool gameTimeDeltaPassed();
};
} // namespace TwoHalfD