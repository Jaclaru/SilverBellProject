file(GLOB_RECURSE InternalLibSource CONFIGURE_DEPENDS *.cpp *.hh)

add_compile_definitions(INTERNALLIB_EXPORTS)

add_library(InternalLib STATIC ${InternalLibSource})

target_include_directories(InternalLib PUBLIC Include)