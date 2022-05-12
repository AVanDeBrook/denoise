# CMakeList.txt : CMake project for denoise, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project("denoise")

# NVAFX Library
add_library(NVAudioEffects INTERFACE)
target_include_directories(NVAudioEffects INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/lib")
target_link_libraries(NVAudioEffects INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/lib/NVAudioEffects.lib")

# lbsndfile Library
add_library(libsndfile INTERFACE)
target_include_directories(libsndfile INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/lib")
target_link_libraries(libsndfile INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/lib/libsndfile-1.lib")

# Source Files
set(SOURCE_FILES "denoise.c" "denoise.h")
source_group("source" FILES ${SOURCE_FILES})
add_executable (denoise ${SOURCE_FILES})
set_property(TARGET denoise PROPERTY CXX_STANDARD 11)
target_link_libraries(denoise PUBLIC NVAudioEffects)
target_link_libraries(denoise PUBLIC libsndfile)
target_link_libraries(denoise ${LINK_LIBS})
