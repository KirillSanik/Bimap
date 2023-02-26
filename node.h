#pragma once

#include <cstdint>
#include <random>
#include <utility>

namespace node_details {
inline std::mt19937_64 rand_generator(time(nullptr));

struct node_base_t {
  node_base_t* left = nullptr;
  node_base_t* right = nullptr;
  node_base_t* father = nullptr;
  std::uint64_t priority;

  explicit node_base_t() : priority(rand_generator()) {}
  void swap(node_base_t& other);

  void update_father() noexcept;
  void update_left_father() noexcept;
  void update_right_father() noexcept;

  static const node_base_t* get_min(const node_base_t* curr_root) noexcept;
  static const node_base_t* get_max(const node_base_t* curr_root) noexcept;
  static const node_base_t* next(const node_base_t* curr_root) noexcept;
  static const node_base_t* prev(const node_base_t* curr_root) noexcept;
};

template <typename T, bool Type>
struct node_ptr_t : node_base_t {
  T value;

  template <typename ValueT>
  explicit node_ptr_t(ValueT&& value) : value(std::forward<ValueT>(value)) {}

  void swap(node_ptr_t& other) {
    node_base_t::swap(static_cast<node_base_t&>(other));
    std::swap(value, other.value);
  }
};

template <typename Key, typename Value>
struct node_t : node_ptr_t<Key, true>, node_ptr_t<Value, false> {
  template <typename Left, typename Right>
  explicit node_t(Left&& key, Right&& value)
      : node_ptr_t<Key, true>(std::forward<Left>(key)),
        node_ptr_t<Value, false>(std::forward<Right>(value)) {}

  void swap(node_t& other) noexcept {
    node_ptr_t<Key, true>::swap(static_cast<node_ptr_t<Key, true>&>(other));
    node_ptr_t<Value, false>::swap(
        static_cast<node_ptr_t<Value, false>&>(other));
  }

  template <bool Type>
  static const node_base_t* get_another_node(const node_base_t* node) {
    using AnotherTypeElem = std::conditional_t<Type, Value, Key>;
    return static_cast<const node_base_t*>(
        static_cast<const node_ptr_t<AnotherTypeElem, !Type>*>(
            get_node_t<Type>(node)));
  }

  template <bool Type>
  static const node_t* get_node_t(const node_base_t* node) {
    using TypeElem = std::conditional_t<Type, Key, Value>;
    return static_cast<const node_t*>(static_cast<const node_ptr_t<TypeElem, Type>*>(node));
  }
};
} // namespace node_details
