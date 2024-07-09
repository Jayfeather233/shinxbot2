#include "eventprocess.h"
#include "processable.h"
#include <dlfcn.h>
#include <string>

template <typename T>
std::pair<T *, void *> load_function(const std::string &dlname,
                                     const std::string &func = "create")
{
    // Function pointer type for the create function
    typedef T *(*create_t)();
    void *handle = dlopen(dlname.c_str(), RTLD_LAZY);
    if (!handle) {
        set_global_log(LOG::ERROR, std::string("Cannot open library: ") + dlerror());
        return std::make_pair(nullptr, nullptr);
    }

    // Load the factory function
    create_t createx = (create_t)dlsym(handle, func.c_str());
    const char *dlsym_error = dlerror();
    if (dlsym_error) {
        set_global_log(LOG::ERROR,
                       std::string("Cannot load symbol 'create': ") + dlsym_error);
        dlclose(handle);
        return std::make_pair(nullptr, nullptr);
    }

    // Use the factory function to create an object
    return std::make_pair(createx(), handle);
}