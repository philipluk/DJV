include(ExternalProject)

ExternalProject_Add(
    DJV
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/DJV
    DEPENDS ${DJV_THIRD_PARTY_DEPS}
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/../..
    CMAKE_ARGS
        -DCMAKE_MODULE_PATH=${CMAKE_MODULE_PATH}
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
        -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}
        -DCMAKE_POLICY_DEFAULT_CMP0091:STRING=NEW
        -DCMAKE_C_FLAGS=${CMAKE_C_FLAGS}
        -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
        -DBUILD_SHARED_LIBS=${BUILD_SHARED_LIBS})