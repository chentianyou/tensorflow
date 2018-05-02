#include <string>
#include <iostream>
#include <memory>

int main() {
    std::unique_ptr<std::string> str;
    str->append("hello world !");
    std::cout << *str << std::endl;
}