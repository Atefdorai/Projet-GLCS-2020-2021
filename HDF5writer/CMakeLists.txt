cmake_minimum_required(VERSION 3.18)

add_library(writelib
		src/hdf5writer.cpp
)
target_include_directories(writelib PUBLIC include/)
target_link_libraries(writelib PUBLIC GLCS2020_project::baselib)
add_library(GLCS2020_project::writelib ALIAS writelib)
