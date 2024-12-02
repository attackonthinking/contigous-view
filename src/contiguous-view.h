#pragma once

#include "runtime-assert.h"

#include <cstddef>
#include <memory>
#include <string_view>
#include <type_traits>

inline constexpr size_t dynamic_extent = static_cast<size_t>(-1);

template <size_t Ext>
struct sizer {
  sizer() = default;

  sizer(size_t) {}

  size_t size() const {
    return Ext;
  }
};

template <>
struct sizer<dynamic_extent> {
  size_t size_;

  sizer(size_t num)
      : size_(num) {}

  size_t size() const {
    return size_;
  }
};

template <typename T, size_t Extent = dynamic_extent>
class contiguous_view {
public:
  using value_type = std::remove_const_t<T>;
  using pointer = T*;
  using const_pointer = const T*;
  using reference = T&;
  using const_reference = const T&;
  using iterator = pointer;
  using const_iterator = const_pointer;

private:
  pointer _first;
  [[no_unique_address]] sizer<Extent> size_;

public:
  contiguous_view() noexcept
    requires (Extent == dynamic_extent || Extent == 0)
      : _first(nullptr)
      , size_(0) {}

  template <typename It>
  explicit(Extent != dynamic_extent) contiguous_view(It first, size_t count)
      : _first(std::to_address(first))
      , size_(count) {
    runtime_assert(count <= size(), "no");
  }

  template <typename It>
  explicit(Extent != dynamic_extent) contiguous_view(It first, It last)
      : contiguous_view(std::to_address(first), last - first) {
    runtime_assert((first <= last && std::distance(first, last) <= static_cast<ptrdiff_t>(size())), "no");
  }

  contiguous_view(const contiguous_view& other) noexcept = default;

  template <typename U, size_t N>
    requires (!(std::is_same_v<U, std::remove_const_t<T>>) || N == dynamic_extent || Extent == dynamic_extent ||
              N == Extent)
  explicit(Extent != dynamic_extent && Extent != N) contiguous_view(const contiguous_view<U, N>& other) noexcept
      : _first(other.begin())
      , size_(other.size()) {
    runtime_assert(other.size() <= size(), "no");
  }

  contiguous_view& operator=(const contiguous_view& other) noexcept = default;

  void swap(contiguous_view& other) {
    std::swap(_first, other._first);
    std::swap(size_, other.size_);
  }

  pointer data() const noexcept {
    return _first;
  }

  size_t size() const noexcept {
    return size_.size();
  }

  size_t size_bytes() const noexcept {
    return size() * sizeof(T);
  }

  bool empty() const noexcept {
    return (size() == 0);
  }

  iterator begin() const noexcept {
    return _first;
  }

  const_iterator cbegin() const noexcept {
    return _first;
  }

  iterator end() const noexcept {
    return _first + size();
  }

  const_iterator cend() const noexcept {
    return _first + size();
  }

  reference operator[](size_t idx) const {
    runtime_assert(idx < size(), "fefe");
    return *(_first + idx);
  }

  reference front() const {
    runtime_assert(size() > 0, "");
    return *_first;
  }

  reference back() const {
    runtime_assert(!empty(), "");
    return *(_first + size() - 1);
  }

  contiguous_view<T, dynamic_extent> subview(size_t offset, size_t count = dynamic_extent) const {
    if (count == dynamic_extent) {
      runtime_assert(offset <= size(), "Offset < size.");
      return contiguous_view<T, dynamic_extent>(_first + offset, _first + size());
    } else {
      runtime_assert(offset + count <= size(), "Size + Offset must be lower or equals than size.");
      return contiguous_view<T, dynamic_extent>(_first + offset, _first + offset + count);
    }
  }

  template <size_t Offset, size_t Count = dynamic_extent>
  auto subview() const {
    static_assert(Offset <= Extent);
    static_assert(Count == dynamic_extent || Count <= Extent - Offset);
    if constexpr (Count == dynamic_extent) {
      runtime_assert(Offset <= size(), "Offset < size.");
      if constexpr (Extent == dynamic_extent) {
        return contiguous_view<T, dynamic_extent>(_first + Offset, _first + size());
      } else {
        return contiguous_view<T, Extent - Offset>(_first + Offset, end());
      }
    } else {
      runtime_assert(Offset + Count <= size(), "Size And Offset must be lower");
      return contiguous_view<T, Count>(_first + Offset, Count);
    }
  }

  template <size_t Count>
  contiguous_view<T, Count> first() const {
    static_assert(Count <= Extent);
    runtime_assert(Count <= size(), "");
    return contiguous_view<T, Count>(begin(), Count);
  }

  contiguous_view<T, dynamic_extent> first(size_t count) const {
    runtime_assert(count <= size(), "y");
    return contiguous_view<T, dynamic_extent>(begin(), count);
  }

  contiguous_view<T, dynamic_extent> last(size_t count) const {
    runtime_assert(count <= size(), "");
    return contiguous_view<T, dynamic_extent>(begin() + size() - count, count);
  }

  template <size_t Count>
  contiguous_view<T, Count> last() const {
    static_assert(Count <= Extent);
    runtime_assert(Count <= size(), "");
    return contiguous_view<T, Count>(begin() + size() - Count, Count);
  }

  inline static constexpr size_t fun = (Extent != dynamic_extent) ? (Extent * sizeof(T)) : dynamic_extent;

  using byte = std::conditional_t<std::is_const_v<T>, const std::byte, std::byte>;

  contiguous_view<byte, fun> as_bytes() const {
    return contiguous_view<byte, fun>(reinterpret_cast<byte*>(begin()), size_bytes());
  }

  explicit operator std::string_view() const
    requires (std::is_same_v<T, const char>)
  {
    return std::string_view(reinterpret_cast<const char*>(begin()), static_cast<std::size_t>(size()));
  }
};
