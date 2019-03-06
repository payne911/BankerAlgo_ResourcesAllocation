cmake_minimum_required(VERSION 3.13)
project(BankerAlgo_RessourcesAllocation C)

set(CMAKE_C_STANDARD 99)

# from: https://stackoverflow.com/questions/26918459/using-local-makefile-for-clion-instead-of-cmake
#add_custom_target(BankerAlgo_RessourcesAllocation COMMAND make -C ${BankerAlgo_RessourcesAllocation_SOURCE_DIR}
#        CLION_EXE_DIR=${PROJECT_BINARY_DIR})

add_executable(BankerAlgo_RessourcesAllocation common/common.c server/main.c client/main.c)