cmake_minimum_required(VERSION 3.20)
set(P "/home/torsten/~/src/puzzle")
set(imgui "${P}/imgui")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Og -DDEBUG -D_DEBUG -g")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -s -DNDEBUG")

#"debug":   ['', ''],
#"release": ['', '-s -zrelro -znow'],

set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -DANDROID -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DANDROID -I${P} -I${imgui} -I${imgui}/backends -std=c++17 -fno-rtti -fexceptions \
-Wall -Wextra \
-Wno-sign-compare \
-Wno-deprecated-declarations \
-Wno-parentheses \
-Wno-misleading-indentation \
-Wno-variadic-macros \
-Wno-unused-parameter \
-Wno-unknown-pragmas \
-Wno-implicit-fallthrough \
-Wno-missing-field-initializers \
-fstrict-enums -Wno-reorder -Wno-reorder-ctor \
-include ${P}/pch.h -Winvalid-pch")
project(jigsaw)

find_package(oboe REQUIRED CONFIG)
file(GLOB COMMON LIST_DIRECTORIES false RELATIVE ${CMAKE_SOURCE_DIR} CONFIGURE_DEPENDS
		"${P}/*.cc"
		"${P}/Persistence/*.cc"
		"${P}/Utility/*.cc"
		"${P}/build_debug/*.cc")
file(GLOB LOCAL LIST_DIRECTORIES false RELATIVE ${CMAKE_SOURCE_DIR} CONFIGURE_DEPENDS "*.cc")
add_library(jigsaw SHARED
		${COMMON}
		${LOCAL}
		${imgui}/imgui.cpp
		${imgui}/imgui_demo.cpp
		${imgui}/imgui_draw.cpp
		${imgui}/imgui_tables.cpp
		${imgui}/imgui_widgets.cpp
		${imgui}/backends/imgui_impl_android.cpp
		${imgui}/backends/imgui_impl_opengl3.cpp
		${imgui}/misc/cpp/imgui_stdlib.cpp)
target_link_libraries(jigsaw GLESv3 EGL m oboe::oboe android log)
