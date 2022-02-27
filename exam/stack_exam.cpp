#include "ap_error.hpp"
#include <iostream>
#include "instrumented.hpp"
#include <iterator>
#include <utility>
#include <vector>               // we use std::vector, however we have all the methods we need in the class vector implemented during the course 
#include <cassert>
#include <algorithm>

template <typename stack, typename T, typename N>
class _iterator{                                                          // iterator class 
    stack* pool;
    N index;
  public:
    using stack_type = N;       
    using value_type = T;
    using reference = value_type&;                                        // standard for a conform iterator
    using pointer = value_type*;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::forward_iterator_tag;

    _iterator(stack* p , stack_type x): pool{p}, index{x} {}
    reference operator*() const noexcept { return pool->value(index); }   // star operator
    _iterator operator++() {                                              // left-increment operator
      index = pool->next(index);
      return *this;
    }
    _iterator operator++(int) {                                           // right-increment operator
      auto tmp = *this;
      ++(*this);
      return tmp;
    }

    friend bool operator==(const _iterator& x, const _iterator& y) noexcept {   // bool operator for full equality comparison
      return x.index == y.index;
    }
    friend bool operator!=(const _iterator& x, const _iterator& y) noexcept {   // bool operator !=
      return !(x == y);
    }
};

template <typename T, typename N = std::size_t>
class stack_pool{
    struct node_t{
      T value;
      N next;

      template <typename X>
      node_t(X&& val, N x)
        : value{std::forward<X>(val)},                          // copy/move ctor
          next{x} {}

      explicit node_t(const N x): value{}, next{x} {}           // custom ctor
    };

    std::vector<node_t> pool;
    using stack_type = N;
    using value_type = T;
    using size_type = typename std::vector<node_t>::size_type;
    stack_type free_nodes{stack_type(0)}; // 0 at the beginning

    node_t& node(const stack_type x) noexcept { return pool[x-1]; }
    const node_t& node(const stack_type x) const noexcept { return pool[x-1]; }

    void _reserve(const stack_type n) {
      const stack_type tmp{capacity()+1};
      pool.reserve(n);                           // use reserve() method of std::vector
      _emplace_back(tmp, n);           
    }

    void _emplace_back(const stack_type start, const stack_type end){
      for(auto i = start; i < end; ++i )
        pool.emplace_back(i + 1);                   // set new free nodes, emplace_back is a std::vector's method
        
      pool.emplace_back(free_nodes);                // this points to the old top of the stack
      free_nodes = start; 
    }

    void check_capacity() {                         // function to check the capacity of the stack for a new push
      if(!capacity()) reserve(1);
      if(!empty(free_nodes))  return;

      reserve(capacity()*2);
    }

    template <typename X>
    stack_type _push(X&& val, const stack_type head) {
      check_capacity();
      auto tmp = free_nodes;                        // saves the first free node
      free_nodes = next(free_nodes);                // next free node
      value(tmp) = std::forward<X>(val);            // change the value of tmp node 
      next(tmp) = head;                             // next value of old free node is set equal to the head (given as input)
      return tmp;                                   // returns old value of free_nodes
    }

    stack_type _pop(const stack_type x) {           // delete first node
      auto tmp = next(x);                           
      next(x) = free_nodes;                         // next idx value of actual node is set as free_node
      free_nodes = x;                               // update free_nodes
      return tmp;                                
    }                                              

  public:
    stack_pool() noexcept = default;                 //default ctor
    explicit stack_pool(const size_type n) { reserve(n); } //custom ctor, reserve n nodes in the pool

    using iterator = _iterator<stack_pool, value_type, stack_type>;           // iterator and const_iterator
    using const_iterator = _iterator<stack_pool, const value_type, stack_type>;

    iterator begin(const stack_type x) { return iterator(this, x); }            
    iterator end(const stack_type) noexcept { return iterator(this, end()); }

    const_iterator begin(const stack_type x) const { return const_iterator(this, x); }
    const_iterator end(const stack_type ) const noexcept { return const_iterator(this, end()); }

    const_iterator cbegin(const stack_type x) const { return const_iterator(this,x); }
    const_iterator cend(const stack_type ) const noexcept { return const_iterator(this,end()); }

    stack_type new_stack() const noexcept { return end(); }                     // return an empty stack

    void reserve(const size_type n) { _reserve(n); }       // reserve n nodes in the pool

    size_type capacity() const noexcept { return pool.capacity(); }             // We can use methods of std::vector

    bool empty(const stack_type x) const noexcept { return x == end(); };       // empty check function

    stack_type end() const noexcept { return stack_type(0); }                   // end of the stack

    value_type& value(const stack_type x) noexcept { return node(x).value; }    // return value/const value of the node
    const value_type& value(const stack_type x) const { return node(x).value; }

    stack_type& next(const stack_type x) noexcept { 
      AP_ERROR(x > 0 && x <= capacity()) << "Invalid index";
      return node(x).next; }  

    const stack_type& next(const stack_type x) const noexcept {
      AP_ERROR(x > 0 && x <= capacity()) << "Invalid index"; 
      return node(x).next; 
    }

    template <typename X>
    stack_type push(X&& val, const stack_type head) { return _push(std::forward<X>(val),head); }    // return the index -> smaller and faster than a ref

    stack_type pop(const stack_type x) { return _pop(x); };

    stack_type free_stack(stack_type x) {
      while(!empty(x)){
        std::cout << x << " " << std::endl;
        x = pop(x);
      } 
      return x;
    } // free entire stack
};

int main(){
  {
    stack_pool<int, std::size_t> pool{};
    auto l = pool.new_stack();
    // l == pool.end() == std::size_t(0)
    l = pool.push(42,l);
    // l == std::size_t(1)
    pool.value(l) = 77;

    std::cout << "\nSCOPE #1\n" << std::endl;
    std::cout << "idx" << "\t" << "value" << std::endl;
    for (std::size_t i{pool.capacity()+1}; i > 0; --i){
        std::cout << i << "\t" << pool.value(i) << std::endl;
    }
  }

  {
    stack_pool<int, std::size_t> pool{};
    auto l = pool.new_stack();
    l = pool.push(10,l); // l == std::size_t(1)
    l = pool.push(11,l); // l == std::size_t(2) <-- later, this node will be deleted
  
    auto l2 = pool.new_stack();
    l2 = pool.push(20,l2); // l2 == std::size_t(3)
  
    l = pool.pop(l); // that node is deleted, so it is added to free_nodes
  
    l2 = pool.push(21,l2); // l2 == std::size_t(2)

    std::cout << "\nSCOPE #2\n" << std::endl;
    std::cout << "idx" << "\t" << "value" << std::endl;
    for (std::size_t i{pool.capacity()+1}; i > 0; --i){
        std::cout << i << "\t" << pool.value(i) << std::endl;
    }
  }

  {
    stack_pool<int> pool{22};
    auto l1 = pool.new_stack();
    // credits: pi as random number generator :)
    l1 = pool.push(3, l1);
    l1 = pool.push(1, l1);
    l1 = pool.push(4, l1);
    l1 = pool.push(1, l1);
    l1 = pool.push(5, l1);
    l1 = pool.push(9, l1);
    l1 = pool.push(2, l1);
    l1 = pool.push(6, l1);
    l1 = pool.push(5, l1);
    l1 = pool.push(3, l1);
    l1 = pool.push(5, l1);
    
    auto l2 = pool.new_stack();
    l2 = pool.push(8, l2);
    l2 = pool.push(9, l2);
    l2 = pool.push(7, l2);
    l2 = pool.push(9, l2);
    l2 = pool.push(3, l2);
    l2 = pool.push(1, l2);
    l2 = pool.push(1, l2);
    l2 = pool.push(5, l2);
    l2 = pool.push(9, l2);
    l2 = pool.push(9, l2);
    l2 = pool.push(7, l2);

    auto M = std::max_element(pool.begin(l1), pool.end(l1));
    assert(*M == 9);

    auto m = std::min_element(pool.begin(l2), pool.end(l2));
    assert(*m == 1);

    std::cout << "\nSCOPE #3\n" << std::endl;
    std::cout << "idx" << "\t" << "value" << std::endl;
    for (std::size_t i{pool.capacity()+1}; i > 0; --i){
        std::cout << i << "\t" << pool.value(i) << std::endl;
    }
  }
  return 0;
  
}
