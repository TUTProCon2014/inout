#pragma once


namespace utils
{
enum class Target
{
    Win32, OSX, Linux,
};


#if defined( __WIN32__ ) || defined( _WIN32 )
    #define TARGET_WIN32
    constexpr Target buildTarget = Target::Win32;
#elif defined( __APPLE_CC__)
    #define TARGET_OSX
    constexpr Target buildTarget = Target::OSX;
#else
    #define TARGET_LINUX
    constexpr Target buildTarget = Target::Linux;
#endif


};