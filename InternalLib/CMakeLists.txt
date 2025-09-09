file(GLOB_RECURSE InternalLibSource CONFIGURE_DEPENDS *.cc *.hh)

add_compile_definitions(INTERNALLIB_EXPORTS)

add_library(InternalLib STATIC ${InternalLibSource})

target_include_directories(InternalLib PUBLIC Include)

# Eigen
target_include_directories(InternalLib PUBLIC ${EIGEN_INCLUDE_DIR})

# yalantinglibs
target_link_libraries(InternalLib PUBLIC yalantinglibs::yalantinglibs)

# 链接 spdlog
target_include_directories(InternalLib PRIVATE ${SPDLOG_INCLUDE_DIR})
target_link_directories(InternalLib PRIVATE ${SPDLOG_LIBRARY_DIR})
target_link_libraries(InternalLib PRIVATE  debug spdlogd optimized spdlog)