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

message("HELLO THERE ${CMAKE_EXE_LINKER_FLAGS}")


find_package (Boost REQUIRED COMPONENTS Program_options)

include_directories(${BOOST_INCLUDE_DIRS})
target_link_libraries(craeftc LINK_PUBLIC ${Boost_LIBRARIES})
include_directories("include")

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${LLVM_LDFLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${LLVM_LIBS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${LLVM_SYS_LIBS}")
