cmake_minimum_required(VERSION 3.6)
project (SondeDecoder)

set(CMAKE_CXX_STANDARD 98)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -Werror")

set(SOURCE_FILES M10Decoder.cpp M10Decoder.h 
    ShibauraSensor.cpp ShibauraSensor.h
    GPS.cpp GPS.h
    Utils.cpp Utils.h
    SondeDecoderLinux.cpp)

add_executable(sondeDecoder ${SOURCE_FILES})

