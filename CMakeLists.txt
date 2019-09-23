CMAKE_MINIMUM_REQUIRED(VERSION 3.7)

#-----------------------------------------------------------------------------
#
# The project options.
#
OPTION(BUILDCFG_ONLY_JONCHKI_CFG "Only build the jonchki configuration files. The default is OFF."
       "OFF")

MESSAGE("BUILDCFG_ONLY_JONCHKI_CFG: ${BUILDCFG_ONLY_JONCHKI_CFG}")


# Generate a version number from a file or VCS.
INCLUDE(cmake/muhkuh_version.cmake)


PROJECT("muhkuh"
        VERSION ${MUHKUH_VERSION_ALL})

# Get the VCS version for the jonchki configuration.
INCLUDE(cmake/version.cmake)

# Detect the platform.
INCLUDE(cmake/platform.cmake)

# Set some installation folders.
INCLUDE(cmake/muhkuh_installation_folders.cmake)

# Python is used for the firmware build script and some test scripts.
FIND_PACKAGE(PythonInterp 2.7 REQUIRED)

# Add a custom target for the netX firmware.
ADD_CUSTOM_TARGET(TARGET_NETX_FIRMWARE ALL
                  COMMAND "${PYTHON_EXECUTABLE}" mbs/mbs
                  WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}
                  COMMENT "Build the netX firmware."
)


# Filter the jonchki configurations.
CONFIGURE_FILE(plugin/installer/lua${BUILDCFG_LUA_VERSION}/katscha.xml
               ${CMAKE_CURRENT_BINARY_DIR}/lua${BUILDCFG_LUA_VERSION}-katscha-${PROJECT_VERSION}.xml
               @ONLY)


IF( NOT BUILDCFG_ONLY_JONCHKI_CFG STREQUAL "ON" )
	SET(libusb_DIR "${CMAKE_CURRENT_BINARY_DIR}/../external/libusb/install/cmake")

	ADD_SUBDIRECTORY(plugin)
ENDIF( NOT BUILDCFG_ONLY_JONCHKI_CFG STREQUAL "ON" )

#-----------------------------------------------------------------------------
