#include "node.h"

void node_details::node_base_t::swap(node_details::node_base_t& other) {
  std::swap(left, other.left);
  std::swap(right, other.right);
  std::swap(father, other.father);
  std::swap(priority, other.priority);
}

void node_details::node_base_t::update_father() noexcept {
  update_left_father();
  update_right_father();
}

const node_details::node_base_t* node_details::node_base_t::get_min(
    const node_details::node_base_t* curr_root) noexcept {
  while (curr_root && curr_root->left) {
    curr_root = curr_root->left;
  }
  return curr_root;
}

const node_details::node_base_t* node_details::node_base_t::get_max(
    const node_details::node_base_t* curr_root) noexcept {
  while (curr_root && curr_root->right) {
    curr_root = curr_root->right;
  }
  return curr_root;
}

const node_details::node_base_t*
node_details::node_base_t::next(const node_details::node_base_t* curr_root) noexcept {
  if (curr_root->right) {
    return get_min(curr_root->right);
  } else {
    while (curr_root->father && curr_root->father->left != curr_root) {
      curr_root = curr_root->father;
    }
    return curr_root->father;
  }
}

const node_details::node_base_t*
node_details::node_base_t::prev(const node_details::node_base_t* curr_root) noexcept {
  if (curr_root->left) {
    return get_max(curr_root->left);
  } else {
    while (curr_root->father && curr_root->father->right != curr_root) {
      curr_root = curr_root->father;
    }
    return curr_root->father;
  }
}

void node_details::node_base_t::update_right_father() noexcept {
  if (right) {
    right->father = this;
  }
}

void node_details::node_base_t::update_left_father() noexcept {
  if (left) {
    left->father = this;
  }
}
