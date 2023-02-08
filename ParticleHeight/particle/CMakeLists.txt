set(NAME ParticleHeight)

set(HEADERS
  Particle.h
  ParticleFinder.h
  TransformMultiple.h
  TransformSingle.h
) # HEADERS    

set(SOURCES
  Particle.cpp
  ParticleFinder.cpp
  ParticleHeight.cpp
  TransformMultiple.cpp
  TransformSingle.cpp
) # SOURCES

add_executable(${NAME}
  ${HEADERS}
  ${SOURCES}
) # add_executable

target_compile_features(${NAME} PRIVATE cxx_right_angle_brackets cxx_lambdas)

target_link_libraries(${NAME} ${LIB_LIST})

install(TARGETS ${NAME} DESTINATION ${BIN_DIR})
