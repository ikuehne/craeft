cmake_minimum_required(VERSION 2.8.9)
project(craeft)

file(GLOB SOURCES "src/*.cpp")

add_executable(craeftc ${SOURCES})

# LLVM stuff
execute_process(COMMAND "llvm-config" "--includedir"
	OUTPUT_VARIABLE LLVM_INCLUDE OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND "llvm-config" "--cppflags"
	OUTPUT_VARIABLE LLVM_FLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND "llvm-config" "--ldflags"
	OUTPUT_VARIABLE LLVM_LDFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND "llvm-config" "--system-libs"
	OUTPUT_VARIABLE LLVM_SYS_LIBS OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND "llvm-config" "--libs" "all"
	OUTPUT_VARIABLE LLVM_LIBS OUTPUT_STRIP_TRAILING_WHITESPACE)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${LLVM_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++14")

find_package (Boost REQUIRED COMPONENTS Program_options)

include_directories(${BOOST_INCLUDE_DIRS})
target_link_libraries(craeftc LINK_PUBLIC ${Boost_LIBRARIES})
include_directories("include")

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${LLVM_LDFLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${LLVM_LIBS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${LLVM_SYS_LIBS}")

# Documentation

find_package(Doxygen)
option(BUILD_DOCUMENTATION "Create and install the HTML based documentation (requires Doxygen)" ${DOXYGEN_FOUND})

if(BUILD_DOCUMENTATION)
    if(NOT DOXYGEN_FOUND)
        message(FATAL_ERROR "Doxygen is needed to build documentation.")
    endif()

    set(doxyfile_in doc/Doxyfile.in)
    set(doxyfile doc/Doxyfile)

    configure_file(${doxyfile_in} ${doxyfile} @ONLY)

    add_custom_target(doc
        COMMAND ${DOXYGEN_EXECUTABLE} ${doxyfile}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating Doxygen documentation"
        VERBATIM)

    set(DOC_INPUT ${CMAKE_PROJECT_BINARY_DIR}/doc/html)
    set(DOC_OUTPUT ${CMAKE_PROJECT_SOURCE_DIR}/doc/html)

    add_custom_command(TARGET doc POST_BUILD
	    COMMAND ${CMAKE_COMMAND} -E remove_directory ../doc/html)
    add_custom_command(TARGET doc POST_BUILD
	    COMMAND ${CMAKE_COMMAND} -E rename doc/html ../doc/html)
endif()
