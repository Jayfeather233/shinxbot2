#include "processable.h"
#include "eventprocess.h"
#include <string>
#include <dlfcn.h>

template<typename T>
std::pair<T *, void *> load_function(const std::string &dlname, const std::string &func = "create")
{
    // Function pointer type for the create function
    typedef T *(*create_t)();
    void *handle = dlopen(dlname.c_str(), RTLD_LAZY);
    if (!handle) {
        std::cerr << "Cannot open library: " << dlerror() << std::endl;
        return std::make_pair(nullptr, nullptr);
    }

    // Load the factory function
    create_t createx = (create_t)dlsym(handle, func.c_str());
    const char *dlsym_error = dlerror();
    if (dlsym_error) {
        std::cerr << "Cannot load symbol 'create': " << dlsym_error
                  << std::endl;
        dlclose(handle);
        return std::make_pair(nullptr, nullptr);
    }

    // Use the factory function to create an object
    return std::make_pair(createx(), handle);
}