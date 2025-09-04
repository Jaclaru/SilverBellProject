file(GLOB_RECURSE ApplicationSource CONFIGURE_DEPENDS *.cc *.hh)

add_executable(Application ${ApplicationSource})
target_include_directories(Application PUBLIC Include)

target_link_libraries(Application PRIVATE Renderer)

# 链接 Qt6 Widgets
target_link_libraries(Application PRIVATE Qt6::Widgets)

# 链接 spdlog
target_include_directories(Application PRIVATE ${SPDLOG_INCLUDE_DIR})
target_link_directories(Application PRIVATE ${SPDLOG_LIBRARY_DIR})
target_link_libraries(Application PRIVATE  debug spdlogd optimized spdlog)