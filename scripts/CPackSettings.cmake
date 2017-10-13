
if(${CMAKE_SIZEOF_VOID_P} EQUAL 8)
    # XXX: not a huge issue, but this seems to have no effect
    #      at least on my system, it still creates a 32-bit installer
    set(NSIS "NSIS64")
else()
    set(NSIS "NSIS")
endif()

if(WIN32)
    set(CPACK_GENERATOR "${NSIS};7Z")
    set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_BINARY_DIR}/win32docs/COPYING")
else()
    set(CPACK_GENERATOR "TGZ")
    set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/COPYING")
endif()

set(CPACK_PACKAGE_NAME "Taisei")
set(CPACK_PACKAGE_VENDOR "Taisei Project")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A free and open-source Tōhō Project fangame")
set(CPACK_PACKAGE_VERSION_MAJOR "${TAISEI_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${TAISEI_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${TAISEI_VERSION_PATCH}")
set(CPACK_PACKAGE_VERSION "${TAISEI_VERSION}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "Taisei Project")
set(CPACK_PACKAGE_ICON "${PROJECT_SOURCE_DIR}/src/taisei.ico")
set(CPACK_MONOLITHIC_INSTALL TRUE)
set(CPACK_PACKAGE_EXECUTABLES "taisei" "Taisei")

set(CPACK_NSIS_EXECUTABLES_DIRECTORY ".")
set(CPACK_NSIS_INSTALLED_ICON_NAME "taisei.exe")
set(CPACK_NSIS_MUI_FINISHPAGE_RUN "taisei.exe")
set(CPACK_NSIS_HELP_LINK "http://taisei-project.org")

include(CPack)
