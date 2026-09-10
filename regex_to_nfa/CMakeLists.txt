cmake_minimum_required(VERSION 3.16)
project(regex_to_nfa C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED_ON)

add_executable(regex_to_nfa src/main.c src/regex.c src/nfa.c)

# Pruebas unitarias del parser, la construccion de Thompson y match_nfa
add_executable(unit_tests tests/test_main.c src/regex.c src/nfa.c)
enable_testing()
add_test(NAME unit_tests COMMAND unit_tests)

# Genera un HTML con el diagrama del NFA a partir del JSON de save_nfa (-o)
add_executable(nfa_visualizer tools/nfa_visualizer.c)
target_link_libraries(nfa_visualizer m)

