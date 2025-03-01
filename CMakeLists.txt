cmake_minimum_required(VERSION 3.11)

project(TransportCatalogue CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# исходные файлы (.cpp) в папке src
file(GLOB SOURCES "src/*.cpp")

if (CMAKE_SYSTEM_NAME MATCHES "^MINGW")
    set(SYSTEM_LIBS -lstdc++)
else()
    set(SYSTEM_LIBS)
endif()

# Создаем цель
add_executable(${PROJECT_NAME} ${SOURCES})