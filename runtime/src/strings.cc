//
// Created by lunbun on 7/9/2022.
//

#include "strings.h"

#include <map>
#include <string>

extern "C" void *Magnetic_new_java_lang_String();

namespace {

class StringPool {
 public:
  void *GetOrInsert(int32_t length, const char *value) {
    std::string_view view(value, length);
    const auto &it = this->strings_.find(view);
    if (it != this->strings_.end()) return it->second;

    void *ptr = Magnetic_new_java_lang_String();
    // TODO: Create a Java char[] and call the String(char[] value, boolean share) constructor
    // TODO: Check to make sure this constructor always exists in all JDK versions.

    this->strings_.emplace(std::string(value, length), ptr);
    return ptr;
  }

 private:
  std::map<std::string, void *, std::less<>> strings_;
};

StringPool pool;

}// namespace

void *Magnetic_rt_string_pool_get(int32_t length, const char *value) { return pool.GetOrInsert(length, value); }
