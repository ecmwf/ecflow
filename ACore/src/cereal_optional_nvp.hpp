#ifndef cereal_optional_nvp_h_
#define cereal_optional_nvp_h_
/*
The MIT License (MIT)
Copyright (c) 2017 Yehonatan Ballas
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

/*
  Example

struct B
{
   int x, y, z;

   template <class Archive>
   void serialize(Archive & ar)
   {
      cereal::make_optional_nvp(ar, "x", x);
      CEREAL_OPTIONAL_NVP(ar, y);
      CEREAL_OPTIONAL_NVP(ar, z, [this]() {return z != 1; }); // conditionally save
   }
};
 */

#include <cereal/details/traits.hpp>
#include <cereal/cereal.hpp>

namespace cereal
{
   class JSONInputArchive;
   class XMLInputArchive;

   // Optionally load an NVP if its name equals to the current node's name
   // Loading members should be done in the same order they were saved
   // return true if NVP found
   template <class Archive, class T>
   typename std::enable_if_t<
      traits::is_same_archive<Archive, JSONInputArchive>::value ||
      traits::is_same_archive<Archive, XMLInputArchive>::value
   , bool>
   make_optional_nvp(Archive& ar, const char* name, T&& value)
   {
      const auto node_name = ar.getNodeName();

      // if names are equal
      if (node_name != nullptr &&
         strcmp(name, node_name) == 0)
      {
         ar(make_nvp(name, std::forward<T>(value)));  // load the NVP. Advances to the next node
         return true;
      }

      return false;
   }


   template <class Archive, class T>
   void make_optional_nvp(OutputArchive<Archive>& ar, const char* name, T&& value)
   {
      ar(make_nvp(name, std::forward<T>(value)));
   }


   // Saves NVP if predicate is true. Useful for avoiding splitting into save & load if also saving optionally.
   template <class Archive, class T, class Predicate>
   void make_optional_nvp(OutputArchive<Archive>& ar, const char* name, T&& value, Predicate predicate)
   {
      if (predicate())
         ar(make_nvp(name, std::forward<T>(value)));
   }

   template <class Archive, class T, class Predicate>
   typename std::enable_if_t<
      traits::is_same_archive<Archive, JSONInputArchive>::value ||
      traits::is_same_archive<Archive, XMLInputArchive>::value
   , bool>
   make_optional_nvp(Archive& ar, const char* name, T&& value, Predicate predicate)
   {
      return make_optional_nvp(ar, name, std::forward<T>(value));
   }
}


// Macros for using the variable name as the NVP name
#define EXPAND(x) x
#define GET_CEREAL_OPTIONAL_NVP_MACRO(_1, _2, _3, NAME, ...) NAME
#define CEREAL_OPTIONAL_NVP(...) EXPAND(GET_CEREAL_OPTIONAL_NVP_MACRO(__VA_ARGS__, CEREAL_OPTIONAL_NVP_3, CEREAL_OPTIONAL_NVP_2)(__VA_ARGS__))

#define CEREAL_OPTIONAL_NVP_2(ar, T) ::cereal::make_optional_nvp(ar, #T, T)
#define CEREAL_OPTIONAL_NVP_3(ar, T, P) ::cereal::make_optional_nvp(ar, #T, T, P)


#endif//cereal_optional_nvp_h
