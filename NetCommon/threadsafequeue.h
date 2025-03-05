#pragma once
#include "common.h"

namespace net
{
    template <typename DataType>
    class ThreadSafeQueue
    {
    public:
        ThreadSafeQueue() = default;
        ThreadSafeQueue(const ThreadSafeQueue<DataType>&) = delete;
        virtual ~ThreadSafeQueue() { clear(); }
        const DataType& front() const
        {
            std::lock_guard lock(m_mutex);
            return m_queue.front();
        }

        const DataType& back() const
        {
            std::lock_guard lock(m_mutex);
            return m_queue.back();
        }

        void push_back(const DataType& item)
        {
            std::lock_guard lock(m_mutex);
            m_queue.emplace_back(std::move(item));
        }

        void push_front(const DataType& item)
        {
            std::lock_guard lock(m_mutex);
            m_queue.emplace_front(std::move(item));
        }

        bool empty() const
        {
            std::lock_guard lock(m_mutex);
            return m_queue.empty();
        }

        size_t count() const
        {
            std::lock_guard lock(m_mutex);
            return m_queue.size();
        }

        void clear()
        {
            std::lock_guard lock(m_mutex);
            m_queue.clear();
        }

        DataType pop_front()
        {
            std::lock_guard lock(m_mutex);
            auto elem = std::move(m_queue.front());
            m_queue.pop_front();
            return elem;
        }

    private:
        std::mutex m_mutex;
        std::deque<DataType> m_queue;
    };
}