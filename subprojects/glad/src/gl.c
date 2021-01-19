#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glad/gl.h>

#ifndef GLAD_IMPL_UTIL_C_
#define GLAD_IMPL_UTIL_C_

#ifdef _MSC_VER
#define GLAD_IMPL_UTIL_SSCANF sscanf_s
#else
#define GLAD_IMPL_UTIL_SSCANF sscanf
#endif

#endif /* GLAD_IMPL_UTIL_C_ */

#ifdef __cplusplus
extern "C" {
#endif



int GLAD_GL_VERSION_1_0 = 0;
int GLAD_GL_VERSION_1_1 = 0;
int GLAD_GL_VERSION_1_2 = 0;
int GLAD_GL_VERSION_1_3 = 0;
int GLAD_GL_VERSION_1_4 = 0;
int GLAD_GL_VERSION_1_5 = 0;
int GLAD_GL_VERSION_2_0 = 0;
int GLAD_GL_VERSION_2_1 = 0;
int GLAD_GL_VERSION_3_0 = 0;
int GLAD_GL_VERSION_3_1 = 0;
int GLAD_GL_VERSION_3_2 = 0;
int GLAD_GL_VERSION_3_3 = 0;
int GLAD_GL_ES_VERSION_2_0 = 0;
int GLAD_GL_ES_VERSION_3_0 = 0;
int GLAD_GL_3DFX_texture_compression_FXT1 = 0;
int GLAD_GL_ARB_ES3_compatibility = 0;
int GLAD_GL_ARB_blend_func_extended = 0;
int GLAD_GL_ARB_clear_texture = 0;
int GLAD_GL_ARB_copy_buffer = 0;
int GLAD_GL_ARB_debug_output = 0;
int GLAD_GL_ARB_depth_texture = 0;
int GLAD_GL_ARB_draw_buffers = 0;
int GLAD_GL_ARB_draw_elements_base_vertex = 0;
int GLAD_GL_ARB_draw_instanced = 0;
int GLAD_GL_ARB_framebuffer_object = 0;
int GLAD_GL_ARB_imaging = 0;
int GLAD_GL_ARB_instanced_arrays = 0;
int GLAD_GL_ARB_internalformat_query2 = 0;
int GLAD_GL_ARB_map_buffer_range = 0;
int GLAD_GL_ARB_pixel_buffer_object = 0;
int GLAD_GL_ARB_provoking_vertex = 0;
int GLAD_GL_ARB_sampler_objects = 0;
int GLAD_GL_ARB_sync = 0;
int GLAD_GL_ARB_texture_compression_bptc = 0;
int GLAD_GL_ARB_texture_compression_rgtc = 0;
int GLAD_GL_ARB_texture_filter_anisotropic = 0;
int GLAD_GL_ARB_texture_multisample = 0;
int GLAD_GL_ARB_timer_query = 0;
int GLAD_GL_ARB_uniform_buffer_object = 0;
int GLAD_GL_ARB_vertex_array_object = 0;
int GLAD_GL_ARB_vertex_type_2_10_10_10_rev = 0;
int GLAD_GL_ARB_viewport_array = 0;
int GLAD_GL_ATI_draw_buffers = 0;
int GLAD_GL_EXT_draw_instanced = 0;
int GLAD_GL_EXT_pixel_buffer_object = 0;
int GLAD_GL_EXT_texture_compression_rgtc = 0;
int GLAD_GL_EXT_texture_compression_s3tc = 0;
int GLAD_GL_EXT_texture_filter_anisotropic = 0;
int GLAD_GL_EXT_texture_sRGB = 0;
int GLAD_GL_EXT_texture_sRGB_R8 = 0;
int GLAD_GL_EXT_texture_sRGB_RG8 = 0;
int GLAD_GL_KHR_debug = 0;
int GLAD_GL_KHR_texture_compression_astc_ldr = 0;
int GLAD_GL_NV_viewport_array2 = 0;
int GLAD_GL_SGIX_depth_texture = 0;
int GLAD_GL_AMD_compressed_ATC_texture = 0;
int GLAD_GL_ANGLE_depth_texture = 0;
int GLAD_GL_ANGLE_instanced_arrays = 0;
int GLAD_GL_ANGLE_texture_compression_dxt3 = 0;
int GLAD_GL_ANGLE_texture_compression_dxt5 = 0;
int GLAD_GL_ANGLE_translated_shader_source = 0;
int GLAD_GL_EXT_clear_texture = 0;
int GLAD_GL_EXT_color_buffer_float = 0;
int GLAD_GL_EXT_draw_buffers = 0;
int GLAD_GL_EXT_float_blend = 0;
int GLAD_GL_EXT_instanced_arrays = 0;
int GLAD_GL_EXT_pvrtc_sRGB = 0;
int GLAD_GL_EXT_sRGB = 0;
int GLAD_GL_EXT_texture_compression_bptc = 0;
int GLAD_GL_EXT_texture_compression_dxt1 = 0;
int GLAD_GL_EXT_texture_compression_s3tc_srgb = 0;
int GLAD_GL_EXT_texture_norm16 = 0;
int GLAD_GL_EXT_texture_rg = 0;
int GLAD_GL_IMG_texture_compression_pvrtc = 0;
int GLAD_GL_IMG_texture_compression_pvrtc2 = 0;
int GLAD_GL_NV_draw_instanced = 0;
int GLAD_GL_NV_instanced_arrays = 0;
int GLAD_GL_NV_pixel_buffer_object = 0;
int GLAD_GL_NV_sRGB_formats = 0;
int GLAD_GL_OES_compressed_ETC1_RGB8_texture = 0;
int GLAD_GL_OES_depth_texture = 0;
int GLAD_GL_OES_draw_buffers_indexed = 0;
int GLAD_GL_OES_texture_compression_astc = 0;
int GLAD_GL_OES_texture_float_linear = 0;
int GLAD_GL_OES_texture_half_float_linear = 0;
int GLAD_GL_OES_vertex_array_object = 0;
int GLAD_GL_OES_viewport_array = 0;



PFNGLACTIVETEXTUREPROC glad_glActiveTexture = NULL;
PFNGLATTACHSHADERPROC glad_glAttachShader = NULL;
PFNGLBEGINCONDITIONALRENDERPROC glad_glBeginConditionalRender = NULL;
PFNGLBEGINQUERYPROC glad_glBeginQuery = NULL;
PFNGLBEGINTRANSFORMFEEDBACKPROC glad_glBeginTransformFeedback = NULL;
PFNGLBINDATTRIBLOCATIONPROC glad_glBindAttribLocation = NULL;
PFNGLBINDBUFFERPROC glad_glBindBuffer = NULL;
PFNGLBINDBUFFERBASEPROC glad_glBindBufferBase = NULL;
PFNGLBINDBUFFERRANGEPROC glad_glBindBufferRange = NULL;
PFNGLBINDFRAGDATALOCATIONPROC glad_glBindFragDataLocation = NULL;
PFNGLBINDFRAGDATALOCATIONINDEXEDPROC glad_glBindFragDataLocationIndexed = NULL;
PFNGLBINDFRAMEBUFFERPROC glad_glBindFramebuffer = NULL;
PFNGLBINDRENDERBUFFERPROC glad_glBindRenderbuffer = NULL;
PFNGLBINDSAMPLERPROC glad_glBindSampler = NULL;
PFNGLBINDTEXTUREPROC glad_glBindTexture = NULL;
PFNGLBINDVERTEXARRAYPROC glad_glBindVertexArray = NULL;
PFNGLBLENDCOLORPROC glad_glBlendColor = NULL;
PFNGLBLENDEQUATIONPROC glad_glBlendEquation = NULL;
PFNGLBLENDEQUATIONSEPARATEPROC glad_glBlendEquationSeparate = NULL;
PFNGLBLENDFUNCPROC glad_glBlendFunc = NULL;
PFNGLBLENDFUNCSEPARATEPROC glad_glBlendFuncSeparate = NULL;
PFNGLBLITFRAMEBUFFERPROC glad_glBlitFramebuffer = NULL;
PFNGLBUFFERDATAPROC glad_glBufferData = NULL;
PFNGLBUFFERSUBDATAPROC glad_glBufferSubData = NULL;
PFNGLCHECKFRAMEBUFFERSTATUSPROC glad_glCheckFramebufferStatus = NULL;
PFNGLCLAMPCOLORPROC glad_glClampColor = NULL;
PFNGLCLEARPROC glad_glClear = NULL;
PFNGLCLEARBUFFERFIPROC glad_glClearBufferfi = NULL;
PFNGLCLEARBUFFERFVPROC glad_glClearBufferfv = NULL;
PFNGLCLEARBUFFERIVPROC glad_glClearBufferiv = NULL;
PFNGLCLEARBUFFERUIVPROC glad_glClearBufferuiv = NULL;
PFNGLCLEARCOLORPROC glad_glClearColor = NULL;
PFNGLCLEARDEPTHPROC glad_glClearDepth = NULL;
PFNGLCLEARSTENCILPROC glad_glClearStencil = NULL;
PFNGLCLEARTEXIMAGEPROC glad_glClearTexImage = NULL;
PFNGLCLEARTEXSUBIMAGEPROC glad_glClearTexSubImage = NULL;
PFNGLCLIENTWAITSYNCPROC glad_glClientWaitSync = NULL;
PFNGLCOLORMASKPROC glad_glColorMask = NULL;
PFNGLCOLORMASKIPROC glad_glColorMaski = NULL;
PFNGLCOMPILESHADERPROC glad_glCompileShader = NULL;
PFNGLCOMPRESSEDTEXIMAGE1DPROC glad_glCompressedTexImage1D = NULL;
PFNGLCOMPRESSEDTEXIMAGE2DPROC glad_glCompressedTexImage2D = NULL;
PFNGLCOMPRESSEDTEXIMAGE3DPROC glad_glCompressedTexImage3D = NULL;
PFNGLCOMPRESSEDTEXSUBIMAGE1DPROC glad_glCompressedTexSubImage1D = NULL;
PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC glad_glCompressedTexSubImage2D = NULL;
PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC glad_glCompressedTexSubImage3D = NULL;
PFNGLCOPYBUFFERSUBDATAPROC glad_glCopyBufferSubData = NULL;
PFNGLCOPYTEXIMAGE1DPROC glad_glCopyTexImage1D = NULL;
PFNGLCOPYTEXIMAGE2DPROC glad_glCopyTexImage2D = NULL;
PFNGLCOPYTEXSUBIMAGE1DPROC glad_glCopyTexSubImage1D = NULL;
PFNGLCOPYTEXSUBIMAGE2DPROC glad_glCopyTexSubImage2D = NULL;
PFNGLCOPYTEXSUBIMAGE3DPROC glad_glCopyTexSubImage3D = NULL;
PFNGLCREATEPROGRAMPROC glad_glCreateProgram = NULL;
PFNGLCREATESHADERPROC glad_glCreateShader = NULL;
PFNGLCULLFACEPROC glad_glCullFace = NULL;
PFNGLDEBUGMESSAGECALLBACKPROC glad_glDebugMessageCallback = NULL;
PFNGLDEBUGMESSAGECALLBACKARBPROC glad_glDebugMessageCallbackARB = NULL;
PFNGLDEBUGMESSAGECONTROLPROC glad_glDebugMessageControl = NULL;
PFNGLDEBUGMESSAGECONTROLARBPROC glad_glDebugMessageControlARB = NULL;
PFNGLDEBUGMESSAGEINSERTPROC glad_glDebugMessageInsert = NULL;
PFNGLDEBUGMESSAGEINSERTARBPROC glad_glDebugMessageInsertARB = NULL;
PFNGLDELETEBUFFERSPROC glad_glDeleteBuffers = NULL;
PFNGLDELETEFRAMEBUFFERSPROC glad_glDeleteFramebuffers = NULL;
PFNGLDELETEPROGRAMPROC glad_glDeleteProgram = NULL;
PFNGLDELETEQUERIESPROC glad_glDeleteQueries = NULL;
PFNGLDELETERENDERBUFFERSPROC glad_glDeleteRenderbuffers = NULL;
PFNGLDELETESAMPLERSPROC glad_glDeleteSamplers = NULL;
PFNGLDELETESHADERPROC glad_glDeleteShader = NULL;
PFNGLDELETESYNCPROC glad_glDeleteSync = NULL;
PFNGLDELETETEXTURESPROC glad_glDeleteTextures = NULL;
PFNGLDELETEVERTEXARRAYSPROC glad_glDeleteVertexArrays = NULL;
PFNGLDEPTHFUNCPROC glad_glDepthFunc = NULL;
PFNGLDEPTHMASKPROC glad_glDepthMask = NULL;
PFNGLDEPTHRANGEPROC glad_glDepthRange = NULL;
PFNGLDEPTHRANGEARRAYDVNVPROC glad_glDepthRangeArraydvNV = NULL;
PFNGLDEPTHRANGEARRAYVPROC glad_glDepthRangeArrayv = NULL;
PFNGLDEPTHRANGEINDEXEDPROC glad_glDepthRangeIndexed = NULL;
PFNGLDEPTHRANGEINDEXEDDNVPROC glad_glDepthRangeIndexeddNV = NULL;
PFNGLDETACHSHADERPROC glad_glDetachShader = NULL;
PFNGLDISABLEPROC glad_glDisable = NULL;
PFNGLDISABLEVERTEXATTRIBARRAYPROC glad_glDisableVertexAttribArray = NULL;
PFNGLDISABLEIPROC glad_glDisablei = NULL;
PFNGLDRAWARRAYSPROC glad_glDrawArrays = NULL;
PFNGLDRAWARRAYSINSTANCEDPROC glad_glDrawArraysInstanced = NULL;
PFNGLDRAWARRAYSINSTANCEDARBPROC glad_glDrawArraysInstancedARB = NULL;
PFNGLDRAWARRAYSINSTANCEDEXTPROC glad_glDrawArraysInstancedEXT = NULL;
PFNGLDRAWBUFFERPROC glad_glDrawBuffer = NULL;
PFNGLDRAWBUFFERSPROC glad_glDrawBuffers = NULL;
PFNGLDRAWBUFFERSARBPROC glad_glDrawBuffersARB = NULL;
PFNGLDRAWBUFFERSATIPROC glad_glDrawBuffersATI = NULL;
PFNGLDRAWELEMENTSPROC glad_glDrawElements = NULL;
PFNGLDRAWELEMENTSBASEVERTEXPROC glad_glDrawElementsBaseVertex = NULL;
PFNGLDRAWELEMENTSINSTANCEDPROC glad_glDrawElementsInstanced = NULL;
PFNGLDRAWELEMENTSINSTANCEDARBPROC glad_glDrawElementsInstancedARB = NULL;
PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXPROC glad_glDrawElementsInstancedBaseVertex = NULL;
PFNGLDRAWELEMENTSINSTANCEDEXTPROC glad_glDrawElementsInstancedEXT = NULL;
PFNGLDRAWRANGEELEMENTSPROC glad_glDrawRangeElements = NULL;
PFNGLDRAWRANGEELEMENTSBASEVERTEXPROC glad_glDrawRangeElementsBaseVertex = NULL;
PFNGLENABLEPROC glad_glEnable = NULL;
PFNGLENABLEVERTEXATTRIBARRAYPROC glad_glEnableVertexAttribArray = NULL;
PFNGLENABLEIPROC glad_glEnablei = NULL;
PFNGLENDCONDITIONALRENDERPROC glad_glEndConditionalRender = NULL;
PFNGLENDQUERYPROC glad_glEndQuery = NULL;
PFNGLENDTRANSFORMFEEDBACKPROC glad_glEndTransformFeedback = NULL;
PFNGLFENCESYNCPROC glad_glFenceSync = NULL;
PFNGLFINISHPROC glad_glFinish = NULL;
PFNGLFLUSHPROC glad_glFlush = NULL;
PFNGLFLUSHMAPPEDBUFFERRANGEPROC glad_glFlushMappedBufferRange = NULL;
PFNGLFRAMEBUFFERRENDERBUFFERPROC glad_glFramebufferRenderbuffer = NULL;
PFNGLFRAMEBUFFERTEXTUREPROC glad_glFramebufferTexture = NULL;
PFNGLFRAMEBUFFERTEXTURE1DPROC glad_glFramebufferTexture1D = NULL;
PFNGLFRAMEBUFFERTEXTURE2DPROC glad_glFramebufferTexture2D = NULL;
PFNGLFRAMEBUFFERTEXTURE3DPROC glad_glFramebufferTexture3D = NULL;
PFNGLFRAMEBUFFERTEXTURELAYERPROC glad_glFramebufferTextureLayer = NULL;
PFNGLFRONTFACEPROC glad_glFrontFace = NULL;
PFNGLGENBUFFERSPROC glad_glGenBuffers = NULL;
PFNGLGENFRAMEBUFFERSPROC glad_glGenFramebuffers = NULL;
PFNGLGENQUERIESPROC glad_glGenQueries = NULL;
PFNGLGENRENDERBUFFERSPROC glad_glGenRenderbuffers = NULL;
PFNGLGENSAMPLERSPROC glad_glGenSamplers = NULL;
PFNGLGENTEXTURESPROC glad_glGenTextures = NULL;
PFNGLGENVERTEXARRAYSPROC glad_glGenVertexArrays = NULL;
PFNGLGENERATEMIPMAPPROC glad_glGenerateMipmap = NULL;
PFNGLGETACTIVEATTRIBPROC glad_glGetActiveAttrib = NULL;
PFNGLGETACTIVEUNIFORMPROC glad_glGetActiveUniform = NULL;
PFNGLGETACTIVEUNIFORMBLOCKNAMEPROC glad_glGetActiveUniformBlockName = NULL;
PFNGLGETACTIVEUNIFORMBLOCKIVPROC glad_glGetActiveUniformBlockiv = NULL;
PFNGLGETACTIVEUNIFORMNAMEPROC glad_glGetActiveUniformName = NULL;
PFNGLGETACTIVEUNIFORMSIVPROC glad_glGetActiveUniformsiv = NULL;
PFNGLGETATTACHEDSHADERSPROC glad_glGetAttachedShaders = NULL;
PFNGLGETATTRIBLOCATIONPROC glad_glGetAttribLocation = NULL;
PFNGLGETBOOLEANI_VPROC glad_glGetBooleani_v = NULL;
PFNGLGETBOOLEANVPROC glad_glGetBooleanv = NULL;
PFNGLGETBUFFERPARAMETERI64VPROC glad_glGetBufferParameteri64v = NULL;
PFNGLGETBUFFERPARAMETERIVPROC glad_glGetBufferParameteriv = NULL;
PFNGLGETBUFFERPOINTERVPROC glad_glGetBufferPointerv = NULL;
PFNGLGETBUFFERSUBDATAPROC glad_glGetBufferSubData = NULL;
PFNGLGETCOMPRESSEDTEXIMAGEPROC glad_glGetCompressedTexImage = NULL;
PFNGLGETDEBUGMESSAGELOGPROC glad_glGetDebugMessageLog = NULL;
PFNGLGETDEBUGMESSAGELOGARBPROC glad_glGetDebugMessageLogARB = NULL;
PFNGLGETDOUBLEI_VPROC glad_glGetDoublei_v = NULL;
PFNGLGETDOUBLEVPROC glad_glGetDoublev = NULL;
PFNGLGETERRORPROC glad_glGetError = NULL;
PFNGLGETFLOATI_VPROC glad_glGetFloati_v = NULL;
PFNGLGETFLOATVPROC glad_glGetFloatv = NULL;
PFNGLGETFRAGDATAINDEXPROC glad_glGetFragDataIndex = NULL;
PFNGLGETFRAGDATALOCATIONPROC glad_glGetFragDataLocation = NULL;
PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC glad_glGetFramebufferAttachmentParameteriv = NULL;
PFNGLGETINTEGER64I_VPROC glad_glGetInteger64i_v = NULL;
PFNGLGETINTEGER64VPROC glad_glGetInteger64v = NULL;
PFNGLGETINTEGERI_VPROC glad_glGetIntegeri_v = NULL;
PFNGLGETINTEGERVPROC glad_glGetIntegerv = NULL;
PFNGLGETINTERNALFORMATI64VPROC glad_glGetInternalformati64v = NULL;
PFNGLGETMULTISAMPLEFVPROC glad_glGetMultisamplefv = NULL;
PFNGLGETOBJECTLABELPROC glad_glGetObjectLabel = NULL;
PFNGLGETOBJECTPTRLABELPROC glad_glGetObjectPtrLabel = NULL;
PFNGLGETPOINTERVPROC glad_glGetPointerv = NULL;
PFNGLGETPROGRAMINFOLOGPROC glad_glGetProgramInfoLog = NULL;
PFNGLGETPROGRAMIVPROC glad_glGetProgramiv = NULL;
PFNGLGETQUERYOBJECTI64VPROC glad_glGetQueryObjecti64v = NULL;
PFNGLGETQUERYOBJECTIVPROC glad_glGetQueryObjectiv = NULL;
PFNGLGETQUERYOBJECTUI64VPROC glad_glGetQueryObjectui64v = NULL;
PFNGLGETQUERYOBJECTUIVPROC glad_glGetQueryObjectuiv = NULL;
PFNGLGETQUERYIVPROC glad_glGetQueryiv = NULL;
PFNGLGETRENDERBUFFERPARAMETERIVPROC glad_glGetRenderbufferParameteriv = NULL;
PFNGLGETSAMPLERPARAMETERIIVPROC glad_glGetSamplerParameterIiv = NULL;
PFNGLGETSAMPLERPARAMETERIUIVPROC glad_glGetSamplerParameterIuiv = NULL;
PFNGLGETSAMPLERPARAMETERFVPROC glad_glGetSamplerParameterfv = NULL;
PFNGLGETSAMPLERPARAMETERIVPROC glad_glGetSamplerParameteriv = NULL;
PFNGLGETSHADERINFOLOGPROC glad_glGetShaderInfoLog = NULL;
PFNGLGETSHADERSOURCEPROC glad_glGetShaderSource = NULL;
PFNGLGETSHADERIVPROC glad_glGetShaderiv = NULL;
PFNGLGETSTRINGPROC glad_glGetString = NULL;
PFNGLGETSTRINGIPROC glad_glGetStringi = NULL;
PFNGLGETSYNCIVPROC glad_glGetSynciv = NULL;
PFNGLGETTEXIMAGEPROC glad_glGetTexImage = NULL;
PFNGLGETTEXLEVELPARAMETERFVPROC glad_glGetTexLevelParameterfv = NULL;
PFNGLGETTEXLEVELPARAMETERIVPROC glad_glGetTexLevelParameteriv = NULL;
PFNGLGETTEXPARAMETERIIVPROC glad_glGetTexParameterIiv = NULL;
PFNGLGETTEXPARAMETERIUIVPROC glad_glGetTexParameterIuiv = NULL;
PFNGLGETTEXPARAMETERFVPROC glad_glGetTexParameterfv = NULL;
PFNGLGETTEXPARAMETERIVPROC glad_glGetTexParameteriv = NULL;
PFNGLGETTRANSFORMFEEDBACKVARYINGPROC glad_glGetTransformFeedbackVarying = NULL;
PFNGLGETUNIFORMBLOCKINDEXPROC glad_glGetUniformBlockIndex = NULL;
PFNGLGETUNIFORMINDICESPROC glad_glGetUniformIndices = NULL;
PFNGLGETUNIFORMLOCATIONPROC glad_glGetUniformLocation = NULL;
PFNGLGETUNIFORMFVPROC glad_glGetUniformfv = NULL;
PFNGLGETUNIFORMIVPROC glad_glGetUniformiv = NULL;
PFNGLGETUNIFORMUIVPROC glad_glGetUniformuiv = NULL;
PFNGLGETVERTEXATTRIBIIVPROC glad_glGetVertexAttribIiv = NULL;
PFNGLGETVERTEXATTRIBIUIVPROC glad_glGetVertexAttribIuiv = NULL;
PFNGLGETVERTEXATTRIBPOINTERVPROC glad_glGetVertexAttribPointerv = NULL;
PFNGLGETVERTEXATTRIBDVPROC glad_glGetVertexAttribdv = NULL;
PFNGLGETVERTEXATTRIBFVPROC glad_glGetVertexAttribfv = NULL;
PFNGLGETVERTEXATTRIBIVPROC glad_glGetVertexAttribiv = NULL;
PFNGLHINTPROC glad_glHint = NULL;
PFNGLISBUFFERPROC glad_glIsBuffer = NULL;
PFNGLISENABLEDPROC glad_glIsEnabled = NULL;
PFNGLISENABLEDIPROC glad_glIsEnabledi = NULL;
PFNGLISFRAMEBUFFERPROC glad_glIsFramebuffer = NULL;
PFNGLISPROGRAMPROC glad_glIsProgram = NULL;
PFNGLISQUERYPROC glad_glIsQuery = NULL;
PFNGLISRENDERBUFFERPROC glad_glIsRenderbuffer = NULL;
PFNGLISSAMPLERPROC glad_glIsSampler = NULL;
PFNGLISSHADERPROC glad_glIsShader = NULL;
PFNGLISSYNCPROC glad_glIsSync = NULL;
PFNGLISTEXTUREPROC glad_glIsTexture = NULL;
PFNGLISVERTEXARRAYPROC glad_glIsVertexArray = NULL;
PFNGLLINEWIDTHPROC glad_glLineWidth = NULL;
PFNGLLINKPROGRAMPROC glad_glLinkProgram = NULL;
PFNGLLOGICOPPROC glad_glLogicOp = NULL;
PFNGLMAPBUFFERPROC glad_glMapBuffer = NULL;
PFNGLMAPBUFFERRANGEPROC glad_glMapBufferRange = NULL;
PFNGLMULTIDRAWARRAYSPROC glad_glMultiDrawArrays = NULL;
PFNGLMULTIDRAWELEMENTSPROC glad_glMultiDrawElements = NULL;
PFNGLMULTIDRAWELEMENTSBASEVERTEXPROC glad_glMultiDrawElementsBaseVertex = NULL;
PFNGLOBJECTLABELPROC glad_glObjectLabel = NULL;
PFNGLOBJECTPTRLABELPROC glad_glObjectPtrLabel = NULL;
PFNGLPIXELSTOREFPROC glad_glPixelStoref = NULL;
PFNGLPIXELSTOREIPROC glad_glPixelStorei = NULL;
PFNGLPOINTPARAMETERFPROC glad_glPointParameterf = NULL;
PFNGLPOINTPARAMETERFVPROC glad_glPointParameterfv = NULL;
PFNGLPOINTPARAMETERIPROC glad_glPointParameteri = NULL;
PFNGLPOINTPARAMETERIVPROC glad_glPointParameteriv = NULL;
PFNGLPOINTSIZEPROC glad_glPointSize = NULL;
PFNGLPOLYGONMODEPROC glad_glPolygonMode = NULL;
PFNGLPOLYGONOFFSETPROC glad_glPolygonOffset = NULL;
PFNGLPOPDEBUGGROUPPROC glad_glPopDebugGroup = NULL;
PFNGLPRIMITIVERESTARTINDEXPROC glad_glPrimitiveRestartIndex = NULL;
PFNGLPROVOKINGVERTEXPROC glad_glProvokingVertex = NULL;
PFNGLPUSHDEBUGGROUPPROC glad_glPushDebugGroup = NULL;
PFNGLQUERYCOUNTERPROC glad_glQueryCounter = NULL;
PFNGLREADBUFFERPROC glad_glReadBuffer = NULL;
PFNGLREADPIXELSPROC glad_glReadPixels = NULL;
PFNGLRENDERBUFFERSTORAGEPROC glad_glRenderbufferStorage = NULL;
PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC glad_glRenderbufferStorageMultisample = NULL;
PFNGLSAMPLECOVERAGEPROC glad_glSampleCoverage = NULL;
PFNGLSAMPLEMASKIPROC glad_glSampleMaski = NULL;
PFNGLSAMPLERPARAMETERIIVPROC glad_glSamplerParameterIiv = NULL;
PFNGLSAMPLERPARAMETERIUIVPROC glad_glSamplerParameterIuiv = NULL;
PFNGLSAMPLERPARAMETERFPROC glad_glSamplerParameterf = NULL;
PFNGLSAMPLERPARAMETERFVPROC glad_glSamplerParameterfv = NULL;
PFNGLSAMPLERPARAMETERIPROC glad_glSamplerParameteri = NULL;
PFNGLSAMPLERPARAMETERIVPROC glad_glSamplerParameteriv = NULL;
PFNGLSCISSORPROC glad_glScissor = NULL;
PFNGLSCISSORARRAYVPROC glad_glScissorArrayv = NULL;
PFNGLSCISSORINDEXEDPROC glad_glScissorIndexed = NULL;
PFNGLSCISSORINDEXEDVPROC glad_glScissorIndexedv = NULL;
PFNGLSHADERSOURCEPROC glad_glShaderSource = NULL;
PFNGLSTENCILFUNCPROC glad_glStencilFunc = NULL;
PFNGLSTENCILFUNCSEPARATEPROC glad_glStencilFuncSeparate = NULL;
PFNGLSTENCILMASKPROC glad_glStencilMask = NULL;
PFNGLSTENCILMASKSEPARATEPROC glad_glStencilMaskSeparate = NULL;
PFNGLSTENCILOPPROC glad_glStencilOp = NULL;
PFNGLSTENCILOPSEPARATEPROC glad_glStencilOpSeparate = NULL;
PFNGLTEXBUFFERPROC glad_glTexBuffer = NULL;
PFNGLTEXIMAGE1DPROC glad_glTexImage1D = NULL;
PFNGLTEXIMAGE2DPROC glad_glTexImage2D = NULL;
PFNGLTEXIMAGE2DMULTISAMPLEPROC glad_glTexImage2DMultisample = NULL;
PFNGLTEXIMAGE3DPROC glad_glTexImage3D = NULL;
PFNGLTEXIMAGE3DMULTISAMPLEPROC glad_glTexImage3DMultisample = NULL;
PFNGLTEXPARAMETERIIVPROC glad_glTexParameterIiv = NULL;
PFNGLTEXPARAMETERIUIVPROC glad_glTexParameterIuiv = NULL;
PFNGLTEXPARAMETERFPROC glad_glTexParameterf = NULL;
PFNGLTEXPARAMETERFVPROC glad_glTexParameterfv = NULL;
PFNGLTEXPARAMETERIPROC glad_glTexParameteri = NULL;
PFNGLTEXPARAMETERIVPROC glad_glTexParameteriv = NULL;
PFNGLTEXSUBIMAGE1DPROC glad_glTexSubImage1D = NULL;
PFNGLTEXSUBIMAGE2DPROC glad_glTexSubImage2D = NULL;
PFNGLTEXSUBIMAGE3DPROC glad_glTexSubImage3D = NULL;
PFNGLTRANSFORMFEEDBACKVARYINGSPROC glad_glTransformFeedbackVaryings = NULL;
PFNGLUNIFORM1FPROC glad_glUniform1f = NULL;
PFNGLUNIFORM1FVPROC glad_glUniform1fv = NULL;
PFNGLUNIFORM1IPROC glad_glUniform1i = NULL;
PFNGLUNIFORM1IVPROC glad_glUniform1iv = NULL;
PFNGLUNIFORM1UIPROC glad_glUniform1ui = NULL;
PFNGLUNIFORM1UIVPROC glad_glUniform1uiv = NULL;
PFNGLUNIFORM2FPROC glad_glUniform2f = NULL;
PFNGLUNIFORM2FVPROC glad_glUniform2fv = NULL;
PFNGLUNIFORM2IPROC glad_glUniform2i = NULL;
PFNGLUNIFORM2IVPROC glad_glUniform2iv = NULL;
PFNGLUNIFORM2UIPROC glad_glUniform2ui = NULL;
PFNGLUNIFORM2UIVPROC glad_glUniform2uiv = NULL;
PFNGLUNIFORM3FPROC glad_glUniform3f = NULL;
PFNGLUNIFORM3FVPROC glad_glUniform3fv = NULL;
PFNGLUNIFORM3IPROC glad_glUniform3i = NULL;
PFNGLUNIFORM3IVPROC glad_glUniform3iv = NULL;
PFNGLUNIFORM3UIPROC glad_glUniform3ui = NULL;
PFNGLUNIFORM3UIVPROC glad_glUniform3uiv = NULL;
PFNGLUNIFORM4FPROC glad_glUniform4f = NULL;
PFNGLUNIFORM4FVPROC glad_glUniform4fv = NULL;
PFNGLUNIFORM4IPROC glad_glUniform4i = NULL;
PFNGLUNIFORM4IVPROC glad_glUniform4iv = NULL;
PFNGLUNIFORM4UIPROC glad_glUniform4ui = NULL;
PFNGLUNIFORM4UIVPROC glad_glUniform4uiv = NULL;
PFNGLUNIFORMBLOCKBINDINGPROC glad_glUniformBlockBinding = NULL;
PFNGLUNIFORMMATRIX2FVPROC glad_glUniformMatrix2fv = NULL;
PFNGLUNIFORMMATRIX2X3FVPROC glad_glUniformMatrix2x3fv = NULL;
PFNGLUNIFORMMATRIX2X4FVPROC glad_glUniformMatrix2x4fv = NULL;
PFNGLUNIFORMMATRIX3FVPROC glad_glUniformMatrix3fv = NULL;
PFNGLUNIFORMMATRIX3X2FVPROC glad_glUniformMatrix3x2fv = NULL;
PFNGLUNIFORMMATRIX3X4FVPROC glad_glUniformMatrix3x4fv = NULL;
PFNGLUNIFORMMATRIX4FVPROC glad_glUniformMatrix4fv = NULL;
PFNGLUNIFORMMATRIX4X2FVPROC glad_glUniformMatrix4x2fv = NULL;
PFNGLUNIFORMMATRIX4X3FVPROC glad_glUniformMatrix4x3fv = NULL;
PFNGLUNMAPBUFFERPROC glad_glUnmapBuffer = NULL;
PFNGLUSEPROGRAMPROC glad_glUseProgram = NULL;
PFNGLVALIDATEPROGRAMPROC glad_glValidateProgram = NULL;
PFNGLVERTEXATTRIB1DPROC glad_glVertexAttrib1d = NULL;
PFNGLVERTEXATTRIB1DVPROC glad_glVertexAttrib1dv = NULL;
PFNGLVERTEXATTRIB1FPROC glad_glVertexAttrib1f = NULL;
PFNGLVERTEXATTRIB1FVPROC glad_glVertexAttrib1fv = NULL;
PFNGLVERTEXATTRIB1SPROC glad_glVertexAttrib1s = NULL;
PFNGLVERTEXATTRIB1SVPROC glad_glVertexAttrib1sv = NULL;
PFNGLVERTEXATTRIB2DPROC glad_glVertexAttrib2d = NULL;
PFNGLVERTEXATTRIB2DVPROC glad_glVertexAttrib2dv = NULL;
PFNGLVERTEXATTRIB2FPROC glad_glVertexAttrib2f = NULL;
PFNGLVERTEXATTRIB2FVPROC glad_glVertexAttrib2fv = NULL;
PFNGLVERTEXATTRIB2SPROC glad_glVertexAttrib2s = NULL;
PFNGLVERTEXATTRIB2SVPROC glad_glVertexAttrib2sv = NULL;
PFNGLVERTEXATTRIB3DPROC glad_glVertexAttrib3d = NULL;
PFNGLVERTEXATTRIB3DVPROC glad_glVertexAttrib3dv = NULL;
PFNGLVERTEXATTRIB3FPROC glad_glVertexAttrib3f = NULL;
PFNGLVERTEXATTRIB3FVPROC glad_glVertexAttrib3fv = NULL;
PFNGLVERTEXATTRIB3SPROC glad_glVertexAttrib3s = NULL;
PFNGLVERTEXATTRIB3SVPROC glad_glVertexAttrib3sv = NULL;
PFNGLVERTEXATTRIB4NBVPROC glad_glVertexAttrib4Nbv = NULL;
PFNGLVERTEXATTRIB4NIVPROC glad_glVertexAttrib4Niv = NULL;
PFNGLVERTEXATTRIB4NSVPROC glad_glVertexAttrib4Nsv = NULL;
PFNGLVERTEXATTRIB4NUBPROC glad_glVertexAttrib4Nub = NULL;
PFNGLVERTEXATTRIB4NUBVPROC glad_glVertexAttrib4Nubv = NULL;
PFNGLVERTEXATTRIB4NUIVPROC glad_glVertexAttrib4Nuiv = NULL;
PFNGLVERTEXATTRIB4NUSVPROC glad_glVertexAttrib4Nusv = NULL;
PFNGLVERTEXATTRIB4BVPROC glad_glVertexAttrib4bv = NULL;
PFNGLVERTEXATTRIB4DPROC glad_glVertexAttrib4d = NULL;
PFNGLVERTEXATTRIB4DVPROC glad_glVertexAttrib4dv = NULL;
PFNGLVERTEXATTRIB4FPROC glad_glVertexAttrib4f = NULL;
PFNGLVERTEXATTRIB4FVPROC glad_glVertexAttrib4fv = NULL;
PFNGLVERTEXATTRIB4IVPROC glad_glVertexAttrib4iv = NULL;
PFNGLVERTEXATTRIB4SPROC glad_glVertexAttrib4s = NULL;
PFNGLVERTEXATTRIB4SVPROC glad_glVertexAttrib4sv = NULL;
PFNGLVERTEXATTRIB4UBVPROC glad_glVertexAttrib4ubv = NULL;
PFNGLVERTEXATTRIB4UIVPROC glad_glVertexAttrib4uiv = NULL;
PFNGLVERTEXATTRIB4USVPROC glad_glVertexAttrib4usv = NULL;
PFNGLVERTEXATTRIBDIVISORPROC glad_glVertexAttribDivisor = NULL;
PFNGLVERTEXATTRIBDIVISORARBPROC glad_glVertexAttribDivisorARB = NULL;
PFNGLVERTEXATTRIBI1IPROC glad_glVertexAttribI1i = NULL;
PFNGLVERTEXATTRIBI1IVPROC glad_glVertexAttribI1iv = NULL;
PFNGLVERTEXATTRIBI1UIPROC glad_glVertexAttribI1ui = NULL;
PFNGLVERTEXATTRIBI1UIVPROC glad_glVertexAttribI1uiv = NULL;
PFNGLVERTEXATTRIBI2IPROC glad_glVertexAttribI2i = NULL;
PFNGLVERTEXATTRIBI2IVPROC glad_glVertexAttribI2iv = NULL;
PFNGLVERTEXATTRIBI2UIPROC glad_glVertexAttribI2ui = NULL;
PFNGLVERTEXATTRIBI2UIVPROC glad_glVertexAttribI2uiv = NULL;
PFNGLVERTEXATTRIBI3IPROC glad_glVertexAttribI3i = NULL;
PFNGLVERTEXATTRIBI3IVPROC glad_glVertexAttribI3iv = NULL;
PFNGLVERTEXATTRIBI3UIPROC glad_glVertexAttribI3ui = NULL;
PFNGLVERTEXATTRIBI3UIVPROC glad_glVertexAttribI3uiv = NULL;
PFNGLVERTEXATTRIBI4BVPROC glad_glVertexAttribI4bv = NULL;
PFNGLVERTEXATTRIBI4IPROC glad_glVertexAttribI4i = NULL;
PFNGLVERTEXATTRIBI4IVPROC glad_glVertexAttribI4iv = NULL;
PFNGLVERTEXATTRIBI4SVPROC glad_glVertexAttribI4sv = NULL;
PFNGLVERTEXATTRIBI4UBVPROC glad_glVertexAttribI4ubv = NULL;
PFNGLVERTEXATTRIBI4UIPROC glad_glVertexAttribI4ui = NULL;
PFNGLVERTEXATTRIBI4UIVPROC glad_glVertexAttribI4uiv = NULL;
PFNGLVERTEXATTRIBI4USVPROC glad_glVertexAttribI4usv = NULL;
PFNGLVERTEXATTRIBIPOINTERPROC glad_glVertexAttribIPointer = NULL;
PFNGLVERTEXATTRIBP1UIPROC glad_glVertexAttribP1ui = NULL;
PFNGLVERTEXATTRIBP1UIVPROC glad_glVertexAttribP1uiv = NULL;
PFNGLVERTEXATTRIBP2UIPROC glad_glVertexAttribP2ui = NULL;
PFNGLVERTEXATTRIBP2UIVPROC glad_glVertexAttribP2uiv = NULL;
PFNGLVERTEXATTRIBP3UIPROC glad_glVertexAttribP3ui = NULL;
PFNGLVERTEXATTRIBP3UIVPROC glad_glVertexAttribP3uiv = NULL;
PFNGLVERTEXATTRIBP4UIPROC glad_glVertexAttribP4ui = NULL;
PFNGLVERTEXATTRIBP4UIVPROC glad_glVertexAttribP4uiv = NULL;
PFNGLVERTEXATTRIBPOINTERPROC glad_glVertexAttribPointer = NULL;
PFNGLVIEWPORTPROC glad_glViewport = NULL;
PFNGLVIEWPORTARRAYVPROC glad_glViewportArrayv = NULL;
PFNGLVIEWPORTINDEXEDFPROC glad_glViewportIndexedf = NULL;
PFNGLVIEWPORTINDEXEDFVPROC glad_glViewportIndexedfv = NULL;
PFNGLWAITSYNCPROC glad_glWaitSync = NULL;
PFNGLBINDTRANSFORMFEEDBACKPROC glad_glBindTransformFeedback = NULL;
PFNGLBINDVERTEXARRAYOESPROC glad_glBindVertexArrayOES = NULL;
PFNGLBLENDEQUATIONSEPARATEIOESPROC glad_glBlendEquationSeparateiOES = NULL;
PFNGLBLENDEQUATIONIOESPROC glad_glBlendEquationiOES = NULL;
PFNGLBLENDFUNCSEPARATEIOESPROC glad_glBlendFuncSeparateiOES = NULL;
PFNGLBLENDFUNCIOESPROC glad_glBlendFunciOES = NULL;
PFNGLCLEARDEPTHFPROC glad_glClearDepthf = NULL;
PFNGLCLEARTEXIMAGEEXTPROC glad_glClearTexImageEXT = NULL;
PFNGLCLEARTEXSUBIMAGEEXTPROC glad_glClearTexSubImageEXT = NULL;
PFNGLCOLORMASKIOESPROC glad_glColorMaskiOES = NULL;
PFNGLDEBUGMESSAGECALLBACKKHRPROC glad_glDebugMessageCallbackKHR = NULL;
PFNGLDEBUGMESSAGECONTROLKHRPROC glad_glDebugMessageControlKHR = NULL;
PFNGLDEBUGMESSAGEINSERTKHRPROC glad_glDebugMessageInsertKHR = NULL;
PFNGLDELETETRANSFORMFEEDBACKSPROC glad_glDeleteTransformFeedbacks = NULL;
PFNGLDELETEVERTEXARRAYSOESPROC glad_glDeleteVertexArraysOES = NULL;
PFNGLDEPTHRANGEARRAYFVOESPROC glad_glDepthRangeArrayfvOES = NULL;
PFNGLDEPTHRANGEINDEXEDFOESPROC glad_glDepthRangeIndexedfOES = NULL;
PFNGLDEPTHRANGEFPROC glad_glDepthRangef = NULL;
PFNGLDISABLEIOESPROC glad_glDisableiOES = NULL;
PFNGLDRAWARRAYSINSTANCEDANGLEPROC glad_glDrawArraysInstancedANGLE = NULL;
PFNGLDRAWARRAYSINSTANCEDNVPROC glad_glDrawArraysInstancedNV = NULL;
PFNGLDRAWBUFFERSEXTPROC glad_glDrawBuffersEXT = NULL;
PFNGLDRAWELEMENTSINSTANCEDANGLEPROC glad_glDrawElementsInstancedANGLE = NULL;
PFNGLDRAWELEMENTSINSTANCEDNVPROC glad_glDrawElementsInstancedNV = NULL;
PFNGLENABLEIOESPROC glad_glEnableiOES = NULL;
PFNGLGENTRANSFORMFEEDBACKSPROC glad_glGenTransformFeedbacks = NULL;
PFNGLGENVERTEXARRAYSOESPROC glad_glGenVertexArraysOES = NULL;
PFNGLGETDEBUGMESSAGELOGKHRPROC glad_glGetDebugMessageLogKHR = NULL;
PFNGLGETFLOATI_VOESPROC glad_glGetFloati_vOES = NULL;
PFNGLGETINTERNALFORMATIVPROC glad_glGetInternalformativ = NULL;
PFNGLGETOBJECTLABELKHRPROC glad_glGetObjectLabelKHR = NULL;
PFNGLGETOBJECTPTRLABELKHRPROC glad_glGetObjectPtrLabelKHR = NULL;
PFNGLGETPOINTERVKHRPROC glad_glGetPointervKHR = NULL;
PFNGLGETPROGRAMBINARYPROC glad_glGetProgramBinary = NULL;
PFNGLGETSHADERPRECISIONFORMATPROC glad_glGetShaderPrecisionFormat = NULL;
PFNGLGETTRANSLATEDSHADERSOURCEANGLEPROC glad_glGetTranslatedShaderSourceANGLE = NULL;
PFNGLINVALIDATEFRAMEBUFFERPROC glad_glInvalidateFramebuffer = NULL;
PFNGLINVALIDATESUBFRAMEBUFFERPROC glad_glInvalidateSubFramebuffer = NULL;
PFNGLISENABLEDIOESPROC glad_glIsEnablediOES = NULL;
PFNGLISTRANSFORMFEEDBACKPROC glad_glIsTransformFeedback = NULL;
PFNGLISVERTEXARRAYOESPROC glad_glIsVertexArrayOES = NULL;
PFNGLOBJECTLABELKHRPROC glad_glObjectLabelKHR = NULL;
PFNGLOBJECTPTRLABELKHRPROC glad_glObjectPtrLabelKHR = NULL;
PFNGLPAUSETRANSFORMFEEDBACKPROC glad_glPauseTransformFeedback = NULL;
PFNGLPOPDEBUGGROUPKHRPROC glad_glPopDebugGroupKHR = NULL;
PFNGLPROGRAMBINARYPROC glad_glProgramBinary = NULL;
PFNGLPROGRAMPARAMETERIPROC glad_glProgramParameteri = NULL;
PFNGLPUSHDEBUGGROUPKHRPROC glad_glPushDebugGroupKHR = NULL;
PFNGLRELEASESHADERCOMPILERPROC glad_glReleaseShaderCompiler = NULL;
PFNGLRESUMETRANSFORMFEEDBACKPROC glad_glResumeTransformFeedback = NULL;
PFNGLSCISSORARRAYVOESPROC glad_glScissorArrayvOES = NULL;
PFNGLSCISSORINDEXEDOESPROC glad_glScissorIndexedOES = NULL;
PFNGLSCISSORINDEXEDVOESPROC glad_glScissorIndexedvOES = NULL;
PFNGLSHADERBINARYPROC glad_glShaderBinary = NULL;
PFNGLTEXSTORAGE2DPROC glad_glTexStorage2D = NULL;
PFNGLTEXSTORAGE3DPROC glad_glTexStorage3D = NULL;
PFNGLVERTEXATTRIBDIVISORANGLEPROC glad_glVertexAttribDivisorANGLE = NULL;
PFNGLVERTEXATTRIBDIVISOREXTPROC glad_glVertexAttribDivisorEXT = NULL;
PFNGLVERTEXATTRIBDIVISORNVPROC glad_glVertexAttribDivisorNV = NULL;
PFNGLVIEWPORTARRAYVOESPROC glad_glViewportArrayvOES = NULL;
PFNGLVIEWPORTINDEXEDFOESPROC glad_glViewportIndexedfOES = NULL;
PFNGLVIEWPORTINDEXEDFVOESPROC glad_glViewportIndexedfvOES = NULL;


static void glad_gl_load_GL_VERSION_1_0( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_GL_VERSION_1_0) return;
    glad_glBlendFunc = (PFNGLBLENDFUNCPROC) load(userptr, "glBlendFunc");
    glad_glClear = (PFNGLCLEARPROC) load(userptr, "glClear");
    glad_glClearColor = (PFNGLCLEARCOLORPROC) load(userptr, "glClearColor");
    glad_glClearDepth = (PFNGLCLEARDEPTHPROC) load(userptr, "glClearDepth");
    glad_glClearStencil = (PFNGLCLEARSTENCILPROC) load(userptr, "glClearStencil");
    glad_glColorMask = (PFNGLCOLORMASKPROC) load(userptr, "glColorMask");
    glad_glCullFace = (PFNGLCULLFACEPROC) load(userptr, "glCullFace");
    glad_glDepthFunc = (PFNGLDEPTHFUNCPROC) load(userptr, "glDepthFunc");
    glad_glDepthMask = (PFNGLDEPTHMASKPROC) load(userptr, "glDepthMask");
    glad_glDepthRange = (PFNGLDEPTHRANGEPROC) load(userptr, "glDepthRange");
    glad_glDisable = (PFNGLDISABLEPROC) load(userptr, "glDisable");
    glad_glDrawBuffer = (PFNGLDRAWBUFFERPROC) load(userptr, "glDrawBuffer");
    glad_glEnable = (PFNGLENABLEPROC) load(userptr, "glEnable");
    glad_glFinish = (PFNGLFINISHPROC) load(userptr, "glFinish");
    glad_glFlush = (PFNGLFLUSHPROC) load(userptr, "glFlush");
    glad_glFrontFace = (PFNGLFRONTFACEPROC) load(userptr, "glFrontFace");
    glad_glGetBooleanv = (PFNGLGETBOOLEANVPROC) load(userptr, "glGetBooleanv");
    glad_glGetDoublev = (PFNGLGETDOUBLEVPROC) load(userptr, "glGetDoublev");
    glad_glGetError = (PFNGLGETERRORPROC) load(userptr, "glGetError");
    glad_glGetFloatv = (PFNGLGETFLOATVPROC) load(userptr, "glGetFloatv");
    glad_glGetIntegerv = (PFNGLGETINTEGERVPROC) load(userptr, "glGetIntegerv");
    glad_glGetString = (PFNGLGETSTRINGPROC) load(userptr, "glGetString");
    glad_glGetTexImage = (PFNGLGETTEXIMAGEPROC) load(userptr, "glGetTexImage");
    glad_glGetTexLevelParameterfv = (PFNGLGETTEXLEVELPARAMETERFVPROC) load(userptr, "glGetTexLevelParameterfv");
    glad_glGetTexLevelParameteriv = (PFNGLGETTEXLEVELPARAMETERIVPROC) load(userptr, "glGetTexLevelParameteriv");
    glad_glGetTexParameterfv = (PFNGLGETTEXPARAMETERFVPROC) load(userptr, "glGetTexParameterfv");
    glad_glGetTexParameteriv = (PFNGLGETTEXPARAMETERIVPROC) load(userptr, "glGetTexParameteriv");
    glad_glHint = (PFNGLHINTPROC) load(userptr, "glHint");
    glad_glIsEnabled = (PFNGLISENABLEDPROC) load(userptr, "glIsEnabled");
    glad_glLineWidth = (PFNGLLINEWIDTHPROC) load(userptr, "glLineWidth");
    glad_glLogicOp = (PFNGLLOGICOPPROC) load(userptr, "glLogicOp");
    glad_glPixelStoref = (PFNGLPIXELSTOREFPROC) load(userptr, "glPixelStoref");
    glad_glPixelStorei = (PFNGLPIXELSTOREIPROC) load(userptr, "glPixelStorei");
    glad_glPointSize = (PFNGLPOINTSIZEPROC) load(userptr, "glPointSize");
    glad_glPolygonMode = (PFNGLPOLYGONMODEPROC) load(userptr, "glPolygonMode");
    glad_glReadBuffer = (PFNGLREADBUFFERPROC) load(userptr, "glReadBuffer");
    glad_glReadPixels = (PFNGLREADPIXELSPROC) load(userptr, "glReadPixels");
    glad_glScissor = (PFNGLSCISSORPROC) load(userptr, "glScissor");
    glad_glStencilFunc = (PFNGLSTENCILFUNCPROC) load(userptr, "glStencilFunc");
    glad_glStencilMask = (PFNGLSTENCILMASKPROC) load(userptr, "glStencilMask");
    glad_glStencilOp = (PFNGLSTENCILOPPROC) load(userptr, "glStencilOp");
    glad_glTexImage1D = (PFNGLTEXIMAGE1DPROC) load(userptr, "glTexImage1D");
    glad_glTexImage2D = (PFNGLTEXIMAGE2DPROC) load(userptr, "glTexImage2D");
    glad_glTexParameterf = (PFNGLTEXPARAMETERFPROC) load(userptr, "glTexParameterf");
    glad_glTexParameterfv = (PFNGLTEXPARAMETERFVPROC) load(userptr, "glTexParameterfv");
    glad_glTexParameteri = (PFNGLTEXPARAMETERIPROC) load(userptr, "glTexParameteri");
    glad_glTexParameteriv = (PFNGLTEXPARAMETERIVPROC) load(userptr, "glTexParameteriv");
    glad_glViewport = (PFNGLVIEWPORTPROC) load(userptr, "glViewport");
}
static void glad_gl_load_GL_VERSION_1_1( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_GL_VERSION_1_1) return;
    glad_glBindTexture = (PFNGLBINDTEXTUREPROC) load(userptr, "glBindTexture");
    glad_glCopyTexImage1D = (PFNGLCOPYTEXIMAGE1DPROC) load(userptr, "glCopyTexImage1D");
    glad_glCopyTexImage2D = (PFNGLCOPYTEXIMAGE2DPROC) load(userptr, "glCopyTexImage2D");
    glad_glCopyTexSubImage1D = (PFNGLCOPYTEXSUBIMAGE1DPROC) load(userptr, "glCopyTexSubImage1D");
    glad_glCopyTexSubImage2D = (PFNGLCOPYTEXSUBIMAGE2DPROC) load(userptr, "glCopyTexSubImage2D");
    glad_glDeleteTextures = (PFNGLDELETETEXTURESPROC) load(userptr, "glDeleteTextures");
    glad_glDrawArrays = (PFNGLDRAWARRAYSPROC) load(userptr, "glDrawArrays");
    glad_glDrawElements = (PFNGLDRAWELEMENTSPROC) load(userptr, "glDrawElements");
    glad_glGenTextures = (PFNGLGENTEXTURESPROC) load(userptr, "glGenTextures");
    glad_glGetPointerv = (PFNGLGETPOINTERVPROC) load(userptr, "glGetPointerv");
    glad_glIsTexture = (PFNGLISTEXTUREPROC) load(userptr, "glIsTexture");
    glad_glPolygonOffset = (PFNGLPOLYGONOFFSETPROC) load(userptr, "glPolygonOffset");
    glad_glTexSubImage1D = (PFNGLTEXSUBIMAGE1DPROC) load(userptr, "glTexSubImage1D");
    glad_glTexSubImage2D = (PFNGLTEXSUBIMAGE2DPROC) load(userptr, "glTexSubImage2D");
}
static void glad_gl_load_GL_VERSION_1_2( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_GL_VERSION_1_2) return;
    glad_glCopyTexSubImage3D = (PFNGLCOPYTEXSUBIMAGE3DPROC) load(userptr, "glCopyTexSubImage3D");
    glad_glDrawRangeElements = (PFNGLDRAWRANGEELEMENTSPROC) load(userptr, "glDrawRangeElements");
    glad_glTexImage3D = (PFNGLTEXIMAGE3DPROC) load(userptr, "glTexImage3D");
    glad_glTexSubImage3D = (PFNGLTEXSUBIMAGE3DPROC) load(userptr, "glTexSubImage3D");
}
static void glad_gl_load_GL_VERSION_1_3( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_GL_VERSION_1_3) return;
    glad_glActiveTexture = (PFNGLACTIVETEXTUREPROC) load(userptr, "glActiveTexture");
    glad_glCompressedTexImage1D = (PFNGLCOMPRESSEDTEXIMAGE1DPROC) load(userptr, "glCompressedTexImage1D");
    glad_glCompressedTexImage2D = (PFNGLCOMPRESSEDTEXIMAGE2DPROC) load(userptr, "glCompressedTexImage2D");
    glad_glCompressedTexImage3D = (PFNGLCOMPRESSEDTEXIMAGE3DPROC) load(userptr, "glCompressedTexImage3D");
    glad_glCompressedTexSubImage1D = (PFNGLCOMPRESSEDTEXSUBIMAGE1DPROC) load(userptr, "glCompressedTexSubImage1D");
    glad_glCompressedTexSubImage2D = (PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC) load(userptr, "glCompressedTexSubImage2D");
    glad_glCompressedTexSubImage3D = (PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC) load(userptr, "glCompressedTexSubImage3D");
    glad_glGetCompressedTexImage = (PFNGLGETCOMPRESSEDTEXIMAGEPROC) load(userptr, "glGetCompressedTexImage");
    glad_glSampleCoverage = (PFNGLSAMPLECOVERAGEPROC) load(userptr, "glSampleCoverage");
}
static void glad_gl_load_GL_VERSION_1_4( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_GL_VERSION_1_4) return;
    glad_glBlendColor = (PFNGLBLENDCOLORPROC) load(userptr, "glBlendColor");
    glad_glBlendEquation = (PFNGLBLENDEQUATIONPROC) load(userptr, "glBlendEquation");
    glad_glBlendFuncSeparate = (PFNGLBLENDFUNCSEPARATEPROC) load(userptr, "glBlendFuncSeparate");
    glad_glMultiDrawArrays = (PFNGLMULTIDRAWARRAYSPROC) load(userptr, "glMultiDrawArrays");
    glad_glMultiDrawElements = (PFNGLMULTIDRAWELEMENTSPROC) load(userptr, "glMultiDrawElements");
    glad_glPointParameterf = (PFNGLPOINTPARAMETERFPROC) load(userptr, "glPointParameterf");
    glad_glPointParameterfv = (PFNGLPOINTPARAMETERFVPROC) load(userptr, "glPointParameterfv");
    glad_glPointParameteri = (PFNGLPOINTPARAMETERIPROC) load(userptr, "glPointParameteri");
    glad_glPointParameteriv = (PFNGLPOINTPARAMETERIVPROC) load(userptr, "glPointParameteriv");
}
static void glad_gl_load_GL_VERSION_1_5( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_GL_VERSION_1_5) return;
    glad_glBeginQuery = (PFNGLBEGINQUERYPROC) load(userptr, "glBeginQuery");
    glad_glBindBuffer = (PFNGLBINDBUFFERPROC) load(userptr, "glBindBuffer");
    glad_glBufferData = (PFNGLBUFFERDATAPROC) load(userptr, "glBufferData");
    glad_glBufferSubData = (PFNGLBUFFERSUBDATAPROC) load(userptr, "glBufferSubData");
    glad_glDeleteBuffers = (PFNGLDELETEBUFFERSPROC) load(userptr, "glDeleteBuffers");
    glad_glDeleteQueries = (PFNGLDELETEQUERIESPROC) load(userptr, "glDeleteQueries");
    glad_glEndQuery = (PFNGLENDQUERYPROC) load(userptr, "glEndQuery");
    glad_glGenBuffers = (PFNGLGENBUFFERSPROC) load(userptr, "glGenBuffers");
    glad_glGenQueries = (PFNGLGENQUERIESPROC) load(userptr, "glGenQueries");
    glad_glGetBufferParameteriv = (PFNGLGETBUFFERPARAMETERIVPROC) load(userptr, "glGetBufferParameteriv");
    glad_glGetBufferPointerv = (PFNGLGETBUFFERPOINTERVPROC) load(userptr, "glGetBufferPointerv");
    glad_glGetBufferSubData = (PFNGLGETBUFFERSUBDATAPROC) load(userptr, "glGetBufferSubData");
    glad_glGetQueryObjectiv = (PFNGLGETQUERYOBJECTIVPROC) load(userptr, "glGetQueryObjectiv");
    glad_glGetQueryObjectuiv = (PFNGLGETQUERYOBJECTUIVPROC) load(userptr, "glGetQueryObjectuiv");
    glad_glGetQueryiv = (PFNGLGETQUERYIVPROC) load(userptr, "glGetQueryiv");
    glad_glIsBuffer = (PFNGLISBUFFERPROC) load(userptr, "glIsBuffer");
    glad_glIsQuery = (PFNGLISQUERYPROC) load(userptr, "glIsQuery");
    glad_glMapBuffer = (PFNGLMAPBUFFERPROC) load(userptr, "glMapBuffer");
    glad_glUnmapBuffer = (PFNGLUNMAPBUFFERPROC) load(userptr, "glUnmapBuffer");
}
static void glad_gl_load_GL_VERSION_2_0( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_GL_VERSION_2_0) return;
    glad_glAttachShader = (PFNGLATTACHSHADERPROC) load(userptr, "glAttachShader");
    glad_glBindAttribLocation = (PFNGLBINDATTRIBLOCATIONPROC) load(userptr, "glBindAttribLocation");
    glad_glBlendEquationSeparate = (PFNGLBLENDEQUATIONSEPARATEPROC) load(userptr, "glBlendEquationSeparate");
    glad_glCompileShader = (PFNGLCOMPILESHADERPROC) load(userptr, "glCompileShader");
    glad_glCreateProgram = (PFNGLCREATEPROGRAMPROC) load(userptr, "glCreateProgram");
    glad_glCreateShader = (PFNGLCREATESHADERPROC) load(userptr, "glCreateShader");
    glad_glDeleteProgram = (PFNGLDELETEPROGRAMPROC) load(userptr, "glDeleteProgram");
    glad_glDeleteShader = (PFNGLDELETESHADERPROC) load(userptr, "glDeleteShader");
    glad_glDetachShader = (PFNGLDETACHSHADERPROC) load(userptr, "glDetachShader");
    glad_glDisableVertexAttribArray = (PFNGLDISABLEVERTEXATTRIBARRAYPROC) load(userptr, "glDisableVertexAttribArray");
    glad_glDrawBuffers = (PFNGLDRAWBUFFERSPROC) load(userptr, "glDrawBuffers");
    glad_glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC) load(userptr, "glEnableVertexAttribArray");
    glad_glGetActiveAttrib = (PFNGLGETACTIVEATTRIBPROC) load(userptr, "glGetActiveAttrib");
    glad_glGetActiveUniform = (PFNGLGETACTIVEUNIFORMPROC) load(userptr, "glGetActiveUniform");
    glad_glGetAttachedShaders = (PFNGLGETATTACHEDSHADERSPROC) load(userptr, "glGetAttachedShaders");
    glad_glGetAttribLocation = (PFNGLGETATTRIBLOCATIONPROC) load(userptr, "glGetAttribLocation");
    glad_glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC) load(userptr, "glGetProgramInfoLog");
    glad_glGetProgramiv = (PFNGLGETPROGRAMIVPROC) load(userptr, "glGetProgramiv");
    glad_glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC) load(userptr, "glGetShaderInfoLog");
    glad_glGetShaderSource = (PFNGLGETSHADERSOURCEPROC) load(userptr, "glGetShaderSource");
    glad_glGetShaderiv = (PFNGLGETSHADERIVPROC) load(userptr, "glGetShaderiv");
    glad_glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC) load(userptr, "glGetUniformLocation");
    glad_glGetUniformfv = (PFNGLGETUNIFORMFVPROC) load(userptr, "glGetUniformfv");
    glad_glGetUniformiv = (PFNGLGETUNIFORMIVPROC) load(userptr, "glGetUniformiv");
    glad_glGetVertexAttribPointerv = (PFNGLGETVERTEXATTRIBPOINTERVPROC) load(userptr, "glGetVertexAttribPointerv");
    glad_glGetVertexAttribdv = (PFNGLGETVERTEXATTRIBDVPROC) load(userptr, "glGetVertexAttribdv");
    glad_glGetVertexAttribfv = (PFNGLGETVERTEXATTRIBFVPROC) load(userptr, "glGetVertexAttribfv");
    glad_glGetVertexAttribiv = (PFNGLGETVERTEXATTRIBIVPROC) load(userptr, "glGetVertexAttribiv");
    glad_glIsProgram = (PFNGLISPROGRAMPROC) load(userptr, "glIsProgram");
    glad_glIsShader = (PFNGLISSHADERPROC) load(userptr, "glIsShader");
    glad_glLinkProgram = (PFNGLLINKPROGRAMPROC) load(userptr, "glLinkProgram");
    glad_glShaderSource = (PFNGLSHADERSOURCEPROC) load(userptr, "glShaderSource");
    glad_glStencilFuncSeparate = (PFNGLSTENCILFUNCSEPARATEPROC) load(userptr, "glStencilFuncSeparate");
    glad_glStencilMaskSeparate = (PFNGLSTENCILMASKSEPARATEPROC) load(userptr, "glStencilMaskSeparate");
    glad_glStencilOpSeparate = (PFNGLSTENCILOPSEPARATEPROC) load(userptr, "glStencilOpSeparate");
    glad_glUniform1f = (PFNGLUNIFORM1FPROC) load(userptr, "glUniform1f");
    glad_glUniform1fv = (PFNGLUNIFORM1FVPROC) load(userptr, "glUniform1fv");
    glad_glUniform1i = (PFNGLUNIFORM1IPROC) load(userptr, "glUniform1i");
    glad_glUniform1iv = (PFNGLUNIFORM1IVPROC) load(userptr, "glUniform1iv");
    glad_glUniform2f = (PFNGLUNIFORM2FPROC) load(userptr, "glUniform2f");
    glad_glUniform2fv = (PFNGLUNIFORM2FVPROC) load(userptr, "glUniform2fv");
    glad_glUniform2i = (PFNGLUNIFORM2IPROC) load(userptr, "glUniform2i");
    glad_glUniform2iv = (PFNGLUNIFORM2IVPROC) load(userptr, "glUniform2iv");
    glad_glUniform3f = (PFNGLUNIFORM3FPROC) load(userptr, "glUniform3f");
    glad_glUniform3fv = (PFNGLUNIFORM3FVPROC) load(userptr, "glUniform3fv");
    glad_glUniform3i = (PFNGLUNIFORM3IPROC) load(userptr, "glUniform3i");
    glad_glUniform3iv = (PFNGLUNIFORM3IVPROC) load(userptr, "glUniform3iv");
    glad_glUniform4f = (PFNGLUNIFORM4FPROC) load(userptr, "glUniform4f");
    glad_glUniform4fv = (PFNGLUNIFORM4FVPROC) load(userptr, "glUniform4fv");
    glad_glUniform4i = (PFNGLUNIFORM4IPROC) load(userptr, "glUniform4i");
    glad_glUniform4iv = (PFNGLUNIFORM4IVPROC) load(userptr, "glUniform4iv");
    glad_glUniformMatrix2fv = (PFNGLUNIFORMMATRIX2FVPROC) load(userptr, "glUniformMatrix2fv");
    glad_glUniformMatrix3fv = (PFNGLUNIFORMMATRIX3FVPROC) load(userptr, "glUniformMatrix3fv");
    glad_glUniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC) load(userptr, "glUniformMatrix4fv");
    glad_glUseProgram = (PFNGLUSEPROGRAMPROC) load(userptr, "glUseProgram");
    glad_glValidateProgram = (PFNGLVALIDATEPROGRAMPROC) load(userptr, "glValidateProgram");
    glad_glVertexAttrib1d = (PFNGLVERTEXATTRIB1DPROC) load(userptr, "glVertexAttrib1d");
    glad_glVertexAttrib1dv = (PFNGLVERTEXATTRIB1DVPROC) load(userptr, "glVertexAttrib1dv");
    glad_glVertexAttrib1f = (PFNGLVERTEXATTRIB1FPROC) load(userptr, "glVertexAttrib1f");
    glad_glVertexAttrib1fv = (PFNGLVERTEXATTRIB1FVPROC) load(userptr, "glVertexAttrib1fv");
    glad_glVertexAttrib1s = (PFNGLVERTEXATTRIB1SPROC) load(userptr, "glVertexAttrib1s");
    glad_glVertexAttrib1sv = (PFNGLVERTEXATTRIB1SVPROC) load(userptr, "glVertexAttrib1sv");
    glad_glVertexAttrib2d = (PFNGLVERTEXATTRIB2DPROC) load(userptr, "glVertexAttrib2d");
    glad_glVertexAttrib2dv = (PFNGLVERTEXATTRIB2DVPROC) load(userptr, "glVertexAttrib2dv");
    glad_glVertexAttrib2f = (PFNGLVERTEXATTRIB2FPROC) load(userptr, "glVertexAttrib2f");
    glad_glVertexAttrib2fv = (PFNGLVERTEXATTRIB2FVPROC) load(userptr, "glVertexAttrib2fv");
    glad_glVertexAttrib2s = (PFNGLVERTEXATTRIB2SPROC) load(userptr, "glVertexAttrib2s");
    glad_glVertexAttrib2sv = (PFNGLVERTEXATTRIB2SVPROC) load(userptr, "glVertexAttrib2sv");
    glad_glVertexAttrib3d = (PFNGLVERTEXATTRIB3DPROC) load(userptr, "glVertexAttrib3d");
    glad_glVertexAttrib3dv = (PFNGLVERTEXATTRIB3DVPROC) load(userptr, "glVertexAttrib3dv");
    glad_glVertexAttrib3f = (PFNGLVERTEXATTRIB3FPROC) load(userptr, "glVertexAttrib3f");
    glad_glVertexAttrib3fv = (PFNGLVERTEXATTRIB3FVPROC) load(userptr, "glVertexAttrib3fv");
    glad_glVertexAttrib3s = (PFNGLVERTEXATTRIB3SPROC) load(userptr, "glVertexAttrib3s");
    glad_glVertexAttrib3sv = (PFNGLVERTEXATTRIB3SVPROC) load(userptr, "glVertexAttrib3sv");
    glad_glVertexAttrib4Nbv = (PFNGLVERTEXATTRIB4NBVPROC) load(userptr, "glVertexAttrib4Nbv");
    glad_glVertexAttrib4Niv = (PFNGLVERTEXATTRIB4NIVPROC) load(userptr, "glVertexAttrib4Niv");
    glad_glVertexAttrib4Nsv = (PFNGLVERTEXATTRIB4NSVPROC) load(userptr, "glVertexAttrib4Nsv");
    glad_glVertexAttrib4Nub = (PFNGLVERTEXATTRIB4NUBPROC) load(userptr, "glVertexAttrib4Nub");
    glad_glVertexAttrib4Nubv = (PFNGLVERTEXATTRIB4NUBVPROC) load(userptr, "glVertexAttrib4Nubv");
    glad_glVertexAttrib4Nuiv = (PFNGLVERTEXATTRIB4NUIVPROC) load(userptr, "glVertexAttrib4Nuiv");
    glad_glVertexAttrib4Nusv = (PFNGLVERTEXATTRIB4NUSVPROC) load(userptr, "glVertexAttrib4Nusv");
    glad_glVertexAttrib4bv = (PFNGLVERTEXATTRIB4BVPROC) load(userptr, "glVertexAttrib4bv");
    glad_glVertexAttrib4d = (PFNGLVERTEXATTRIB4DPROC) load(userptr, "glVertexAttrib4d");
    glad_glVertexAttrib4dv = (PFNGLVERTEXATTRIB4DVPROC) load(userptr, "glVertexAttrib4dv");
    glad_glVertexAttrib4f = (PFNGLVERTEXATTRIB4FPROC) load(userptr, "glVertexAttrib4f");
    glad_glVertexAttrib4fv = (PFNGLVERTEXATTRIB4FVPROC) load(userptr, "glVertexAttrib4fv");
    glad_glVertexAttrib4iv = (PFNGLVERTEXATTRIB4IVPROC) load(userptr, "glVertexAttrib4iv");
    glad_glVertexAttrib4s = (PFNGLVERTEXATTRIB4SPROC) load(userptr, "glVertexAttrib4s");
    glad_glVertexAttrib4sv = (PFNGLVERTEXATTRIB4SVPROC) load(userptr, "glVertexAttrib4sv");
    glad_glVertexAttrib4ubv = (PFNGLVERTEXATTRIB4UBVPROC) load(userptr, "glVertexAttrib4ubv");
    glad_glVertexAttrib4uiv = (PFNGLVERTEXATTRIB4UIVPROC) load(userptr, "glVertexAttrib4uiv");
    glad_glVertexAttrib4usv = (PFNGLVERTEXATTRIB4USVPROC) load(userptr, "glVertexAttrib4usv");
    glad_glVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC) load(userptr, "glVertexAttribPointer");
}
static void glad_gl_load_GL_VERSION_2_1( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_GL_VERSION_2_1) return;
    glad_glUniformMatrix2x3fv = (PFNGLUNIFORMMATRIX2X3FVPROC) load(userptr, "glUniformMatrix2x3fv");
    glad_glUniformMatrix2x4fv = (PFNGLUNIFORMMATRIX2X4FVPROC) load(userptr, "glUniformMatrix2x4fv");
    glad_glUniformMatrix3x2fv = (PFNGLUNIFORMMATRIX3X2FVPROC) load(userptr, "glUniformMatrix3x2fv");
    glad_glUniformMatrix3x4fv = (PFNGLUNIFORMMATRIX3X4FVPROC) load(userptr, "glUniformMatrix3x4fv");
    glad_glUniformMatrix4x2fv = (PFNGLUNIFORMMATRIX4X2FVPROC) load(userptr, "glUniformMatrix4x2fv");
    glad_glUniformMatrix4x3fv = (PFNGLUNIFORMMATRIX4X3FVPROC) load(userptr, "glUniformMatrix4x3fv");
}
static void glad_gl_load_GL_VERSION_3_0( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_GL_VERSION_3_0) return;
    glad_glBeginConditionalRender = (PFNGLBEGINCONDITIONALRENDERPROC) load(userptr, "glBeginConditionalRender");
    glad_glBeginTransformFeedback = (PFNGLBEGINTRANSFORMFEEDBACKPROC) load(userptr, "glBeginTransformFeedback");
    glad_glBindBufferBase = (PFNGLBINDBUFFERBASEPROC) load(userptr, "glBindBufferBase");
    glad_glBindBufferRange = (PFNGLBINDBUFFERRANGEPROC) load(userptr, "glBindBufferRange");
    glad_glBindFragDataLocation = (PFNGLBINDFRAGDATALOCATIONPROC) load(userptr, "glBindFragDataLocation");
    glad_glBindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC) load(userptr, "glBindFramebuffer");
    glad_glBindRenderbuffer = (PFNGLBINDRENDERBUFFERPROC) load(userptr, "glBindRenderbuffer");
    glad_glBindVertexArray = (PFNGLBINDVERTEXARRAYPROC) load(userptr, "glBindVertexArray");
    glad_glBlitFramebuffer = (PFNGLBLITFRAMEBUFFERPROC) load(userptr, "glBlitFramebuffer");
    glad_glCheckFramebufferStatus = (PFNGLCHECKFRAMEBUFFERSTATUSPROC) load(userptr, "glCheckFramebufferStatus");
    glad_glClampColor = (PFNGLCLAMPCOLORPROC) load(userptr, "glClampColor");
    glad_glClearBufferfi = (PFNGLCLEARBUFFERFIPROC) load(userptr, "glClearBufferfi");
    glad_glClearBufferfv = (PFNGLCLEARBUFFERFVPROC) load(userptr, "glClearBufferfv");
    glad_glClearBufferiv = (PFNGLCLEARBUFFERIVPROC) load(userptr, "glClearBufferiv");
    glad_glClearBufferuiv = (PFNGLCLEARBUFFERUIVPROC) load(userptr, "glClearBufferuiv");
    glad_glColorMaski = (PFNGLCOLORMASKIPROC) load(userptr, "glColorMaski");
    glad_glDeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSPROC) load(userptr, "glDeleteFramebuffers");
    glad_glDeleteRenderbuffers = (PFNGLDELETERENDERBUFFERSPROC) load(userptr, "glDeleteRenderbuffers");
    glad_glDeleteVertexArrays = (PFNGLDELETEVERTEXARRAYSPROC) load(userptr, "glDeleteVertexArrays");
    glad_glDisablei = (PFNGLDISABLEIPROC) load(userptr, "glDisablei");
    glad_glEnablei = (PFNGLENABLEIPROC) load(userptr, "glEnablei");
    glad_glEndConditionalRender = (PFNGLENDCONDITIONALRENDERPROC) load(userptr, "glEndConditionalRender");
    glad_glEndTransformFeedback = (PFNGLENDTRANSFORMFEEDBACKPROC) load(userptr, "glEndTransformFeedback");
    glad_glFlushMappedBufferRange = (PFNGLFLUSHMAPPEDBUFFERRANGEPROC) load(userptr, "glFlushMappedBufferRange");
    glad_glFramebufferRenderbuffer = (PFNGLFRAMEBUFFERRENDERBUFFERPROC) load(userptr, "glFramebufferRenderbuffer");
    glad_glFramebufferTexture1D = (PFNGLFRAMEBUFFERTEXTURE1DPROC) load(userptr, "glFramebufferTexture1D");
    glad_glFramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DPROC) load(userptr, "glFramebufferTexture2D");
    glad_glFramebufferTexture3D = (PFNGLFRAMEBUFFERTEXTURE3DPROC) load(userptr, "glFramebufferTexture3D");
    glad_glFramebufferTextureLayer = (PFNGLFRAMEBUFFERTEXTURELAYERPROC) load(userptr, "glFramebufferTextureLayer");
    glad_glGenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC) load(userptr, "glGenFramebuffers");
    glad_glGenRenderbuffers = (PFNGLGENRENDERBUFFERSPROC) load(userptr, "glGenRenderbuffers");
    glad_glGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC) load(userptr, "glGenVertexArrays");
    glad_glGenerateMipmap = (PFNGLGENERATEMIPMAPPROC) load(userptr, "glGenerateMipmap");
    glad_glGetBooleani_v = (PFNGLGETBOOLEANI_VPROC) load(userptr, "glGetBooleani_v");
    glad_glGetFragDataLocation = (PFNGLGETFRAGDATALOCATIONPROC) load(userptr, "glGetFragDataLocation");
    glad_glGetFramebufferAttachmentParameteriv = (PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC) load(userptr, "glGetFramebufferAttachmentParameteriv");
    glad_glGetIntegeri_v = (PFNGLGETINTEGERI_VPROC) load(userptr, "glGetIntegeri_v");
    glad_glGetRenderbufferParameteriv = (PFNGLGETRENDERBUFFERPARAMETERIVPROC) load(userptr, "glGetRenderbufferParameteriv");
    glad_glGetStringi = (PFNGLGETSTRINGIPROC) load(userptr, "glGetStringi");
    glad_glGetTexParameterIiv = (PFNGLGETTEXPARAMETERIIVPROC) load(userptr, "glGetTexParameterIiv");
    glad_glGetTexParameterIuiv = (PFNGLGETTEXPARAMETERIUIVPROC) load(userptr, "glGetTexParameterIuiv");
    glad_glGetTransformFeedbackVarying = (PFNGLGETTRANSFORMFEEDBACKVARYINGPROC) load(userptr, "glGetTransformFeedbackVarying");
    glad_glGetUniformuiv = (PFNGLGETUNIFORMUIVPROC) load(userptr, "glGetUniformuiv");
    glad_glGetVertexAttribIiv = (PFNGLGETVERTEXATTRIBIIVPROC) load(userptr, "glGetVertexAttribIiv");
    glad_glGetVertexAttribIuiv = (PFNGLGETVERTEXATTRIBIUIVPROC) load(userptr, "glGetVertexAttribIuiv");
    glad_glIsEnabledi = (PFNGLISENABLEDIPROC) load(userptr, "glIsEnabledi");
    glad_glIsFramebuffer = (PFNGLISFRAMEBUFFERPROC) load(userptr, "glIsFramebuffer");
    glad_glIsRenderbuffer = (PFNGLISRENDERBUFFERPROC) load(userptr, "glIsRenderbuffer");
    glad_glIsVertexArray = (PFNGLISVERTEXARRAYPROC) load(userptr, "glIsVertexArray");
    glad_glMapBufferRange = (PFNGLMAPBUFFERRANGEPROC) load(userptr, "glMapBufferRange");
    glad_glRenderbufferStorage = (PFNGLRENDERBUFFERSTORAGEPROC) load(userptr, "glRenderbufferStorage");
    glad_glRenderbufferStorageMultisample = (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC) load(userptr, "glRenderbufferStorageMultisample");
    glad_glTexParameterIiv = (PFNGLTEXPARAMETERIIVPROC) load(userptr, "glTexParameterIiv");
    glad_glTexParameterIuiv = (PFNGLTEXPARAMETERIUIVPROC) load(userptr, "glTexParameterIuiv");
    glad_glTransformFeedbackVaryings = (PFNGLTRANSFORMFEEDBACKVARYINGSPROC) load(userptr, "glTransformFeedbackVaryings");
    glad_glUniform1ui = (PFNGLUNIFORM1UIPROC) load(userptr, "glUniform1ui");
    glad_glUniform1uiv = (PFNGLUNIFORM1UIVPROC) load(userptr, "glUniform1uiv");
    glad_glUniform2ui = (PFNGLUNIFORM2UIPROC) load(userptr, "glUniform2ui");
    glad_glUniform2uiv = (PFNGLUNIFORM2UIVPROC) load(userptr, "glUniform2uiv");
    glad_glUniform3ui = (PFNGLUNIFORM3UIPROC) load(userptr, "glUniform3ui");
    glad_glUniform3uiv = (PFNGLUNIFORM3UIVPROC) load(userptr, "glUniform3uiv");
    glad_glUniform4ui = (PFNGLUNIFORM4UIPROC) load(userptr, "glUniform4ui");
    glad_glUniform4uiv = (PFNGLUNIFORM4UIVPROC) load(userptr, "glUniform4uiv");
    glad_glVertexAttribI1i = (PFNGLVERTEXATTRIBI1IPROC) load(userptr, "glVertexAttribI1i");
    glad_glVertexAttribI1iv = (PFNGLVERTEXATTRIBI1IVPROC) load(userptr, "glVertexAttribI1iv");
    glad_glVertexAttribI1ui = (PFNGLVERTEXATTRIBI1UIPROC) load(userptr, "glVertexAttribI1ui");
    glad_glVertexAttribI1uiv = (PFNGLVERTEXATTRIBI1UIVPROC) load(userptr, "glVertexAttribI1uiv");
    glad_glVertexAttribI2i = (PFNGLVERTEXATTRIBI2IPROC) load(userptr, "glVertexAttribI2i");
    glad_glVertexAttribI2iv = (PFNGLVERTEXATTRIBI2IVPROC) load(userptr, "glVertexAttribI2iv");
    glad_glVertexAttribI2ui = (PFNGLVERTEXATTRIBI2UIPROC) load(userptr, "glVertexAttribI2ui");
    glad_glVertexAttribI2uiv = (PFNGLVERTEXATTRIBI2UIVPROC) load(userptr, "glVertexAttribI2uiv");
    glad_glVertexAttribI3i = (PFNGLVERTEXATTRIBI3IPROC) load(userptr, "glVertexAttribI3i");
    glad_glVertexAttribI3iv = (PFNGLVERTEXATTRIBI3IVPROC) load(userptr, "glVertexAttribI3iv");
    glad_glVertexAttribI3ui = (PFNGLVERTEXATTRIBI3UIPROC) load(userptr, "glVertexAttribI3ui");
    glad_glVertexAttribI3uiv = (PFNGLVERTEXATTRIBI3UIVPROC) load(userptr, "glVertexAttribI3uiv");
    glad_glVertexAttribI4bv = (PFNGLVERTEXATTRIBI4BVPROC) load(userptr, "glVertexAttribI4bv");
    glad_glVertexAttribI4i = (PFNGLVERTEXATTRIBI4IPROC) load(userptr, "glVertexAttribI4i");
    glad_glVertexAttribI4iv = (PFNGLVERTEXATTRIBI4IVPROC) load(userptr, "glVertexAttribI4iv");
    glad_glVertexAttribI4sv = (PFNGLVERTEXATTRIBI4SVPROC) load(userptr, "glVertexAttribI4sv");
    glad_glVertexAttribI4ubv = (PFNGLVERTEXATTRIBI4UBVPROC) load(userptr, "glVertexAttribI4ubv");
    glad_glVertexAttribI4ui = (PFNGLVERTEXATTRIBI4UIPROC) load(userptr, "glVertexAttribI4ui");
    glad_glVertexAttribI4uiv = (PFNGLVERTEXATTRIBI4UIVPROC) load(userptr, "glVertexAttribI4uiv");
    glad_glVertexAttribI4usv = (PFNGLVERTEXATTRIBI4USVPROC) load(userptr, "glVertexAttribI4usv");
    glad_glVertexAttribIPointer = (PFNGLVERTEXATTRIBIPOINTERPROC) load(userptr, "glVertexAttribIPointer");
}
static void glad_gl_load_GL_VERSION_3_1( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_GL_VERSION_3_1) return;
    glad_glBindBufferBase = (PFNGLBINDBUFFERBASEPROC) load(userptr, "glBindBufferBase");
    glad_glBindBufferRange = (PFNGLBINDBUFFERRANGEPROC) load(userptr, "glBindBufferRange");
    glad_glCopyBufferSubData = (PFNGLCOPYBUFFERSUBDATAPROC) load(userptr, "glCopyBufferSubData");
    glad_glDrawArraysInstanced = (PFNGLDRAWARRAYSINSTANCEDPROC) load(userptr, "glDrawArraysInstanced");
    glad_glDrawElementsInstanced = (PFNGLDRAWELEMENTSINSTANCEDPROC) load(userptr, "glDrawElementsInstanced");
    glad_glGetActiveUniformBlockName = (PFNGLGETACTIVEUNIFORMBLOCKNAMEPROC) load(userptr, "glGetActiveUniformBlockName");
    glad_glGetActiveUniformBlockiv = (PFNGLGETACTIVEUNIFORMBLOCKIVPROC) load(userptr, "glGetActiveUniformBlockiv");
    glad_glGetActiveUniformName = (PFNGLGETACTIVEUNIFORMNAMEPROC) load(userptr, "glGetActiveUniformName");
    glad_glGetActiveUniformsiv = (PFNGLGETACTIVEUNIFORMSIVPROC) load(userptr, "glGetActiveUniformsiv");
    glad_glGetIntegeri_v = (PFNGLGETINTEGERI_VPROC) load(userptr, "glGetIntegeri_v");
    glad_glGetUniformBlockIndex = (PFNGLGETUNIFORMBLOCKINDEXPROC) load(userptr, "glGetUniformBlockIndex");
    glad_glGetUniformIndices = (PFNGLGETUNIFORMINDICESPROC) load(userptr, "glGetUniformIndices");
    glad_glPrimitiveRestartIndex = (PFNGLPRIMITIVERESTARTINDEXPROC) load(userptr, "glPrimitiveRestartIndex");
    glad_glTexBuffer = (PFNGLTEXBUFFERPROC) load(userptr, "glTexBuffer");
    glad_glUniformBlockBinding = (PFNGLUNIFORMBLOCKBINDINGPROC) load(userptr, "glUniformBlockBinding");
}
static void glad_gl_load_GL_VERSION_3_2( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_GL_VERSION_3_2) return;
    glad_glClientWaitSync = (PFNGLCLIENTWAITSYNCPROC) load(userptr, "glClientWaitSync");
    glad_glDeleteSync = (PFNGLDELETESYNCPROC) load(userptr, "glDeleteSync");
    glad_glDrawElementsBaseVertex = (PFNGLDRAWELEMENTSBASEVERTEXPROC) load(userptr, "glDrawElementsBaseVertex");
    glad_glDrawElementsInstancedBaseVertex = (PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXPROC) load(userptr, "glDrawElementsInstancedBaseVertex");
    glad_glDrawRangeElementsBaseVertex = (PFNGLDRAWRANGEELEMENTSBASEVERTEXPROC) load(userptr, "glDrawRangeElementsBaseVertex");
    glad_glFenceSync = (PFNGLFENCESYNCPROC) load(userptr, "glFenceSync");
    glad_glFramebufferTexture = (PFNGLFRAMEBUFFERTEXTUREPROC) load(userptr, "glFramebufferTexture");
    glad_glGetBufferParameteri64v = (PFNGLGETBUFFERPARAMETERI64VPROC) load(userptr, "glGetBufferParameteri64v");
    glad_glGetInteger64i_v = (PFNGLGETINTEGER64I_VPROC) load(userptr, "glGetInteger64i_v");
    glad_glGetInteger64v = (PFNGLGETINTEGER64VPROC) load(userptr, "glGetInteger64v");
    glad_glGetMultisamplefv = (PFNGLGETMULTISAMPLEFVPROC) load(userptr, "glGetMultisamplefv");
    glad_glGetSynciv = (PFNGLGETSYNCIVPROC) load(userptr, "glGetSynciv");
    glad_glIsSync = (PFNGLISSYNCPROC) load(userptr, "glIsSync");
    glad_glMultiDrawElementsBaseVertex = (PFNGLMULTIDRAWELEMENTSBASEVERTEXPROC) load(userptr, "glMultiDrawElementsBaseVertex");
    glad_glProvokingVertex = (PFNGLPROVOKINGVERTEXPROC) load(userptr, "glProvokingVertex");
    glad_glSampleMaski = (PFNGLSAMPLEMASKIPROC) load(userptr, "glSampleMaski");
    glad_glTexImage2DMultisample = (PFNGLTEXIMAGE2DMULTISAMPLEPROC) load(userptr, "glTexImage2DMultisample");
    glad_glTexImage3DMultisample = (PFNGLTEXIMAGE3DMULTISAMPLEPROC) load(userptr, "glTexImage3DMultisample");
    glad_glWaitSync = (PFNGLWAITSYNCPROC) load(userptr, "glWaitSync");
}
static void glad_gl_load_GL_VERSION_3_3( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_GL_VERSION_3_3) return;
    glad_glBindFragDataLocationIndexed = (PFNGLBINDFRAGDATALOCATIONINDEXEDPROC) load(userptr, "glBindFragDataLocationIndexed");
    glad_glBindSampler = (PFNGLBINDSAMPLERPROC) load(userptr, "glBindSampler");
    glad_glDeleteSamplers = (PFNGLDELETESAMPLERSPROC) load(userptr, "glDeleteSamplers");
    glad_glGenSamplers = (PFNGLGENSAMPLERSPROC) load(userptr, "glGenSamplers");
    glad_glGetFragDataIndex = (PFNGLGETFRAGDATAINDEXPROC) load(userptr, "glGetFragDataIndex");
    glad_glGetQueryObjecti64v = (PFNGLGETQUERYOBJECTI64VPROC) load(userptr, "glGetQueryObjecti64v");
    glad_glGetQueryObjectui64v = (PFNGLGETQUERYOBJECTUI64VPROC) load(userptr, "glGetQueryObjectui64v");
    glad_glGetSamplerParameterIiv = (PFNGLGETSAMPLERPARAMETERIIVPROC) load(userptr, "glGetSamplerParameterIiv");
    glad_glGetSamplerParameterIuiv = (PFNGLGETSAMPLERPARAMETERIUIVPROC) load(userptr, "glGetSamplerParameterIuiv");
    glad_glGetSamplerParameterfv = (PFNGLGETSAMPLERPARAMETERFVPROC) load(userptr, "glGetSamplerParameterfv");
    glad_glGetSamplerParameteriv = (PFNGLGETSAMPLERPARAMETERIVPROC) load(userptr, "glGetSamplerParameteriv");
    glad_glIsSampler = (PFNGLISSAMPLERPROC) load(userptr, "glIsSampler");
    glad_glQueryCounter = (PFNGLQUERYCOUNTERPROC) load(userptr, "glQueryCounter");
    glad_glSamplerParameterIiv = (PFNGLSAMPLERPARAMETERIIVPROC) load(userptr, "glSamplerParameterIiv");
    glad_glSamplerParameterIuiv = (PFNGLSAMPLERPARAMETERIUIVPROC) load(userptr, "glSamplerParameterIuiv");
    glad_glSamplerParameterf = (PFNGLSAMPLERPARAMETERFPROC) load(userptr, "glSamplerParameterf");
    glad_glSamplerParameterfv = (PFNGLSAMPLERPARAMETERFVPROC) load(userptr, "glSamplerParameterfv");
    glad_glSamplerParameteri = (PFNGLSAMPLERPARAMETERIPROC) load(userptr, "glSamplerParameteri");
    glad_glSamplerParameteriv = (PFNGLSAMPLERPARAMETERIVPROC) load(userptr, "glSamplerParameteriv");
    glad_glVertexAttribDivisor = (PFNGLVERTEXATTRIBDIVISORPROC) load(userptr, "glVertexAttribDivisor");
    glad_glVertexAttribP1ui = (PFNGLVERTEXATTRIBP1UIPROC) load(userptr, "glVertexAttribP1ui");
    glad_glVertexAttribP1uiv = (PFNGLVERTEXATTRIBP1UIVPROC) load(userptr, "glVertexAttribP1uiv");
    glad_glVertexAttribP2ui = (PFNGLVERTEXATTRIBP2UIPROC) load(userptr, "glVertexAttribP2ui");
    glad_glVertexAttribP2uiv = (PFNGLVERTEXATTRIBP2UIVPROC) load(userptr, "glVertexAttribP2uiv");
    glad_glVertexAttribP3ui = (PFNGLVERTEXATTRIBP3UIPROC) load(userptr, "glVertexAttribP3ui");
    glad_glVertexAttribP3uiv = (PFNGLVERTEXATTRIBP3UIVPROC) load(userptr, "glVertexAttribP3uiv");
    glad_glVertexAttribP4ui = (PFNGLVERTEXATTRIBP4UIPROC) load(userptr, "glVertexAttribP4ui");
    glad_glVertexAttribP4uiv = (PFNGLVERTEXATTRIBP4UIVPROC) load(userptr, "glVertexAttribP4uiv");
}
static void glad_gl_load_GL_ES_VERSION_2_0( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_GL_ES_VERSION_2_0) return;
    glad_glActiveTexture = (PFNGLACTIVETEXTUREPROC) load(userptr, "glActiveTexture");
    glad_glAttachShader = (PFNGLATTACHSHADERPROC) load(userptr, "glAttachShader");
    glad_glBindAttribLocation = (PFNGLBINDATTRIBLOCATIONPROC) load(userptr, "glBindAttribLocation");
    glad_glBindBuffer = (PFNGLBINDBUFFERPROC) load(userptr, "glBindBuffer");
    glad_glBindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC) load(userptr, "glBindFramebuffer");
    glad_glBindRenderbuffer = (PFNGLBINDRENDERBUFFERPROC) load(userptr, "glBindRenderbuffer");
    glad_glBindTexture = (PFNGLBINDTEXTUREPROC) load(userptr, "glBindTexture");
    glad_glBlendColor = (PFNGLBLENDCOLORPROC) load(userptr, "glBlendColor");
    glad_glBlendEquation = (PFNGLBLENDEQUATIONPROC) load(userptr, "glBlendEquation");
    glad_glBlendEquationSeparate = (PFNGLBLENDEQUATIONSEPARATEPROC) load(userptr, "glBlendEquationSeparate");
    glad_glBlendFunc = (PFNGLBLENDFUNCPROC) load(userptr, "glBlendFunc");
    glad_glBlendFuncSeparate = (PFNGLBLENDFUNCSEPARATEPROC) load(userptr, "glBlendFuncSeparate");
    glad_glBufferData = (PFNGLBUFFERDATAPROC) load(userptr, "glBufferData");
    glad_glBufferSubData = (PFNGLBUFFERSUBDATAPROC) load(userptr, "glBufferSubData");
    glad_glCheckFramebufferStatus = (PFNGLCHECKFRAMEBUFFERSTATUSPROC) load(userptr, "glCheckFramebufferStatus");
    glad_glClear = (PFNGLCLEARPROC) load(userptr, "glClear");
    glad_glClearColor = (PFNGLCLEARCOLORPROC) load(userptr, "glClearColor");
    glad_glClearDepthf = (PFNGLCLEARDEPTHFPROC) load(userptr, "glClearDepthf");
    glad_glClearStencil = (PFNGLCLEARSTENCILPROC) load(userptr, "glClearStencil");
    glad_glColorMask = (PFNGLCOLORMASKPROC) load(userptr, "glColorMask");
    glad_glCompileShader = (PFNGLCOMPILESHADERPROC) load(userptr, "glCompileShader");
    glad_glCompressedTexImage2D = (PFNGLCOMPRESSEDTEXIMAGE2DPROC) load(userptr, "glCompressedTexImage2D");
    glad_glCompressedTexSubImage2D = (PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC) load(userptr, "glCompressedTexSubImage2D");
    glad_glCopyTexImage2D = (PFNGLCOPYTEXIMAGE2DPROC) load(userptr, "glCopyTexImage2D");
    glad_glCopyTexSubImage2D = (PFNGLCOPYTEXSUBIMAGE2DPROC) load(userptr, "glCopyTexSubImage2D");
    glad_glCreateProgram = (PFNGLCREATEPROGRAMPROC) load(userptr, "glCreateProgram");
    glad_glCreateShader = (PFNGLCREATESHADERPROC) load(userptr, "glCreateShader");
    glad_glCullFace = (PFNGLCULLFACEPROC) load(userptr, "glCullFace");
    glad_glDeleteBuffers = (PFNGLDELETEBUFFERSPROC) load(userptr, "glDeleteBuffers");
    glad_glDeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSPROC) load(userptr, "glDeleteFramebuffers");
    glad_glDeleteProgram = (PFNGLDELETEPROGRAMPROC) load(userptr, "glDeleteProgram");
    glad_glDeleteRenderbuffers = (PFNGLDELETERENDERBUFFERSPROC) load(userptr, "glDeleteRenderbuffers");
    glad_glDeleteShader = (PFNGLDELETESHADERPROC) load(userptr, "glDeleteShader");
    glad_glDeleteTextures = (PFNGLDELETETEXTURESPROC) load(userptr, "glDeleteTextures");
    glad_glDepthFunc = (PFNGLDEPTHFUNCPROC) load(userptr, "glDepthFunc");
    glad_glDepthMask = (PFNGLDEPTHMASKPROC) load(userptr, "glDepthMask");
    glad_glDepthRangef = (PFNGLDEPTHRANGEFPROC) load(userptr, "glDepthRangef");
    glad_glDetachShader = (PFNGLDETACHSHADERPROC) load(userptr, "glDetachShader");
    glad_glDisable = (PFNGLDISABLEPROC) load(userptr, "glDisable");
    glad_glDisableVertexAttribArray = (PFNGLDISABLEVERTEXATTRIBARRAYPROC) load(userptr, "glDisableVertexAttribArray");
    glad_glDrawArrays = (PFNGLDRAWARRAYSPROC) load(userptr, "glDrawArrays");
    glad_glDrawElements = (PFNGLDRAWELEMENTSPROC) load(userptr, "glDrawElements");
    glad_glEnable = (PFNGLENABLEPROC) load(userptr, "glEnable");
    glad_glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC) load(userptr, "glEnableVertexAttribArray");
    glad_glFinish = (PFNGLFINISHPROC) load(userptr, "glFinish");
    glad_glFlush = (PFNGLFLUSHPROC) load(userptr, "glFlush");
    glad_glFramebufferRenderbuffer = (PFNGLFRAMEBUFFERRENDERBUFFERPROC) load(userptr, "glFramebufferRenderbuffer");
    glad_glFramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DPROC) load(userptr, "glFramebufferTexture2D");
    glad_glFrontFace = (PFNGLFRONTFACEPROC) load(userptr, "glFrontFace");
    glad_glGenBuffers = (PFNGLGENBUFFERSPROC) load(userptr, "glGenBuffers");
    glad_glGenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC) load(userptr, "glGenFramebuffers");
    glad_glGenRenderbuffers = (PFNGLGENRENDERBUFFERSPROC) load(userptr, "glGenRenderbuffers");
    glad_glGenTextures = (PFNGLGENTEXTURESPROC) load(userptr, "glGenTextures");
    glad_glGenerateMipmap = (PFNGLGENERATEMIPMAPPROC) load(userptr, "glGenerateMipmap");
    glad_glGetActiveAttrib = (PFNGLGETACTIVEATTRIBPROC) load(userptr, "glGetActiveAttrib");
    glad_glGetActiveUniform = (PFNGLGETACTIVEUNIFORMPROC) load(userptr, "glGetActiveUniform");
    glad_glGetAttachedShaders = (PFNGLGETATTACHEDSHADERSPROC) load(userptr, "glGetAttachedShaders");
    glad_glGetAttribLocation = (PFNGLGETATTRIBLOCATIONPROC) load(userptr, "glGetAttribLocation");
    glad_glGetBooleanv = (PFNGLGETBOOLEANVPROC) load(userptr, "glGetBooleanv");
    glad_glGetBufferParameteriv = (PFNGLGETBUFFERPARAMETERIVPROC) load(userptr, "glGetBufferParameteriv");
    glad_glGetError = (PFNGLGETERRORPROC) load(userptr, "glGetError");
    glad_glGetFloatv = (PFNGLGETFLOATVPROC) load(userptr, "glGetFloatv");
    glad_glGetFramebufferAttachmentParameteriv = (PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC) load(userptr, "glGetFramebufferAttachmentParameteriv");
    glad_glGetIntegerv = (PFNGLGETINTEGERVPROC) load(userptr, "glGetIntegerv");
    glad_glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC) load(userptr, "glGetProgramInfoLog");
    glad_glGetProgramiv = (PFNGLGETPROGRAMIVPROC) load(userptr, "glGetProgramiv");
    glad_glGetRenderbufferParameteriv = (PFNGLGETRENDERBUFFERPARAMETERIVPROC) load(userptr, "glGetRenderbufferParameteriv");
    glad_glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC) load(userptr, "glGetShaderInfoLog");
    glad_glGetShaderPrecisionFormat = (PFNGLGETSHADERPRECISIONFORMATPROC) load(userptr, "glGetShaderPrecisionFormat");
    glad_glGetShaderSource = (PFNGLGETSHADERSOURCEPROC) load(userptr, "glGetShaderSource");
    glad_glGetShaderiv = (PFNGLGETSHADERIVPROC) load(userptr, "glGetShaderiv");
    glad_glGetString = (PFNGLGETSTRINGPROC) load(userptr, "glGetString");
    glad_glGetTexParameterfv = (PFNGLGETTEXPARAMETERFVPROC) load(userptr, "glGetTexParameterfv");
    glad_glGetTexParameteriv = (PFNGLGETTEXPARAMETERIVPROC) load(userptr, "glGetTexParameteriv");
    glad_glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC) load(userptr, "glGetUniformLocation");
    glad_glGetUniformfv = (PFNGLGETUNIFORMFVPROC) load(userptr, "glGetUniformfv");
    glad_glGetUniformiv = (PFNGLGETUNIFORMIVPROC) load(userptr, "glGetUniformiv");
    glad_glGetVertexAttribPointerv = (PFNGLGETVERTEXATTRIBPOINTERVPROC) load(userptr, "glGetVertexAttribPointerv");
    glad_glGetVertexAttribfv = (PFNGLGETVERTEXATTRIBFVPROC) load(userptr, "glGetVertexAttribfv");
    glad_glGetVertexAttribiv = (PFNGLGETVERTEXATTRIBIVPROC) load(userptr, "glGetVertexAttribiv");
    glad_glHint = (PFNGLHINTPROC) load(userptr, "glHint");
    glad_glIsBuffer = (PFNGLISBUFFERPROC) load(userptr, "glIsBuffer");
    glad_glIsEnabled = (PFNGLISENABLEDPROC) load(userptr, "glIsEnabled");
    glad_glIsFramebuffer = (PFNGLISFRAMEBUFFERPROC) load(userptr, "glIsFramebuffer");
    glad_glIsProgram = (PFNGLISPROGRAMPROC) load(userptr, "glIsProgram");
    glad_glIsRenderbuffer = (PFNGLISRENDERBUFFERPROC) load(userptr, "glIsRenderbuffer");
    glad_glIsShader = (PFNGLISSHADERPROC) load(userptr, "glIsShader");
    glad_glIsTexture = (PFNGLISTEXTUREPROC) load(userptr, "glIsTexture");
    glad_glLineWidth = (PFNGLLINEWIDTHPROC) load(userptr, "glLineWidth");
    glad_glLinkProgram = (PFNGLLINKPROGRAMPROC) load(userptr, "glLinkProgram");
    glad_glPixelStorei = (PFNGLPIXELSTOREIPROC) load(userptr, "glPixelStorei");
    glad_glPolygonOffset = (PFNGLPOLYGONOFFSETPROC) load(userptr, "glPolygonOffset");
    glad_glReadPixels = (PFNGLREADPIXELSPROC) load(userptr, "glReadPixels");
    glad_glReleaseShaderCompiler = (PFNGLRELEASESHADERCOMPILERPROC) load(userptr, "glReleaseShaderCompiler");
    glad_glRenderbufferStorage = (PFNGLRENDERBUFFERSTORAGEPROC) load(userptr, "glRenderbufferStorage");
    glad_glSampleCoverage = (PFNGLSAMPLECOVERAGEPROC) load(userptr, "glSampleCoverage");
    glad_glScissor = (PFNGLSCISSORPROC) load(userptr, "glScissor");
    glad_glShaderBinary = (PFNGLSHADERBINARYPROC) load(userptr, "glShaderBinary");
    glad_glShaderSource = (PFNGLSHADERSOURCEPROC) load(userptr, "glShaderSource");
    glad_glStencilFunc = (PFNGLSTENCILFUNCPROC) load(userptr, "glStencilFunc");
    glad_glStencilFuncSeparate = (PFNGLSTENCILFUNCSEPARATEPROC) load(userptr, "glStencilFuncSeparate");
    glad_glStencilMask = (PFNGLSTENCILMASKPROC) load(userptr, "glStencilMask");
    glad_glStencilMaskSeparate = (PFNGLSTENCILMASKSEPARATEPROC) load(userptr, "glStencilMaskSeparate");
    glad_glStencilOp = (PFNGLSTENCILOPPROC) load(userptr, "glStencilOp");
    glad_glStencilOpSeparate = (PFNGLSTENCILOPSEPARATEPROC) load(userptr, "glStencilOpSeparate");
    glad_glTexImage2D = (PFNGLTEXIMAGE2DPROC) load(userptr, "glTexImage2D");
    glad_glTexParameterf = (PFNGLTEXPARAMETERFPROC) load(userptr, "glTexParameterf");
    glad_glTexParameterfv = (PFNGLTEXPARAMETERFVPROC) load(userptr, "glTexParameterfv");
    glad_glTexParameteri = (PFNGLTEXPARAMETERIPROC) load(userptr, "glTexParameteri");
    glad_glTexParameteriv = (PFNGLTEXPARAMETERIVPROC) load(userptr, "glTexParameteriv");
    glad_glTexSubImage2D = (PFNGLTEXSUBIMAGE2DPROC) load(userptr, "glTexSubImage2D");
    glad_glUniform1f = (PFNGLUNIFORM1FPROC) load(userptr, "glUniform1f");
    glad_glUniform1fv = (PFNGLUNIFORM1FVPROC) load(userptr, "glUniform1fv");
    glad_glUniform1i = (PFNGLUNIFORM1IPROC) load(userptr, "glUniform1i");
    glad_glUniform1iv = (PFNGLUNIFORM1IVPROC) load(userptr, "glUniform1iv");
    glad_glUniform2f = (PFNGLUNIFORM2FPROC) load(userptr, "glUniform2f");
    glad_glUniform2fv = (PFNGLUNIFORM2FVPROC) load(userptr, "glUniform2fv");
    glad_glUniform2i = (PFNGLUNIFORM2IPROC) load(userptr, "glUniform2i");
    glad_glUniform2iv = (PFNGLUNIFORM2IVPROC) load(userptr, "glUniform2iv");
    glad_glUniform3f = (PFNGLUNIFORM3FPROC) load(userptr, "glUniform3f");
    glad_glUniform3fv = (PFNGLUNIFORM3FVPROC) load(userptr, "glUniform3fv");
    glad_glUniform3i = (PFNGLUNIFORM3IPROC) load(userptr, "glUniform3i");
    glad_glUniform3iv = (PFNGLUNIFORM3IVPROC) load(userptr, "glUniform3iv");
    glad_glUniform4f = (PFNGLUNIFORM4FPROC) load(userptr, "glUniform4f");
    glad_glUniform4fv = (PFNGLUNIFORM4FVPROC) load(userptr, "glUniform4fv");
    glad_glUniform4i = (PFNGLUNIFORM4IPROC) load(userptr, "glUniform4i");
    glad_glUniform4iv = (PFNGLUNIFORM4IVPROC) load(userptr, "glUniform4iv");
    glad_glUniformMatrix2fv = (PFNGLUNIFORMMATRIX2FVPROC) load(userptr, "glUniformMatrix2fv");
    glad_glUniformMatrix3fv = (PFNGLUNIFORMMATRIX3FVPROC) load(userptr, "glUniformMatrix3fv");
    glad_glUniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC) load(userptr, "glUniformMatrix4fv");
    glad_glUseProgram = (PFNGLUSEPROGRAMPROC) load(userptr, "glUseProgram");
    glad_glValidateProgram = (PFNGLVALIDATEPROGRAMPROC) load(userptr, "glValidateProgram");
    glad_glVertexAttrib1f = (PFNGLVERTEXATTRIB1FPROC) load(userptr, "glVertexAttrib1f");
    glad_glVertexAttrib1fv = (PFNGLVERTEXATTRIB1FVPROC) load(userptr, "glVertexAttrib1fv");
    glad_glVertexAttrib2f = (PFNGLVERTEXATTRIB2FPROC) load(userptr, "glVertexAttrib2f");
    glad_glVertexAttrib2fv = (PFNGLVERTEXATTRIB2FVPROC) load(userptr, "glVertexAttrib2fv");
    glad_glVertexAttrib3f = (PFNGLVERTEXATTRIB3FPROC) load(userptr, "glVertexAttrib3f");
    glad_glVertexAttrib3fv = (PFNGLVERTEXATTRIB3FVPROC) load(userptr, "glVertexAttrib3fv");
    glad_glVertexAttrib4f = (PFNGLVERTEXATTRIB4FPROC) load(userptr, "glVertexAttrib4f");
    glad_glVertexAttrib4fv = (PFNGLVERTEXATTRIB4FVPROC) load(userptr, "glVertexAttrib4fv");
    glad_glVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC) load(userptr, "glVertexAttribPointer");
    glad_glViewport = (PFNGLVIEWPORTPROC) load(userptr, "glViewport");
}
static void glad_gl_load_GL_ES_VERSION_3_0( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_GL_ES_VERSION_3_0) return;
    glad_glBeginQuery = (PFNGLBEGINQUERYPROC) load(userptr, "glBeginQuery");
    glad_glBeginTransformFeedback = (PFNGLBEGINTRANSFORMFEEDBACKPROC) load(userptr, "glBeginTransformFeedback");
    glad_glBindBufferBase = (PFNGLBINDBUFFERBASEPROC) load(userptr, "glBindBufferBase");
    glad_glBindBufferRange = (PFNGLBINDBUFFERRANGEPROC) load(userptr, "glBindBufferRange");
    glad_glBindSampler = (PFNGLBINDSAMPLERPROC) load(userptr, "glBindSampler");
    glad_glBindTransformFeedback = (PFNGLBINDTRANSFORMFEEDBACKPROC) load(userptr, "glBindTransformFeedback");
    glad_glBindVertexArray = (PFNGLBINDVERTEXARRAYPROC) load(userptr, "glBindVertexArray");
    glad_glBlitFramebuffer = (PFNGLBLITFRAMEBUFFERPROC) load(userptr, "glBlitFramebuffer");
    glad_glClearBufferfi = (PFNGLCLEARBUFFERFIPROC) load(userptr, "glClearBufferfi");
    glad_glClearBufferfv = (PFNGLCLEARBUFFERFVPROC) load(userptr, "glClearBufferfv");
    glad_glClearBufferiv = (PFNGLCLEARBUFFERIVPROC) load(userptr, "glClearBufferiv");
    glad_glClearBufferuiv = (PFNGLCLEARBUFFERUIVPROC) load(userptr, "glClearBufferuiv");
    glad_glClientWaitSync = (PFNGLCLIENTWAITSYNCPROC) load(userptr, "glClientWaitSync");
    glad_glCompressedTexImage3D = (PFNGLCOMPRESSEDTEXIMAGE3DPROC) load(userptr, "glCompressedTexImage3D");
    glad_glCompressedTexSubImage3D = (PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC) load(userptr, "glCompressedTexSubImage3D");
    glad_glCopyBufferSubData = (PFNGLCOPYBUFFERSUBDATAPROC) load(userptr, "glCopyBufferSubData");
    glad_glCopyTexSubImage3D = (PFNGLCOPYTEXSUBIMAGE3DPROC) load(userptr, "glCopyTexSubImage3D");
    glad_glDeleteQueries = (PFNGLDELETEQUERIESPROC) load(userptr, "glDeleteQueries");
    glad_glDeleteSamplers = (PFNGLDELETESAMPLERSPROC) load(userptr, "glDeleteSamplers");
    glad_glDeleteSync = (PFNGLDELETESYNCPROC) load(userptr, "glDeleteSync");
    glad_glDeleteTransformFeedbacks = (PFNGLDELETETRANSFORMFEEDBACKSPROC) load(userptr, "glDeleteTransformFeedbacks");
    glad_glDeleteVertexArrays = (PFNGLDELETEVERTEXARRAYSPROC) load(userptr, "glDeleteVertexArrays");
    glad_glDrawArraysInstanced = (PFNGLDRAWARRAYSINSTANCEDPROC) load(userptr, "glDrawArraysInstanced");
    glad_glDrawBuffers = (PFNGLDRAWBUFFERSPROC) load(userptr, "glDrawBuffers");
    glad_glDrawElementsInstanced = (PFNGLDRAWELEMENTSINSTANCEDPROC) load(userptr, "glDrawElementsInstanced");
    glad_glDrawRangeElements = (PFNGLDRAWRANGEELEMENTSPROC) load(userptr, "glDrawRangeElements");
    glad_glEndQuery = (PFNGLENDQUERYPROC) load(userptr, "glEndQuery");
    glad_glEndTransformFeedback = (PFNGLENDTRANSFORMFEEDBACKPROC) load(userptr, "glEndTransformFeedback");
    glad_glFenceSync = (PFNGLFENCESYNCPROC) load(userptr, "glFenceSync");
    glad_glFlushMappedBufferRange = (PFNGLFLUSHMAPPEDBUFFERRANGEPROC) load(userptr, "glFlushMappedBufferRange");
    glad_glFramebufferTextureLayer = (PFNGLFRAMEBUFFERTEXTURELAYERPROC) load(userptr, "glFramebufferTextureLayer");
    glad_glGenQueries = (PFNGLGENQUERIESPROC) load(userptr, "glGenQueries");
    glad_glGenSamplers = (PFNGLGENSAMPLERSPROC) load(userptr, "glGenSamplers");
    glad_glGenTransformFeedbacks = (PFNGLGENTRANSFORMFEEDBACKSPROC) load(userptr, "glGenTransformFeedbacks");
    glad_glGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC) load(userptr, "glGenVertexArrays");
    glad_glGetActiveUniformBlockName = (PFNGLGETACTIVEUNIFORMBLOCKNAMEPROC) load(userptr, "glGetActiveUniformBlockName");
    glad_glGetActiveUniformBlockiv = (PFNGLGETACTIVEUNIFORMBLOCKIVPROC) load(userptr, "glGetActiveUniformBlockiv");
    glad_glGetActiveUniformsiv = (PFNGLGETACTIVEUNIFORMSIVPROC) load(userptr, "glGetActiveUniformsiv");
    glad_glGetBufferParameteri64v = (PFNGLGETBUFFERPARAMETERI64VPROC) load(userptr, "glGetBufferParameteri64v");
    glad_glGetBufferPointerv = (PFNGLGETBUFFERPOINTERVPROC) load(userptr, "glGetBufferPointerv");
    glad_glGetFragDataLocation = (PFNGLGETFRAGDATALOCATIONPROC) load(userptr, "glGetFragDataLocation");
    glad_glGetInteger64i_v = (PFNGLGETINTEGER64I_VPROC) load(userptr, "glGetInteger64i_v");
    glad_glGetInteger64v = (PFNGLGETINTEGER64VPROC) load(userptr, "glGetInteger64v");
    glad_glGetIntegeri_v = (PFNGLGETINTEGERI_VPROC) load(userptr, "glGetIntegeri_v");
    glad_glGetInternalformativ = (PFNGLGETINTERNALFORMATIVPROC) load(userptr, "glGetInternalformativ");
    glad_glGetProgramBinary = (PFNGLGETPROGRAMBINARYPROC) load(userptr, "glGetProgramBinary");
    glad_glGetQueryObjectuiv = (PFNGLGETQUERYOBJECTUIVPROC) load(userptr, "glGetQueryObjectuiv");
    glad_glGetQueryiv = (PFNGLGETQUERYIVPROC) load(userptr, "glGetQueryiv");
    glad_glGetSamplerParameterfv = (PFNGLGETSAMPLERPARAMETERFVPROC) load(userptr, "glGetSamplerParameterfv");
    glad_glGetSamplerParameteriv = (PFNGLGETSAMPLERPARAMETERIVPROC) load(userptr, "glGetSamplerParameteriv");
    glad_glGetStringi = (PFNGLGETSTRINGIPROC) load(userptr, "glGetStringi");
    glad_glGetSynciv = (PFNGLGETSYNCIVPROC) load(userptr, "glGetSynciv");
    glad_glGetTransformFeedbackVarying = (PFNGLGETTRANSFORMFEEDBACKVARYINGPROC) load(userptr, "glGetTransformFeedbackVarying");
    glad_glGetUniformBlockIndex = (PFNGLGETUNIFORMBLOCKINDEXPROC) load(userptr, "glGetUniformBlockIndex");
    glad_glGetUniformIndices = (PFNGLGETUNIFORMINDICESPROC) load(userptr, "glGetUniformIndices");
    glad_glGetUniformuiv = (PFNGLGETUNIFORMUIVPROC) load(userptr, "glGetUniformuiv");
    glad_glGetVertexAttribIiv = (PFNGLGETVERTEXATTRIBIIVPROC) load(userptr, "glGetVertexAttribIiv");
    glad_glGetVertexAttribIuiv = (PFNGLGETVERTEXATTRIBIUIVPROC) load(userptr, "glGetVertexAttribIuiv");
    glad_glInvalidateFramebuffer = (PFNGLINVALIDATEFRAMEBUFFERPROC) load(userptr, "glInvalidateFramebuffer");
    glad_glInvalidateSubFramebuffer = (PFNGLINVALIDATESUBFRAMEBUFFERPROC) load(userptr, "glInvalidateSubFramebuffer");
    glad_glIsQuery = (PFNGLISQUERYPROC) load(userptr, "glIsQuery");
    glad_glIsSampler = (PFNGLISSAMPLERPROC) load(userptr, "glIsSampler");
    glad_glIsSync = (PFNGLISSYNCPROC) load(userptr, "glIsSync");
    glad_glIsTransformFeedback = (PFNGLISTRANSFORMFEEDBACKPROC) load(userptr, "glIsTransformFeedback");
    glad_glIsVertexArray = (PFNGLISVERTEXARRAYPROC) load(userptr, "glIsVertexArray");
    glad_glMapBufferRange = (PFNGLMAPBUFFERRANGEPROC) load(userptr, "glMapBufferRange");
    glad_glPauseTransformFeedback = (PFNGLPAUSETRANSFORMFEEDBACKPROC) load(userptr, "glPauseTransformFeedback");
    glad_glProgramBinary = (PFNGLPROGRAMBINARYPROC) load(userptr, "glProgramBinary");
    glad_glProgramParameteri = (PFNGLPROGRAMPARAMETERIPROC) load(userptr, "glProgramParameteri");
    glad_glReadBuffer = (PFNGLREADBUFFERPROC) load(userptr, "glReadBuffer");
    glad_glRenderbufferStorageMultisample = (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC) load(userptr, "glRenderbufferStorageMultisample");
    glad_glResumeTransformFeedback = (PFNGLRESUMETRANSFORMFEEDBACKPROC) load(userptr, "glResumeTransformFeedback");
    glad_glSamplerParameterf = (PFNGLSAMPLERPARAMETERFPROC) load(userptr, "glSamplerParameterf");
    glad_glSamplerParameterfv = (PFNGLSAMPLERPARAMETERFVPROC) load(userptr, "glSamplerParameterfv");
    glad_glSamplerParameteri = (PFNGLSAMPLERPARAMETERIPROC) load(userptr, "glSamplerParameteri");
    glad_glSamplerParameteriv = (PFNGLSAMPLERPARAMETERIVPROC) load(userptr, "glSamplerParameteriv");
    glad_glTexImage3D = (PFNGLTEXIMAGE3DPROC) load(userptr, "glTexImage3D");
    glad_glTexStorage2D = (PFNGLTEXSTORAGE2DPROC) load(userptr, "glTexStorage2D");
    glad_glTexStorage3D = (PFNGLTEXSTORAGE3DPROC) load(userptr, "glTexStorage3D");
    glad_glTexSubImage3D = (PFNGLTEXSUBIMAGE3DPROC) load(userptr, "glTexSubImage3D");
    glad_glTransformFeedbackVaryings = (PFNGLTRANSFORMFEEDBACKVARYINGSPROC) load(userptr, "glTransformFeedbackVaryings");
    glad_glUniform1ui = (PFNGLUNIFORM1UIPROC) load(userptr, "glUniform1ui");
    glad_glUniform1uiv = (PFNGLUNIFORM1UIVPROC) load(userptr, "glUniform1uiv");
    glad_glUniform2ui = (PFNGLUNIFORM2UIPROC) load(userptr, "glUniform2ui");
    glad_glUniform2uiv = (PFNGLUNIFORM2UIVPROC) load(userptr, "glUniform2uiv");
    glad_glUniform3ui = (PFNGLUNIFORM3UIPROC) load(userptr, "glUniform3ui");
    glad_glUniform3uiv = (PFNGLUNIFORM3UIVPROC) load(userptr, "glUniform3uiv");
    glad_glUniform4ui = (PFNGLUNIFORM4UIPROC) load(userptr, "glUniform4ui");
    glad_glUniform4uiv = (PFNGLUNIFORM4UIVPROC) load(userptr, "glUniform4uiv");
    glad_glUniformBlockBinding = (PFNGLUNIFORMBLOCKBINDINGPROC) load(userptr, "glUniformBlockBinding");
    glad_glUniformMatrix2x3fv = (PFNGLUNIFORMMATRIX2X3FVPROC) load(userptr, "glUniformMatrix2x3fv");
    glad_glUniformMatrix2x4fv = (PFNGLUNIFORMMATRIX2X4FVPROC) load(userptr, "glUniformMatrix2x4fv");
    glad_glUniformMatrix3x2fv = (PFNGLUNIFORMMATRIX3X2FVPROC) load(userptr, "glUniformMatrix3x2fv");
    glad_glUniformMatrix3x4fv = (PFNGLUNIFORMMATRIX3X4FVPROC) load(userptr, "glUniformMatrix3x4fv");
    glad_glUniformMatrix4x2fv = (PFNGLUNIFORMMATRIX4X2FVPROC) load(userptr, "glUniformMatrix4x2fv");
    glad_glUniformMatrix4x3fv = (PFNGLUNIFORMMATRIX4X3FVPROC) load(userptr, "glUniformMatrix4x3fv");
    glad_glUnmapBuffer = (PFNGLUNMAPBUFFERPROC) load(userptr, "glUnmapBuffer");
    glad_glVertexAttribDivisor = (PFNGLVERTEXATTRIBDIVISORPROC) load(userptr, "glVertexAttribDivisor");
    glad_glVertexAttribI4i = (PFNGLVERTEXATTRIBI4IPROC) load(userptr, "glVertexAttribI4i");
    glad_glVertexAttribI4iv = (PFNGLVERTEXATTRIBI4IVPROC) load(userptr, "glVertexAttribI4iv");
    glad_glVertexAttribI4ui = (PFNGLVERTEXATTRIBI4UIPROC) load(userptr, "glVertexAttribI4ui");
    glad_glVertexAttribI4uiv = (PFNGLVERTEXATTRIBI4UIVPROC) load(userptr, "glVertexAttribI4uiv");
    glad_glVertexAttribIPointer = (PFNGLVERTEXATTRIBIPOINTERPROC) load(userptr, "glVertexAttribIPointer");
    glad_glWaitSync = (PFNGLWAITSYNCPROC) load(userptr, "glWaitSync");
}
static void glad_gl_load_GL_ARB_blend_func_extended( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_GL_ARB_blend_func_extended) return;
    glad_glBindFragDataLocationIndexed = (PFNGLBINDFRAGDATALOCATIONINDEXEDPROC) load(userptr, "glBindFragDataLocationIndexed");
    glad_glGetFragDataIndex = (PFNGLGETFRAGDATAINDEXPROC) load(userptr, "glGetFragDataIndex");
}
static void glad_gl_load_GL_ARB_clear_texture( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_GL_ARB_clear_texture) return;
    glad_glClearTexImage = (PFNGLCLEARTEXIMAGEPROC) load(userptr, "glClearTexImage");
    glad_glClearTexSubImage = (PFNGLCLEARTEXSUBIMAGEPROC) load(userptr, "glClearTexSubImage");
}
static void glad_gl_load_GL_ARB_copy_buffer( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_GL_ARB_copy_buffer) return;
    glad_glCopyBufferSubData = (PFNGLCOPYBUFFERSUBDATAPROC) load(userptr, "glCopyBufferSubData");
}
static void glad_gl_load_GL_ARB_debug_output( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_GL_ARB_debug_output) return;
    glad_glDebugMessageCallbackARB = (PFNGLDEBUGMESSAGECALLBACKARBPROC) load(userptr, "glDebugMessageCallbackARB");
    glad_glDebugMessageControlARB = (PFNGLDEBUGMESSAGECONTROLARBPROC) load(userptr, "glDebugMessageControlARB");
    glad_glDebugMessageInsertARB = (PFNGLDEBUGMESSAGEINSERTARBPROC) load(userptr, "glDebugMessageInsertARB");
    glad_glGetDebugMessageLogARB = (PFNGLGETDEBUGMESSAGELOGARBPROC) load(userptr, "glGetDebugMessageLogARB");
}
static void glad_gl_load_GL_ARB_draw_buffers( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_GL_ARB_draw_buffers) return;
    glad_glDrawBuffersARB = (PFNGLDRAWBUFFERSARBPROC) load(userptr, "glDrawBuffersARB");
}
static void glad_gl_load_GL_ARB_draw_elements_base_vertex( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_GL_ARB_draw_elements_base_vertex) return;
    glad_glDrawElementsBaseVertex = (PFNGLDRAWELEMENTSBASEVERTEXPROC) load(userptr, "glDrawElementsBaseVertex");
    glad_glDrawElementsInstancedBaseVertex = (PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXPROC) load(userptr, "glDrawElementsInstancedBaseVertex");
    glad_glDrawRangeElementsBaseVertex = (PFNGLDRAWRANGEELEMENTSBASEVERTEXPROC) load(userptr, "glDrawRangeElementsBaseVertex");
    glad_glMultiDrawElementsBaseVertex = (PFNGLMULTIDRAWELEMENTSBASEVERTEXPROC) load(userptr, "glMultiDrawElementsBaseVertex");
}
static void glad_gl_load_GL_ARB_draw_instanced( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_GL_ARB_draw_instanced) return;
    glad_glDrawArraysInstancedARB = (PFNGLDRAWARRAYSINSTANCEDARBPROC) load(userptr, "glDrawArraysInstancedARB");
    glad_glDrawElementsInstancedARB = (PFNGLDRAWELEMENTSINSTANCEDARBPROC) load(userptr, "glDrawElementsInstancedARB");
}
static void glad_gl_load_GL_ARB_framebuffer_object( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_GL_ARB_framebuffer_object) return;
    glad_glBindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC) load(userptr, "glBindFramebuffer");
    glad_glBindRenderbuffer = (PFNGLBINDRENDERBUFFERPROC) load(userptr, "glBindRenderbuffer");
    glad_glBlitFramebuffer = (PFNGLBLITFRAMEBUFFERPROC) load(userptr, "glBlitFramebuffer");
    glad_glCheckFramebufferStatus = (PFNGLCHECKFRAMEBUFFERSTATUSPROC) load(userptr, "glCheckFramebufferStatus");
    glad_glDeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSPROC) load(userptr, "glDeleteFramebuffers");
    glad_glDeleteRenderbuffers = (PFNGLDELETERENDERBUFFERSPROC) load(userptr, "glDeleteRenderbuffers");
    glad_glFramebufferRenderbuffer = (PFNGLFRAMEBUFFERRENDERBUFFERPROC) load(userptr, "glFramebufferRenderbuffer");
    glad_glFramebufferTexture1D = (PFNGLFRAMEBUFFERTEXTURE1DPROC) load(userptr, "glFramebufferTexture1D");
    glad_glFramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DPROC) load(userptr, "glFramebufferTexture2D");
    glad_glFramebufferTexture3D = (PFNGLFRAMEBUFFERTEXTURE3DPROC) load(userptr, "glFramebufferTexture3D");
    glad_glFramebufferTextureLayer = (PFNGLFRAMEBUFFERTEXTURELAYERPROC) load(userptr, "glFramebufferTextureLayer");
    glad_glGenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC) load(userptr, "glGenFramebuffers");
    glad_glGenRenderbuffers = (PFNGLGENRENDERBUFFERSPROC) load(userptr, "glGenRenderbuffers");
    glad_glGenerateMipmap = (PFNGLGENERATEMIPMAPPROC) load(userptr, "glGenerateMipmap");
    glad_glGetFramebufferAttachmentParameteriv = (PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC) load(userptr, "glGetFramebufferAttachmentParameteriv");
    glad_glGetRenderbufferParameteriv = (PFNGLGETRENDERBUFFERPARAMETERIVPROC) load(userptr, "glGetRenderbufferParameteriv");
    glad_glIsFramebuffer = (PFNGLISFRAMEBUFFERPROC) load(userptr, "glIsFramebuffer");
    glad_glIsRenderbuffer = (PFNGLISRENDERBUFFERPROC) load(userptr, "glIsRenderbuffer");
    glad_glRenderbufferStorage = (PFNGLRENDERBUFFERSTORAGEPROC) load(userptr, "glRenderbufferStorage");
    glad_glRenderbufferStorageMultisample = (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC) load(userptr, "glRenderbufferStorageMultisample");
}
static void glad_gl_load_GL_ARB_imaging( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_GL_ARB_imaging) return;
    glad_glBlendColor = (PFNGLBLENDCOLORPROC) load(userptr, "glBlendColor");
    glad_glBlendEquation = (PFNGLBLENDEQUATIONPROC) load(userptr, "glBlendEquation");
}
static void glad_gl_load_GL_ARB_instanced_arrays( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_GL_ARB_instanced_arrays) return;
    glad_glVertexAttribDivisorARB = (PFNGLVERTEXATTRIBDIVISORARBPROC) load(userptr, "glVertexAttribDivisorARB");
}
static void glad_gl_load_GL_ARB_internalformat_query2( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_GL_ARB_internalformat_query2) return;
    glad_glGetInternalformati64v = (PFNGLGETINTERNALFORMATI64VPROC) load(userptr, "glGetInternalformati64v");
}
static void glad_gl_load_GL_ARB_map_buffer_range( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_GL_ARB_map_buffer_range) return;
    glad_glFlushMappedBufferRange = (PFNGLFLUSHMAPPEDBUFFERRANGEPROC) load(userptr, "glFlushMappedBufferRange");
    glad_glMapBufferRange = (PFNGLMAPBUFFERRANGEPROC) load(userptr, "glMapBufferRange");
}
static void glad_gl_load_GL_ARB_provoking_vertex( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_GL_ARB_provoking_vertex) return;
    glad_glProvokingVertex = (PFNGLPROVOKINGVERTEXPROC) load(userptr, "glProvokingVertex");
}
static void glad_gl_load_GL_ARB_sampler_objects( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_GL_ARB_sampler_objects) return;
    glad_glBindSampler = (PFNGLBINDSAMPLERPROC) load(userptr, "glBindSampler");
    glad_glDeleteSamplers = (PFNGLDELETESAMPLERSPROC) load(userptr, "glDeleteSamplers");
    glad_glGenSamplers = (PFNGLGENSAMPLERSPROC) load(userptr, "glGenSamplers");
    glad_glGetSamplerParameterIiv = (PFNGLGETSAMPLERPARAMETERIIVPROC) load(userptr, "glGetSamplerParameterIiv");
    glad_glGetSamplerParameterIuiv = (PFNGLGETSAMPLERPARAMETERIUIVPROC) load(userptr, "glGetSamplerParameterIuiv");
    glad_glGetSamplerParameterfv = (PFNGLGETSAMPLERPARAMETERFVPROC) load(userptr, "glGetSamplerParameterfv");
    glad_glGetSamplerParameteriv = (PFNGLGETSAMPLERPARAMETERIVPROC) load(userptr, "glGetSamplerParameteriv");
    glad_glIsSampler = (PFNGLISSAMPLERPROC) load(userptr, "glIsSampler");
    glad_glSamplerParameterIiv = (PFNGLSAMPLERPARAMETERIIVPROC) load(userptr, "glSamplerParameterIiv");
    glad_glSamplerParameterIuiv = (PFNGLSAMPLERPARAMETERIUIVPROC) load(userptr, "glSamplerParameterIuiv");
    glad_glSamplerParameterf = (PFNGLSAMPLERPARAMETERFPROC) load(userptr, "glSamplerParameterf");
    glad_glSamplerParameterfv = (PFNGLSAMPLERPARAMETERFVPROC) load(userptr, "glSamplerParameterfv");
    glad_glSamplerParameteri = (PFNGLSAMPLERPARAMETERIPROC) load(userptr, "glSamplerParameteri");
    glad_glSamplerParameteriv = (PFNGLSAMPLERPARAMETERIVPROC) load(userptr, "glSamplerParameteriv");
}
static void glad_gl_load_GL_ARB_sync( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_GL_ARB_sync) return;
    glad_glClientWaitSync = (PFNGLCLIENTWAITSYNCPROC) load(userptr, "glClientWaitSync");
    glad_glDeleteSync = (PFNGLDELETESYNCPROC) load(userptr, "glDeleteSync");
    glad_glFenceSync = (PFNGLFENCESYNCPROC) load(userptr, "glFenceSync");
    glad_glGetInteger64v = (PFNGLGETINTEGER64VPROC) load(userptr, "glGetInteger64v");
    glad_glGetSynciv = (PFNGLGETSYNCIVPROC) load(userptr, "glGetSynciv");
    glad_glIsSync = (PFNGLISSYNCPROC) load(userptr, "glIsSync");
    glad_glWaitSync = (PFNGLWAITSYNCPROC) load(userptr, "glWaitSync");
}
static void glad_gl_load_GL_ARB_texture_multisample( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_GL_ARB_texture_multisample) return;
    glad_glGetMultisamplefv = (PFNGLGETMULTISAMPLEFVPROC) load(userptr, "glGetMultisamplefv");
    glad_glSampleMaski = (PFNGLSAMPLEMASKIPROC) load(userptr, "glSampleMaski");
    glad_glTexImage2DMultisample = (PFNGLTEXIMAGE2DMULTISAMPLEPROC) load(userptr, "glTexImage2DMultisample");
    glad_glTexImage3DMultisample = (PFNGLTEXIMAGE3DMULTISAMPLEPROC) load(userptr, "glTexImage3DMultisample");
}
static void glad_gl_load_GL_ARB_timer_query( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_GL_ARB_timer_query) return;
    glad_glGetQueryObjecti64v = (PFNGLGETQUERYOBJECTI64VPROC) load(userptr, "glGetQueryObjecti64v");
    glad_glGetQueryObjectui64v = (PFNGLGETQUERYOBJECTUI64VPROC) load(userptr, "glGetQueryObjectui64v");
    glad_glQueryCounter = (PFNGLQUERYCOUNTERPROC) load(userptr, "glQueryCounter");
}
static void glad_gl_load_GL_ARB_uniform_buffer_object( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_GL_ARB_uniform_buffer_object) return;
    glad_glBindBufferBase = (PFNGLBINDBUFFERBASEPROC) load(userptr, "glBindBufferBase");
    glad_glBindBufferRange = (PFNGLBINDBUFFERRANGEPROC) load(userptr, "glBindBufferRange");
    glad_glGetActiveUniformBlockName = (PFNGLGETACTIVEUNIFORMBLOCKNAMEPROC) load(userptr, "glGetActiveUniformBlockName");
    glad_glGetActiveUniformBlockiv = (PFNGLGETACTIVEUNIFORMBLOCKIVPROC) load(userptr, "glGetActiveUniformBlockiv");
    glad_glGetActiveUniformName = (PFNGLGETACTIVEUNIFORMNAMEPROC) load(userptr, "glGetActiveUniformName");
    glad_glGetActiveUniformsiv = (PFNGLGETACTIVEUNIFORMSIVPROC) load(userptr, "glGetActiveUniformsiv");
    glad_glGetIntegeri_v = (PFNGLGETINTEGERI_VPROC) load(userptr, "glGetIntegeri_v");
    glad_glGetUniformBlockIndex = (PFNGLGETUNIFORMBLOCKINDEXPROC) load(userptr, "glGetUniformBlockIndex");
    glad_glGetUniformIndices = (PFNGLGETUNIFORMINDICESPROC) load(userptr, "glGetUniformIndices");
    glad_glUniformBlockBinding = (PFNGLUNIFORMBLOCKBINDINGPROC) load(userptr, "glUniformBlockBinding");
}
static void glad_gl_load_GL_ARB_vertex_array_object( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_GL_ARB_vertex_array_object) return;
    glad_glBindVertexArray = (PFNGLBINDVERTEXARRAYPROC) load(userptr, "glBindVertexArray");
    glad_glDeleteVertexArrays = (PFNGLDELETEVERTEXARRAYSPROC) load(userptr, "glDeleteVertexArrays");
    glad_glGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC) load(userptr, "glGenVertexArrays");
    glad_glIsVertexArray = (PFNGLISVERTEXARRAYPROC) load(userptr, "glIsVertexArray");
}
static void glad_gl_load_GL_ARB_vertex_type_2_10_10_10_rev( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_GL_ARB_vertex_type_2_10_10_10_rev) return;
    glad_glVertexAttribP1ui = (PFNGLVERTEXATTRIBP1UIPROC) load(userptr, "glVertexAttribP1ui");
    glad_glVertexAttribP1uiv = (PFNGLVERTEXATTRIBP1UIVPROC) load(userptr, "glVertexAttribP1uiv");
    glad_glVertexAttribP2ui = (PFNGLVERTEXATTRIBP2UIPROC) load(userptr, "glVertexAttribP2ui");
    glad_glVertexAttribP2uiv = (PFNGLVERTEXATTRIBP2UIVPROC) load(userptr, "glVertexAttribP2uiv");
    glad_glVertexAttribP3ui = (PFNGLVERTEXATTRIBP3UIPROC) load(userptr, "glVertexAttribP3ui");
    glad_glVertexAttribP3uiv = (PFNGLVERTEXATTRIBP3UIVPROC) load(userptr, "glVertexAttribP3uiv");
    glad_glVertexAttribP4ui = (PFNGLVERTEXATTRIBP4UIPROC) load(userptr, "glVertexAttribP4ui");
    glad_glVertexAttribP4uiv = (PFNGLVERTEXATTRIBP4UIVPROC) load(userptr, "glVertexAttribP4uiv");
}
static void glad_gl_load_GL_ARB_viewport_array( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_GL_ARB_viewport_array) return;
    glad_glDepthRangeArraydvNV = (PFNGLDEPTHRANGEARRAYDVNVPROC) load(userptr, "glDepthRangeArraydvNV");
    glad_glDepthRangeArrayv = (PFNGLDEPTHRANGEARRAYVPROC) load(userptr, "glDepthRangeArrayv");
    glad_glDepthRangeIndexed = (PFNGLDEPTHRANGEINDEXEDPROC) load(userptr, "glDepthRangeIndexed");
    glad_glDepthRangeIndexeddNV = (PFNGLDEPTHRANGEINDEXEDDNVPROC) load(userptr, "glDepthRangeIndexeddNV");
    glad_glGetDoublei_v = (PFNGLGETDOUBLEI_VPROC) load(userptr, "glGetDoublei_v");
    glad_glGetFloati_v = (PFNGLGETFLOATI_VPROC) load(userptr, "glGetFloati_v");
    glad_glScissorArrayv = (PFNGLSCISSORARRAYVPROC) load(userptr, "glScissorArrayv");
    glad_glScissorIndexed = (PFNGLSCISSORINDEXEDPROC) load(userptr, "glScissorIndexed");
    glad_glScissorIndexedv = (PFNGLSCISSORINDEXEDVPROC) load(userptr, "glScissorIndexedv");
    glad_glViewportArrayv = (PFNGLVIEWPORTARRAYVPROC) load(userptr, "glViewportArrayv");
    glad_glViewportIndexedf = (PFNGLVIEWPORTINDEXEDFPROC) load(userptr, "glViewportIndexedf");
    glad_glViewportIndexedfv = (PFNGLVIEWPORTINDEXEDFVPROC) load(userptr, "glViewportIndexedfv");
}
static void glad_gl_load_GL_ATI_draw_buffers( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_GL_ATI_draw_buffers) return;
    glad_glDrawBuffersATI = (PFNGLDRAWBUFFERSATIPROC) load(userptr, "glDrawBuffersATI");
}
static void glad_gl_load_GL_EXT_draw_instanced( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_GL_EXT_draw_instanced) return;
    glad_glDrawArraysInstancedEXT = (PFNGLDRAWARRAYSINSTANCEDEXTPROC) load(userptr, "glDrawArraysInstancedEXT");
    glad_glDrawElementsInstancedEXT = (PFNGLDRAWELEMENTSINSTANCEDEXTPROC) load(userptr, "glDrawElementsInstancedEXT");
}
static void glad_gl_load_GL_KHR_debug( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_GL_KHR_debug) return;
    glad_glDebugMessageCallback = (PFNGLDEBUGMESSAGECALLBACKPROC) load(userptr, "glDebugMessageCallback");
    glad_glDebugMessageCallbackKHR = (PFNGLDEBUGMESSAGECALLBACKKHRPROC) load(userptr, "glDebugMessageCallbackKHR");
    glad_glDebugMessageControl = (PFNGLDEBUGMESSAGECONTROLPROC) load(userptr, "glDebugMessageControl");
    glad_glDebugMessageControlKHR = (PFNGLDEBUGMESSAGECONTROLKHRPROC) load(userptr, "glDebugMessageControlKHR");
    glad_glDebugMessageInsert = (PFNGLDEBUGMESSAGEINSERTPROC) load(userptr, "glDebugMessageInsert");
    glad_glDebugMessageInsertKHR = (PFNGLDEBUGMESSAGEINSERTKHRPROC) load(userptr, "glDebugMessageInsertKHR");
    glad_glGetDebugMessageLog = (PFNGLGETDEBUGMESSAGELOGPROC) load(userptr, "glGetDebugMessageLog");
    glad_glGetDebugMessageLogKHR = (PFNGLGETDEBUGMESSAGELOGKHRPROC) load(userptr, "glGetDebugMessageLogKHR");
    glad_glGetObjectLabel = (PFNGLGETOBJECTLABELPROC) load(userptr, "glGetObjectLabel");
    glad_glGetObjectLabelKHR = (PFNGLGETOBJECTLABELKHRPROC) load(userptr, "glGetObjectLabelKHR");
    glad_glGetObjectPtrLabel = (PFNGLGETOBJECTPTRLABELPROC) load(userptr, "glGetObjectPtrLabel");
    glad_glGetObjectPtrLabelKHR = (PFNGLGETOBJECTPTRLABELKHRPROC) load(userptr, "glGetObjectPtrLabelKHR");
    glad_glGetPointerv = (PFNGLGETPOINTERVPROC) load(userptr, "glGetPointerv");
    glad_glGetPointervKHR = (PFNGLGETPOINTERVKHRPROC) load(userptr, "glGetPointervKHR");
    glad_glObjectLabel = (PFNGLOBJECTLABELPROC) load(userptr, "glObjectLabel");
    glad_glObjectLabelKHR = (PFNGLOBJECTLABELKHRPROC) load(userptr, "glObjectLabelKHR");
    glad_glObjectPtrLabel = (PFNGLOBJECTPTRLABELPROC) load(userptr, "glObjectPtrLabel");
    glad_glObjectPtrLabelKHR = (PFNGLOBJECTPTRLABELKHRPROC) load(userptr, "glObjectPtrLabelKHR");
    glad_glPopDebugGroup = (PFNGLPOPDEBUGGROUPPROC) load(userptr, "glPopDebugGroup");
    glad_glPopDebugGroupKHR = (PFNGLPOPDEBUGGROUPKHRPROC) load(userptr, "glPopDebugGroupKHR");
    glad_glPushDebugGroup = (PFNGLPUSHDEBUGGROUPPROC) load(userptr, "glPushDebugGroup");
    glad_glPushDebugGroupKHR = (PFNGLPUSHDEBUGGROUPKHRPROC) load(userptr, "glPushDebugGroupKHR");
}
static void glad_gl_load_GL_ANGLE_instanced_arrays( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_GL_ANGLE_instanced_arrays) return;
    glad_glDrawArraysInstancedANGLE = (PFNGLDRAWARRAYSINSTANCEDANGLEPROC) load(userptr, "glDrawArraysInstancedANGLE");
    glad_glDrawElementsInstancedANGLE = (PFNGLDRAWELEMENTSINSTANCEDANGLEPROC) load(userptr, "glDrawElementsInstancedANGLE");
    glad_glVertexAttribDivisorANGLE = (PFNGLVERTEXATTRIBDIVISORANGLEPROC) load(userptr, "glVertexAttribDivisorANGLE");
}
static void glad_gl_load_GL_ANGLE_translated_shader_source( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_GL_ANGLE_translated_shader_source) return;
    glad_glGetTranslatedShaderSourceANGLE = (PFNGLGETTRANSLATEDSHADERSOURCEANGLEPROC) load(userptr, "glGetTranslatedShaderSourceANGLE");
}
static void glad_gl_load_GL_EXT_clear_texture( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_GL_EXT_clear_texture) return;
    glad_glClearTexImageEXT = (PFNGLCLEARTEXIMAGEEXTPROC) load(userptr, "glClearTexImageEXT");
    glad_glClearTexSubImageEXT = (PFNGLCLEARTEXSUBIMAGEEXTPROC) load(userptr, "glClearTexSubImageEXT");
}
static void glad_gl_load_GL_EXT_draw_buffers( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_GL_EXT_draw_buffers) return;
    glad_glDrawBuffersEXT = (PFNGLDRAWBUFFERSEXTPROC) load(userptr, "glDrawBuffersEXT");
}
static void glad_gl_load_GL_EXT_instanced_arrays( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_GL_EXT_instanced_arrays) return;
    glad_glDrawArraysInstancedEXT = (PFNGLDRAWARRAYSINSTANCEDEXTPROC) load(userptr, "glDrawArraysInstancedEXT");
    glad_glDrawElementsInstancedEXT = (PFNGLDRAWELEMENTSINSTANCEDEXTPROC) load(userptr, "glDrawElementsInstancedEXT");
    glad_glVertexAttribDivisorEXT = (PFNGLVERTEXATTRIBDIVISOREXTPROC) load(userptr, "glVertexAttribDivisorEXT");
}
static void glad_gl_load_GL_NV_draw_instanced( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_GL_NV_draw_instanced) return;
    glad_glDrawArraysInstancedNV = (PFNGLDRAWARRAYSINSTANCEDNVPROC) load(userptr, "glDrawArraysInstancedNV");
    glad_glDrawElementsInstancedNV = (PFNGLDRAWELEMENTSINSTANCEDNVPROC) load(userptr, "glDrawElementsInstancedNV");
}
static void glad_gl_load_GL_NV_instanced_arrays( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_GL_NV_instanced_arrays) return;
    glad_glVertexAttribDivisorNV = (PFNGLVERTEXATTRIBDIVISORNVPROC) load(userptr, "glVertexAttribDivisorNV");
}
static void glad_gl_load_GL_OES_draw_buffers_indexed( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_GL_OES_draw_buffers_indexed) return;
    glad_glBlendEquationSeparateiOES = (PFNGLBLENDEQUATIONSEPARATEIOESPROC) load(userptr, "glBlendEquationSeparateiOES");
    glad_glBlendEquationiOES = (PFNGLBLENDEQUATIONIOESPROC) load(userptr, "glBlendEquationiOES");
    glad_glBlendFuncSeparateiOES = (PFNGLBLENDFUNCSEPARATEIOESPROC) load(userptr, "glBlendFuncSeparateiOES");
    glad_glBlendFunciOES = (PFNGLBLENDFUNCIOESPROC) load(userptr, "glBlendFunciOES");
    glad_glColorMaskiOES = (PFNGLCOLORMASKIOESPROC) load(userptr, "glColorMaskiOES");
    glad_glDisableiOES = (PFNGLDISABLEIOESPROC) load(userptr, "glDisableiOES");
    glad_glEnableiOES = (PFNGLENABLEIOESPROC) load(userptr, "glEnableiOES");
    glad_glIsEnablediOES = (PFNGLISENABLEDIOESPROC) load(userptr, "glIsEnablediOES");
}
static void glad_gl_load_GL_OES_vertex_array_object( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_GL_OES_vertex_array_object) return;
    glad_glBindVertexArrayOES = (PFNGLBINDVERTEXARRAYOESPROC) load(userptr, "glBindVertexArrayOES");
    glad_glDeleteVertexArraysOES = (PFNGLDELETEVERTEXARRAYSOESPROC) load(userptr, "glDeleteVertexArraysOES");
    glad_glGenVertexArraysOES = (PFNGLGENVERTEXARRAYSOESPROC) load(userptr, "glGenVertexArraysOES");
    glad_glIsVertexArrayOES = (PFNGLISVERTEXARRAYOESPROC) load(userptr, "glIsVertexArrayOES");
}
static void glad_gl_load_GL_OES_viewport_array( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_GL_OES_viewport_array) return;
    glad_glDepthRangeArrayfvOES = (PFNGLDEPTHRANGEARRAYFVOESPROC) load(userptr, "glDepthRangeArrayfvOES");
    glad_glDepthRangeIndexedfOES = (PFNGLDEPTHRANGEINDEXEDFOESPROC) load(userptr, "glDepthRangeIndexedfOES");
    glad_glDisableiOES = (PFNGLDISABLEIOESPROC) load(userptr, "glDisableiOES");
    glad_glEnableiOES = (PFNGLENABLEIOESPROC) load(userptr, "glEnableiOES");
    glad_glGetFloati_vOES = (PFNGLGETFLOATI_VOESPROC) load(userptr, "glGetFloati_vOES");
    glad_glIsEnablediOES = (PFNGLISENABLEDIOESPROC) load(userptr, "glIsEnablediOES");
    glad_glScissorArrayvOES = (PFNGLSCISSORARRAYVOESPROC) load(userptr, "glScissorArrayvOES");
    glad_glScissorIndexedOES = (PFNGLSCISSORINDEXEDOESPROC) load(userptr, "glScissorIndexedOES");
    glad_glScissorIndexedvOES = (PFNGLSCISSORINDEXEDVOESPROC) load(userptr, "glScissorIndexedvOES");
    glad_glViewportArrayvOES = (PFNGLVIEWPORTARRAYVOESPROC) load(userptr, "glViewportArrayvOES");
    glad_glViewportIndexedfOES = (PFNGLVIEWPORTINDEXEDFOESPROC) load(userptr, "glViewportIndexedfOES");
    glad_glViewportIndexedfvOES = (PFNGLVIEWPORTINDEXEDFVOESPROC) load(userptr, "glViewportIndexedfvOES");
}


static void glad_gl_resolve_aliases(void) {
    if (glad_glBindVertexArray == NULL && glad_glBindVertexArrayOES != NULL) glad_glBindVertexArray = (PFNGLBINDVERTEXARRAYPROC)glad_glBindVertexArrayOES;
    if (glad_glBindVertexArrayOES == NULL && glad_glBindVertexArray != NULL) glad_glBindVertexArrayOES = (PFNGLBINDVERTEXARRAYOESPROC)glad_glBindVertexArray;
    if (glad_glClearTexImage == NULL && glad_glClearTexImageEXT != NULL) glad_glClearTexImage = (PFNGLCLEARTEXIMAGEPROC)glad_glClearTexImageEXT;
    if (glad_glClearTexImageEXT == NULL && glad_glClearTexImage != NULL) glad_glClearTexImageEXT = (PFNGLCLEARTEXIMAGEEXTPROC)glad_glClearTexImage;
    if (glad_glClearTexSubImage == NULL && glad_glClearTexSubImageEXT != NULL) glad_glClearTexSubImage = (PFNGLCLEARTEXSUBIMAGEPROC)glad_glClearTexSubImageEXT;
    if (glad_glClearTexSubImageEXT == NULL && glad_glClearTexSubImage != NULL) glad_glClearTexSubImageEXT = (PFNGLCLEARTEXSUBIMAGEEXTPROC)glad_glClearTexSubImage;
    if (glad_glColorMaski == NULL && glad_glColorMaskiOES != NULL) glad_glColorMaski = (PFNGLCOLORMASKIPROC)glad_glColorMaskiOES;
    if (glad_glColorMaskiOES == NULL && glad_glColorMaski != NULL) glad_glColorMaskiOES = (PFNGLCOLORMASKIOESPROC)glad_glColorMaski;
    if (glad_glDebugMessageCallback == NULL && glad_glDebugMessageCallbackARB != NULL) glad_glDebugMessageCallback = (PFNGLDEBUGMESSAGECALLBACKPROC)glad_glDebugMessageCallbackARB;
    if (glad_glDebugMessageCallback == NULL && glad_glDebugMessageCallbackKHR != NULL) glad_glDebugMessageCallback = (PFNGLDEBUGMESSAGECALLBACKPROC)glad_glDebugMessageCallbackKHR;
    if (glad_glDebugMessageCallbackARB == NULL && glad_glDebugMessageCallback != NULL) glad_glDebugMessageCallbackARB = (PFNGLDEBUGMESSAGECALLBACKARBPROC)glad_glDebugMessageCallback;
    if (glad_glDebugMessageCallbackARB == NULL && glad_glDebugMessageCallbackKHR != NULL) glad_glDebugMessageCallbackARB = (PFNGLDEBUGMESSAGECALLBACKARBPROC)glad_glDebugMessageCallbackKHR;
    if (glad_glDebugMessageCallbackKHR == NULL && glad_glDebugMessageCallback != NULL) glad_glDebugMessageCallbackKHR = (PFNGLDEBUGMESSAGECALLBACKKHRPROC)glad_glDebugMessageCallback;
    if (glad_glDebugMessageCallbackKHR == NULL && glad_glDebugMessageCallbackARB != NULL) glad_glDebugMessageCallbackKHR = (PFNGLDEBUGMESSAGECALLBACKKHRPROC)glad_glDebugMessageCallbackARB;
    if (glad_glDebugMessageControl == NULL && glad_glDebugMessageControlARB != NULL) glad_glDebugMessageControl = (PFNGLDEBUGMESSAGECONTROLPROC)glad_glDebugMessageControlARB;
    if (glad_glDebugMessageControl == NULL && glad_glDebugMessageControlKHR != NULL) glad_glDebugMessageControl = (PFNGLDEBUGMESSAGECONTROLPROC)glad_glDebugMessageControlKHR;
    if (glad_glDebugMessageControlARB == NULL && glad_glDebugMessageControl != NULL) glad_glDebugMessageControlARB = (PFNGLDEBUGMESSAGECONTROLARBPROC)glad_glDebugMessageControl;
    if (glad_glDebugMessageControlARB == NULL && glad_glDebugMessageControlKHR != NULL) glad_glDebugMessageControlARB = (PFNGLDEBUGMESSAGECONTROLARBPROC)glad_glDebugMessageControlKHR;
    if (glad_glDebugMessageControlKHR == NULL && glad_glDebugMessageControl != NULL) glad_glDebugMessageControlKHR = (PFNGLDEBUGMESSAGECONTROLKHRPROC)glad_glDebugMessageControl;
    if (glad_glDebugMessageControlKHR == NULL && glad_glDebugMessageControlARB != NULL) glad_glDebugMessageControlKHR = (PFNGLDEBUGMESSAGECONTROLKHRPROC)glad_glDebugMessageControlARB;
    if (glad_glDebugMessageInsert == NULL && glad_glDebugMessageInsertARB != NULL) glad_glDebugMessageInsert = (PFNGLDEBUGMESSAGEINSERTPROC)glad_glDebugMessageInsertARB;
    if (glad_glDebugMessageInsert == NULL && glad_glDebugMessageInsertKHR != NULL) glad_glDebugMessageInsert = (PFNGLDEBUGMESSAGEINSERTPROC)glad_glDebugMessageInsertKHR;
    if (glad_glDebugMessageInsertARB == NULL && glad_glDebugMessageInsert != NULL) glad_glDebugMessageInsertARB = (PFNGLDEBUGMESSAGEINSERTARBPROC)glad_glDebugMessageInsert;
    if (glad_glDebugMessageInsertARB == NULL && glad_glDebugMessageInsertKHR != NULL) glad_glDebugMessageInsertARB = (PFNGLDEBUGMESSAGEINSERTARBPROC)glad_glDebugMessageInsertKHR;
    if (glad_glDebugMessageInsertKHR == NULL && glad_glDebugMessageInsert != NULL) glad_glDebugMessageInsertKHR = (PFNGLDEBUGMESSAGEINSERTKHRPROC)glad_glDebugMessageInsert;
    if (glad_glDebugMessageInsertKHR == NULL && glad_glDebugMessageInsertARB != NULL) glad_glDebugMessageInsertKHR = (PFNGLDEBUGMESSAGEINSERTKHRPROC)glad_glDebugMessageInsertARB;
    if (glad_glDeleteVertexArrays == NULL && glad_glDeleteVertexArraysOES != NULL) glad_glDeleteVertexArrays = (PFNGLDELETEVERTEXARRAYSPROC)glad_glDeleteVertexArraysOES;
    if (glad_glDeleteVertexArraysOES == NULL && glad_glDeleteVertexArrays != NULL) glad_glDeleteVertexArraysOES = (PFNGLDELETEVERTEXARRAYSOESPROC)glad_glDeleteVertexArrays;
    if (glad_glDisablei == NULL && glad_glDisableiOES != NULL) glad_glDisablei = (PFNGLDISABLEIPROC)glad_glDisableiOES;
    if (glad_glDisableiOES == NULL && glad_glDisablei != NULL) glad_glDisableiOES = (PFNGLDISABLEIOESPROC)glad_glDisablei;
    if (glad_glDrawArraysInstanced == NULL && glad_glDrawArraysInstancedANGLE != NULL) glad_glDrawArraysInstanced = (PFNGLDRAWARRAYSINSTANCEDPROC)glad_glDrawArraysInstancedANGLE;
    if (glad_glDrawArraysInstanced == NULL && glad_glDrawArraysInstancedARB != NULL) glad_glDrawArraysInstanced = (PFNGLDRAWARRAYSINSTANCEDPROC)glad_glDrawArraysInstancedARB;
    if (glad_glDrawArraysInstanced == NULL && glad_glDrawArraysInstancedEXT != NULL) glad_glDrawArraysInstanced = (PFNGLDRAWARRAYSINSTANCEDPROC)glad_glDrawArraysInstancedEXT;
    if (glad_glDrawArraysInstanced == NULL && glad_glDrawArraysInstancedNV != NULL) glad_glDrawArraysInstanced = (PFNGLDRAWARRAYSINSTANCEDPROC)glad_glDrawArraysInstancedNV;
    if (glad_glDrawArraysInstancedANGLE == NULL && glad_glDrawArraysInstanced != NULL) glad_glDrawArraysInstancedANGLE = (PFNGLDRAWARRAYSINSTANCEDANGLEPROC)glad_glDrawArraysInstanced;
    if (glad_glDrawArraysInstancedANGLE == NULL && glad_glDrawArraysInstancedARB != NULL) glad_glDrawArraysInstancedANGLE = (PFNGLDRAWARRAYSINSTANCEDANGLEPROC)glad_glDrawArraysInstancedARB;
    if (glad_glDrawArraysInstancedANGLE == NULL && glad_glDrawArraysInstancedEXT != NULL) glad_glDrawArraysInstancedANGLE = (PFNGLDRAWARRAYSINSTANCEDANGLEPROC)glad_glDrawArraysInstancedEXT;
    if (glad_glDrawArraysInstancedANGLE == NULL && glad_glDrawArraysInstancedNV != NULL) glad_glDrawArraysInstancedANGLE = (PFNGLDRAWARRAYSINSTANCEDANGLEPROC)glad_glDrawArraysInstancedNV;
    if (glad_glDrawArraysInstancedARB == NULL && glad_glDrawArraysInstanced != NULL) glad_glDrawArraysInstancedARB = (PFNGLDRAWARRAYSINSTANCEDARBPROC)glad_glDrawArraysInstanced;
    if (glad_glDrawArraysInstancedARB == NULL && glad_glDrawArraysInstancedANGLE != NULL) glad_glDrawArraysInstancedARB = (PFNGLDRAWARRAYSINSTANCEDARBPROC)glad_glDrawArraysInstancedANGLE;
    if (glad_glDrawArraysInstancedARB == NULL && glad_glDrawArraysInstancedEXT != NULL) glad_glDrawArraysInstancedARB = (PFNGLDRAWARRAYSINSTANCEDARBPROC)glad_glDrawArraysInstancedEXT;
    if (glad_glDrawArraysInstancedARB == NULL && glad_glDrawArraysInstancedNV != NULL) glad_glDrawArraysInstancedARB = (PFNGLDRAWARRAYSINSTANCEDARBPROC)glad_glDrawArraysInstancedNV;
    if (glad_glDrawArraysInstancedEXT == NULL && glad_glDrawArraysInstanced != NULL) glad_glDrawArraysInstancedEXT = (PFNGLDRAWARRAYSINSTANCEDEXTPROC)glad_glDrawArraysInstanced;
    if (glad_glDrawArraysInstancedEXT == NULL && glad_glDrawArraysInstancedANGLE != NULL) glad_glDrawArraysInstancedEXT = (PFNGLDRAWARRAYSINSTANCEDEXTPROC)glad_glDrawArraysInstancedANGLE;
    if (glad_glDrawArraysInstancedEXT == NULL && glad_glDrawArraysInstancedARB != NULL) glad_glDrawArraysInstancedEXT = (PFNGLDRAWARRAYSINSTANCEDEXTPROC)glad_glDrawArraysInstancedARB;
    if (glad_glDrawArraysInstancedEXT == NULL && glad_glDrawArraysInstancedNV != NULL) glad_glDrawArraysInstancedEXT = (PFNGLDRAWARRAYSINSTANCEDEXTPROC)glad_glDrawArraysInstancedNV;
    if (glad_glDrawArraysInstancedNV == NULL && glad_glDrawArraysInstanced != NULL) glad_glDrawArraysInstancedNV = (PFNGLDRAWARRAYSINSTANCEDNVPROC)glad_glDrawArraysInstanced;
    if (glad_glDrawArraysInstancedNV == NULL && glad_glDrawArraysInstancedANGLE != NULL) glad_glDrawArraysInstancedNV = (PFNGLDRAWARRAYSINSTANCEDNVPROC)glad_glDrawArraysInstancedANGLE;
    if (glad_glDrawArraysInstancedNV == NULL && glad_glDrawArraysInstancedARB != NULL) glad_glDrawArraysInstancedNV = (PFNGLDRAWARRAYSINSTANCEDNVPROC)glad_glDrawArraysInstancedARB;
    if (glad_glDrawArraysInstancedNV == NULL && glad_glDrawArraysInstancedEXT != NULL) glad_glDrawArraysInstancedNV = (PFNGLDRAWARRAYSINSTANCEDNVPROC)glad_glDrawArraysInstancedEXT;
    if (glad_glDrawBuffers == NULL && glad_glDrawBuffersARB != NULL) glad_glDrawBuffers = (PFNGLDRAWBUFFERSPROC)glad_glDrawBuffersARB;
    if (glad_glDrawBuffers == NULL && glad_glDrawBuffersATI != NULL) glad_glDrawBuffers = (PFNGLDRAWBUFFERSPROC)glad_glDrawBuffersATI;
    if (glad_glDrawBuffers == NULL && glad_glDrawBuffersEXT != NULL) glad_glDrawBuffers = (PFNGLDRAWBUFFERSPROC)glad_glDrawBuffersEXT;
    if (glad_glDrawBuffersARB == NULL && glad_glDrawBuffers != NULL) glad_glDrawBuffersARB = (PFNGLDRAWBUFFERSARBPROC)glad_glDrawBuffers;
    if (glad_glDrawBuffersARB == NULL && glad_glDrawBuffersATI != NULL) glad_glDrawBuffersARB = (PFNGLDRAWBUFFERSARBPROC)glad_glDrawBuffersATI;
    if (glad_glDrawBuffersARB == NULL && glad_glDrawBuffersEXT != NULL) glad_glDrawBuffersARB = (PFNGLDRAWBUFFERSARBPROC)glad_glDrawBuffersEXT;
    if (glad_glDrawBuffersATI == NULL && glad_glDrawBuffers != NULL) glad_glDrawBuffersATI = (PFNGLDRAWBUFFERSATIPROC)glad_glDrawBuffers;
    if (glad_glDrawBuffersATI == NULL && glad_glDrawBuffersARB != NULL) glad_glDrawBuffersATI = (PFNGLDRAWBUFFERSATIPROC)glad_glDrawBuffersARB;
    if (glad_glDrawBuffersATI == NULL && glad_glDrawBuffersEXT != NULL) glad_glDrawBuffersATI = (PFNGLDRAWBUFFERSATIPROC)glad_glDrawBuffersEXT;
    if (glad_glDrawBuffersEXT == NULL && glad_glDrawBuffers != NULL) glad_glDrawBuffersEXT = (PFNGLDRAWBUFFERSEXTPROC)glad_glDrawBuffers;
    if (glad_glDrawBuffersEXT == NULL && glad_glDrawBuffersARB != NULL) glad_glDrawBuffersEXT = (PFNGLDRAWBUFFERSEXTPROC)glad_glDrawBuffersARB;
    if (glad_glDrawBuffersEXT == NULL && glad_glDrawBuffersATI != NULL) glad_glDrawBuffersEXT = (PFNGLDRAWBUFFERSEXTPROC)glad_glDrawBuffersATI;
    if (glad_glDrawElementsInstanced == NULL && glad_glDrawElementsInstancedANGLE != NULL) glad_glDrawElementsInstanced = (PFNGLDRAWELEMENTSINSTANCEDPROC)glad_glDrawElementsInstancedANGLE;
    if (glad_glDrawElementsInstanced == NULL && glad_glDrawElementsInstancedARB != NULL) glad_glDrawElementsInstanced = (PFNGLDRAWELEMENTSINSTANCEDPROC)glad_glDrawElementsInstancedARB;
    if (glad_glDrawElementsInstanced == NULL && glad_glDrawElementsInstancedEXT != NULL) glad_glDrawElementsInstanced = (PFNGLDRAWELEMENTSINSTANCEDPROC)glad_glDrawElementsInstancedEXT;
    if (glad_glDrawElementsInstanced == NULL && glad_glDrawElementsInstancedNV != NULL) glad_glDrawElementsInstanced = (PFNGLDRAWELEMENTSINSTANCEDPROC)glad_glDrawElementsInstancedNV;
    if (glad_glDrawElementsInstancedANGLE == NULL && glad_glDrawElementsInstanced != NULL) glad_glDrawElementsInstancedANGLE = (PFNGLDRAWELEMENTSINSTANCEDANGLEPROC)glad_glDrawElementsInstanced;
    if (glad_glDrawElementsInstancedANGLE == NULL && glad_glDrawElementsInstancedARB != NULL) glad_glDrawElementsInstancedANGLE = (PFNGLDRAWELEMENTSINSTANCEDANGLEPROC)glad_glDrawElementsInstancedARB;
    if (glad_glDrawElementsInstancedANGLE == NULL && glad_glDrawElementsInstancedEXT != NULL) glad_glDrawElementsInstancedANGLE = (PFNGLDRAWELEMENTSINSTANCEDANGLEPROC)glad_glDrawElementsInstancedEXT;
    if (glad_glDrawElementsInstancedANGLE == NULL && glad_glDrawElementsInstancedNV != NULL) glad_glDrawElementsInstancedANGLE = (PFNGLDRAWELEMENTSINSTANCEDANGLEPROC)glad_glDrawElementsInstancedNV;
    if (glad_glDrawElementsInstancedARB == NULL && glad_glDrawElementsInstanced != NULL) glad_glDrawElementsInstancedARB = (PFNGLDRAWELEMENTSINSTANCEDARBPROC)glad_glDrawElementsInstanced;
    if (glad_glDrawElementsInstancedARB == NULL && glad_glDrawElementsInstancedANGLE != NULL) glad_glDrawElementsInstancedARB = (PFNGLDRAWELEMENTSINSTANCEDARBPROC)glad_glDrawElementsInstancedANGLE;
    if (glad_glDrawElementsInstancedARB == NULL && glad_glDrawElementsInstancedEXT != NULL) glad_glDrawElementsInstancedARB = (PFNGLDRAWELEMENTSINSTANCEDARBPROC)glad_glDrawElementsInstancedEXT;
    if (glad_glDrawElementsInstancedARB == NULL && glad_glDrawElementsInstancedNV != NULL) glad_glDrawElementsInstancedARB = (PFNGLDRAWELEMENTSINSTANCEDARBPROC)glad_glDrawElementsInstancedNV;
    if (glad_glDrawElementsInstancedEXT == NULL && glad_glDrawElementsInstanced != NULL) glad_glDrawElementsInstancedEXT = (PFNGLDRAWELEMENTSINSTANCEDEXTPROC)glad_glDrawElementsInstanced;
    if (glad_glDrawElementsInstancedEXT == NULL && glad_glDrawElementsInstancedANGLE != NULL) glad_glDrawElementsInstancedEXT = (PFNGLDRAWELEMENTSINSTANCEDEXTPROC)glad_glDrawElementsInstancedANGLE;
    if (glad_glDrawElementsInstancedEXT == NULL && glad_glDrawElementsInstancedARB != NULL) glad_glDrawElementsInstancedEXT = (PFNGLDRAWELEMENTSINSTANCEDEXTPROC)glad_glDrawElementsInstancedARB;
    if (glad_glDrawElementsInstancedEXT == NULL && glad_glDrawElementsInstancedNV != NULL) glad_glDrawElementsInstancedEXT = (PFNGLDRAWELEMENTSINSTANCEDEXTPROC)glad_glDrawElementsInstancedNV;
    if (glad_glDrawElementsInstancedNV == NULL && glad_glDrawElementsInstanced != NULL) glad_glDrawElementsInstancedNV = (PFNGLDRAWELEMENTSINSTANCEDNVPROC)glad_glDrawElementsInstanced;
    if (glad_glDrawElementsInstancedNV == NULL && glad_glDrawElementsInstancedANGLE != NULL) glad_glDrawElementsInstancedNV = (PFNGLDRAWELEMENTSINSTANCEDNVPROC)glad_glDrawElementsInstancedANGLE;
    if (glad_glDrawElementsInstancedNV == NULL && glad_glDrawElementsInstancedARB != NULL) glad_glDrawElementsInstancedNV = (PFNGLDRAWELEMENTSINSTANCEDNVPROC)glad_glDrawElementsInstancedARB;
    if (glad_glDrawElementsInstancedNV == NULL && glad_glDrawElementsInstancedEXT != NULL) glad_glDrawElementsInstancedNV = (PFNGLDRAWELEMENTSINSTANCEDNVPROC)glad_glDrawElementsInstancedEXT;
    if (glad_glEnablei == NULL && glad_glEnableiOES != NULL) glad_glEnablei = (PFNGLENABLEIPROC)glad_glEnableiOES;
    if (glad_glEnableiOES == NULL && glad_glEnablei != NULL) glad_glEnableiOES = (PFNGLENABLEIOESPROC)glad_glEnablei;
    if (glad_glGenVertexArrays == NULL && glad_glGenVertexArraysOES != NULL) glad_glGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC)glad_glGenVertexArraysOES;
    if (glad_glGenVertexArraysOES == NULL && glad_glGenVertexArrays != NULL) glad_glGenVertexArraysOES = (PFNGLGENVERTEXARRAYSOESPROC)glad_glGenVertexArrays;
    if (glad_glGetDebugMessageLog == NULL && glad_glGetDebugMessageLogARB != NULL) glad_glGetDebugMessageLog = (PFNGLGETDEBUGMESSAGELOGPROC)glad_glGetDebugMessageLogARB;
    if (glad_glGetDebugMessageLog == NULL && glad_glGetDebugMessageLogKHR != NULL) glad_glGetDebugMessageLog = (PFNGLGETDEBUGMESSAGELOGPROC)glad_glGetDebugMessageLogKHR;
    if (glad_glGetDebugMessageLogARB == NULL && glad_glGetDebugMessageLog != NULL) glad_glGetDebugMessageLogARB = (PFNGLGETDEBUGMESSAGELOGARBPROC)glad_glGetDebugMessageLog;
    if (glad_glGetDebugMessageLogARB == NULL && glad_glGetDebugMessageLogKHR != NULL) glad_glGetDebugMessageLogARB = (PFNGLGETDEBUGMESSAGELOGARBPROC)glad_glGetDebugMessageLogKHR;
    if (glad_glGetDebugMessageLogKHR == NULL && glad_glGetDebugMessageLog != NULL) glad_glGetDebugMessageLogKHR = (PFNGLGETDEBUGMESSAGELOGKHRPROC)glad_glGetDebugMessageLog;
    if (glad_glGetDebugMessageLogKHR == NULL && glad_glGetDebugMessageLogARB != NULL) glad_glGetDebugMessageLogKHR = (PFNGLGETDEBUGMESSAGELOGKHRPROC)glad_glGetDebugMessageLogARB;
    if (glad_glGetFloati_v == NULL && glad_glGetFloati_vOES != NULL) glad_glGetFloati_v = (PFNGLGETFLOATI_VPROC)glad_glGetFloati_vOES;
    if (glad_glGetFloati_vOES == NULL && glad_glGetFloati_v != NULL) glad_glGetFloati_vOES = (PFNGLGETFLOATI_VOESPROC)glad_glGetFloati_v;
    if (glad_glGetObjectLabel == NULL && glad_glGetObjectLabelKHR != NULL) glad_glGetObjectLabel = (PFNGLGETOBJECTLABELPROC)glad_glGetObjectLabelKHR;
    if (glad_glGetObjectLabelKHR == NULL && glad_glGetObjectLabel != NULL) glad_glGetObjectLabelKHR = (PFNGLGETOBJECTLABELKHRPROC)glad_glGetObjectLabel;
    if (glad_glGetObjectPtrLabel == NULL && glad_glGetObjectPtrLabelKHR != NULL) glad_glGetObjectPtrLabel = (PFNGLGETOBJECTPTRLABELPROC)glad_glGetObjectPtrLabelKHR;
    if (glad_glGetObjectPtrLabelKHR == NULL && glad_glGetObjectPtrLabel != NULL) glad_glGetObjectPtrLabelKHR = (PFNGLGETOBJECTPTRLABELKHRPROC)glad_glGetObjectPtrLabel;
    if (glad_glGetPointerv == NULL && glad_glGetPointervKHR != NULL) glad_glGetPointerv = (PFNGLGETPOINTERVPROC)glad_glGetPointervKHR;
    if (glad_glGetPointervKHR == NULL && glad_glGetPointerv != NULL) glad_glGetPointervKHR = (PFNGLGETPOINTERVKHRPROC)glad_glGetPointerv;
    if (glad_glIsEnabledi == NULL && glad_glIsEnablediOES != NULL) glad_glIsEnabledi = (PFNGLISENABLEDIPROC)glad_glIsEnablediOES;
    if (glad_glIsEnablediOES == NULL && glad_glIsEnabledi != NULL) glad_glIsEnablediOES = (PFNGLISENABLEDIOESPROC)glad_glIsEnabledi;
    if (glad_glIsVertexArray == NULL && glad_glIsVertexArrayOES != NULL) glad_glIsVertexArray = (PFNGLISVERTEXARRAYPROC)glad_glIsVertexArrayOES;
    if (glad_glIsVertexArrayOES == NULL && glad_glIsVertexArray != NULL) glad_glIsVertexArrayOES = (PFNGLISVERTEXARRAYOESPROC)glad_glIsVertexArray;
    if (glad_glObjectLabel == NULL && glad_glObjectLabelKHR != NULL) glad_glObjectLabel = (PFNGLOBJECTLABELPROC)glad_glObjectLabelKHR;
    if (glad_glObjectLabelKHR == NULL && glad_glObjectLabel != NULL) glad_glObjectLabelKHR = (PFNGLOBJECTLABELKHRPROC)glad_glObjectLabel;
    if (glad_glObjectPtrLabel == NULL && glad_glObjectPtrLabelKHR != NULL) glad_glObjectPtrLabel = (PFNGLOBJECTPTRLABELPROC)glad_glObjectPtrLabelKHR;
    if (glad_glObjectPtrLabelKHR == NULL && glad_glObjectPtrLabel != NULL) glad_glObjectPtrLabelKHR = (PFNGLOBJECTPTRLABELKHRPROC)glad_glObjectPtrLabel;
    if (glad_glPopDebugGroup == NULL && glad_glPopDebugGroupKHR != NULL) glad_glPopDebugGroup = (PFNGLPOPDEBUGGROUPPROC)glad_glPopDebugGroupKHR;
    if (glad_glPopDebugGroupKHR == NULL && glad_glPopDebugGroup != NULL) glad_glPopDebugGroupKHR = (PFNGLPOPDEBUGGROUPKHRPROC)glad_glPopDebugGroup;
    if (glad_glPushDebugGroup == NULL && glad_glPushDebugGroupKHR != NULL) glad_glPushDebugGroup = (PFNGLPUSHDEBUGGROUPPROC)glad_glPushDebugGroupKHR;
    if (glad_glPushDebugGroupKHR == NULL && glad_glPushDebugGroup != NULL) glad_glPushDebugGroupKHR = (PFNGLPUSHDEBUGGROUPKHRPROC)glad_glPushDebugGroup;
    if (glad_glScissorArrayv == NULL && glad_glScissorArrayvOES != NULL) glad_glScissorArrayv = (PFNGLSCISSORARRAYVPROC)glad_glScissorArrayvOES;
    if (glad_glScissorArrayvOES == NULL && glad_glScissorArrayv != NULL) glad_glScissorArrayvOES = (PFNGLSCISSORARRAYVOESPROC)glad_glScissorArrayv;
    if (glad_glScissorIndexed == NULL && glad_glScissorIndexedOES != NULL) glad_glScissorIndexed = (PFNGLSCISSORINDEXEDPROC)glad_glScissorIndexedOES;
    if (glad_glScissorIndexedOES == NULL && glad_glScissorIndexed != NULL) glad_glScissorIndexedOES = (PFNGLSCISSORINDEXEDOESPROC)glad_glScissorIndexed;
    if (glad_glScissorIndexedv == NULL && glad_glScissorIndexedvOES != NULL) glad_glScissorIndexedv = (PFNGLSCISSORINDEXEDVPROC)glad_glScissorIndexedvOES;
    if (glad_glScissorIndexedvOES == NULL && glad_glScissorIndexedv != NULL) glad_glScissorIndexedvOES = (PFNGLSCISSORINDEXEDVOESPROC)glad_glScissorIndexedv;
    if (glad_glVertexAttribDivisor == NULL && glad_glVertexAttribDivisorANGLE != NULL) glad_glVertexAttribDivisor = (PFNGLVERTEXATTRIBDIVISORPROC)glad_glVertexAttribDivisorANGLE;
    if (glad_glVertexAttribDivisor == NULL && glad_glVertexAttribDivisorARB != NULL) glad_glVertexAttribDivisor = (PFNGLVERTEXATTRIBDIVISORPROC)glad_glVertexAttribDivisorARB;
    if (glad_glVertexAttribDivisor == NULL && glad_glVertexAttribDivisorEXT != NULL) glad_glVertexAttribDivisor = (PFNGLVERTEXATTRIBDIVISORPROC)glad_glVertexAttribDivisorEXT;
    if (glad_glVertexAttribDivisor == NULL && glad_glVertexAttribDivisorNV != NULL) glad_glVertexAttribDivisor = (PFNGLVERTEXATTRIBDIVISORPROC)glad_glVertexAttribDivisorNV;
    if (glad_glVertexAttribDivisorANGLE == NULL && glad_glVertexAttribDivisor != NULL) glad_glVertexAttribDivisorANGLE = (PFNGLVERTEXATTRIBDIVISORANGLEPROC)glad_glVertexAttribDivisor;
    if (glad_glVertexAttribDivisorANGLE == NULL && glad_glVertexAttribDivisorARB != NULL) glad_glVertexAttribDivisorANGLE = (PFNGLVERTEXATTRIBDIVISORANGLEPROC)glad_glVertexAttribDivisorARB;
    if (glad_glVertexAttribDivisorANGLE == NULL && glad_glVertexAttribDivisorEXT != NULL) glad_glVertexAttribDivisorANGLE = (PFNGLVERTEXATTRIBDIVISORANGLEPROC)glad_glVertexAttribDivisorEXT;
    if (glad_glVertexAttribDivisorANGLE == NULL && glad_glVertexAttribDivisorNV != NULL) glad_glVertexAttribDivisorANGLE = (PFNGLVERTEXATTRIBDIVISORANGLEPROC)glad_glVertexAttribDivisorNV;
    if (glad_glVertexAttribDivisorARB == NULL && glad_glVertexAttribDivisor != NULL) glad_glVertexAttribDivisorARB = (PFNGLVERTEXATTRIBDIVISORARBPROC)glad_glVertexAttribDivisor;
    if (glad_glVertexAttribDivisorARB == NULL && glad_glVertexAttribDivisorANGLE != NULL) glad_glVertexAttribDivisorARB = (PFNGLVERTEXATTRIBDIVISORARBPROC)glad_glVertexAttribDivisorANGLE;
    if (glad_glVertexAttribDivisorARB == NULL && glad_glVertexAttribDivisorEXT != NULL) glad_glVertexAttribDivisorARB = (PFNGLVERTEXATTRIBDIVISORARBPROC)glad_glVertexAttribDivisorEXT;
    if (glad_glVertexAttribDivisorARB == NULL && glad_glVertexAttribDivisorNV != NULL) glad_glVertexAttribDivisorARB = (PFNGLVERTEXATTRIBDIVISORARBPROC)glad_glVertexAttribDivisorNV;
    if (glad_glVertexAttribDivisorEXT == NULL && glad_glVertexAttribDivisor != NULL) glad_glVertexAttribDivisorEXT = (PFNGLVERTEXATTRIBDIVISOREXTPROC)glad_glVertexAttribDivisor;
    if (glad_glVertexAttribDivisorEXT == NULL && glad_glVertexAttribDivisorANGLE != NULL) glad_glVertexAttribDivisorEXT = (PFNGLVERTEXATTRIBDIVISOREXTPROC)glad_glVertexAttribDivisorANGLE;
    if (glad_glVertexAttribDivisorEXT == NULL && glad_glVertexAttribDivisorARB != NULL) glad_glVertexAttribDivisorEXT = (PFNGLVERTEXATTRIBDIVISOREXTPROC)glad_glVertexAttribDivisorARB;
    if (glad_glVertexAttribDivisorEXT == NULL && glad_glVertexAttribDivisorNV != NULL) glad_glVertexAttribDivisorEXT = (PFNGLVERTEXATTRIBDIVISOREXTPROC)glad_glVertexAttribDivisorNV;
    if (glad_glVertexAttribDivisorNV == NULL && glad_glVertexAttribDivisor != NULL) glad_glVertexAttribDivisorNV = (PFNGLVERTEXATTRIBDIVISORNVPROC)glad_glVertexAttribDivisor;
    if (glad_glVertexAttribDivisorNV == NULL && glad_glVertexAttribDivisorANGLE != NULL) glad_glVertexAttribDivisorNV = (PFNGLVERTEXATTRIBDIVISORNVPROC)glad_glVertexAttribDivisorANGLE;
    if (glad_glVertexAttribDivisorNV == NULL && glad_glVertexAttribDivisorARB != NULL) glad_glVertexAttribDivisorNV = (PFNGLVERTEXATTRIBDIVISORNVPROC)glad_glVertexAttribDivisorARB;
    if (glad_glVertexAttribDivisorNV == NULL && glad_glVertexAttribDivisorEXT != NULL) glad_glVertexAttribDivisorNV = (PFNGLVERTEXATTRIBDIVISORNVPROC)glad_glVertexAttribDivisorEXT;
    if (glad_glViewportArrayv == NULL && glad_glViewportArrayvOES != NULL) glad_glViewportArrayv = (PFNGLVIEWPORTARRAYVPROC)glad_glViewportArrayvOES;
    if (glad_glViewportArrayvOES == NULL && glad_glViewportArrayv != NULL) glad_glViewportArrayvOES = (PFNGLVIEWPORTARRAYVOESPROC)glad_glViewportArrayv;
    if (glad_glViewportIndexedf == NULL && glad_glViewportIndexedfOES != NULL) glad_glViewportIndexedf = (PFNGLVIEWPORTINDEXEDFPROC)glad_glViewportIndexedfOES;
    if (glad_glViewportIndexedfOES == NULL && glad_glViewportIndexedf != NULL) glad_glViewportIndexedfOES = (PFNGLVIEWPORTINDEXEDFOESPROC)glad_glViewportIndexedf;
    if (glad_glViewportIndexedfv == NULL && glad_glViewportIndexedfvOES != NULL) glad_glViewportIndexedfv = (PFNGLVIEWPORTINDEXEDFVPROC)glad_glViewportIndexedfvOES;
    if (glad_glViewportIndexedfvOES == NULL && glad_glViewportIndexedfv != NULL) glad_glViewportIndexedfvOES = (PFNGLVIEWPORTINDEXEDFVOESPROC)glad_glViewportIndexedfv;
}

#if defined(GL_ES_VERSION_3_0) || defined(GL_VERSION_3_0)
#define GLAD_GL_IS_SOME_NEW_VERSION 1
#else
#define GLAD_GL_IS_SOME_NEW_VERSION 0
#endif

static int glad_gl_get_extensions( int version, const char **out_exts, unsigned int *out_num_exts_i, char ***out_exts_i) {
#if GLAD_GL_IS_SOME_NEW_VERSION
    if(GLAD_VERSION_MAJOR(version) < 3) {
#else
    (void) version;
    (void) out_num_exts_i;
    (void) out_exts_i;
#endif
        if (glad_glGetString == NULL) {
            return 0;
        }
        *out_exts = (const char *)glad_glGetString(GL_EXTENSIONS);
#if GLAD_GL_IS_SOME_NEW_VERSION
    } else {
        unsigned int index = 0;
        unsigned int num_exts_i = 0;
        char **exts_i = NULL;
        if (glad_glGetStringi == NULL || glad_glGetIntegerv == NULL) {
            return 0;
        }
        glad_glGetIntegerv(GL_NUM_EXTENSIONS, (int*) &num_exts_i);
        if (num_exts_i > 0) {
            exts_i = (char **) malloc(num_exts_i * (sizeof *exts_i));
        }
        if (exts_i == NULL) {
            return 0;
        }
        for(index = 0; index < num_exts_i; index++) {
            const char *gl_str_tmp = (const char*) glad_glGetStringi(GL_EXTENSIONS, index);
            size_t len = strlen(gl_str_tmp) + 1;

            char *local_str = (char*) malloc(len * sizeof(char));
            if(local_str != NULL) {
                memcpy(local_str, gl_str_tmp, len * sizeof(char));
            }

            exts_i[index] = local_str;
        }

        *out_num_exts_i = num_exts_i;
        *out_exts_i = exts_i;
    }
#endif
    return 1;
}
static void glad_gl_free_extensions(char **exts_i, unsigned int num_exts_i) {
    if (exts_i != NULL) {
        unsigned int index;
        for(index = 0; index < num_exts_i; index++) {
            free((void *) (exts_i[index]));
        }
        free((void *)exts_i);
        exts_i = NULL;
    }
}
static int glad_gl_has_extension(int version, const char *exts, unsigned int num_exts_i, char **exts_i, const char *ext) {
    if(GLAD_VERSION_MAJOR(version) < 3 || !GLAD_GL_IS_SOME_NEW_VERSION) {
        const char *extensions;
        const char *loc;
        const char *terminator;
        extensions = exts;
        if(extensions == NULL || ext == NULL) {
            return 0;
        }
        while(1) {
            loc = strstr(extensions, ext);
            if(loc == NULL) {
                return 0;
            }
            terminator = loc + strlen(ext);
            if((loc == extensions || *(loc - 1) == ' ') &&
                (*terminator == ' ' || *terminator == '\0')) {
                return 1;
            }
            extensions = terminator;
        }
    } else {
        unsigned int index;
        for(index = 0; index < num_exts_i; index++) {
            const char *e = exts_i[index];
            if(strcmp(e, ext) == 0) {
                return 1;
            }
        }
    }
    return 0;
}

static GLADapiproc glad_gl_get_proc_from_userptr(void *userptr, const char* name) {
    return (GLAD_GNUC_EXTENSION (GLADapiproc (*)(const char *name)) userptr)(name);
}

static int glad_gl_find_extensions_gl( int version) {
    const char *exts = NULL;
    unsigned int num_exts_i = 0;
    char **exts_i = NULL;
    if (!glad_gl_get_extensions(version, &exts, &num_exts_i, &exts_i)) return 0;

    GLAD_GL_3DFX_texture_compression_FXT1 = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_3DFX_texture_compression_FXT1");
    GLAD_GL_ARB_ES3_compatibility = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ARB_ES3_compatibility");
    GLAD_GL_ARB_blend_func_extended = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ARB_blend_func_extended");
    GLAD_GL_ARB_clear_texture = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ARB_clear_texture");
    GLAD_GL_ARB_copy_buffer = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ARB_copy_buffer");
    GLAD_GL_ARB_debug_output = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ARB_debug_output");
    GLAD_GL_ARB_depth_texture = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ARB_depth_texture");
    GLAD_GL_ARB_draw_buffers = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ARB_draw_buffers");
    GLAD_GL_ARB_draw_elements_base_vertex = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ARB_draw_elements_base_vertex");
    GLAD_GL_ARB_draw_instanced = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ARB_draw_instanced");
    GLAD_GL_ARB_framebuffer_object = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ARB_framebuffer_object");
    GLAD_GL_ARB_imaging = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ARB_imaging");
    GLAD_GL_ARB_instanced_arrays = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ARB_instanced_arrays");
    GLAD_GL_ARB_internalformat_query2 = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ARB_internalformat_query2");
    GLAD_GL_ARB_map_buffer_range = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ARB_map_buffer_range");
    GLAD_GL_ARB_pixel_buffer_object = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ARB_pixel_buffer_object");
    GLAD_GL_ARB_provoking_vertex = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ARB_provoking_vertex");
    GLAD_GL_ARB_sampler_objects = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ARB_sampler_objects");
    GLAD_GL_ARB_sync = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ARB_sync");
    GLAD_GL_ARB_texture_compression_bptc = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ARB_texture_compression_bptc");
    GLAD_GL_ARB_texture_compression_rgtc = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ARB_texture_compression_rgtc");
    GLAD_GL_ARB_texture_filter_anisotropic = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ARB_texture_filter_anisotropic");
    GLAD_GL_ARB_texture_multisample = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ARB_texture_multisample");
    GLAD_GL_ARB_timer_query = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ARB_timer_query");
    GLAD_GL_ARB_uniform_buffer_object = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ARB_uniform_buffer_object");
    GLAD_GL_ARB_vertex_array_object = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ARB_vertex_array_object");
    GLAD_GL_ARB_vertex_type_2_10_10_10_rev = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ARB_vertex_type_2_10_10_10_rev");
    GLAD_GL_ARB_viewport_array = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ARB_viewport_array");
    GLAD_GL_ATI_draw_buffers = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ATI_draw_buffers");
    GLAD_GL_EXT_draw_instanced = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_draw_instanced");
    GLAD_GL_EXT_pixel_buffer_object = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_pixel_buffer_object");
    GLAD_GL_EXT_texture_compression_rgtc = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_texture_compression_rgtc");
    GLAD_GL_EXT_texture_compression_s3tc = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_texture_compression_s3tc");
    GLAD_GL_EXT_texture_filter_anisotropic = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_texture_filter_anisotropic");
    GLAD_GL_EXT_texture_sRGB = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_texture_sRGB");
    GLAD_GL_EXT_texture_sRGB_R8 = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_texture_sRGB_R8");
    GLAD_GL_EXT_texture_sRGB_RG8 = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_texture_sRGB_RG8");
    GLAD_GL_KHR_debug = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_KHR_debug");
    GLAD_GL_KHR_texture_compression_astc_ldr = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_KHR_texture_compression_astc_ldr");
    GLAD_GL_NV_viewport_array2 = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_NV_viewport_array2");
    GLAD_GL_SGIX_depth_texture = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_SGIX_depth_texture");

    glad_gl_free_extensions(exts_i, num_exts_i);

    return 1;
}

static int glad_gl_find_core_gl(void) {
    int i, major, minor;
    const char* version;
    const char* prefixes[] = {
        "OpenGL ES-CM ",
        "OpenGL ES-CL ",
        "OpenGL ES ",
        NULL
    };
    version = (const char*) glad_glGetString(GL_VERSION);
    if (!version) return 0;
    for (i = 0;  prefixes[i];  i++) {
        const size_t length = strlen(prefixes[i]);
        if (strncmp(version, prefixes[i], length) == 0) {
            version += length;
            break;
        }
    }

    GLAD_IMPL_UTIL_SSCANF(version, "%d.%d", &major, &minor);

    GLAD_GL_VERSION_1_0 = (major == 1 && minor >= 0) || major > 1;
    GLAD_GL_VERSION_1_1 = (major == 1 && minor >= 1) || major > 1;
    GLAD_GL_VERSION_1_2 = (major == 1 && minor >= 2) || major > 1;
    GLAD_GL_VERSION_1_3 = (major == 1 && minor >= 3) || major > 1;
    GLAD_GL_VERSION_1_4 = (major == 1 && minor >= 4) || major > 1;
    GLAD_GL_VERSION_1_5 = (major == 1 && minor >= 5) || major > 1;
    GLAD_GL_VERSION_2_0 = (major == 2 && minor >= 0) || major > 2;
    GLAD_GL_VERSION_2_1 = (major == 2 && minor >= 1) || major > 2;
    GLAD_GL_VERSION_3_0 = (major == 3 && minor >= 0) || major > 3;
    GLAD_GL_VERSION_3_1 = (major == 3 && minor >= 1) || major > 3;
    GLAD_GL_VERSION_3_2 = (major == 3 && minor >= 2) || major > 3;
    GLAD_GL_VERSION_3_3 = (major == 3 && minor >= 3) || major > 3;

    return GLAD_MAKE_VERSION(major, minor);
}

int gladLoadGLUserPtr( GLADuserptrloadfunc load, void *userptr) {
    int version;

    glad_glGetString = (PFNGLGETSTRINGPROC) load(userptr, "glGetString");
    if(glad_glGetString == NULL) return 0;
    if(glad_glGetString(GL_VERSION) == NULL) return 0;
    version = glad_gl_find_core_gl();

    glad_gl_load_GL_VERSION_1_0(load, userptr);
    glad_gl_load_GL_VERSION_1_1(load, userptr);
    glad_gl_load_GL_VERSION_1_2(load, userptr);
    glad_gl_load_GL_VERSION_1_3(load, userptr);
    glad_gl_load_GL_VERSION_1_4(load, userptr);
    glad_gl_load_GL_VERSION_1_5(load, userptr);
    glad_gl_load_GL_VERSION_2_0(load, userptr);
    glad_gl_load_GL_VERSION_2_1(load, userptr);
    glad_gl_load_GL_VERSION_3_0(load, userptr);
    glad_gl_load_GL_VERSION_3_1(load, userptr);
    glad_gl_load_GL_VERSION_3_2(load, userptr);
    glad_gl_load_GL_VERSION_3_3(load, userptr);

    if (!glad_gl_find_extensions_gl(version)) return 0;
    glad_gl_load_GL_ARB_blend_func_extended(load, userptr);
    glad_gl_load_GL_ARB_clear_texture(load, userptr);
    glad_gl_load_GL_ARB_copy_buffer(load, userptr);
    glad_gl_load_GL_ARB_debug_output(load, userptr);
    glad_gl_load_GL_ARB_draw_buffers(load, userptr);
    glad_gl_load_GL_ARB_draw_elements_base_vertex(load, userptr);
    glad_gl_load_GL_ARB_draw_instanced(load, userptr);
    glad_gl_load_GL_ARB_framebuffer_object(load, userptr);
    glad_gl_load_GL_ARB_imaging(load, userptr);
    glad_gl_load_GL_ARB_instanced_arrays(load, userptr);
    glad_gl_load_GL_ARB_internalformat_query2(load, userptr);
    glad_gl_load_GL_ARB_map_buffer_range(load, userptr);
    glad_gl_load_GL_ARB_provoking_vertex(load, userptr);
    glad_gl_load_GL_ARB_sampler_objects(load, userptr);
    glad_gl_load_GL_ARB_sync(load, userptr);
    glad_gl_load_GL_ARB_texture_multisample(load, userptr);
    glad_gl_load_GL_ARB_timer_query(load, userptr);
    glad_gl_load_GL_ARB_uniform_buffer_object(load, userptr);
    glad_gl_load_GL_ARB_vertex_array_object(load, userptr);
    glad_gl_load_GL_ARB_vertex_type_2_10_10_10_rev(load, userptr);
    glad_gl_load_GL_ARB_viewport_array(load, userptr);
    glad_gl_load_GL_ATI_draw_buffers(load, userptr);
    glad_gl_load_GL_EXT_draw_instanced(load, userptr);
    glad_gl_load_GL_KHR_debug(load, userptr);


    glad_gl_resolve_aliases();

    return version;
}


int gladLoadGL( GLADloadfunc load) {
    return gladLoadGLUserPtr( glad_gl_get_proc_from_userptr, GLAD_GNUC_EXTENSION (void*) load);
}

static int glad_gl_find_extensions_gles2( int version) {
    const char *exts = NULL;
    unsigned int num_exts_i = 0;
    char **exts_i = NULL;
    if (!glad_gl_get_extensions(version, &exts, &num_exts_i, &exts_i)) return 0;

    GLAD_GL_EXT_draw_instanced = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_draw_instanced");
    GLAD_GL_EXT_texture_compression_rgtc = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_texture_compression_rgtc");
    GLAD_GL_EXT_texture_compression_s3tc = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_texture_compression_s3tc");
    GLAD_GL_EXT_texture_filter_anisotropic = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_texture_filter_anisotropic");
    GLAD_GL_EXT_texture_sRGB_R8 = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_texture_sRGB_R8");
    GLAD_GL_EXT_texture_sRGB_RG8 = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_texture_sRGB_RG8");
    GLAD_GL_KHR_debug = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_KHR_debug");
    GLAD_GL_KHR_texture_compression_astc_ldr = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_KHR_texture_compression_astc_ldr");
    GLAD_GL_NV_viewport_array2 = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_NV_viewport_array2");
    GLAD_GL_AMD_compressed_ATC_texture = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_AMD_compressed_ATC_texture");
    GLAD_GL_ANGLE_depth_texture = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ANGLE_depth_texture");
    GLAD_GL_ANGLE_instanced_arrays = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ANGLE_instanced_arrays");
    GLAD_GL_ANGLE_texture_compression_dxt3 = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ANGLE_texture_compression_dxt3");
    GLAD_GL_ANGLE_texture_compression_dxt5 = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ANGLE_texture_compression_dxt5");
    GLAD_GL_ANGLE_translated_shader_source = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ANGLE_translated_shader_source");
    GLAD_GL_EXT_clear_texture = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_clear_texture");
    GLAD_GL_EXT_color_buffer_float = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_color_buffer_float");
    GLAD_GL_EXT_draw_buffers = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_draw_buffers");
    GLAD_GL_EXT_float_blend = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_float_blend");
    GLAD_GL_EXT_instanced_arrays = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_instanced_arrays");
    GLAD_GL_EXT_pvrtc_sRGB = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_pvrtc_sRGB");
    GLAD_GL_EXT_sRGB = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_sRGB");
    GLAD_GL_EXT_texture_compression_bptc = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_texture_compression_bptc");
    GLAD_GL_EXT_texture_compression_dxt1 = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_texture_compression_dxt1");
    GLAD_GL_EXT_texture_compression_s3tc_srgb = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_texture_compression_s3tc_srgb");
    GLAD_GL_EXT_texture_norm16 = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_texture_norm16");
    GLAD_GL_EXT_texture_rg = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_texture_rg");
    GLAD_GL_IMG_texture_compression_pvrtc = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_IMG_texture_compression_pvrtc");
    GLAD_GL_IMG_texture_compression_pvrtc2 = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_IMG_texture_compression_pvrtc2");
    GLAD_GL_NV_draw_instanced = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_NV_draw_instanced");
    GLAD_GL_NV_instanced_arrays = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_NV_instanced_arrays");
    GLAD_GL_NV_pixel_buffer_object = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_NV_pixel_buffer_object");
    GLAD_GL_NV_sRGB_formats = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_NV_sRGB_formats");
    GLAD_GL_OES_compressed_ETC1_RGB8_texture = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_OES_compressed_ETC1_RGB8_texture");
    GLAD_GL_OES_depth_texture = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_OES_depth_texture");
    GLAD_GL_OES_draw_buffers_indexed = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_OES_draw_buffers_indexed");
    GLAD_GL_OES_texture_compression_astc = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_OES_texture_compression_astc");
    GLAD_GL_OES_texture_float_linear = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_OES_texture_float_linear");
    GLAD_GL_OES_texture_half_float_linear = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_OES_texture_half_float_linear");
    GLAD_GL_OES_vertex_array_object = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_OES_vertex_array_object");
    GLAD_GL_OES_viewport_array = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_OES_viewport_array");

    glad_gl_free_extensions(exts_i, num_exts_i);

    return 1;
}

static int glad_gl_find_core_gles2(void) {
    int i, major, minor;
    const char* version;
    const char* prefixes[] = {
        "OpenGL ES-CM ",
        "OpenGL ES-CL ",
        "OpenGL ES ",
        NULL
    };
    version = (const char*) glad_glGetString(GL_VERSION);
    if (!version) return 0;
    for (i = 0;  prefixes[i];  i++) {
        const size_t length = strlen(prefixes[i]);
        if (strncmp(version, prefixes[i], length) == 0) {
            version += length;
            break;
        }
    }

    GLAD_IMPL_UTIL_SSCANF(version, "%d.%d", &major, &minor);

    GLAD_GL_ES_VERSION_2_0 = (major == 2 && minor >= 0) || major > 2;
    GLAD_GL_ES_VERSION_3_0 = (major == 3 && minor >= 0) || major > 3;

    return GLAD_MAKE_VERSION(major, minor);
}

int gladLoadGLES2UserPtr( GLADuserptrloadfunc load, void *userptr) {
    int version;

    glad_glGetString = (PFNGLGETSTRINGPROC) load(userptr, "glGetString");
    if(glad_glGetString == NULL) return 0;
    if(glad_glGetString(GL_VERSION) == NULL) return 0;
    version = glad_gl_find_core_gles2();

    glad_gl_load_GL_ES_VERSION_2_0(load, userptr);
    glad_gl_load_GL_ES_VERSION_3_0(load, userptr);

    if (!glad_gl_find_extensions_gles2(version)) return 0;
    glad_gl_load_GL_EXT_draw_instanced(load, userptr);
    glad_gl_load_GL_KHR_debug(load, userptr);
    glad_gl_load_GL_ANGLE_instanced_arrays(load, userptr);
    glad_gl_load_GL_ANGLE_translated_shader_source(load, userptr);
    glad_gl_load_GL_EXT_clear_texture(load, userptr);
    glad_gl_load_GL_EXT_draw_buffers(load, userptr);
    glad_gl_load_GL_EXT_instanced_arrays(load, userptr);
    glad_gl_load_GL_NV_draw_instanced(load, userptr);
    glad_gl_load_GL_NV_instanced_arrays(load, userptr);
    glad_gl_load_GL_OES_draw_buffers_indexed(load, userptr);
    glad_gl_load_GL_OES_vertex_array_object(load, userptr);
    glad_gl_load_GL_OES_viewport_array(load, userptr);


    glad_gl_resolve_aliases();

    return version;
}


int gladLoadGLES2( GLADloadfunc load) {
    return gladLoadGLES2UserPtr( glad_gl_get_proc_from_userptr, GLAD_GNUC_EXTENSION (void*) load);
}






#ifdef __cplusplus
}
#endif
