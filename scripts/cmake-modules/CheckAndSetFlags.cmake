#.rst:
# CheckAndSetFlags
# ------------------
#
# Check whether the C compiler supports a given flag pair: one to compiler,
# and the another one to linker.
#
# CHECK_AND_SET_FLAGS
# (
#    <test name>
#    <compiler flag> <destination for compiler flag>
#    [<linker flag> [<destination for linker flag>]]
# )
#
# ::
#
#   <test name> - for diagnostic purposes, will be shown in CMake log
#   <compiler flag> - compiler flag
#   <destination for compiler flag> - if test succeeds, this flag will go here
#   <linker flag> - corresponding linker flag, if needed
#   <destination for linker flag> - if specified and test succeeds, this flag
#                                   will go here
#
# Example:
#   CHECK_AND_SET_FLAGS(NDR_SUP "-Wnull-dereference" DBG_CFLAGS)
#     - adds "-Wnull-dereference" to DBG_CFLAGS if build of test source
#       succeeds with this flags passed compile stage call of cc.
#
#   CHECK_AND_SET_FLAGS(ST_SUP "-fsanitize=thread -fPIE" DBG_CF "-pie" DBG_LDF)
#     - adds "-fsanitize=thread -fPIE" to DBG_CF and "-pie" to DBG_LDF
#       if build of test source succeeds with these flags passed to cc
#       for compile and link stage, respectively.
#
#   CHECK_AND_SET_FLAGS(ST_SUP "-fsanitize=address" DBG_F "-fsanitize=address")
#     - adds "-fsanitize=address" to DBG_F if build of test source succeeds
#       with this flag passed to cc for both compile and link stages.
#
# This does the following things:
#   - calls CHECK_C_COMPILER_AND_LINKER_FLAG macro to check if compiler
#     (and, possibly, linker) supports specified flags. If linker flag
#     is omitted, no additional flag is passed to linker; if you need to pass
#     the same flag both to compiler and linker, simply repeat it
#     as fourth parameter of this macro.
#   - if check succeeds, compiler flag goes to specified variable. In addition,
#     if another variable is specified for linker flag, linker flag goes there.
#     if no such variable specified, linker flag will not go anywhere.
#     It is suitable for cases when there is a flag for both compiler
#     and linker and only one variable being used for it.

include(CheckCCompilerAndLinkerFlag)

macro(CHECK_AND_SET_FLAGS _TESTNAME _CFLAG _CPARAMSTRING)
	# Forbid reentry to this macro
	if (DEFINED _CASF_LD_ARGS OR DEFINED _CASF_NUM_LD_ARGS
	OR DEFINED _CASF_LDFLAG OR DEFINED _CASF_LDPARAMSTRING)
		message(ERROR "CHECK_AND_SET_FLAGS reentry detected!")
	endif()

	# Get optional arguments _LDFLAG and _LDPARAMSTRING from _LD_ARGS if they exist
	set (_CASF_LD_ARGS ${ARGN})
	list(LENGTH _CASF_LD_ARGS _CASF_NUM_LD_ARGS)
	if (${_CASF_NUM_LD_ARGS} GREATER 0)
		list(GET _CASF_LD_ARGS 0 _CASF_LDFLAG)
	else()
		set(_CASF_LDFLAG "")
	endif ()
	if (${_CASF_NUM_LD_ARGS} GREATER 1)
		list(GET _CASF_LD_ARGS 1 _CASF_LDPARAMSTRING)
	endif ()

	# If check succeeded, set _CPARAMSTRING (and _LDPARAMSTRING, if needed)
	CHECK_C_COMPILER_AND_LINKER_FLAG("${_CFLAG}" "${_CASF_LDFLAG}" "${_TESTNAME}")
	if(${${_TESTNAME}})
		set(${_CPARAMSTRING} "${${_CPARAMSTRING}} ${_CFLAG}")
		if(DEFINED _CASF_LDPARAMSTRING)
			set(${_CASF_LDPARAMSTRING}
			"${${_CASF_LDPARAMSTRING}} ${_CASF_LDFLAG}")
		endif()
	else()
		message(STATUS "  ${_CFLAG} not supported, will not be used.")
	endif()
	
	# Unset working variables
	unset(_CASF_LD_ARGS)
	unset(_CASF_NUM_LD_ARGS)
	unset(_CASF_LDFLAG)
	unset(_CASF_LDPARAMSTRING)
endmacro()
