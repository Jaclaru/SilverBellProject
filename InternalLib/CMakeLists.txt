file(GLOB_RECURSE InternalLibSource CONFIGURE_DEPENDS *.cc *.hh)

add_compile_definitions(INTERNALLIB_EXPORTS)

add_library(InternalLib SHARED ${InternalLibSource})

target_include_directories(InternalLib PUBLIC Include)

# Eigen
target_include_directories(InternalLib PUBLIC ${EIGEN_INCLUDE_DIR})

# yalantinglibs
target_link_libraries(InternalLib PUBLIC yalantinglibs::yalantinglibs)

# 链接 spdlog
target_link_libraries(InternalLib PUBLIC spdlog::spdlog)

# stb
target_include_directories(InternalLib PRIVATE ${STB_INCLUDE_DIR}) # 不需要暴露给外部

# tinyobjloader
target_include_directories(InternalLib PRIVATE ${TINYOBJLOADER_INCLUDE_DIR})

# xxHash
target_include_directories(InternalLib PRIVATE ${XXHASH_INCLUDE_DIR})