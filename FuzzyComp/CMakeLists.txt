cmake_minimum_required(VERSION 3.30)

project(FuzzyComp VERSION 0.1.0.0 DESCRIPTION "Fuzzy text searcher for 1C" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

set(CMAKE_INCLUDE_CURRENT_DIR ON)

math(EXPR BITNESS "${CMAKE_SIZEOF_VOID_P} * 8")
if(APPLE)
    set(POSTFIX "_Mac")
elseif(UNIX)
    set(POSTFIX "_Lin")
elseif(WIN32)
    file(GENERATE OUTPUT comp.def INPUT comp.def.in TARGET ${CMAKE_PROJECT_NAME})
    list(APPEND EXTRA_SRC comp.def)
    set(POSTFIX "_Win")
    add_definitions(-DUNICODE)
    if (${BITNESS} EQUAL 32)
        add_definitions(-D_USE_32BIT_TIME_T)
    endif()
endif()

string(APPEND POSTFIX _${BITNESS})

set(1C_LIB_DIR "${CMAKE_SOURCE_DIR}/../VNCOMPS83" CACHE STRING "1C external component support library location")
include_directories("${1C_LIB_DIR}/include")
link_directories("${1C_LIB_DIR}/lib")

find_package(Boost REQUIRED COMPONENTS algorithm headers)
find_package(RapidJSON REQUIRED)

file(GENERATE OUTPUT version.h CONTENT "#define VERSION ${CMAKE_PROJECT_VERSION_MAJOR}${CMAKE_PROJECT_VERSION_MINOR}${CMAKE_PROJECT_VERSION_PATCH}${CMAKE_PROJECT_VERSION_TWEAK}")

set(SOURCES dllmain.cpp helpers.h helpers.cpp
            ComponentImpl.h ComponentImpl.cpp
            AbstractIndex.h
            NGramVector.h
            AsymmetricNGramIndex.h
            SymmetricNGramIndex.h
            ${EXTRA_SRC})

add_library(${CMAKE_PROJECT_NAME} SHARED ${SOURCES})
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE INC_OLE1 HV_PROJECT_NAME=${CMAKE_PROJECT_NAME})
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Boost::algorithm Boost::headers rapidjson)

if(MSVC)
    target_link_options(${CMAKE_PROJECT_NAME} PRIVATE "$<$<CONFIG:Debug>:/ASSEMBLYDEBUG>")
endif(MSVC)

set_property(TARGET ${CMAKE_PROJECT_NAME} PROPERTY OUTPUT_NAME "${CMAKE_PROJECT_NAME}${POSTFIX}")
