cmake_minimum_required(VERSION 3.5.0)
project(vk VERSION 0.1.0 LANGUAGES C CXX)

# Опция для выбора файла
option(USE_TASK_SCHEDULER "Use task_scheduler.cpp" OFF)

if(USE_TASK_SCHEDULER)
    set(SOURCE_FILES task_scheduler.cpp)
else()
    set(SOURCE_FILES main.cpp)
endif()

add_executable(vk ${SOURCE_FILES})