find_package(wxWidgets CONFIG REQUIRED)

add_executable(test_auth test_auth.cpp)
target_link_libraries(test_auth PRIVATE remont_core wx::core wx::base)

add_executable(test_order test_order.cpp)
target_link_libraries(test_order PRIVATE remont_core wx::core wx::base)

add_executable(test_warehouse test_warehouse.cpp)
target_link_libraries(test_warehouse PRIVATE remont_core wx::core wx::base)