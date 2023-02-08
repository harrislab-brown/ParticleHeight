set(NAME ray)

set(HEADERS
  OpticalLayer.h
  OpticalMedium.h
  OpticalPattern.h
  OpticalScene.h
  OpticalSphere.h
  Ray.h
) # HEADERS    

set(SOURCES
  OpticalLayer.cpp
  OpticalPattern.cpp
  OpticalScene.cpp
  OpticalSphere.cpp
  Ray.cpp
) # SOURCES

add_library(${NAME}
    ${HEADERS}
    ${SOURCES}
) # add_library

target_compile_features(${NAME} PRIVATE cxx_lambdas)

target_link_libraries(${NAME} ${LIB_LIST})
