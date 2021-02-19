include(ExternalProject)

set(OpenSubdiv_ARGS
    -DCMAKE_MODULE_PATH=${CMAKE_MODULE_PATH}
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
    -DCMAKE_INSTALL_LIBDIR=${CMAKE_INSTALL_PREFIX}/lib
    -DCMAKE_PREFIX_PATH=${CMAKE_INSTALL_PREFIX}
    -DCMAKE_POLICY_DEFAULT_CMP0091:STRING=NEW
    -DCMAKE_FIND_FRAMEWORK=${CMAKE_FIND_FRAMEWORK}
    -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
    -DCMAKE_C_FLAGS=${CMAKE_C_FLAGS}
    -DCMAKE_MSVC_RUNTIME_LIBRARY=${CMAKE_MSVC_RUNTIME_LIBRARY}
    -DBUILD_SHARED_LIBS=${BUILD_SHARED_LIBS})
if(CMAKE_CXX_STANDARD)
    set(OpenSubdiv_ARGS ${OpenSubdiv_ARGS} -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD})
endif()

ExternalProject_Add(
    OpenSubdiv
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/OpenSubdiv
    DEPENDS ${OpenSubdiv_DEPS}
    GIT_REPOSITORY "https://github.com/PixarAnimationStudios/OpenSubdiv.git"
    GIT_TAG v3_4_4
    CMAKE_ARGS ${OpenSubdiv_ARGS})
