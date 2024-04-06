#include <iostream>

class Base {
public:
    Base() {
        std::cout << "Base constructor" << std::endl;
    }
    // забыл сделать деструктор виртуальным, поэтому утечка памяти
    ~Base() {
        std::cout << "Base destructor" << std::endl;
    }
};

class Derived : public Base {
private:
    int* data;
public:
    Derived() {
        std::cout << "Derived constructor" << std::endl;
        data = new int[10];
    }
    ~Derived() {
        std::cout << "Derived destructor" << std::endl;
        delete[] data;
    }
};

int main() {
    Base* ptr = new Derived();
    delete ptr; 
    return 0;
}
