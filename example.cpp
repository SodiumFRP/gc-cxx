#include "gc.h"
#include <iostream>
#include <experimental/optional>

using namespace std;
using namespace std::experimental;
using namespace bacon_gc;

static int my_obj_1_count = 0;

struct MyObj1 {
    optional<Gc<MyObj1>> next;

    MyObj1() {
        ++my_obj_1_count;
    }

    ~MyObj1() {
        --my_obj_1_count;
    }
};

namespace bacon_gc {
    template <>
    struct Trace<MyObj1> {
        template <typename F>
        static void trace(const MyObj1& a, F&& k) {
            if (a.next) {
                k(a.next.value().__node());
            }
        }
    };
}

void example1() {
    {
        Gc<MyObj1> a(new MyObj1());
        Gc<MyObj1> b(new MyObj1());
        Gc<MyObj1> c(new MyObj1());
        a.value().next = b;
        b.value().next = c;
        c.value().next = a;
        cout << "my_obj_1_count before end of local scope: " << my_obj_1_count << endl;
    }
    cout << "my_obj_1_count after end of local scope: " << my_obj_1_count << endl;
}

int main() {
    example1();
    return 0;
}
