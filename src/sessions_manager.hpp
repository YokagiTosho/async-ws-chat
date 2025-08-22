#ifndef SESSIONS_BROADCAST_HPP
#define SESSIONS_BROADCAST_HPP

#include "session.hpp"
#include "debug.hpp"

#include <unordered_set>


class sessions_manager {
public:

	void add(std::shared_ptr<session> &s) {
		m_sessions.insert(s);
	}

	void remove(const std::shared_ptr<session> &session) {
		if (auto it = m_sessions.find(session); it != m_sessions.end()) {
			m_sessions.erase(session);
			__debug("Removed session");
		}
	}

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
	std::unordered_set<std::shared_ptr<session>> m_sessions;
};

#endif
