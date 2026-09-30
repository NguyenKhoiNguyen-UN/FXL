# Tests CMakeLists.txt

# Lexer tests
add_executable(test_lexer ${CMAKE_SOURCE_DIR}/tests/lexer/test_lexer.cpp)
target_link_libraries(test_lexer fxl_compiler)
add_test(NAME lexer_tests COMMAND test_lexer)

# Parser tests
add_executable(test_parser ${CMAKE_SOURCE_DIR}/tests/parser/test_parser.cpp)
target_link_libraries(test_parser fxl_compiler)
add_test(NAME parser_tests COMMAND test_parser)

# Semantic tests
add_executable(test_semantic ${CMAKE_SOURCE_DIR}/tests/semantic/test_semantic.cpp)
target_link_libraries(test_semantic fxl_compiler)
add_test(NAME semantic_tests COMMAND test_semantic)

# AutoSet tests
add_executable(test_autoset ${CMAKE_SOURCE_DIR}/tests/autoset/test_autoset.cpp)
target_link_libraries(test_autoset fxl_compiler)
add_test(NAME autoset_tests COMMAND test_autoset)

# Codegen tests
add_executable(test_codegen ${CMAKE_SOURCE_DIR}/tests/codegen/test_codegen.cpp)
target_link_libraries(test_codegen fxl_compiler)
add_test(NAME codegen_tests COMMAND test_codegen)