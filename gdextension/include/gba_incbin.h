/**
 * @file gba_incbin.h
 * @brief Null definitions for INCBIN macros - replaced by runtime initialization
 * 
 * This header provides empty macro definitions for INCBIN_U8/U16/U32.
 * The actual INCBIN calls are transformed to null pointer declarations by
 * incbin_transform.py, and populated at runtime by gba_assets_init().
 * 
 * DO NOT USE THESE MACROS DIRECTLY - they are empty by design.
 * All asset loading is handled via runtime ROM extraction.
 */

#ifndef GBA_INCBIN_H
#define GBA_INCBIN_H

/*
 * INCBIN_U8/U16/U32: Null definitions
 * 
 * These macros are intentionally empty. The preprocessor script
 * (incbin_transform.py) transforms all INCBIN declarations in the
 * pokefirered source files from:
 * 
 *   static const u16 arr[] = INCBIN_U16("path");
 * 
 * To:
 * 
 *   static const u16 *arr = NULL;
 * 
 * The runtime initializer (gba_assets_init()) then populates these
 * pointers from the loaded ROM buffer.
 */

#define INCBIN_U8(path)   /* transformed to NULL by preprocessor */
#define INCBIN_U16(path)  /* transformed to NULL by preprocessor */
#define INCBIN_U32(path)  /* transformed to NULL by preprocessor */

#endif /* GBA_INCBIN_H */
