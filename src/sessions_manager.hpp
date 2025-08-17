#ifndef SESSIONS_BROADCAST_HPP
#define SESSIONS_BROADCAST_HPP

#include "session.hpp"

#include <unordered_set>
#include <algorithm>

#define USE_UNORDERED_SET
#undef USE_UNORDERED_SET

class sessions_manager {
public:
#ifdef USE_UNORDERED_SET
	void add(std::shared_ptr<session> s) {
		m_sessions.insert(s);
	}

	void remove(std::shared_ptr<session> session) {
		if (auto it = m_sessions.find(session); it != m_sessions.end()) {
			m_sessions.erase(session);
			std::cout << "Removed session" << std::endl;
		}
	}
#else
	void add(std::shared_ptr<session> s) {
		m_sessions.push_back(s);
	}

	void remove(std::shared_ptr<session> session) {
		auto n = std::find(m_sessions.begin(), m_sessions.end(), session);
		if (n != m_sessions.end()) {
			m_sessions.erase(n);
			std::cout << "Removed session" << std::endl;
		}
	}
#endif
	sessions_manager()
	{}

	sessions_manager(sessions_manager &&m)
		: m_sessions(std::move(m.m_sessions))
	{}

	auto begin() {
		return m_sessions.begin();
	}

	auto end() {
		return m_sessions.end();
	}

	auto cbegin() const {
		return m_sessions.cbegin();
	}

	auto cend() const {
		return m_sessions.cend();
	}

	void print_size() const {
		std::cout << "Count: " << m_sessions.size() << std::endl;
	}

	void broadcast(const std::string &msg) {
			for (auto session: m_sessions) {
				session->do_write(msg);
			}
	}

	void close_sessions() {
		// on session closed 'sessions_manager::remove' will be invoked whill 
		// will erase element, but its done sequentially so should be safe
		// in very rare cases a client can disconnect while closing server
		// so may throw
		for (auto &s: m_sessions) {
			s->close();
		}
	}

private:
#ifdef USE_UNORDERED_SET
	std::unordered_set<std::shared_ptr<session>> m_sessions;
#else
	std::vector<std::shared_ptr<session>> m_sessions;
#endif
};


#endif
