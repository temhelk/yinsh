#include <yinsh-gui/system.hpp>

#ifdef __linux__
#include <sys/sysinfo.h>
#elif defined(_WIN32)
#include <Windows.h>
#endif

#if defined(__linux__)
std::size_t get_system_memory() {
    struct sysinfo system_info;
    sysinfo(&system_info);

    return system_info.totalram;
}
#elif defined(_WIN32)
std::size_t get_system_memory() {
    std::size_t total_memory;
    GetPhysicallyInstalledSystemMemory(&total_memory);
    return total_memory * 1024;
}
#endif

