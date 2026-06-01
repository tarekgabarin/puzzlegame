// Single translation unit that bakes raygui's implementation. Every other .c
// file just #includes "raygui.h" without the implementation define.

#if defined(__GNUC__) || defined(__clang__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wunused-function"
    #pragma GCC diagnostic ignored "-Wunused-variable"
#endif

#define RAYGUI_IMPLEMENTATION
#include "vendor/raygui.h"

#if defined(__GNUC__) || defined(__clang__)
    #pragma GCC diagnostic pop
#endif
