add_library(bno055 STATIC bno055.c)
target_include_directories(bno055 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(bno055 PRIVATE strict_warnings)
