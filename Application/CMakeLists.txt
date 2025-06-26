file(GLOB_RECURSE ApplicationSource CONFIGURE_DEPENDS *.cpp *.h)

add_executable(Application ${ApplicationSource})
target_include_directories(Application PUBLIC Public)
target_include_directories(Application PRIVATE Private)


find_package(Qt6 COMPONENTS Core Gui Widgets REQUIRED)

set(CMAKE_AUTOMOC ON)

# 链接 Qt5 Widgets
target_link_libraries(Application PRIVATE Qt6::Widgets)