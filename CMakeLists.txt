cmake_minimum_required (VERSION 2.8)
project (dfterm2)

SET(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})

FIND_PACKAGE(Lua REQUIRED)
INCLUDE_DIRECTORIES(${LUA_INCLUDE_PATH})

FIND_PACKAGE(Boost 1.41.0 REQUIRED COMPONENTS thread system)
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(.)
link_directories(${Boost_LIBRARY_DIRS})

FIND_PACKAGE(ICU REQUIRED)
FIND_PACKAGE(PCRE REQUIRED)

add_definitions(-DNO_DFHACK -D_LARGEFILE_SOURCE)

IF(UNIX)
  add_definitions(-DLINUX_BUILD)
ENDIF(UNIX)

FIND_PACKAGE (Threads REQUIRED)

SET(NO_CURSES 0)

SET(COMMON_SOURCE src/main.cc src/client.cc src/logger.cc src/slot.cc src/cp437_to_unicode.cc src/configuration_interface.cc src/configuration_db.cc src/sqlite3.c src/state.cc src/usergroup_serialize.cc src/id.cc src/hash.cc src/rng.cc src/minimal_http_server.cc src/server_to_server_configuration_pair.cc src/server_to_server_session.cc src/lua_configuration.cc src/sockets.cc src/telnet.cc src/nanoclock.cc src/cpp_regexes.cc src/types.cc src/socketevents.cc src/socketaddressrange.cc src/termemu.cc src/utf8.cc src/interface_ncurses.cc src/keypress.cc)

# Some parts of dfterm2 work very differently on different platforms and use different source files.
# Maybe we should add directories for platform-dependent files at some point.
IF (WIN32)
    add_executable(dfterm2 ${COMMON_SOURCE} src/slot_dfglue.cc src/pointerpath.cc)
    target_link_libraries(dfterm2 psapi)
ELSE (WIN32)
    add_executable(dfterm2 ${COMMON_SOURCE} src/slot_terminal.cc src/slot_linux_common.cc src/bsd_pty.cc src/pty.cc)
    IF (NOT CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
        target_link_libraries(dfterm2 dl)
    ENDIF (NOT CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
ENDIF (WIN32)

SET(COMMON_LIBS ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARIES} ${ICU_LIBRARIES} ${PCRE_LIBRARIES} ${LUA_LIBRARIES})
IF (WIN32)
    SET(COMMON_LIBS ${COMMON_LIBS} ws2_32)
ENDIF (WIN32)

TARGET_LINK_LIBRARIES (dfterm2 ${COMMON_LIBS})

add_executable(dfterm2_configure src/dfterm2_configure.cc src/configuration_db.cc src/sqlite3.c src/usergroup_serialize.cc src/logger.cc src/id.cc src/hash.cc src/rng.cc src/lua_configuration.cc src/utf8.cc src/types.cc src/socketaddressrange.cc src/sockets.cc src/cpp_regexes.cc)
IF(NOT WIN32)
    IF (NOT CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
        target_link_libraries(dfterm2_configure dl ${COMMON_LIBS})
    ELSE (NOT CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
        target_link_libraries(dfterm2_configure ${COMMON_LIBS})
    ENDIF(NOT CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
ELSE(NOT WIN32)
    target_link_libraries(dfterm2_configure ${COMMON_LIBS})
ENDIF(NOT WIN32)

add_executable(dfterm2_sha512 src/dfterm2_sha512.cc src/hash.cc)
IF(NOT WIN32)
    IF (NOT CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
        target_link_libraries(dfterm2_sha512 dl ${COMMON_LIBS})
    ENDIF (NOT CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
ELSE(NOT WIN32)
    target_link_libraries(dfterm2_sha512 ${COMMON_LIBS})
ENDIF(NOT WIN32)

FIND_PACKAGE(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})
target_link_libraries(dfterm2 ${OPENSSL_LIBRARIES})
target_link_libraries(dfterm2_configure ${OPENSSL_LIBRARIES})
target_link_libraries(dfterm2_sha512 ${OPENSSL_LIBRARIES})

IF (NOT NO_CURSES)
    FIND_PACKAGE(Ncursesw REQUIRED)
    INCLUDE_DIRECTORIES(${CURSES_INCLUDE_DIR})
    INCLUDE_DIRECTORIES("/usr/include/ncursesw")
    target_link_libraries(dfterm2 ${CURSES_NCURSESW_LIBRARY})
ENDIF(NOT NO_CURSES)

include_directories(.)
include_directories(${ICU_INCLUDE_PATH})
include_directories(${BOOST_ROOT})
include_directories(${CURSES_INCLUDE_DIR})
include_directories(${PCRE_INCLUDE_PATH})

# The injection glue is only for Win32
IF (WIN32)
    add_library(dfterm_injection_glue SHARED src/dfterm_injection_glue.cc)
    target_link_libraries(dfterm_injection_glue psapi)
ENDIF (WIN32)

IF (NO_CURSES)
    IF (MSVC)
        ADD_DEFINITIONS("/DNO_CURSES")
    ENDIF (MSVC)
    IF (CMAKE_COMPILER_IS_GNUCXX)
        ADD_DEFINITIONS("-DNO_CURSES")
    ENDIF ()
ENDIF (NO_CURSES)

IF (PROFILE)
    IF (CMAKE_COMPILER_IS_GNUCXX)
        ADD_DEFINITIONS("-pg")
    ENDIF (CMAKE_COMPILER_IS_GNUCXX)

    SET_TARGET_PROPERTIES(dfterm2_sha512 dfterm2 dfterm2_configure PROPERTIES LINK_FLAGS -pg)
ENDIF (PROFILE)

SET_TARGET_PROPERTIES(dfterm2_sha512 dfterm2 dfterm2_configure PROPERTIES COMPILE_DEFINITIONS _UNICODE)

install(TARGETS dfterm2 dfterm2_configure dfterm2_sha512 RUNTIME DESTINATION bin LIBRARY DESTINATION lib)

