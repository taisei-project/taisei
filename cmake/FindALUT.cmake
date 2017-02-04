# Locate ALUT
# This module defines XXX_FOUND, XXX_INCLUDE_DIRS and XXX_LIBRARIES standard variables
#
# $ALUTDIR is an environment variable that would
# correspond to the ./configure --prefix=$ALUTDIR
# used in building ALUT.
#
# Created by Sukender (Benoit Neil). Based on FindOpenAL.cmake module. (taken from osgaudio)

IF(ALUT_USE_AL_SUBDIR)
		SET(ALUT_HEADER_NAMES "AL/alut.h")
		SET(ALUT_HEADER_SUFFIXES include)
ELSE()
		SET(ALUT_HEADER_NAMES "alut.h")
		SET(ALUT_HEADER_SUFFIXES include/AL include/OpenAL include)
ENDIF()

SET(ALUT_SEARCH_PATHS
		~/Library/Frameworks
		/Library/Frameworks
		/usr/local
		/usr
		/sw # Fink
		/opt/local # DarwinPorts
		/opt/csw # Blastwave
		/opt
)

FIND_PATH(ALUT_INCLUDE_DIR NAMES ${ALUT_HEADER_NAMES}
		HINTS
		$ENV{ALUTDIR}
		$ENV{ALUT_PATH}
		PATH_SUFFIXES ${ALUT_HEADER_SUFFIXES}
		PATHS ${ALUT_SEARCH_PATHS}
)

FIND_LIBRARY(ALUT_LIBRARY 
		NAMES alut libalut
		HINTS
		$ENV{ALUTDIR}
		$ENV{ALUT_PATH}
		PATH_SUFFIXES lib64 lib libs64 libs libs/Win32 libs/Win64 Release
		PATHS ${ALUT_SEARCH_PATHS}
)

# First search for d-suffixed libs
FIND_LIBRARY(ALUT_LIBRARY_DEBUG 
		NAMES alutd libalutd
		HINTS
		$ENV{ALUTDIR}
		$ENV{ALUT_PATH}
		PATH_SUFFIXES lib64 lib libs64 libs libs/Win32 libs/Win64 Debug
		PATHS ${ALUT_SEARCH_PATHS}
)

IF(NOT ALUT_LIBRARY_DEBUG)
		# Then search for non suffixed libs if necessary, but only in debug dirs
		FIND_LIBRARY(ALUT_LIBRARY_DEBUG 
				NAMES alut libalut
				HINTS
				$ENV{ALUTDIR}
				$ENV{ALUT_PATH}
				PATH_SUFFIXES Debug
				PATHS ${ALUT_SEARCH_PATHS}
		)
ENDIF()

IF(ALUT_LIBRARY)
		IF(ALUT_LIBRARY_DEBUG)
				SET(ALUT_LIBRARIES optimized "${ALUT_LIBRARY}" debug "${ALUT_LIBRARY_DEBUG}")
		ELSE()
				SET(ALUT_LIBRARIES "${ALUT_LIBRARY}")           # Could add "general" keyword, but it is optional
		ENDIF()
ENDIF()

# handle the QUIETLY and REQUIRED arguments and set XXX_FOUND to TRUE if all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(ALUT DEFAULT_MSG ALUT_LIBRARIES ALUT_INCLUDE_DIR)