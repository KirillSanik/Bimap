#pragma once

#include "cartesian_tree.h"
#include "node.h"

#include <stdexcept>

template <typename Left, typename Right, typename CompareLeft = std::less<Left>,
          typename CompareRight = std::less<Right>>
struct bimap {
private:
  using left_t = Left;
  using right_t = Right;
  using node_t = node_details::node_t<left_t, right_t>;
  using left_node_t = node_details::node_ptr_t<left_t, true>;
  using right_node_t = node_details::node_ptr_t<right_t, false>;
  using node_base_t = node_details::node_base_t;
  using left_tree_t = cartesian_tree::treap<left_t, CompareLeft, true>;
  using right_tree_t = cartesian_tree::treap<right_t, CompareRight, false>;
  using left_tree_iter = typename left_tree_t::iterator;
  using right_tree_iter = typename right_tree_t::iterator;

  left_tree_t left_tree;
  right_tree_t right_tree;
  std::size_t cnt_elem = 0;

  template <bool Type>
  using type_tree_iter =
      std::conditional_t<Type, left_tree_iter, right_tree_iter>;

  template <bool Type>
  struct base_it : public type_tree_iter<Type> {
  private:
    explicit base_it(type_tree_iter<Type> it) : type_tree_iter<Type>(it) {}
    explicit base_it(const node_base_t* node) : type_tree_iter<Type>(node) {}

    friend bimap;

  public:
    base_it(base_it const& other) noexcept = default;

    base_it<!Type> flip() const {
      if (type_tree_iter<Type>::current_element->father == nullptr) {
        return base_it<!Type>(type_tree_iter<Type>::current_element->right);
      }
      return base_it<!Type>(node_t::template get_another_node<Type>(
          type_tree_iter<Type>::current_element));
    }
  };

public:
  using left_iterator = base_it<true>;
  using right_iterator = base_it<false>;

  // Создает bimap не содержащий ни одной пары.
  bimap(CompareLeft compare_left = CompareLeft(),
        CompareRight compare_right = CompareRight())
      : left_tree(std::move(compare_left)),
        right_tree(std::move(compare_right)) {
    left_tree.root.right = &right_tree.root;
    right_tree.root.right = &left_tree.root;
  }

  // Конструкторы от других и присваивания
  bimap(bimap const& other)
      : left_tree(CompareLeft(static_cast<CompareLeft>(other.left_tree))),
        right_tree(CompareRight(static_cast<CompareRight>(other.right_tree))) {
    left_tree.root.right = &right_tree.root;
    right_tree.root.right = &left_tree.root;
    for (auto it = other.begin_left(); it != other.end_left(); ++it) {
      insert(*it, *it.flip());
    }
  }
  bimap(bimap&& other) noexcept
      : left_tree(std::move(static_cast<CompareLeft&>(other.left_tree))),
        right_tree(std::move(static_cast<CompareRight&>(other.right_tree))),
        cnt_elem(other.cnt_elem) {
    left_tree.root.swap(other.left_tree.root);
    right_tree.root.swap(other.right_tree.root);
  }

  bimap& operator=(bimap const& other) {
    if (this != &other) {
      bimap(other).swap(*this);
    }
    return *this;
  }
  bimap& operator=(bimap&& other) noexcept {
    if (this != &other) {
      bimap(std::move(other)).swap(*this);
    }
    return *this;
  }

  void swap(bimap& other) noexcept {
    left_tree.swap(other.left_tree);
    right_tree.swap(other.right_tree);
    std::swap(cnt_elem, other.cnt_elem);
  }

  // Деструктор. Вызывается при удалении объектов bimap.
  // Инвалидирует все итераторы ссылающиеся на элементы этого bimap
  // (включая итераторы ссылающиеся на элементы следующие за последними).
  ~bimap() {
    delete_nodes<true>(left_tree.root.left);
  }

private:
  template <bool Type>
  std::size_t delete_nodes(node_details::node_base_t* curr_node) {
    std::size_t tmp = 0;
    if (curr_node) {
      tmp += delete_nodes<Type>(curr_node->left);
      tmp += delete_nodes<Type>(curr_node->right);
      delete node_t::template get_node_t<Type>(curr_node);
      ++tmp;
    }
    return tmp;
  }

  template <bool Type>
  void remove_another_nodes(node_details::node_base_t* curr_node) {
    if (curr_node) {
      remove_another_nodes<Type>(curr_node->left);
      remove_another_nodes<Type>(curr_node->right);
      if constexpr (Type) {
        right_tree.remove(node_t::template get_another_node<true>(curr_node));
      } else {
        left_tree.remove(node_t::template get_another_node<false>(curr_node));
      }
    }
  }

  template <typename LeftT, typename RightT>
  left_iterator insert_forward(LeftT&& left, RightT&& right) {
    if (left_tree.contains(left) || right_tree.contains(right)) {
      return end_left();
    }
    node_t* tmp =
        new node_t(std::forward<LeftT>(left), std::forward<RightT>(right));
    node_base_t* left_ptr = left_tree.insert(static_cast<left_node_t*>(tmp));
    right_tree.insert(static_cast<right_node_t*>(tmp));
    cnt_elem++;
    return left_iterator(left_ptr);
  }

public:
  // Вставка пары (left, right), возвращает итератор на left.
  // Если такой left или такой right уже присутствуют в bimap, вставка не
  // производится и возвращается end_left().
  left_iterator insert(left_t&& left, right_t&& right) {
    return insert_forward(std::move(left), std::move(right));
  }

  left_iterator insert(left_t&& left, right_t const& right) {
    return insert_forward(std::move(left), right);
  }

  left_iterator insert(left_t const& left, right_t&& right) {
    return insert_forward(left, std::move(right));
  }

  left_iterator insert(left_t const& left, right_t const& right) {
    return insert_forward(left, right);
  }

  // Удаляет элемент и соответствующий ему парный.
  // erase невалидного итератора неопределен.
  // erase(end_left()) и erase(end_right()) неопределены.
  // Пусть it ссылается на некоторый элемент e.
  // erase инвалидирует все итераторы ссылающиеся на e и на элемент парный к e.
  left_iterator erase_left(left_iterator it) {
    cnt_elem--;
    right_tree.remove(it.flip());
    left_tree_iter res = left_tree.remove(it);
    delete node_t::template get_node_t<true>(it.current_element);
    return left_iterator(res);
  }
  // Аналогично erase, но по ключу, удаляет элемент если он присутствует, иначе
  // не делает ничего Возвращает была ли пара удалена
  bool erase_left(left_t const& left) {
    const node_base_t* tmp = left_tree.remove(left);
    if (tmp) {
      right_tree.remove(node_t::template get_another_node<true>(tmp));
      delete node_t::template get_node_t<true>(tmp);
      cnt_elem--;
      return true;
    }
    return false;
  }

  right_iterator erase_right(right_iterator it) {
    cnt_elem--;
    left_tree.remove(it.flip());
    right_tree_iter res = right_tree.remove(it);
    delete node_t::template get_node_t<false>(it.current_element);
    return right_iterator(res);
  }
  bool erase_right(right_t const& right) {
    const node_base_t* tmp = right_tree.remove(right);
    if (tmp) {
      left_tree.remove(node_t::template get_another_node<false>(tmp));
      delete node_t::template get_node_t<false>(tmp);
      cnt_elem--;
      return true;
    }
    return false;
  }

  // erase от ренжа, удаляет [first, last), возвращает итератор на последний
  // элемент за удаленной последовательностью
  left_iterator erase_left(left_iterator first, left_iterator last) {
    node_base_t* tmp = left_tree.remove(first, last);
    remove_another_nodes<true>(tmp);
    cnt_elem -= delete_nodes<true>(tmp);
    return last;
  }
  right_iterator erase_right(right_iterator first, right_iterator last) {
    node_base_t* tmp = right_tree.remove(first, last);
    remove_another_nodes<false>(tmp);
    cnt_elem -= delete_nodes<false>(tmp);
    return last;
  }

  // Возвращает итератор по элементу. Если не найден - соответствующий end()
  left_iterator find_left(left_t const& left) const {
    const node_base_t* tmp = left_tree.find(left);
    return tmp ? left_iterator(tmp) : end_left();
  }
  right_iterator find_right(right_t const& right) const {
    const node_base_t* tmp = right_tree.find(right);
    return tmp ? right_iterator(tmp) : end_right();
  }

  // Возвращает противоположный элемент по элементу
  // Если элемента не существует -- бросает std::out_of_range
  right_t const& at_left(left_t const& key) const {
    const node_base_t* node = left_tree.find(key);
    if (!node) {
      throw std::out_of_range("not founded key");
    }
    return static_cast<const right_node_t*>(node_t::template get_node_t<true>(node))
        ->value;
  }
  left_t const& at_right(right_t const& key) const {
    const node_base_t* node = right_tree.find(key);
    if (!node) {
      throw std::out_of_range("not founded key");
    }
    return static_cast<const left_node_t*>(node_t::template get_node_t<false>(node))
        ->value;
  }

  // Возвращает противоположный элемент по элементу
  // Если элемента не существует, добавляет его в bimap и на противоположную
  // сторону кладет дефолтный элемент, ссылку на который и возвращает
  // Если дефолтный элемент уже лежит в противоположной паре - должен поменять
  // соответствующий ему элемент на запрашиваемый (смотри тесты)
  template <typename = std::enable_if<std::is_default_constructible_v<right_t>>>
  right_t const& at_left_or_default(left_t const& key) {
    left_iterator iter_l = find_left(key);
    if (iter_l != end_left()) {
      return *iter_l.flip();
    }
    right_t tmp = right_t();
    erase_right(tmp);
    return *insert(key, std::move(tmp)).flip();
  }
  template <typename = std::enable_if<std::is_default_constructible_v<left_t>>>
  left_t const& at_right_or_default(right_t const& key) {
    right_iterator iter_r = find_right(key);
    if (iter_r != end_right()) {
      return *iter_r.flip();
    }
    left_t tmp = left_t();
    erase_left(tmp);
    return *insert(std::move(tmp), key);
  }

  // lower и upper bound'ы по каждой стороне
  // Возвращают итераторы на соответствующие элементы
  // Смотри std::lower_bound, std::upper_bound.
  left_iterator lower_bound_left(const left_t& left) const {
    return left_iterator(left_tree.lower_bound(left_tree.root.left, left));
  }
  left_iterator upper_bound_left(const left_t& left) const {
    return left_iterator(left_tree.upper_bound(left_tree.root.left, left));
  }

  right_iterator lower_bound_right(const right_t& right) const {
    return right_iterator(right_tree.lower_bound(right_tree.root.left, right));
  }
  right_iterator upper_bound_right(const right_t& right) const {
    return right_iterator(right_tree.upper_bound(right_tree.root.left, right));
  }

  // Возващает итератор на минимальный по порядку left.
  left_iterator begin_left() const {
    return left_iterator(node_base_t::get_min(left_tree.root.left));
  }
  // Возващает итератор на следующий за последним по порядку left.
  left_iterator end_left() const {
    return left_iterator(&left_tree.root);
  }

  // Возващает итератор на минимальный по порядку right.
  right_iterator begin_right() const {
    return right_iterator(node_base_t::get_min(right_tree.root.left));
  }
  // Возващает итератор на следующий за последним по порядку right.
  right_iterator end_right() const {
    return right_iterator(&right_tree.root);
  }

  // Проверка на пустоту
  bool empty() const {
    return size() == 0;
  }

  // Возвращает размер бимапы (кол-во пар)
  std::size_t size() const {
    return cnt_elem;
  }

  // операторы сравнения
  friend bool operator==(bimap const& a, bimap const& b) {
    if (&a == &b) {
      return true;
    }
    if (a.size() != b.size()) {
      return false;
    }
    for (auto it_a = a.begin_left(), it_b = b.begin_left();
         it_a != a.end_left() && it_b != b.end_left(); ++it_a, ++it_b) {
      if (!a.left_tree.equal(*it_a, *it_b) ||
          !a.right_tree.equal(*it_a.flip(), *it_b.flip())) {
        return false;
      }
    }
    return true;
  }
  friend bool operator!=(bimap const& a, bimap const& b) {
    return !(a == b);
  }
};
