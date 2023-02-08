cmake_minimum_required(VERSION 3.8...3.20)

project(ParticleHeight LANGUAGES CXX)

# set the c++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# This variable is used to locate the bin directory where the
# application will be installed
set(CMAKE_INSTALL_PREFIX
    ${PROJECT_SOURCE_DIR}/..
    CACHE PATH "Project bin directory" FORCE)

message("CMAKE_INSTALL_PREFIX = ${CMAKE_INSTALL_PREFIX}")

# find opencv
# can use PATHS option on Windows to specify the opencv build directory
find_package(OpenCV REQUIRED PATHS "C:/Program Files/opencv 4.5.2/build_opencv")

# nlopt should be installed at these paths by default, or change these to where nlopt is actually installed
set(NLOPT_INCLUDE_DIRS "C:/Program Files (x86)/nlopt/include")
set(NLOPT_LIBS "C:/Program Files (x86)/nlopt/lib/nlopt.lib")

# set overall include directories
set(INC_DIR ${OpenCV_INCLUDE_DIRS} ${NLOPT_INCLUDE_DIRS})
set(BIN_DIR ${CMAKE_INSTALL_PREFIX}/bin)

# add current dir to include search path
include_directories(${PROJECT_SOURCE_DIR} ${INC_DIR})

add_subdirectory(image)
add_subdirectory(ray)
add_subdirectory(util)

# library list with opencv and nlopt
set(LIB_LIST image ray util ${OpenCV_LIBS} ${NLOPT_LIBS})

# build the executable in particle
add_subdirectory(particle)

