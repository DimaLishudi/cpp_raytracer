add_catch(test_raytracer tests/test.cpp)

if (TEST_SOLUTION)
    target_include_directories(test_raytracer PRIVATE ../tests/raytracer-geom)
    target_include_directories(test_raytracer PRIVATE ../tests/raytracer-reader)
else()
    target_include_directories(test_raytracer PRIVATE ../raytracer-geom)
    target_include_directories(test_raytracer PRIVATE ../raytracer-reader)
endif()

target_link_libraries(test_raytracer PRIVATE ${PNG_LIBRARY})
target_include_directories(test_raytracer PRIVATE ${PNG_INCLUDE_DIRS})
