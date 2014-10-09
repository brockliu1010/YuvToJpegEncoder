cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
project(YuvToJpegEncoder)

# collect source code
file(GLOB HEADERS_FILES include/*.hpp include/*.h)
file(GLOB SRC_FILES src/*.cpp src*.cc)

# find lib jpeg
find_package(JPEG REQUIRED)
include_directories(${JPEG_INCLUDE_DIR})
#link_directories(${JPEG_LIBRARIES})

# add other CXX_FLAGS
set(CMAKE_CXX_FLAGS -std=c++11)

# set exe name
set(EXE_NAME sample)

# target source code and library to exe
add_executable(${EXE_NAME} ${SRC_FILES} ${HEADERS_FILES})

# target neccessary library to exe
target_link_libraries(${EXE_NAME} ${OpenCV_LIBS} ${JPEG_LIBRARIES}) 