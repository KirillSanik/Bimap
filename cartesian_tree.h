#pragma once

#include "node.h"

namespace cartesian_tree {

template <typename T, typename Compare, bool Type>
struct treap : Compare {
  using node_base_t = node_details::node_base_t;
  using node_value_t = node_details::node_ptr_t<T, Type>;

  node_base_t root;

  struct iterator {
  public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = const T;
    using difference_type = std::ptrdiff_t;
    using pointer = value_type*;
    using reference = value_type&;

  protected:
    const node_base_t* current_element;
    friend treap;

  public:
    explicit iterator(const node_base_t* node) noexcept : current_element(node) {}
    iterator(iterator const& other) noexcept = default;

    reference operator*() const {
      return static_cast<const node_value_t*>(current_element)->value;
    }
    pointer operator->() const {
      return &static_cast<const node_value_t*>(current_element)->value;
    }

    iterator& operator++() {
      current_element = node_base_t::next(current_element);
      return *this;
    }
    iterator operator++(int) {
      iterator res(current_element);
      ++(*this);
      return res;
    }

    iterator& operator--() {
      current_element = node_base_t::prev(current_element);
      return *this;
    }
    iterator operator--(int) {
      iterator res(current_element);
      --(*this);
      return res;
    }

    bool operator==(const iterator& rhs) const noexcept {
      return current_element == rhs.current_element;
    }

    bool operator!=(const iterator& rhs) const noexcept {
      return !(current_element == rhs.current_element);
    }
  };

  explicit treap(Compare&& cmp) : Compare(std::move(cmp)) {}

  void swap(treap& other) noexcept {
    std::swap(static_cast<Compare&>(*this), static_cast<Compare&>(other));
    root.swap(other.root);
  }

  bool equal(T const& lhs, T const& rhs) const noexcept {
    return !Compare::operator()(lhs, rhs) && !Compare::operator()(rhs, lhs);
  }

  std::pair<node_base_t*, node_base_t*> split(node_base_t* curr_root,
                                              T const& value) noexcept {
    if (!curr_root) {
      return {nullptr, nullptr};
    }
    if (Compare::operator()(static_cast<node_value_t*>(curr_root)->value,
                            value)) {
      auto treaps = split(curr_root->right, value);
      curr_root->right = treaps.first;
      curr_root->update_father();
      return {curr_root, treaps.second};
    } else {
      auto treaps = split(curr_root->left, value);
      curr_root->left = treaps.second;
      curr_root->update_father();
      return {treaps.first, curr_root};
    }
  }

  node_base_t* merge(node_base_t* first, node_base_t* second) noexcept {
    if (!second) {
      return first;
    }
    if (!first) {
      return second;
    }
    if (first->priority > second->priority) {
      first->right = merge(first->right, second);
      first->update_father();
      return first;
    } else {
      second->left = merge(first, second->left);
      second->update_father();
      return second;
    }
  }

  node_base_t* insert(node_value_t* inserted_node) {
    auto treaps = split(root.left, inserted_node->value);
    treaps.first =
        merge(treaps.first, static_cast<node_base_t*>(inserted_node));
    root.left = merge(treaps.first, treaps.second);
    root.update_left_father();
    return static_cast<node_base_t*>(inserted_node);
  }

  node_base_t* remove(iterator it_first, iterator it_last) {
    auto treaps1 = split(root.left, *it_first);
    std::pair<node_base_t*, node_base_t*> treaps2;
    if (it_last.current_element == &root) {
      treaps2 = {treaps1.second, nullptr};
    } else {
      treaps2 = split(treaps1.second, *it_last);
    }
    root.left = merge(treaps1.first, treaps2.second);
    root.update_father();
    return treaps2.first;
  }

  iterator remove(iterator it) {
    iterator res = iterator(remove(it.current_element).first);
    return res;
  }

  const node_base_t* remove(T const& value) {
    const node_base_t* tmp_node = find(value);
    if (!tmp_node) {
      return nullptr;
    }
    remove(tmp_node);
    return tmp_node;
  }

  std::pair<const node_base_t*, bool> remove(const node_base_t* deleted_node) {
    if (!deleted_node) {
      return {nullptr, false};
    }
    const node_base_t* res = node_base_t::next(deleted_node);
    node_base_t* tmp_node_value =
        merge(deleted_node->left, deleted_node->right);
    if (deleted_node->father->right &&
        deleted_node == deleted_node->father->right) {
      deleted_node->father->right = tmp_node_value;
    } else {
      deleted_node->father->left = tmp_node_value;
    }
    deleted_node->father->update_father();
    return {res, true};
  }

  bool contains(T const& value) const noexcept {
    return find(value);
  }

  const node_base_t* find(T const& value) const noexcept {
    iterator it = lower_bound(root.left, value);
    if (it.current_element == &root || !equal(*it, value)) {
      return nullptr;
    }
    return it.current_element;
  }

  iterator lower_bound(node_base_t* curr_node, T const& value) const noexcept {
    if (curr_node) {
      if (!Compare::operator()(static_cast<node_value_t*>(curr_node)->value,
                               value)) {
        iterator tmp_res = lower_bound(curr_node->left, value);
        if (tmp_res.current_element == &root) {
          return iterator(curr_node);
        }
        return tmp_res;
      } else {
        return lower_bound(curr_node->right, value);
      }
    }
    return iterator(&root);
  }
  iterator upper_bound(node_base_t* curr_node, T const& value) const noexcept {
    iterator res = lower_bound(curr_node, value);
    if (res.current_element != &root && equal(*res, value)) {
      return iterator(node_base_t::next(res.current_element));
    }
    return res;
  }
};
} // namespace cartesian_tree
