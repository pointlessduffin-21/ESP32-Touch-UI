# touch_test/src/CMakeLists.txt
# References display + touch drivers from the parent project's src/ directory.

set(PARENT_SRC "${CMAKE_SOURCE_DIR}/../src")

FILE(GLOB lcd_sources   "${PARENT_SRC}/lcd/*.c")
FILE(GLOB touch_sources "${PARENT_SRC}/touch/*.c")

idf_component_register(
    SRCS
        "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
        "${PARENT_SRC}/display.cpp"
        "${PARENT_SRC}/lvgl_port_v9.c"
        ${lcd_sources}
        ${touch_sources}
    INCLUDE_DIRS
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${PARENT_SRC}"
)
