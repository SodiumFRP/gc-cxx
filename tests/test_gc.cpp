#include "test_gc.h"
#include <bacon_gc/gc.h>
#include <boost/optional.hpp>

#include <cppunit/ui/text/TestRunner.h>
#include <stdio.h>
#include <ctype.h>
#include <iostream>

using namespace std;
using namespace boost;

void test_sodium::tearDown()
{
}

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

template <>
struct Trace<MyObj1> {
    template <typename F>
    static void trace(const MyObj1& a, F&& k) {
        if (a.next) {
            k(a.next.value().__node());
        }
    }
};

void test_sodium::cycle()
{
    {
        Gc<MyObj1> a(new MyObj1());
        Gc<MyObj1> b(new MyObj1());
        Gc<MyObj1> c(new MyObj1());
        a.value().next = b;
        b.value().next = c;
        c.value().next = a;
    }
    CPPUNIT_ASSERT(my_obj_1_count == 0);
}

int main(int argc, char* argv[])
{
    CppUnit::TextUi::TestRunner runner;
    runner.addTest( test_sodium::suite() );
    bool success = runner.run();
    return success ? 0 : 1;
}
