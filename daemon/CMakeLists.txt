cmake_minimum_required(VERSION 3.22)
project(redmagic_trigger_bridge C)

add_executable(redmagic-trigger-bridge src/main.c)
target_compile_features(redmagic-trigger-bridge PRIVATE c_std_11)
target_compile_options(redmagic-trigger-bridge PRIVATE
    -Wall -Wextra -Werror -Wformat=2 -Wshadow -Wconversion -O2)
target_link_options(redmagic-trigger-bridge PRIVATE -static)
