#include <iostream>
#include <string>
#include <utility>

// Use:
// gcc -Wall -Wextra -Werror Tracer.cpp -lstdc++
// g++ -Wall -Wextra -Werror Tracer.cpp

class MyType {
public:
   MyType(std::string str) : mName(std::move(str)) {
      std::cout << "MyType::MyType " << mName << '\n';
   }

   ~MyType() {
      std::cout << "MyType::~MyType " << mName << '\n';
   }

   MyType(const MyType& other) : mName(other.mName) {
      std::cout << "MyType::MyType(const MyType&) " << mName << '\n';
   }

   MyType(MyType&& other) noexcept : mName(std::move(other.mName)) {
      std::cout << "MyType::MyType(MyType&&) " << mName << '\n';
   }

   MyType& operator=(const MyType& other) {
      if (this != &other)
         mName = other.mName;
      std::cout << "MyType::operator=(const MyType&) " << mName << '\n';
      return *this;
   }

   MyType& operator=(MyType&& other) noexcept {
      if (this != &other)
         mName = std::move(other.mName);
      std::cout << "MyType::operator=(MyType&&) " << mName << '\n';
      return *this;
   }

private:
   std::string mName;
};

int main()
{
   {
      MyType type("ABC");
      auto tmoved = std::move(type);
   }

   {
      MyType tassigned("XYZ");
      MyType temp("ABC");
      tassigned = std::move(temp);
   }
}
