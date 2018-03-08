#include "bacon_gc/gc.h"
#include <iostream>

using namespace std;
using namespace bacon_gc;

static int my_obj_1_count = 0;

struct MyObj1 {
    Gc<MyObj1> next;

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
                Trace<Gc<MyObj1>>::trace(a.next, k);
            }
        }
    };
}

void example1() {
    {
        Gc<MyObj1> a(new MyObj1());
        Gc<MyObj1> b(new MyObj1());
        Gc<MyObj1> c(new MyObj1());
        a->next = b;
        b->next = c;
        c->next = a;
        cout << "my_obj_1_count before end of local scope: " << my_obj_1_count << endl;
    }
    cout << "my_obj_1_count after end of local scope: " << my_obj_1_count << endl;
}

int main() {
    example1();
    return 0;
}
