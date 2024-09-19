#include <yinsh-gui/system.hpp>

#if defined(__linux__)
#include <sys/sysinfo.h>
#elif defined(_WIN32)
#include <Windows.h>
#elif defined(EMSCRIPTEN)
#include <emscripten/threading.h>
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
#elif defined(EMSCRIPTEN)
std::size_t get_system_memory() {
    // Return 1.5GB
    return 1536 * 1024 * 1024;
}
#endif

#if defined(__linux__)
int get_system_threads() {
    return get_nprocs();
}
#elif defined(_WIN32)
int get_system_threads() {
    SYSTEM_INFO system_info;
    GetSystemInfo(&system_info);

    return system_info.dwNumberOfProcessors;
}
#elif defined(EMSCRIPTEN)
int get_system_threads() {
    return emscripten_num_logical_cores();
}
#endif
