#ifndef SESSIONS_BROADCAST_HPP
#define SESSIONS_BROADCAST_HPP

#include "session.hpp"
#include "debug.hpp"

#define USE_UNORDERED_SET

//#undef USE_UNORDERED_SET

#if defined(USE_UNORDERED_SET)
#include <unordered_set>
#else
#include <algorithm>
#endif

class sessions_manager {
public:
#if defined(USE_UNORDERED_SET)
	void add(std::shared_ptr<session> &s) {
		m_sessions.insert(s);
	}

	void remove(const std::shared_ptr<session> &session) {
		if (auto it = m_sessions.find(session); it != m_sessions.end()) {
			m_sessions.erase(session);
			__debug("Removed session");
		}
	}
#else
	void add(std::shared_ptr<session> &s) {
		m_sessions.push_back(s);
	}

	void remove(const std::shared_ptr<session> &session) {
		auto n = std::find(m_sessions.begin(), m_sessions.end(), session);
		if (n != m_sessions.end()) {
			m_sessions.erase(n);
			__debug("Removed session");
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

	auto size() const {
		return m_sessions.size();

	}

	void broadcast(const std::string &msg) {
			for (auto session: m_sessions) {
				session->write(msg);
			}
	}

	void close_sessions() {
		for (auto &s: m_sessions) {
			s->close();
		}
		m_sessions.clear();
		__debug("Closed sessions");
	}

private:
#if defined(USE_UNORDERED_SET)
	std::unordered_set<std::shared_ptr<session>> m_sessions;
#else
	std::vector<std::shared_ptr<session>> m_sessions;
#endif
};

#endif
