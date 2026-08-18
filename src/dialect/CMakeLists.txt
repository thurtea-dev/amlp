add_library(dialect
    LpcDialect.cpp
    FluffOsBootApi.cpp
    LdmudBootApi.cpp
    MasterUidBoot.cpp
    DialectSelect.cpp
)
target_include_directories(dialect PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(dialect PUBLIC core config vm)
