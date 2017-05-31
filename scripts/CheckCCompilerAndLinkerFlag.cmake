#.rst:
# CheckCCompilerAndLinkerFlag
# ------------------
#
# Check whether the C compiler supports a given flag pair: one to compiler,
# and the another one to linker.
#
# CHECK_C_COMPILER_AND_LINKER_FLAG(<compiler flag> <linker flag> <var>)
#
# ::
#
#   <compiler flag> - the compiler flag
#   <linker flag> - corresponding linker flag
#   <var>  - variable to store the result
#
# Example:
#   CHECK_C_COMPILER_AND_LINKER_FLAG("-fsanitize=thread -fPIE" "-pie" ST_SUP)
#
# This internally calls the check_c_source_compiles macro and sets
# CMAKE_REQUIRED_DEFINITIONS to compiler flag, and CMAKE_REQUIRED_LIBRARIES
# to linker flag. See help for CheckCSourceCompiles for a listing of
# variables that can otherwise modify the build. The result only tells that
# the compiler and linker do not give an error message when they encounter
# such flags. If the flag has any effect or even a specific one is beyond
# the scope of this module.

#=============================================================================
# Based on CheckCCompilerFlag.cmake by:
# Copyright 2006-2011 Kitware, Inc.
# Copyright 2006 Alexander Neundorf <neundorf@kde.org>
# Copyright 2011 Matthias Kretz <kretz@kde.org>
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distribute this file outside of CMake, substitute the full
#  License text for the above reference.)

include(CheckCSourceCompiles)
include(CMakeCheckCompilerFlagCommonPatterns OPTIONAL)

macro (CHECK_C_COMPILER_AND_LINKER_FLAG _CFLAG _LDFLAG _RESULT)
   set(SAFE_CMAKE_REQUIRED_DEFINITIONS "${CMAKE_REQUIRED_DEFINITIONS}")
   set(SAFE_CMAKE_REQUIRED_LIBRARIES "${CMAKE_REQUIRED_LIBRARIES}")
   set(CMAKE_REQUIRED_DEFINITIONS "${_CFLAG}")
   set(CMAKE_REQUIRED_LIBRARIES "${_LDFLAG}")

   # Normalize locale during test compilation.
   set(_CheckCCompilerAndLinkerFlag_LOCALE_VARS LC_ALL LC_MESSAGES LANG)
   foreach(v ${_CheckCCompilerAndLinkerFlag_LOCALE_VARS})
     set(_CheckCCompilerAndLinkerFlag_SAVED_${v} "$ENV{${v}}")
     set(ENV{${v}} C)
   endforeach()
   if(COMMAND CHECK_COMPILER_FLAG_COMMON_PATTERNS)
     CHECK_COMPILER_FLAG_COMMON_PATTERNS(_CheckCCompilerAndLinkerFlag_COMMON_PATTERNS)
   endif()
   CHECK_C_SOURCE_COMPILES("int main(void) { return 0; }" ${_RESULT}
     # Some compilers do not fail with a bad flag
     FAIL_REGEX "command line option .* is valid for .* but not for C" # GNU
     ${_CheckCCompilerFlag_COMMON_PATTERNS}
     )
   foreach(v ${_CheckCCompilerAndLinkerFlag_LOCALE_VARS})
     set(ENV{${v}} ${_CheckCCompilerAndLinkerFlag_SAVED_${v}})
     unset(_CheckCCompilerAndLinkerFlag_SAVED_${v})
   endforeach()
   unset(_CheckCCompilerAndLinkerFlag_LOCALE_VARS)
   unset(_CheckCCompilerAndLinkerFlag_COMMON_PATTERNS)

   set (CMAKE_REQUIRED_DEFINITIONS "${SAFE_CMAKE_REQUIRED_DEFINITIONS}")
   set (CMAKE_REQUIRED_LIBRARIES "${SAFE_CMAKE_REQUIRED_LIBRARIES}")
endmacro ()
