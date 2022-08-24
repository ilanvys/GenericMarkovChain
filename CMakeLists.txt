cmake_minimum_required(VERSION 3.16)
project(ex3b_ilan_vys C)

set(CMAKE_C_STANDARD 99)

add_executable(ex3b_ilan_vys linked_list.c
        tweets_generator.c
#        snakes_and_ladders.c
        markov_chain.h
        markov_chain.c)
