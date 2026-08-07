add_executable(lpcdriver_tests
    test_lexer.cpp
)
target_include_directories(lpcdriver_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(lpcdriver_tests
    PRIVATE
        core config compiler vm object efun apply net scheduler
)
add_test(NAME lpcdriver_tests COMMAND lpcdriver_tests)
