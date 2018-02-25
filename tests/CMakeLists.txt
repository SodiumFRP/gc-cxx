add_executable( test_gc test_gc.cpp )
target_link_libraries( test_gc cppunit libbacon_gc )
add_test( test_gc test_gc )
