#ifndef CIRCULARBUFFER_H
#define CIRCULARBUFFER_H

#include <array>
#include <cassert>


// Will keep but think is over kill
// Want to process all inputs each frame so using array
// where input pointer return to 0 each frame is easier
template <typename T, std::size_t m_size>
class CircularBuffer
{
private:
    std::array<T, m_size> m_buffer{};
    int m_startP { 0 };
    int m_endP { 0 };

public:

    CircularBuffer()
    {
        static_assert(m_size > 0, "CircularBuffer size must be greater than 0");
    }

    constexpr std::size_t size() const { return m_size; }
    
    std::size_t capacity() const { return (m_size + m_startP - m_endP); }
    bool empty() const { return m_startP == m_endP; }
    bool full() const { return (m_endP + 1) % m_size == m_startP; }

    void push(T pushData) {
        assert(!full());
        m_buffer[m_endP] = pushData;
        m_endP = (m_endP + 1) % m_size;
    }

    T pop() {
        assert(!empty());
        T returnVal = m_buffer[m_startP];
        m_startP = (m_startP + 1) % m_size;
        return returnVal;
    }
};

#endif
