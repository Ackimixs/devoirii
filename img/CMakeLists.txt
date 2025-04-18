add_executable(img
        main.cpp
)

add_executable(ex
    example.cpp
)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/Images DESTINATION ${CMAKE_CURRENT_BINARY_DIR})