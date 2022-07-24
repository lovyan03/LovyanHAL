# CMakeLists for ESP-IDF

set(COMPONENT_ADD_INCLUDEDIRS
    src
    )
file(GLOB SRCS
     src/lhal/platforms/esp32/*.cpp
     )

register_component()
