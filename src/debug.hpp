#ifndef DEBUG_HPP
#define DEBUG_HPP

#include <iostream>
#include <string_view>

template<typename... Args>
inline void __debug(Args... args) {
#if defined(DEBUG)
	((std::cout << args << " "), ...) << std::endl;
#endif
}

#endif
