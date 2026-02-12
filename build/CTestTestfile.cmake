# CMake generated Testfile for 
# Source directory: /home/selim/ADAS-Miles
# Build directory: /home/selim/ADAS-Miles/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(SignalBusTests "/home/selim/ADAS-Miles/build/test_signal_bus")
set_tests_properties(SignalBusTests PROPERTIES  _BACKTRACE_TRIPLES "/home/selim/ADAS-Miles/CMakeLists.txt;92;add_test;/home/selim/ADAS-Miles/CMakeLists.txt;0;")
add_test(E2ETests "/home/selim/ADAS-Miles/build/test_e2e")
set_tests_properties(E2ETests PROPERTIES  _BACKTRACE_TRIPLES "/home/selim/ADAS-Miles/CMakeLists.txt;96;add_test;/home/selim/ADAS-Miles/CMakeLists.txt;0;")
