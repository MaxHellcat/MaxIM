#ifndef BASIC_HPP
#define BASIC_HPP

// Modify the following defines if you have to target a platform prior to the ones specified below.
// Refer to MSDN for the latest info on corresponding values for different platforms.
#ifndef _WIN32_WINNT            // Specifies that the minimum required platform is Windows Vista.
#define _WIN32_WINNT 0x0600     // Change this to the appropriate value to target other versions of Windows.
#endif

// STL library
#include <iostream>

typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned int dword;

#define IM_DEBUG

// TODO: Class for logging?
void log(const char * s)
{
	s;
#ifdef IM_DEBUG
	std::cout << s << std::endl;
#endif
}


#endif // #ifndef BASIC_HPP