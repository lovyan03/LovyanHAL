cmake_minimum_required (VERSION 3.8)
project(LHAL_Sample)

add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

# ビルド対象にするファイルを指定する;
# LovyanHALのあるパスと位置関係を変えた場合は相対パス記述を環境に合わせて調整すること;
file(GLOB Target_Files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS 
    *.cpp
    ../../../LovyanHAL/src/lhal/*.cpp
    ../../../LovyanHAL/src/lhal/platforms_pc/*.cpp
    ../../../LovyanHAL/src/lhal/platforms_pc/windows/*.cpp
    )
add_executable (LHAL_Sample ${Target_Files} "user_code.cpp")

# includeするファイルのパスを指定する;
# LovyanHALのあるパスと位置関係を変えた場合は相対パス記述を環境に合わせて調整すること;
target_include_directories(LHAL_Sample PUBLIC "../../../LovyanHAL/src/")

target_compile_features(LHAL_Sample PUBLIC cxx_std_17)
