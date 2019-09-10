cmake_minimum_required(VERSION 3.7)


PROJECT("katscha_plugin"
        VERSION ${MUHKUH_VERSION_ALL})

# Select the LUA version.
INCLUDE(${CMAKE_HOME_DIRECTORY}/cmake/muhkuh_lua.cmake)

# Enable tests (see here for details: http://www.vtk.org/Wiki/CMake/Testing_With_CTest)
ENABLE_TESTING()

FIND_PACKAGE(SWIG REQUIRED)
INCLUDE(${SWIG_USE_FILE})

# Add a custom target for the swig runtime for LUA.
INCLUDE(${CMAKE_HOME_DIRECTORY}/cmake/muhkuh_swig_lua_runtime.cmake)

FIND_PACKAGE(libusb REQUIRED)


##----------------------------------------------------------------------------
##
## Get the netX USB firmware binaries (in form of C data arrays) and headers.
##
#
#SET(NETX_USB_FIRMWARE_SOURCES ${CMAKE_HOME_DIRECTORY}/targets/plugins/romloader/usb/netx/usbmon_netx10.c
#                              ${CMAKE_HOME_DIRECTORY}/targets/plugins/romloader/usb/netx/usbmon_netx56.c
#                              ${CMAKE_HOME_DIRECTORY}/targets/plugins/romloader/usb/netx/usbmon_netx500.c)
#
#SET(NETX_USB_FIRMWARE_HEADERS ${CMAKE_HOME_DIRECTORY}/targets/plugins/romloader/usb/netx/usbmon_netx10.h
#                              ${CMAKE_HOME_DIRECTORY}/targets/plugins/romloader/usb/netx/usbmon_netx56.h
#                              ${CMAKE_HOME_DIRECTORY}/targets/plugins/romloader/usb/netx/usbmon_netx500.h)


#----------------------------------------------------------------------------
#
# Set the platform specific parts.
#


SET(katscha_SOURCES
	katscha.cpp
)

SET(katscha_HEADERS
	katscha.h
	muhkuh_static_assert.h
)


ADD_LIBRARY(TARGET_katscha_lib STATIC ${katscha_SOURCES})
#ADD_DEPENDENCIES(TARGET_katscha_lib TARGET_NETX_FIRMWARE)
TARGET_LINK_LIBRARIES(TARGET_katscha_lib libusb::TARGET_libusb)
TARGET_INCLUDE_DIRECTORIES(TARGET_katscha_lib
                           PRIVATE ${LUA_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${SWIG_RUNTIME_OUTPUT_PATH})
# This static library will be linked with a shared object later. Add "-fPIC" on GCC systems or X64 builds break.
IF((CMAKE_CXX_COMPILER_ID STREQUAL "GNU") AND (CMAKE_SYSTEM_NAME STREQUAL "Linux"))
	SET_TARGET_PROPERTIES(TARGET_katscha_lib PROPERTIES COMPILE_FLAGS -fPIC)
ENDIF((CMAKE_CXX_COMPILER_ID STREQUAL "GNU") AND (CMAKE_SYSTEM_NAME STREQUAL "Linux"))


#----------------------------------------------------------------------------
#
# Build the lua plugin.
#
SET_SOURCE_FILES_PROPERTIES(katscha.i PROPERTIES CPLUSPLUS ON)
SET_PROPERTY(SOURCE katscha.i PROPERTY SWIG_FLAGS -I${CMAKE_HOME_DIRECTORY} -I${CMAKE_BINARY_DIR}/version_information)
IF(CMAKE_VERSION VERSION_LESS 3.8.0)
	SWIG_ADD_MODULE(TARGET_plugins_katscha_LUA lua katscha.i)
ELSE(CMAKE_VERSION VERSION_LESS 3.8.0)
	SWIG_ADD_LIBRARY(TARGET_plugins_katscha_LUA
	                 TYPE MODULE
	                 LANGUAGE LUA
	                 SOURCES katscha.i)
ENDIF(CMAKE_VERSION VERSION_LESS 3.8.0)
SWIG_LINK_LIBRARIES(TARGET_plugins_katscha_LUA TARGET_katscha_lib ${LUA_LIBRARIES})
TARGET_INCLUDE_DIRECTORIES(TARGET_plugins_katscha_LUA
                           PRIVATE ${LUA_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

# Set the name of the output file to "katscha".
SET_TARGET_PROPERTIES(TARGET_plugins_katscha_LUA PROPERTIES PREFIX "" OUTPUT_NAME "katscha")

IF((CMAKE_SYSTEM_NAME STREQUAL "Windows") AND (${CMAKE_COMPILER_IS_GNUCC}))
	# On minGW link all compiler libraries static.
	SET_PROPERTY(TARGET TARGET_plugins_katscha_LUA PROPERTY LINK_FLAGS "-static -static-libgcc -static-libstdc++")
ENDIF((CMAKE_SYSTEM_NAME STREQUAL "Windows") AND (${CMAKE_COMPILER_IS_GNUCC}))

# Install the LUA module.
INSTALL(TARGETS TARGET_plugins_katscha_LUA DESTINATION ${INSTALL_DIR_LUA_MODULES})

# Add tests for this module.
IF((CMAKE_SYSTEM_NAME STREQUAL "Windows") AND (${CMAKE_COMPILER_IS_GNUCC}))
	# Here are the MinGW specific tests.
	ADD_TEST(NAME romloader_usb_MinGW_DLL_dependencies
	         COMMAND "${PYTHON_EXECUTABLE}" ${TEST_SCRIPT_DIR}/mingw_dll_dependencies.py -u lua5.1 -u lua5.2 -u lua5.3 -u lua5.4 $<TARGET_FILE:TARGET_plugins_katscha_LUA>)
ENDIF((CMAKE_SYSTEM_NAME STREQUAL "Windows") AND (${CMAKE_COMPILER_IS_GNUCC}))
