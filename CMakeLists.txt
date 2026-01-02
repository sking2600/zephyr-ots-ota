cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ots_ota_app)

target_sources(app PRIVATE src/main.c src/ots_ota.c)
