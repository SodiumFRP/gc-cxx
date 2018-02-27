#include "test_gc.h"
#include <bacon_gc/gc.h>
#include <boost/optional.hpp>

#include <cppunit/ui/text/TestRunner.h>
#include <stdio.h>
#include <ctype.h>
#include <iostream>

using namespace std;
using namespace boost;
using namespace bacon_gc;

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

static int my_obj_2_count = 0;
static int my_obj_2_finalize_count = 0;

struct MyObj2 {
    optional<Gc<MyObj2>> next;
    int val;

    MyObj2(): val(0) {
        ++my_obj_2_count;
    }

    ~MyObj2() {
        --my_obj_2_count;
    }
};

namespace bacon_gc {
    template <>
    struct Trace<MyObj2> {
        template <typename F>
        static void trace(const MyObj2& a, F&& k) {
            if (a.next) {
                k(a.next.value().__node());
            }
        }
    };

    template <>
    struct Finalize<MyObj2> {
        static void finalize(MyObj2& a) {
            a.next.value().value().val++;
            ++my_obj_2_finalize_count;
        }
    };
}


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

void test_sodium::finalize()
{
    {
        Gc<MyObj2> a(new MyObj2());
        Gc<MyObj2> b(new MyObj2());
        Gc<MyObj2> c(new MyObj2());
        a.value().next = b;
        b.value().next = c;
        c.value().next = a;
    }
    CPPUNIT_ASSERT(my_obj_2_count == 0 && my_obj_2_finalize_count == 3);
}

int main(int argc, char* argv[])
{
    CppUnit::TextUi::TestRunner runner;
    runner.addTest( test_sodium::suite() );
    bool success = runner.run();
    return success ? 0 : 1;
}
