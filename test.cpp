#include <iostream>
#include <stdexcept>


class Base {
public:
    virtual void foo() {
        throw std::runtime_error("lalala error");
        std::cout << "Base::foo()" << std::endl;
    }

    virtual void bar() {
        std::cout << "Base::bar()" << std::endl;
    }
};

int main() {
    Base obj;
    uintptr_t* vptr = reinterpret_cast<uintptr_t*>(&obj);

    // Assuming the vptr is the first data member in the object layout
    uintptr_t* vtable = reinterpret_cast<uintptr_t*>(*vptr);

    // Assuming the virtual function table entry for foo() is the first entry
    typedef void (*FunctionPointer)();
    FunctionPointer firstFunction = reinterpret_cast<FunctionPointer>(vtable[0]);
    try
    {
        // Call the first function in the virtual function table
        firstFunction();
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
    }

    return 0;
}
