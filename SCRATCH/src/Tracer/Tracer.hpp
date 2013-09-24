#ifndef TRACER_HPP_
#define TRACER_HPP_

#include <iostream>
#include <string>

class Tracer {
public:
   Tracer() { std::cout << "Tracer::Tracer()\n";}
   Tracer(const Tracer& rhs) { std::cout << "Tracer::Tracer(const Tracer& rhs)\n";}
   Tracer& operator=(const Tracer& rhs) { std::cout << "Tracer::operator=(const Tracer& rhs)\n"; return *this;}
   ~Tracer() { std::cout << "Tracer::~Tracer()\n"; }

   void print(const std::string& s) const { std::cout << s << '\n';}
};

#endif
