//
// Created by lunbun on 6/16/2022.
//

#pragma once

#include <exception>
#include <string>

namespace magnetic {

class BadBytecode : public std::exception {
 public:
  explicit BadBytecode(std::string cause) : cause_(std::move(cause)) {}

  [[nodiscard]] const char *what() const noexcept override { return this->cause_.c_str(); }

 private:
  std::string cause_;
};

}// namespace magnetic
