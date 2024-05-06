set(COMMON_HEADERS
  adapter.h
  array.h
  chrono.h
  input.h
  json.h
)

set(COMMON_SOURCES
  chrono.cpp
  input.cpp
  json.cpp
)

add_library(common STATIC ${COMMON_HEADERS} ${COMMON_SOURCES})

target_include_directories(common PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/..
                                         ${CMAKE_CURRENT_SOURCE_DIR}/..)

set_target_properties(common PROPERTIES
  # Language settings
  LINKER_LANGUAGE CXX
  C_STANDARD 17
  C_STANDARD_REQUIRED ON
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED ON
  CXX_VISIBILITY_PRESET default

  # Visual Studio
  FOLDER ${PROJECT_NAME}

  # Project Information
  VERSION ${PROJECT_VERSION}
)

target_compile_features(common PUBLIC cxx_std_17)
