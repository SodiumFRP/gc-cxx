#ifndef _TEST_GC_H_
#define _TEST_GC_H_

#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>
#include <bacon_gc/gc.h>
#include <string>

class test_sodium : public CppUnit::TestFixture {
    CPPUNIT_TEST_SUITE(test_sodium);
    // gc tests
    CPPUNIT_TEST(cycle);
    CPPUNIT_TEST(finalize);
    CPPUNIT_TEST_SUITE_END();

public:
    virtual void tearDown();

    void cycle();
    void finalize();
};

#endif
