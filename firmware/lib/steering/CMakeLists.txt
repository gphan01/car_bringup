add_library(steering STATIC steering.c)
target_include_directories(steering PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(steering PRIVATE strict_warnings)
