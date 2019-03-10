cmake_minimum_required(VERSION 3.13)
project(BankerAlgo_RessourcesAllocation C)

set(CMAKE_C_STANDARD 11)

add_executable(BankerAlgo_RessourcesAllocation
        common/common.c
        server/main.c server/server_thread.c
        client/main.c client/client_thread.c)