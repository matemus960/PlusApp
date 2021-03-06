INCLUDE(InstallRequiredSystemLibraries)

IF("${PLUSAPP_PACKAGE_EDITION}" STREQUAL "")
  SET(PLUSAPP_PACKAGE_EDITION_PLATFORM "${PLUSLIB_PLATFORM}")
ELSE()
  SET(PLUSAPP_PACKAGE_EDITION_PLATFORM "${PLUSAPP_PACKAGE_EDITION}-${PLUSLIB_PLATFORM}")
ENDIF()

SET(CPACK_GENERATOR "NSIS;ZIP")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Plus(Public software Library for UltraSound) for ${PLUSAPP_PACKAGE_EDITION_PLATFORM}")
SET(CPACK_PACKAGE_VENDOR "PerkLab, Queen's University")
# SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/ReadMe.txt")
SET(CPACK_RESOURCE_FILE_LICENSE "${PLUSLIB_SOURCE_DIR}/src/License.txt")
SET(CPACK_PACKAGE_VERSION_MAJOR ${PLUSAPP_VERSION_MAJOR})
SET(CPACK_PACKAGE_VERSION_MINOR ${PLUSAPP_VERSION_MINOR})
SET(CPACK_PACKAGE_VERSION_PATCH ${PLUSAPP_VERSION_PATCH})
SET(CPACK_NSIS_PACKAGE_NAME "Plus Applications ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}.${PLUSAPP_COMMIT_DATE} (${PLUSAPP_PACKAGE_EDITION_PLATFORM})" )
SET(CPACK_PACKAGE_FILE_NAME "PlusApp-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}.${PLUSAPP_COMMIT_DATE_NO_DASHES}-${PLUSAPP_PACKAGE_EDITION_PLATFORM}" )
SET(CPACK_PACKAGE_INSTALL_DIRECTORY "${CPACK_PACKAGE_FILE_NAME}")
SET(CPACK_INSTALL_CMAKE_PROJECTS "${PlusApp_BINARY_DIR};PlusApp;ALL;/")

# Install into c:\Users\<username>\PlusApp_...
SET(CPACK_NSIS_INSTALL_ROOT "$PROFILE")

# Do not ask for admin access rights(no UAC dialog), to allow installation without admin access
SET(CPACK_NSIS_DEFINES ${CPACK_NSIS_DEFINES} "RequestExecutionLevel user")

SET(CPACK_PACKAGE_EXECUTABLES 
  "PlusServerLauncher" "Plus Server Launcher"
  "fCal" "Free-hand calibration(fCal)"
  )

SET(CPACK_NSIS_EXTRA_INSTALL_COMMANDS)
SET(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS)
IF(WIN32)
  # Windows users may not be familiar how to open a command prompt, so create a shortcut for that
  LIST(APPEND CPACK_NSIS_EXTRA_INSTALL_COMMANDS "
    CreateShortCut \\\"$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\Plus command prompt.lnk\\\" \\\"$INSTDIR\\\\bin\\\\StartPlusCommandPrompt.bat\\\" \\\"$INSTDIR\\\\bin\\\\StartPlusCommandPrompt.bat\\\" \\\"$INSTDIR\\\\bin\\\\StartPlusCommandPrompt.ico\\\"
    ")
  LIST(APPEND CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "
    !insertmacro MUI_STARTMENU_GETFOLDER Application $MUI_TEMP
    Delete \\\"$SMPROGRAMS\\\\$MUI_TEMP\\\\Plus command prompt.lnk\\\"
    ")
  IF(BUILD_DOCUMENTATION)
    # Create a shortcut to documentation as well
    LIST(APPEND CPACK_NSIS_EXTRA_INSTALL_COMMANDS "
      CreateShortCut \\\"$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\Plus user manual.lnk\\\" \\\"$INSTDIR\\\\doc\\\\PlusApp-UserManual.chm\\\"
      ")
    LIST(APPEND CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "
      !insertmacro MUI_STARTMENU_GETFOLDER Application $MUI_TEMP
      Delete \\\"$SMPROGRAMS\\\\$MUI_TEMP\\\\Plus user manual.lnk\\\"
      ")
  ENDIF()
ENDIF()

IF(EXISTS "${PLUSLIB_DIR}/CMakeCache.txt")
  LIST(APPEND CPACK_INSTALL_CMAKE_PROJECTS "${PLUSLIB_DIR};PlusLib;RuntimeExecutables;/")
  LIST(APPEND CPACK_INSTALL_CMAKE_PROJECTS "${PLUSLIB_DIR};PlusLib;RuntimeLibraries;/")
  LIST(APPEND CPACK_INSTALL_CMAKE_PROJECTS "${PLUSLIB_DIR};PlusLib;Scripts;/")
ELSE()
  MESSAGE(WARNING "Unable to set PLUSLIB_DIR for package generation!")
ENDIF()
  
IF(EXISTS "${VTK_DIR}/CMakeCache.txt")
  LIST(APPEND CPACK_INSTALL_CMAKE_PROJECTS "${VTK_DIR};VTK;RuntimeLibraries;/")
ELSE()
  MESSAGE(WARNING "Unable to set VTK_DIR for package generation!")
ENDIF()

IF(EXISTS "${ITK_DIR}/CMakeCache.txt")
  SET(ITK_USE_REVIEW OFF)
  LIST(APPEND CPACK_INSTALL_CMAKE_PROJECTS "${ITK_DIR};ITK;RuntimeLibraries;/") 
ELSE()
  MESSAGE(WARNING "Unable to set ITK_DIR for package generation!")
ENDIF()

IF(PLUS_USE_OpenIGTLink)
  IF(EXISTS "${OpenIGTLink_DIR}/CMakeCache.txt")
    LIST(APPEND CPACK_INSTALL_CMAKE_PROJECTS "${OpenIGTLink_DIR};OpenIGTLink;RuntimeLibraries;/")
  ELSE()
    MESSAGE(WARNING "Unable to set OpenIGTLink_DIR for package generation!")
  ENDIF()
  
  IF(EXISTS "${OpenIGTLinkIO_DIR}/CMakeCache.txt")
    LIST(APPEND CPACK_INSTALL_CMAKE_PROJECTS "${OpenIGTLinkIO_DIR};OpenIGTLinkIO;RuntimeLibraries;/")
  ELSE()
    MESSAGE(WARNING "Unable to set OpenIGTLinkIO_DIR for package generation!")
  ENDIF()
ENDIF()

IF(PLUS_USE_OvrvisionPro)
  IF(EXISTS "${OvrvisionPro_DIR}/CMakeCache.txt")
    LIST(APPEND CPACK_INSTALL_CMAKE_PROJECTS "${OvrvisionPro_DIR};OvrvisionPro;RuntimeLibraries;/")
  ELSE()
    MESSAGE(WARNING "Unable to set OvrvisionPro_DIR for package generation!")
  ENDIF()

  IF(EXISTS "${OpenCV_DIR}/CMakeCache.txt")
    LIST(APPEND CPACK_INSTALL_CMAKE_PROJECTS "${OpenCV_DIR};OpenCV;libs;/")
  ELSE()
    MESSAGE(WARNING "Unable to set OpenCV_DIR for package generation!")
  ENDIF()
ENDIF()

#-----------------------------------------------------------------------------
# Installation vars.
# PLUSAPP_INSTALL_BIN_DIR          - binary dir(executables)
# PLUSAPP_INSTALL_LIB_DIR          - library dir(libs)
# PLUSAPP_INSTALL_DATA_DIR         - share dir(say, examples, data, etc)
# PLUSAPP_INSTALL_CONFIG_DIR       - config dir(configuration files)
# PLUSAPP_INSTALL_SCRIPTS_DIR      - scripts dir
# PLUSAPP_INSTALL_INCLUDE_DIR      - include dir(headers)
# PLUSAPP_INSTALL_PACKAGE_DIR      - package/export configuration files
# PLUSAPP_INSTALL_NO_DEVELOPMENT   - do not install development files
# PLUSAPP_INSTALL_NO_RUNTIME       - do not install runtime files
# PLUSAPP_INSTALL_NO_DOCUMENTATION - do not install documentation files
# Applications
# RuntimeLibraries
# Development

IF(NOT PLUSAPP_INSTALL_BIN_DIR)
  SET(PLUSAPP_INSTALL_BIN_DIR "bin")
ENDIF()

IF(NOT PLUSAPP_INSTALL_LIB_DIR)
  SET(PLUSAPP_INSTALL_LIB_DIR "lib")
ENDIF()

IF(NOT PLUSAPP_INSTALL_DATA_DIR)
  SET(PLUSAPP_INSTALL_DATA_DIR "data")
ENDIF()

IF(NOT PLUSAPP_INSTALL_CONFIG_DIR)
  SET(PLUSAPP_INSTALL_CONFIG_DIR "config")
ENDIF()

IF(NOT PLUSAPP_INSTALL_SCRIPTS_DIR)
  SET(PLUSAPP_INSTALL_SCRIPTS_DIR "scripts")
ENDIF()

IF(NOT PLUSAPP_INSTALL_INCLUDE_DIR)
  SET(PLUSAPP_INSTALL_INCLUDE_DIR "include")
ENDIF()

IF(NOT PLUSAPP_INSTALL_DOCUMENTATION_DIR)
  SET(PLUSAPP_INSTALL_DOCUMENTATION_DIR "doc")
ENDIF()

IF(NOT PLUSAPP_INSTALL_NO_DOCUMENTATION)
  SET(PLUSAPP_INSTALL_NO_DOCUMENTATION 0)
ENDIF()

INCLUDE(CPack)