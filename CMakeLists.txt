cmake_minimum_required( VERSION 2.8.9 )

project( gen_png )

include_directories( $CMAKE_SOURCE_DIR )

add_executable( gen_png gen_png.c )

target_link_libraries( gen_png png z m )

