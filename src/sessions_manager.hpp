#ifndef SESSIONS_BROADCAST_HPP
#define SESSIONS_BROADCAST_HPP

#include "session.hpp"

#include <unordered_set>

class sessions_manager {
public:
	void add(std::shared_ptr<session> s) {
		m_sessions.insert(s);
	}

	void remove(std::shared_ptr<session> session) {
		if (auto it = m_sessions.find(session); it != m_sessions.end()) {
			m_sessions.erase(session);
		}
	}

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

	void print() const {
		std::cout << "Count: " << m_sessions.size() << std::endl;
	}

	void broadcast(const std::string &msg) {
			for (auto session: m_sessions) {
				session->do_write(msg);
			}
	}

private:
	std::unordered_set<std::shared_ptr<session>> m_sessions;
};


#endif
