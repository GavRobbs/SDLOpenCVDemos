add_executable(CoinCollector main.cpp)

set_target_properties(CoinCollector PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_BINARY_DIR}/Debug
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_BINARY_DIR}/Release
)

add_custom_command(TARGET CoinCollector POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_CURRENT_SOURCE_DIR}/coin.png
    $<TARGET_FILE_DIR:CoinCollector>/coin.png
)