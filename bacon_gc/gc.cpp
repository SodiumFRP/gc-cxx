#include "gc.h"
#include <algorithm>
#include <memory>
#include <vector>
#include <assert.h>
#include <mutex>

namespace bacon_gc {

    struct Ctx {
        std::vector<Node*> roots;
        bool collecting_cycles;
        std::vector<Node*> to_be_freed;
    };

    static Ctx ctx;
    static std::mutex ctx_mutex;

    template<class R, class F>
    R with_ctx(F&& k) {
        std::lock_guard<std::mutex> guard(ctx_mutex);
        return k(ctx);
    }

    template<class F>
    void with_ctx_void(F&& k) {
        std::lock_guard<std::mutex> guard(ctx_mutex);
        k(ctx);
    }

    static void release(Node* s);
    static void possible_root(Node* s);
    static void deferred_free(Node* s);
    static void mark_roots();
    static void scan_roots();
    static void collect_roots();
    static void mark_gray(Node* s);
    static void scan(Node* s);
    static void scan_black(Node* s);
    static void collect_white(Node* s);

    template<class F>
    void trace_node(Node* node, F&& k) {
        if (node->strong > 0) {
            node->trace_(std::function<void(Node*)>(k));
        }
    }

    void increment(Node* s) {
        ++s->strong;
        s->colour = Colour::Black;
    }

    void decrement(Node* s) {
        if (s->strong > 0) {
            --s->strong;
            if (s->strong == 0) {
                release(s);
            } else {
                possible_root(s);
            }
        }
    }

    void increment_weak(Node* s) {
        ++s->weak;
    }

    void decrement_weak(Node* s) {
        if (s->weak > 0) {
            --s->weak;
            if (s->weak == 0) {
                release(s);
            }
        }
    }

    static void system_free(Node* s);

    static void release(Node* s) {
        assert(s->strong == 0);
        s->colour = Colour::Black;
        if (!s->buffered) {
            deferred_free(s);
        }
    }

    static void deferred_free(Node* s) {
        with_ctx_void([s](Ctx& ctx) {
            ctx.to_be_freed.push_back(s);
        });
    }

    static void system_free(Node* s) {
        assert(s->strong == 0 && s->weak > 0);
        s->cleanup();
        if (s->weak > 0) {
            --s->weak;
            if (s->weak == 0) {
                // FIXME: Uncommenting this line causes a crash later on. Figure out why.
                //delete s;
            }
        }
    }

    static void possible_root(Node* s) {
        assert(s->strong > 0);
        if (s->colour != Colour::Purple) {
            s->colour = Colour::Purple;
            if (!s->buffered) {
                s->buffered = true;
                with_ctx_void([s](Ctx& ctx) {
                    if (std::find(ctx.roots.begin(), ctx.roots.end(), s) == ctx.roots.end()) {
                        ctx.roots.push_back(s);
                    }
                });
            }
        }
    }

    void collect_cycles() {
        bool skip = with_ctx<bool>([](Ctx& ctx) {
            if (ctx.collecting_cycles) {
                return true;
            }
            ctx.collecting_cycles = true;
            return false;
        });
        if (skip) {
            return;
        }

        mark_roots();
        scan_roots();
        collect_roots();

        std::vector<Node*> to_be_freed;
        with_ctx_void([&to_be_freed](Ctx& ctx) {
            to_be_freed.insert(to_be_freed.end(), ctx.to_be_freed.begin(), ctx.to_be_freed.end());
            ctx.to_be_freed.clear();
        });
        for (std::vector<Node*>::iterator it = to_be_freed.begin(); it != to_be_freed.end(); ++it) {
            (*it)->finalize();
        }
        for (std::vector<Node*>::iterator it = to_be_freed.begin(); it != to_be_freed.end(); ++it) {
            system_free(*it);
        }

        with_ctx_void([](Ctx& ctx) {
            ctx.collecting_cycles = false;
        });
    }

    static void mark_roots() {
        std::vector<Node*> roots = with_ctx<std::vector<Node*> >([](Ctx& ctx) { return ctx.roots; });
        std::vector<Node*> new_roots;
        for (std::vector<Node*>::iterator s_it = roots.begin(); s_it != roots.end(); ++s_it) {
            Node* s = *s_it;
            if (s->colour == Colour::Purple && s->strong > 0) {
                mark_gray(s);
                new_roots.push_back(s);
            } else {
                s->buffered = false;
                if (s->colour == Colour::Black && s->strong == 0) {
                    deferred_free(s);
                }
            }
        }
        with_ctx_void([new_roots](Ctx& ctx) { ctx.roots = new_roots; });
    }

    static void scan_roots() {
        std::vector<Node*> roots = with_ctx<std::vector<Node*> >([](Ctx& ctx) { return ctx.roots; });
        for (std::vector<Node*>::iterator s_it = roots.begin(); s_it != roots.end(); ++s_it) {
            Node* s = *s_it;
            scan(s);
        }
    }

    static void collect_roots() {
        std::vector<Node*> roots = with_ctx<std::vector<Node*> >([](Ctx& ctx) { return ctx.roots; });
        for (std::vector<Node*>::iterator s_it = roots.begin(); s_it != roots.end(); ++s_it) {
            Node* s = *s_it;
            s->buffered = false;
            collect_white(s);
        }
        with_ctx_void([](Ctx& ctx) {
            ctx.roots.clear();
        });
    }

    static void mark_gray(Node* s) {
        if (s->colour != Colour::Gray) {
            s->colour = Colour::Gray;
            trace_node(s, [](Node* t) {
                --t->strong;
                mark_gray(t);
            });
        }
    }

    static void scan(Node* s) {
        if (s->colour == Colour::Gray) {
            if (s->strong > 0) {
                scan_black(s);
            } else {
                s->colour = Colour::White;
                trace_node(s, [](Node* t) {
                    scan(t);
                });
            }
        }
    }

    static void scan_black(Node* s) {
        s->colour = Colour::Black;
        trace_node(s, [](Node* t) {
            ++t->strong;
            if (t->colour != Colour::Black) {
                scan_black(t);
            }
        });
    }

    static void collect_white(Node* s) {
        if (s->colour == Colour::White && !s->buffered) {
            s->colour = Colour::Black;
            trace_node(s, [](Node* t) {
                collect_white(t);
            });
            deferred_free(s);
        }
    }

} // end namespace bacon_gc
