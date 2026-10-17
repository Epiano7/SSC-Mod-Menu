#pragma once
// miniz 3.1.2, MIT; amalgamation from the official release. Runtime and
// standalone fixtures each compile their recording code in one translation unit.
#if defined(__GNUC__)
#pragma GCC system_header
#endif
#define MINIZ_NO_ZLIB_COMPATIBLE_NAMES
#include "miniz.c"
