cmake_minimum_required (VERSION 2.6)

project (chess CXX)

add_executable(chess chess.cpp user_interface.cpp main.cpp game.cpp game.h)

set_property(TARGET chess PROPERTY CXX_STANDARD 11)
set_property(TARGET chess PROPERTY CXX_STANDARD_REQUIRED ON) 
