#pragma once

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <initializer_list>
#include <sstream>
#include <stdexcept>

class runtime_assertion_error : std::runtime_error {
  using std::runtime_error::runtime_error;
};

class element {
public:
  element(int value)
      : value(value) {}

  element(const element&) = delete;
  element& operator=(const element&) = delete;

  void update_if_non_const(int new_value) {
    value = new_value;
  }

  void update_if_non_const([[maybe_unused]] int new_value) const {}

  operator int() const noexcept {
    return value;
  }

private:
  int value;
};

template <typename T = element, typename... Ts>
std::array<T, sizeof...(Ts)> make_array(Ts... values) {
  return std::array<T, sizeof...(Ts)>{T(values)...};
}

template <typename It>
auto obfuscate_iterators(It first, It last) {
  // TODO replace with std::counting_iterator
  return std::make_pair(counting_iterator(first, last - first), counting_iterator(last, 0));
}

inline std::ostream& operator<<(std::ostream& out, std::byte b) {
  auto flags = out.flags();
  out << "0x" << std::hex << std::uppercase << static_cast<unsigned>(b);
  out.setf(flags);
  return out;
}

template <typename T, typename U, size_t N, size_t M>
void expect_eq(const contiguous_view<U, M>& actual, const contiguous_view<T, N>& expected) {
  EXPECT_EQ(actual.size(), expected.size());

  auto expected_begin = expected.begin();
  auto expected_end = expected.end();
  auto actual_begin = actual.begin();
  auto actual_end = actual.end();

  if (std::equal(actual_begin, actual_end, expected_begin, expected_end)) {
    return;
  }

  std::stringstream out;
  out << "  expected: {";

  bool add_comma = false;
  std::for_each(expected_begin, expected_end, [&add_comma, &out](const auto& e) {
    if (add_comma) {
      out << ", ";
    }
    out << e;
    add_comma = true;
  });

  out << "}\n  actual:   {";

  add_comma = false;
  std::for_each(actual_begin, actual_end, [&add_comma, &out](const auto& e) {
    if (add_comma) {
      out << ", ";
    }
    out << e;
    add_comma = true;
  });

  out << "}";

  ADD_FAILURE() << out.str();
}

template <typename T = int, typename U, size_t N>
void expect_eq(const contiguous_view<U, N>& actual, std::initializer_list<T> expected) {
  expect_eq(actual, contiguous_view<const T>(expected.begin(), expected.end()));
}
