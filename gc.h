#ifndef _SODIUM_GC_H_
#define _SODIUM_GC_H_

/*
 * A Pure Reference Counting Garbage Collector
 * DAVID F. BACON, CLEMENT R. ATTANASIO, V.T. RAJAN, STEPHEN E. SMITH
 */

#include <stdint.h>
#include <functional>
#include <utility>
#include <vector>
#include <boost/optional.hpp>

enum Colour {
    Black,
    Purple,
    White,
    Gray
};

struct Node {
    uint32_t strong;
    uint32_t weak;
    Colour colour;
    bool buffered;
    std::function<void(std::function<void(Node*)>)> trace_;
    std::function<void()> cleanup;
};

template<class TF, class CF>
Node* new_gc_node(TF tf, CF cf) {
    Node* node = new Node;
    node->strong = 1;
    node->weak = 1;
    node->colour = Colour::Black;
    node->buffered = false;
    node->trace_ = std::function<void(std::function<void(Node*)>)>(tf);
    node->cleanup = std::function<void()>(cf);
    return node;
}

void increment(Node* s);

void decrement(Node* s);

void collect_cycles();

template <class A>
class Gc;

template <typename A>
struct Trace {
    template <typename F>
    static void trace(const A& a, F&& k);
};

template <typename A>
struct Trace<std::vector<A>> {
    template <typename F>
    static void trace(const std::vector<A>& a, F&& k) {
        for (typename std::vector<A>::iterator it = a.begin(); it != a.end(); ++it) {
            k(*it);
        }
    }
};

template <typename A>
struct Trace<Gc<A>> {
    template <typename F>
    static void trace(const Gc<A>& a, F&& k) {
        k(a.__node());
    }
};

template <class A>
class GcWeak;

template <class A>
class Gc {
public:
    Gc(): Gc(NULL) {}

    Gc(A* value, Node* node) {
        this->_value = value;
        this->_node = node;
    }

    Gc(A* value) {
        A* value2 = value;
        this->_value = value2;
        auto cleanup = [value2]() {
            if (value2 != NULL) {
                delete value2;
            }
        };
        A* value3 = this->_value;
        this->_node = new_gc_node(
            [value3](std::function<void(Node*)> k) {
                if (value3 != NULL) {
                    Trace<A>::trace(*value3, k);
                }
            },
            cleanup
        );
    }

    Gc(const Gc<A>& gc) {
        this->_value = gc._value;
        this->_node = gc._node;
        increment(this->_node);
    }

    ~Gc() {
        decrement(_node);
        collect_cycles();
    }

    A& operator*() const {
        return *_value;
    }

    A* operator->() const {
        return _value;
    }

    A& value() {
        return *_value;
    }

    operator bool() const {
        return _value != NULL;
    }

    const A& value() const {
        return *_value;
    }

    int strong_count() const {
        return _node->strong;
    }

    int weak_count() const {
        return _node->weak;
    }

    A* get() const {
        return _value;
    }

    Node* __node() const {
        return _node;
    }

    A* __value() const {
        return _value;
    }

    GcWeak<A>&& downgrade() const {
        return GcWeak<A>(*this);
    }

    template <class B>
    Gc<B> static_cast_() const {
        increment(this->_node);
        return Gc<B>((B*)_value, _node);
    }

private:
    A* _value;
    Node* _node;
};

template <class A>
class GcWeak {
public:
    GcWeak(const Gc<A>& gc) {
        Node* node = gc.__node();
        ++node->weak;
        this->_value = gc.__value();
        this->_node = node;
    }

    GcWeak(const GcWeak<A>& gc) {
        ++gc._node->weak;
        this->_value = gc._value;
        this->_node = gc._node;
    }

    ~GcWeak() {
        if (_node->weak > 0) {
            --_node->weak;
            if (_node->weak == 0) {
                delete _node;
            }
        }
    }

    Gc<A> lock() const {
        return upgrade().value_or(Gc<A>());
    }

    boost::optional<Gc<A> > upgrade() const {
        if (_node->strong == 0) {
            return {};
        } else {
            ++_node->strong;
            return Gc<A>(_value, _node);
        }
    }
private:
    A* _value;
    Node* _node;
};

#endif
