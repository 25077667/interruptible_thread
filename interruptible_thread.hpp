#ifndef __SCC_INTERRUPTABLE_THREAD_HPP__
#define __SCC_INTERRUPTABLE_THREAD_HPP__

#pragma once

#ifdef _WIN32
#include <internal/windows_intt.hpp>
#elif __linux__ || __APPLE__ || __FreeBSD__
#include <internal/posix_intt.hpp>
#else
#error "Unsupported platform"
#endif

#endif // __SCC_INTERRUPTABLE_THREAD_HPP__