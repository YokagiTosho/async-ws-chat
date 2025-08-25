#ifndef PROTO_PARSER_HPP
#define PROTO_PARSER_HPP

#include <cctype>
#include <iostream>
#include "debug.hpp"
#include <unordered_map>
#include <algorithm>
#include <sstream>
#include <unordered_set>

class proto_parser {
public:
	enum class state {
		get_nick,
		get_room,
		ready,
	};


	enum class error {
		ok,
	};

	state next_state(const std::string &buf) {
		if (m_state == state::ready) return m_state;

		std::stringstream ss(buf);
		std::string directive, value;

		ss >> directive;

		if (!directive_exists(directive)) {
			return m_state;
		}

		/*
		  @ read only the first word
		  @ even if there is a space, because no space are allowed, do not care about the rest
		*/
		ss >> value;

		if (!value.size()) {
			return m_state;
		}

		// remove forbidden chars
		filter_string(value);

		switch (m_directives[directive]) {
		case state::get_nick:
			m_nick = std::move(value);

			m_state = state::get_room;
			break;
		case state::get_room:
			m_room = std::move(value);

			m_state = state::ready;
			break;
		default:
			break;
		}

		return m_state;
	}

	const std::string &nick() const
	{ return m_nick; }

	const std::string &room() const
	{ return m_room; }

	const state get_state() const
	{ return m_state; }

private:
	void filter_string(std::string &s) {
		s.erase(std::remove_if(s.begin(), s.end(),
							   [this](auto c)
							   { return m_forbidden_chars.find(c) != m_forbidden_chars.end(); }),
				s.end());
	}

	bool directive_exists(const std::string &directive) {
		auto directive_it = m_directives.find(directive);

		if (directive_it == m_directives.end()) {
			std::cerr
				<< "Incorrect directive"
				<< std::endl;

			return false;
		}
		return true;
	}

	std::string m_nick;
	std::string m_room;
	state m_state { state::get_nick };

	std::unordered_map<std::string, state>
	m_directives {
		{"NICK", state::get_nick},
		{"ROOM", state::get_room}
	};

	std::unordered_set<char> m_forbidden_chars
		{ '*', '\\', '$', '+',
		  '$', ',', '@', '<',
		  '>', '&', '?', ':',
		  ';', '"', '\'', '[',
		  ']', '%', '!', '{',
		  '}', '^', '(', ')',
		  '#', '~', '`', '.',
		  '|', '/', '-',
		};

};

#endif
