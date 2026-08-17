add_executable(amlp_tests
    test_lexer.cpp
)
target_include_directories(amlp_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(amlp_tests
    PRIVATE
        core config compiler vm object efun apply dialect net scheduler
)
add_test(NAME amlp_tests COMMAND amlp_tests)
