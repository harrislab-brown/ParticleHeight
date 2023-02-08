set(NAME util)

set(HEADERS
  Settings.h
  vf3.h
) # HEADERS    

set(SOURCES
  Settings.cpp
) # SOURCES

add_library(${NAME}
    ${HEADERS}
    ${SOURCES}
) # add_library

target_compile_features(${NAME} PRIVATE cxx_lambdas)

target_link_libraries(${NAME} ${LIB_LIST})
