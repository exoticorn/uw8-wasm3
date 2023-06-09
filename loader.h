/* Automatically generated by wasm2c */
#ifndef LOADER_H_GENERATED_
#define LOADER_H_GENERATED_

#include <stdint.h>

#include "wasm-rt.h"

/* TODO(binji): only use stdint.h types in header */
#ifndef WASM_RT_CORE_TYPES_DEFINED
#define WASM_RT_CORE_TYPES_DEFINED
typedef uint8_t u8;
typedef int8_t s8;
typedef uint16_t u16;
typedef int16_t s16;
typedef uint32_t u32;
typedef int32_t s32;
typedef uint64_t u64;
typedef int64_t s64;
typedef float f32;
typedef double f64;
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct Z_env_instance_t;
extern wasm_rt_memory_t* Z_envZ_memory(struct Z_env_instance_t*);

typedef struct Z_loader_instance_t {
  /* import: 'env' 'memory' */
  wasm_rt_memory_t *Z_envZ_memory;
  u32 w2c_g0;
  u32 w2c_g1;
  u32 w2c_g2;
} Z_loader_instance_t;

void Z_loader_init_module(void);
void Z_loader_instantiate(Z_loader_instance_t*, struct Z_env_instance_t*);
void Z_loader_free(Z_loader_instance_t*);


/* export: 'load_uw8' */
u32 Z_loaderZ_load_uw8(Z_loader_instance_t*, u32);

/* export: 'uncompress' */
u32 Z_loaderZ_uncompress(Z_loader_instance_t*, u32, u32);

#ifdef __cplusplus
}
#endif

#endif  /* LOADER_H_GENERATED_ */
