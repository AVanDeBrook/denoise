set(SOURCE denoise.c denoise.h)
set(SOURCE_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR})
source_group("source" FILES ${SOURCE})

add_executable(denoise ${SOURCE})

target_include_directories(denoise PUBLIC ${SOURCE_INCLUDE} ${LIB_INCLUDE})

target_link_libraries(denoise PUBLIC NVAudioEffects)
target_link_libraries(denoise PUBLIC libsndfile)
target_link_libraries(denoise ${LINK_LIBRARIES})
