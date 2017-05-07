cmake_minimum_required(VERSION 2.6)
project(itprime_test_project)

add_executable(itprime_test_project main.cpp)

install(TARGETS itprime_test_project RUNTIME DESTINATION bin)
