#include <array>
#include <cstddef>
#include <iterator>

namespace TwoHalfD
{
template <typename T, int capacity, bool override = false> class CircularBuffer
{
  private:
    std::array<T, capacity> m_buffer;
    int m_size;
    int m_front;
    int m_back;

  public:
    CircularBuffer() : m_size(0), m_front(0), m_back(0) {}

    int size() const noexcept
    {
        return m_size;
    }
    int getCapacity() const noexcept
    {
        return capacity;
    }

    bool empty() const noexcept
    {
        return m_size == 0;
    }

    void push(const T &item)
    {
        if (m_size == capacity && !override)
            return;

        m_buffer[m_front] = item;
        m_front = (m_front + 1) % capacity;

        if (m_size < capacity)
        {
            ++m_size;
        }
        else if (override)
        {
            m_back = (m_back + 1) % capacity;
        }
    }

    void push(T &&item)
    {
        if (m_size == capacity && !override)
            return;

        m_buffer[m_front] = std::move(item);

        m_front = (m_front + 1) % capacity;

        if (m_size < capacity)
            ++m_size;

        else if (override)
            m_back = (m_back + 1) % capacity;
    }

    T &top()
    {
        return m_buffer[m_back];
    }

    const T &top() const
    {
        return m_buffer[m_back];
    }

    void pop()
    {
        if (m_size == 0)
        {
            return;
        }

        m_back = (m_back + 1) % capacity;
        --m_size;
    }

    // NOTE: Mutating ops (push that overwrites, pop) will invalidate existing iterators
    class Iterator
    {
      private:
        CircularBuffer *m_circBuf;
        int m_index; // offset from m_back: 0..m_size

      public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = T;
        using pointer = T *;
        using reference = T &;

        Iterator(CircularBuffer *buf = nullptr, int index = 0) : m_circBuf(buf), m_index(index) {}

        reference operator*() const
        {
            int actualIndex = (m_circBuf->m_back + m_index) % capacity;
            return m_circBuf->m_buffer[actualIndex];
        }

        pointer operator->() const
        {
            return &**this;
        }

        Iterator &operator++()
        {
            ++m_index;
            return *this;
        }

        Iterator operator++(int)
        {
            Iterator tmp = *this;
            ++m_index;
            return tmp;
        }

        bool operator==(const Iterator &other) const
        {
            return m_circBuf == other.m_circBuf && m_index == other.m_index;
        }

        bool operator!=(const Iterator &other) const
        {
            return !(*this == other);
        }
    };

    Iterator begin()
    {
        return Iterator(this, 0);
    }
    Iterator end()
    {
        return Iterator(this, m_size);
    }

    class ConstIterator
    {
      private:
        const CircularBuffer *m_circBuf;
        int m_index;

      public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = T;
        using pointer = const T *;
        using reference = const T &;

        ConstIterator(const CircularBuffer *buf = nullptr, int index = 0) : m_circBuf(buf), m_index(index) {}

        reference operator*() const
        {
            int actualIndex = (m_circBuf->m_back + m_index) % capacity;
            return m_circBuf->m_buffer[actualIndex];
        }

        pointer operator->() const
        {
            return &**this;
        }

        ConstIterator &operator++()
        {
            ++m_index;
            return *this;
        }

        ConstIterator operator++(int)
        {
            ConstIterator tmp = *this;
            ++m_index;
            return tmp;
        }

        bool operator==(const ConstIterator &other) const
        {
            return m_circBuf == other.m_circBuf && m_index == other.m_index;
        }

        bool operator!=(const ConstIterator &other) const
        {
            return !(*this == other);
        }
    };

    ConstIterator begin() const
    {
        return ConstIterator(this, 0);
    }
    ConstIterator end() const
    {
        return ConstIterator(this, m_size);
    }
};
} // namespace TwoHalfD
