# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

# find Qt5 modules
find_package(Qt5 "5.2.0" NO_MODULE REQUIRED Core Widgets Test)

# set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})

include_directories(
  # ${CMAKE_SOURCE_DIR}/src/include # TODO
  ${CMAKE_CURRENT_BINARY_DIR}
)

# Test: Test1
set(Test1Src Test1.cpp)
add_executable(Test1 ${Test1Src})
target_link_libraries(Test1 Qt5::Core Qt5::Test kreenshoteditor)
add_test(NAME Test1 COMMAND Test1)

# # Test: MetaPos
# set(TestMetaPosSrc TestMetaPos.cpp)
# add_executable(TestMetaPos ${TestMetaPosSrc})
# target_link_libraries(TestMetaPos Qt5::Core Qt5::Test qtikzcore)
# add_test(NAME TestMetaPos COMMAND TestMetaPos)


# kate: indent-width 4; replace-tabs on;