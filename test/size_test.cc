#include <string>
#include <iostream>
#include "test/static_test.h"

int main(){
    std::cout << sizeof(int64_t) << std::endl;
    std::cout << sizeof(std::string) << std::endl;
    std::cout << RngSupport::Max << std::endl;
    std::cout << RngSupport::Min << std::endl;
    return 0;
}