cmake_minimum_required( VERSION 2.8 )

set( CMAKE_INSTALL_PREFIX /usr/local )

project(gc)
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
        add_definitions(-DGC_NO_CXX11)
    endif()
endif()


set( GC_BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/gc )

set( CMAKE_INCLUDE_CURRENT_DIR ON )

find_package( Boost REQUIRED )

file( GLOB_RECURSE 
    GC_INCLUDE_FILES 
    ${GC_BASE_DIR}/*.h
    )

file( GLOB_RECURSE 
    GC_SOURCE_FILES
    ${GC_BASE_DIR}/*.cpp
    )

file(
    COPY ${GC_INCLUDE_FILES} 
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/include/gc
    )

include_directories( ${Boost_INCLUDE_DIR} )

set( ALL_SOURCES ${GC_INCLUDE_FILES} ${GC_SOURCE_FILES} )

add_library( gc SHARED ${ALL_SOURCES} )
add_library( libgc STATIC ${ALL_SOURCES} )

set_target_properties( libgc PROPERTIES OUTPUT_NAME gc )

# INSTALL
# -------

install (
    TARGETS gc libgc
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    )

# Install header files
install (
    FILES ${GC_INCLUDE_FILES}
    DESTINATION include/gc
    )
