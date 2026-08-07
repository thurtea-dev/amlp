add_library(efun
    EfunTable.cpp
)
target_include_directories(efun PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(efun PUBLIC core vm object net crypt)
