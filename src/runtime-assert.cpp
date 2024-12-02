#include "runtime-assert.h"

void runtime_assert(bool condition, const std::string& message) {
  if (!condition) {
    throw assertion_error(message);
  }
}
