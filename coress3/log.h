#ifndef CORE_LOG_H_
#define CORE_LOG_H_

#include <thread>

#if not defined(NDEBUG)
#define LOG(msg) \
    cout << boolalpha << __TIME__ << " T:" << this_thread::get_id() << " " << __FILE__ << "(" << __LINE__ << "): " << msg << std::endl
#else
#define LOG(msg)
#endif

#endif //CORE_LOG_H_
