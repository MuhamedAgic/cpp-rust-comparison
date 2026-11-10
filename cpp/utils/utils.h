#include <typeinfo>
#include <cxxabi.h>
#include <cstdlib>
#include <memory>

template <typename T>
std::string demangle() {
    int status = 0;

    std::unique_ptr<char, decltype(&std::free)> name(
        abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, &status),
        &std::free
    );

    return status == 0 ? name.get() : typeid(T).name();
}