﻿cmake_minimum_required (VERSION 3.8)

project("denoise")

# NVAFX Library
add_library(NVAudioEffects INTERFACE)
target_include_directories(NVAudioEffects INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/lib)
target_link_libraries(NVAudioEffects INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/lib/NVAudioEffects.lib)

# lbsndfile Library
add_library(libsndfile INTERFACE)
target_include_directories(libsndfile INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/lib)
target_link_libraries(libsndfile INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/lib/libsndfile-1.lib)

# include directory for libraries
set(LIB_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/lib)

add_subdirectory(source)
