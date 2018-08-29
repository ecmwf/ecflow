#ifndef perf_timer_HPP_
#define perf_timer_HPP_
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
// Description :
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#include <chrono>

// remove when we use c++17
template<typename F, typename... Args>
auto invoke(F f, Args&&... args) -> decltype(std::ref(f)(std::forward<Args>(args)...))
{
  return std::ref(f)(std::forward<Args>(args)...);
}

template <typename Time = std::chrono::microseconds,
          typename Clock = std::chrono::high_resolution_clock>
struct perf_timer {
   template <typename F, typename... Args>
   static Time duration(F&& f, Args... args) {
      auto start = Clock::now();

      // std::invoke(std::forward<F>(f), std::forward<Args>(args)...); // c++17
      invoke(std::forward<F>(f), std::forward<Args>(args)...);

      auto end = Clock::now();
      return std::chrono::duration_cast<Time>(end - start);
   }
};

template<class Resolution = std::chrono::milliseconds>
class Timer {
public:
   using Clock = std::conditional_t<std::chrono::high_resolution_clock::is_steady,
                                    std::chrono::high_resolution_clock,
                                    std::chrono::steady_clock>;
private:
   const Clock::time_point start_ = Clock::now();
public:
   Timer() = default;
   ~Timer() {}
   void elapsed(const char* msg) const {
      const auto end = Clock::now();
      std::cout << msg << " " << std::chrono::duration_cast<Resolution>(end - start_).count() << std::endl;
   }

   Resolution elapsed() const {
      return std::chrono::duration_cast<Resolution>(Clock::now() - start_);
   }
};


#endif
