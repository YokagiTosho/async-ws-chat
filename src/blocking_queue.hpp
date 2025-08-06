#ifndef __BLOCKING_QUEUE_HPP
#define __BLOCKING_QUEUE_HPP

#include <queue>
#include <mutex>
#include <condition_variable>

template<typename T>
class BQueue {
private:
	std::queue<T> m_queue;
	std::mutex m_mutex;
	std::condition_variable m_not_empty;
	std::condition_variable m_not_full;
	size_t m_capacity;
public:
	BQueue(size_t capacity = 0)
		: m_capacity(capacity)
	{}

	void push(const T& value) {
		std::unique_lock<std::mutex> lck(m_mutex);

		if (m_capacity > 0) {
			m_not_full.wait(lck, [this]{ return m_queue.size() < m_capacity; });
		}
		m_queue.push(value);
		m_not_empty.notify_one();
	}

	void push(T &&value) {
		std::unique_lock<std::mutex> lck(m_mutex);

		if (m_capacity > 0) {
			m_not_full.wait(lck, [this]{ return m_queue.size() < m_capacity; });
		}
		m_queue.push(std::forward<T>(value));
		m_not_empty.notify_one();
	}

	T pop() {
		std::unique_lock<std::mutex> lck(m_mutex);
		m_not_empty.wait(lck, [this]{ return !m_queue.empty(); });

		T value = m_queue.front();
		m_queue.pop();

		if (m_capacity > 0) {
			m_not_full.notify_one();
		}
		return value;
	}

	auto size() const {
		return m_queue.size();
	}

};

#endif
