#pragma once
#include <stdexcept>

struct assertion_error : std::runtime_error {
  using std::runtime_error::runtime_error;
};

void runtime_assert(bool condition, const std::string& message);
