cmake_minimum_required( VERSION 2.8 )

set( CMAKE_INSTALL_PREFIX /usr/local )

project(bacon_gc)
include(CheckCXXCompilerFlag)

check_cxx_compiler_flag("-std=c++11" SUPPORTS_CXX11)
if( SUPPORTS_CXX11 )
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
else()
    check_cxx_compiler_flag("-std=c++0x" SUPPORTS_CXX0X)
    if( SUPPORTS_CXX0X )
        set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
    else()
        message("C++11 is not supported")
        add_definitions(-DBACON_GC_NO_CXX11)
    endif()
endif()

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wall -Wno-deprecated-declarations" )

set( BACON_GC_BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/bacon_gc )

set( CMAKE_INCLUDE_CURRENT_DIR ON )

find_package( Boost REQUIRED )

set(BACON_GC_INCLUDE_FILES
    ${BACON_GC_BASE_DIR}/gc.h
    )

set(BACON_GC_SOURCE_FILES
    ${BACON_GC_BASE_DIR}/gc.cpp
    )

file(
    COPY ${BACON_GC_INCLUDE_FILES} 
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/include/bacon_gc
    )

include_directories( ${CMAKE_SOURCE_DIR} ${Boost_INCLUDE_DIR} )

set( ALL_SOURCES ${BACON_GC_INCLUDE_FILES} ${BACON_GC_SOURCE_FILES} )

add_library( bacon_gc SHARED ${ALL_SOURCES} )
add_library( libbacon_gc STATIC ${ALL_SOURCES} )

set_target_properties( libbacon_gc PROPERTIES OUTPUT_NAME bacon_gc )

# Testing

enable_testing()
add_subdirectory(tests)

# INSTALL
# -------

install (
    TARGETS bacon_gc libbacon_gc
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    )

# Install header files
install (
    FILES ${BACON_GC_INCLUDE_FILES}
    DESTINATION include/bacon_gc
    )
