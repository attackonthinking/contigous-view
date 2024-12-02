#include "contiguous-view.h"

#include "counting-iterator.h"
#include "test-utils.h"

#include <gtest/gtest.h>

#include <array>
#include <bit>
#include <iterator>
#include <type_traits>
#include <utility>

namespace {

template <typename IsStatic>
class common_tests : public ::testing::Test {
protected:
  template <typename T, size_t Extent>
  using view = contiguous_view<T, IsStatic::value ? Extent : dynamic_extent>;
};

template <typename IsStatic>
class assert_test : public ::testing::Test {
protected:
  template <typename T, size_t Extent>
  using view = contiguous_view<T, IsStatic::value ? Extent : dynamic_extent>;
};

using tested_extents = ::testing::Types<std::false_type, std::true_type>;

class extent_name_generator {
public:
  template <typename IsStatic>
  static std::string GetName(int) {
    if (IsStatic::value) {
      return "static";
    } else {
      return "dynamic";
    }
  }
};

TYPED_TEST_SUITE(common_tests, tested_extents, extent_name_generator);
TYPED_TEST_SUITE(assert_test, tested_extents, extent_name_generator);

} // namespace

template class contiguous_view<element>;
template class contiguous_view<element, 3>;

TYPED_TEST(common_tests, two_iterators_ctor) {
  auto c = make_array(10, 20, 30);
  auto [first, last] = obfuscate_iterators(c.begin(), c.end());

  typename TestFixture::template view<element, 3> v(first, last);

  EXPECT_EQ(v.data(), c.data());
  EXPECT_EQ(v.size(), 3);
  EXPECT_EQ(v.size_bytes(), 3 * sizeof(element));
  EXPECT_FALSE(v.empty());

  expect_eq(v, {10, 20, 30});
}

TYPED_TEST(common_tests, two_iterators_ctor_empty) {
  auto c = make_array();
  auto [first, last] = obfuscate_iterators(c.begin(), c.end());
  typename TestFixture::template view<element, 0> v(first, last);

  EXPECT_EQ(v.data(), c.data());
  EXPECT_EQ(v.size(), 0);
  EXPECT_EQ(v.size_bytes(), 0);
  EXPECT_TRUE(v.empty());

  expect_eq(v, {});
}

TYPED_TEST(common_tests, iterator_and_count_ctor) {
  auto c = make_array(10, 20, 30);
  auto [first, last] = obfuscate_iterators(c.begin(), c.end());

  typename TestFixture::template view<element, 3> v(first, last);

  EXPECT_EQ(v.data(), c.data());
  EXPECT_EQ(v.size(), 3);
  EXPECT_EQ(v.size_bytes(), 3 * sizeof(element));
  EXPECT_FALSE(v.empty());

  expect_eq(v, {10, 20, 30});
}

TYPED_TEST(common_tests, iterator_and_count_ctor_empty) {
  auto c = make_array();
  auto [first, last] = obfuscate_iterators(c.begin(), c.end());

  typename TestFixture::template view<element, 0> v(first, 0);

  EXPECT_EQ(v.data(), c.data());
  EXPECT_EQ(v.size(), 0);
  EXPECT_EQ(v.size_bytes(), 0);
  EXPECT_TRUE(v.empty());

  expect_eq(v, {});
}

TYPED_TEST(common_tests, copy_ctor) {
  auto c = make_array(10, 20, 30);
  typename TestFixture::template view<element, 3> v(c.begin(), c.end());

  typename TestFixture::template view<element, 3> copy = std::as_const(v);

  EXPECT_EQ(v.data(), copy.data());
  EXPECT_EQ(v.size(), copy.size());

  expect_eq(copy, {10, 20, 30});
}

TYPED_TEST(common_tests, copy_assignment) {
  auto c1 = make_array(10, 20, 30);
  auto c2 = make_array(40, 50, 60);

  typename TestFixture::template view<element, 3> v1(c1.begin(), c1.end());
  typename TestFixture::template view<element, 3> v2(c2.begin(), c2.end());

  v2 = std::as_const(v1);

  EXPECT_EQ(v1.data(), v2.data());
  EXPECT_EQ(v1.size(), v2.size());

  expect_eq(v2, {10, 20, 30});
}

TYPED_TEST(common_tests, subscript) {
  auto c = make_array(10, 20, 30);
  const typename TestFixture::template view<element, 3> v(c.begin(), c.end());

  v[1].update_if_non_const(42);

  EXPECT_EQ(v[0], 10);
  EXPECT_EQ(v[1], 42);
  EXPECT_EQ(v[2], 30);

  EXPECT_EQ(&v[0], &c[0]);
  EXPECT_EQ(&v[1], &c[1]);
  EXPECT_EQ(&v[2], &c[2]);
}

TYPED_TEST(common_tests, subscript_const) {
  auto c = make_array(10, 20, 30);
  typename TestFixture::template view<const element, 3> v(c.begin(), c.end());

  v[1].update_if_non_const(42);

  EXPECT_EQ(v[0], 10);
  EXPECT_EQ(v[1], 20);
  EXPECT_EQ(v[2], 30);

  EXPECT_EQ(&v[0], &c[0]);
  EXPECT_EQ(&v[1], &c[1]);
  EXPECT_EQ(&v[2], &c[2]);
}

TYPED_TEST(common_tests, front_back) {
  auto c = make_array(10, 20, 30);
  const typename TestFixture::template view<element, 3> v(c.begin(), c.end());

  EXPECT_EQ(v.front(), 10);
  EXPECT_EQ(v.back(), 30);

  EXPECT_EQ(&v.front(), &c.front());
  EXPECT_EQ(&v.back(), &c.back());

  v.front().update_if_non_const(42);
  v.back().update_if_non_const(43);

  EXPECT_EQ(v.front(), 42);
  EXPECT_EQ(v.back(), 43);
}

TYPED_TEST(common_tests, front_back_const) {
  auto c = make_array(10, 20, 30);
  typename TestFixture::template view<const element, 3> v(c.begin(), c.end());

  EXPECT_EQ(v.front(), 10);
  EXPECT_EQ(v.back(), 30);

  EXPECT_EQ(&v.front(), &c.front());
  EXPECT_EQ(&v.back(), &c.back());

  v.front().update_if_non_const(42);
  v.back().update_if_non_const(43);

  EXPECT_EQ(v.front(), 10);
  EXPECT_EQ(v.back(), 30);
}

TYPED_TEST(common_tests, subview) {
  auto c = make_array(10, 20, 30, 40, 50);
  typename TestFixture::template view<element, 5> v(c.begin(), c.end());

  {
    contiguous_view<element, 3> static_slice = v.template subview<2, 3>();
    contiguous_view<element> dynamic_slice = v.subview(2, 3);

    expect_eq(static_slice, {30, 40, 50});
    expect_eq(dynamic_slice, {30, 40, 50});
  }

  {
    contiguous_view<element, 2> static_slice = v.template subview<1, 2>();
    contiguous_view<element> dynamic_slice = v.subview(1, 2);

    expect_eq(static_slice, {20, 30});
    expect_eq(dynamic_slice, {20, 30});
  }

  {
    contiguous_view<element, 0> static_slice = v.template subview<5, 0>();
    contiguous_view<element> dynamic_slice = v.subview(5, 0);

    expect_eq(static_slice, {});
    expect_eq(dynamic_slice, {});
  }
}

TYPED_TEST(common_tests, subview_dynamic_extent) {
  auto c = make_array(10, 20, 30, 40, 50);
  typename TestFixture::template view<element, 5> v(c.begin(), c.end());

  {
    typename TestFixture::template view<element, 5> static_slice = v.template subview<0, dynamic_extent>();
    contiguous_view<element> dynamic_slice = v.subview(0, dynamic_extent);

    expect_eq(static_slice, {10, 20, 30, 40, 50});
    expect_eq(dynamic_slice, {10, 20, 30, 40, 50});
  }

  {
    typename TestFixture::template view<element, 3> static_slice = v.template subview<2, dynamic_extent>();
    contiguous_view<element> dynamic_slice = v.subview(2, dynamic_extent);

    expect_eq(static_slice, {30, 40, 50});
    expect_eq(dynamic_slice, {30, 40, 50});
  }

  {
    typename TestFixture::template view<element, 0> static_slice = v.template subview<5, dynamic_extent>();
    contiguous_view<element> dynamic_slice = v.subview(5, dynamic_extent);

    expect_eq(static_slice, {});
    expect_eq(dynamic_slice, {});
  }
}

TYPED_TEST(common_tests, first) {
  auto c = make_array(10, 20, 30);
  typename TestFixture::template view<element, 3> v(c.begin(), c.end());

  contiguous_view<element, 2> static_slice = v.template first<2>();
  contiguous_view<element> dynamic_slice = v.first(2);

  expect_eq(static_slice, {10, 20});
  expect_eq(dynamic_slice, {10, 20});
}

TYPED_TEST(common_tests, last) {
  auto c = make_array(10, 20, 30);
  typename TestFixture::template view<element, 3> v(c.begin(), c.end());

  contiguous_view<element, 2> static_slice = v.template last<2>();
  contiguous_view<element> dynamic_slice = v.last(2);

  expect_eq(static_slice, {20, 30});
  expect_eq(dynamic_slice, {20, 30});
}

TYPED_TEST(common_tests, as_bytes) {
  if constexpr (std::endian::native != std::endian::little) {
    GTEST_SKIP();
  } else {
    auto ints = make_array<std::uint32_t>(0x11223344, 0xABABCDEF);
    auto bytes = make_array<std::byte>(0x44, 0x33, 0x22, 0x11, 0xEF, 0xCD, 0xAB, 0xAB);

    typename TestFixture::template view<std::uint32_t, 2> ints_view(ints.begin(), ints.end());
    typename TestFixture::template view<std::byte, 8> bytes_view(bytes.begin(), bytes.end());

    typename TestFixture::template view<std::byte, 8> as_bytes = std::as_const(ints_view).as_bytes();

    EXPECT_EQ(ints_view.size_bytes(), 8);
    EXPECT_EQ(as_bytes.size(), 8);
    EXPECT_EQ(as_bytes.size_bytes(), 8);

    expect_eq(as_bytes, bytes_view);

    as_bytes[3] = std::byte(0x80);
    as_bytes[4] = std::byte(0x42);

    expect_eq<std::uint32_t>(ints_view, {0x80223344, 0xABABCD42});
  }
}

TYPED_TEST(common_tests, as_bytes_const) {
  if constexpr (std::endian::native != std::endian::little) {
    GTEST_SKIP();
  } else {
    auto ints = make_array<std::uint32_t>(0x11223344, 0xABABCDEF);
    auto bytes = make_array<std::byte>(0x44, 0x33, 0x22, 0x11, 0xEF, 0xCD, 0xAB, 0xAB);

    typename TestFixture::template view<const std::uint32_t, 2> ints_view(ints.begin(), ints.end());
    typename TestFixture::template view<std::byte, 8> bytes_view(bytes.begin(), bytes.end());

    typename TestFixture::template view<const std::byte, 8> as_bytes = ints_view.as_bytes();

    EXPECT_EQ(ints_view.size_bytes(), 8);
    EXPECT_EQ(as_bytes.size(), 8);
    EXPECT_EQ(as_bytes.size_bytes(), 8);

    expect_eq(as_bytes, bytes_view);
  }
}

TYPED_TEST(common_tests, traits) {
  using writable_contiguous_view = typename TestFixture::template view<element, 3>;
  using const_contiguous_view = typename TestFixture::template view<const element, 3>;

  EXPECT_TRUE(std::is_trivially_copyable_v<writable_contiguous_view>);
  EXPECT_TRUE(std::is_trivially_copyable_v<const_contiguous_view>);

  EXPECT_TRUE((std::is_same_v<typename writable_contiguous_view::value_type, element>) );
  EXPECT_TRUE((std::is_same_v<typename const_contiguous_view::value_type, element>) );
}

TEST(dynamic_extent_tests, default_ctor) {
  contiguous_view<element> v;

  EXPECT_EQ(v.data(), nullptr);
  EXPECT_EQ(v.size(), 0);
  EXPECT_EQ(v.size_bytes(), 0);
  EXPECT_TRUE(v.empty());

  expect_eq(v, {});
}

TEST(dynamic_extent_tests, copy_assignment) {
  auto c1 = make_array(10, 20, 30);
  auto c2 = make_array(42);

  contiguous_view<element> v1(c1.begin(), c1.end());
  contiguous_view<element> v2(c2.begin(), c2.end());

  v2 = std::as_const(v1);

  EXPECT_EQ(v1.data(), v2.data());
  EXPECT_EQ(v1.size(), v2.size());

  expect_eq(v2, {10, 20, 30});
}

TEST(static_extent_tests, traits) {
  using contiguous_view = contiguous_view<element, 10>;

  EXPECT_GE(sizeof(contiguous_view::pointer), sizeof(contiguous_view));
}

TEST(conversion_tests, dynamic_add_const) {
  auto c = make_array(10, 20, 30);
  contiguous_view<element> v(c.begin(), c.end());

  contiguous_view<const element> cv = v;

  EXPECT_EQ(v.data(), cv.data());
  EXPECT_EQ(v.size(), cv.size());

  expect_eq(cv, {10, 20, 30});
}

TEST(conversion_tests, static_add_const) {
  auto c = make_array(10, 20, 30);
  contiguous_view<element, 3> v(c.begin(), c.end());

  contiguous_view<const element, 3> cv = v;

  EXPECT_EQ(v.data(), cv.data());
  EXPECT_EQ(v.size(), cv.size());

  expect_eq(cv, {10, 20, 30});
}

TEST(conversion_tests, dynamic_to_static) {
  auto c = make_array(10, 20, 30);
  contiguous_view<element> v1(c.begin(), c.end());

  contiguous_view<element, 3> v2(v1);

  EXPECT_EQ(v1.data(), v2.data());
  EXPECT_EQ(v1.size(), v2.size());

  expect_eq(v2, {10, 20, 30});
}

TEST(conversion_tests, static_to_dynamic) {
  auto c = make_array(10, 20, 30);
  contiguous_view<element, 3> v1(c.begin(), c.end());

  contiguous_view<element> v2 = v1;

  EXPECT_EQ(v1.data(), v2.data());
  EXPECT_EQ(v1.size(), v2.size());

  expect_eq(v2, {10, 20, 30});
}

TEST(conversion_tests, illegal) {
  EXPECT_FALSE((std::is_convertible_v<contiguous_view<element>, contiguous_view<element, 3>>) );
  EXPECT_FALSE((std::is_constructible_v<contiguous_view<element, 3>, contiguous_view<element, 2>>) );
  EXPECT_FALSE((std::is_convertible_v<contiguous_view<const char, dynamic_extent>, std::string_view>) );
  EXPECT_FALSE((std::is_constructible_v<contiguous_view<char, dynamic_extent>, std::string_view>) );
  EXPECT_FALSE((std::is_constructible_v<contiguous_view<const int, dynamic_extent>, std::string_view>) );
  EXPECT_FALSE((std::is_constructible_v<contiguous_view<int, dynamic_extent>, std::string_view>) );
}

TEST(conversion_tests, to_string_view) {
  std::string test("abacaba");
  contiguous_view<const char, dynamic_extent> v1(test.begin(), test.end());
  contiguous_view<const char, 7> v2(test.begin(), test.end());
  std::string_view view(v1);
  std::string_view view2(v2);
  EXPECT_EQ(v1.size(), v2.size());
  EXPECT_EQ(v1.size(), test.size());
  EXPECT_EQ(v1.size(), view.size());
  EXPECT_EQ(v1.size(), view2.size());

  EXPECT_EQ(v1.data(), v2.data());
  EXPECT_EQ(v1.data(), test.data());
  EXPECT_EQ(v1.data(), view.data());
  EXPECT_EQ(v1.data(), view2.data());
}

TYPED_TEST(assert_test, get_by_idx) {
  auto c = make_array(10, 20, 30);
  typename TestFixture::template view<element, 3> v1(c.begin(), c.end());
  EXPECT_THROW(v1[static_cast<size_t>(-1)], assertion_error);
  EXPECT_THROW(v1[v1.size()], assertion_error);
  typename TestFixture::template view<element, 0> v2;
  EXPECT_THROW(v2[0], assertion_error);
}

TEST(assert_test, back) {
  contiguous_view<element, dynamic_extent> v;
  EXPECT_THROW(v.back(), assertion_error);
}

TYPED_TEST(assert_test, front) {
  typename TestFixture::template view<element, 0> v;
  EXPECT_THROW(v.front(), assertion_error);
}

TEST(assert_test, last_dynamic) {
  auto c = make_array(10, 20, 30);
  contiguous_view<element, dynamic_extent> v(c.begin(), c.end());
  EXPECT_THROW(v.last<static_cast<size_t>(-1)>(), assertion_error);
  EXPECT_THROW(v.last<4>(), assertion_error);
}

TEST(assert_test, first_dynamic) {
  auto c = make_array(10, 20, 30);
  contiguous_view<element, dynamic_extent> v(c.begin(), c.end());
  EXPECT_THROW(v.first<static_cast<size_t>(-1)>(), assertion_error);
  EXPECT_THROW(v.first<4>(), assertion_error);
}

TEST(assert_test, subview_dynamic) {
  auto c = make_array(10, 20, 30);
  contiguous_view<element, dynamic_extent> v(c.begin(), c.end());
  auto l = [&]() {
    v.subview<static_cast<size_t>(-1), dynamic_extent>();
  };
  EXPECT_THROW(l(), assertion_error);
  auto l2 = [&]() {
    v.subview<4, dynamic_extent>();
  };
  EXPECT_THROW(l2(), assertion_error);

  auto l3 = [&]() {
    v.subview<0, static_cast<size_t>(-2)>();
  };
  EXPECT_THROW(l3(), assertion_error);
  auto l4 = [&]() {
    v.subview<0, 4>();
  };
  EXPECT_THROW(l4(), assertion_error);
}

TYPED_TEST(assert_test, last) {
  auto c = make_array(10, 20, 30);
  typename TestFixture::template view<element, 3> v(c.begin(), c.end());
  EXPECT_THROW(v.last(static_cast<size_t>(-1)), assertion_error);
  EXPECT_THROW(v.last(v.size() + 1), assertion_error);
}

TYPED_TEST(assert_test, first) {
  auto c = make_array(10, 20, 30);
  typename TestFixture::template view<element, 3> v(c.begin(), c.end());
  EXPECT_THROW(v.first(static_cast<size_t>(-1)), assertion_error);
  EXPECT_THROW(v.first(v.size() + 1), assertion_error);
}

TYPED_TEST(assert_test, subview) {
  auto c = make_array(10, 20, 30);
  typename TestFixture::template view<element, 3> v(c.begin(), c.end());
  auto l1 = [&]() {
    v.subview(static_cast<size_t>(-1), dynamic_extent);
  };
  EXPECT_THROW(l1(), assertion_error);
  auto l2 = [&]() {
    v.subview(v.size() + 1, dynamic_extent);
  };
  EXPECT_THROW(l2(), assertion_error);

  auto l3 = [&]() {
    v.subview(0, static_cast<size_t>(-2));
  };
  EXPECT_THROW(l3(), assertion_error);
  auto l4 = [&]() {
    v.subview(0, v.size() + 1);
  };
  EXPECT_THROW(l4(), assertion_error);
}

TEST(assert_test, iterator_constructor) {
  auto l = []() {
    auto c = make_array(10, 20, 30);
    [[maybe_unused]] contiguous_view<element, 2> v(c.begin(), 3);
  };
  EXPECT_THROW(l(), assertion_error);
}

TEST(assert_test, range_constructor_static) {
  auto l = []() {
    auto c = make_array(10, 20, 30);
    [[maybe_unused]] contiguous_view<element, 2> v(c.begin(), c.end());
  };
  EXPECT_THROW(l(), assertion_error);
}

TEST(assert_test, view_constructor) {
  auto l = []() {
    auto c = make_array(10, 20, 30);
    contiguous_view<element, dynamic_extent> v(c.begin(), 3);
    [[maybe_unused]] contiguous_view<element, 2> v1(v);
  };
  EXPECT_DEATH_IF_SUPPORTED(l(), "");
}

TYPED_TEST(assert_test, range_constructor) {
  auto l = []() {
    auto c = make_array(10, 20, 30);
    typename TestFixture::template view<element, 3> v(c.end(), c.begin());
  };
  EXPECT_THROW(l(), assertion_error);
}
