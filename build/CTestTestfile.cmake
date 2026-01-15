# CMake generated Testfile for 
# Source directory: D:/Project/MoM/PulseMoM
# Build directory: D:/Project/MoM/PulseMoM/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if(CTEST_CONFIGURATION_TYPE MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test(ComprehensiveValidation "D:/Project/MoM/PulseMoM/build/Debug/comprehensive_test_suite.exe")
  set_tests_properties(ComprehensiveValidation PROPERTIES  _BACKTRACE_TRIPLES "D:/Project/MoM/PulseMoM/CMakeLists.txt;515;add_test;D:/Project/MoM/PulseMoM/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test(ComprehensiveValidation "D:/Project/MoM/PulseMoM/build/Release/comprehensive_test_suite.exe")
  set_tests_properties(ComprehensiveValidation PROPERTIES  _BACKTRACE_TRIPLES "D:/Project/MoM/PulseMoM/CMakeLists.txt;515;add_test;D:/Project/MoM/PulseMoM/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
  add_test(ComprehensiveValidation "D:/Project/MoM/PulseMoM/build/MinSizeRel/comprehensive_test_suite.exe")
  set_tests_properties(ComprehensiveValidation PROPERTIES  _BACKTRACE_TRIPLES "D:/Project/MoM/PulseMoM/CMakeLists.txt;515;add_test;D:/Project/MoM/PulseMoM/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
  add_test(ComprehensiveValidation "D:/Project/MoM/PulseMoM/build/RelWithDebInfo/comprehensive_test_suite.exe")
  set_tests_properties(ComprehensiveValidation PROPERTIES  _BACKTRACE_TRIPLES "D:/Project/MoM/PulseMoM/CMakeLists.txt;515;add_test;D:/Project/MoM/PulseMoM/CMakeLists.txt;0;")
else()
  add_test(ComprehensiveValidation NOT_AVAILABLE)
endif()
