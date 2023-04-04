/* Automatically generated by wasm2c */
#ifndef PLATFORM_H_GENERATED_
#define PLATFORM_H_GENERATED_

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

typedef struct Z_platform_instance_t {
  struct Z_env_instance_t* Z_env_instance;
  /* import: 'env' 'memory' */
  wasm_rt_memory_t *Z_envZ_memory;
  u64 w2c_g0;
  u32 w2c_g1;
  u32 w2c_g2;
  u32 w2c_g3;
  u32 w2c_g4;
  u32 w2c_g5;
  u32 w2c_g6;
} Z_platform_instance_t;

void Z_platform_init_module(void);
void Z_platform_instantiate(Z_platform_instance_t*, struct Z_env_instance_t*);
void Z_platform_free(Z_platform_instance_t*);

/* import: 'env' 'cos' */
f32 Z_envZ_cos(struct Z_env_instance_t*, f32);
/* import: 'env' 'exp' */
f32 Z_envZ_exp(struct Z_env_instance_t*, f32);
/* import: 'env' 'logChar' */
void Z_envZ_logChar(struct Z_env_instance_t*, u32);
/* import: 'env' 'pow' */
f32 Z_envZ_pow(struct Z_env_instance_t*, f32, f32);
/* import: 'env' 'sin' */
f32 Z_envZ_sin(struct Z_env_instance_t*, f32);

/* export: 'time' */
f32 Z_platformZ_time(Z_platform_instance_t*);

/* export: 'isButtonPressed' */
u32 Z_platformZ_isButtonPressed(Z_platform_instance_t*, u32);

/* export: 'isButtonTriggered' */
u32 Z_platformZ_isButtonTriggered(Z_platform_instance_t*, u32);

/* export: 'random' */
u32 Z_platformZ_random(Z_platform_instance_t*);

/* export: 'random64' */
u64 Z_platformZ_random64(Z_platform_instance_t*);

/* export: 'randomf' */
f32 Z_platformZ_randomf(Z_platform_instance_t*);

/* export: 'randomSeed' */
void Z_platformZ_randomSeed(Z_platform_instance_t*, u32);

/* export: 'fmod' */
f32 Z_platformZ_fmod(Z_platform_instance_t*, f32, f32);

/* export: 'cls' */
void Z_platformZ_cls(Z_platform_instance_t*, u32);

/* export: 'setPixel' */
void Z_platformZ_setPixel(Z_platform_instance_t*, u32, u32, u32);

/* export: 'getPixel' */
u32 Z_platformZ_getPixel(Z_platform_instance_t*, u32, u32);

/* export: 'hline' */
void Z_platformZ_hline(Z_platform_instance_t*, u32, u32, u32, u32);

/* export: 'rectangle' */
void Z_platformZ_rectangle(Z_platform_instance_t*, f32, f32, f32, f32, u32);

/* export: 'rectangleOutline' */
void Z_platformZ_rectangleOutline(Z_platform_instance_t*, f32, f32, f32, f32, u32);

/* export: 'circle' */
void Z_platformZ_circle(Z_platform_instance_t*, f32, f32, f32, u32);

/* export: 'circleOutline' */
void Z_platformZ_circleOutline(Z_platform_instance_t*, f32, f32, f32, u32);

/* export: 'line' */
void Z_platformZ_line(Z_platform_instance_t*, f32, f32, f32, f32, u32);

/* export: 'blitSprite' */
void Z_platformZ_blitSprite(Z_platform_instance_t*, u32, u32, u32, u32, u32);

/* export: 'grabSprite' */
void Z_platformZ_grabSprite(Z_platform_instance_t*, u32, u32, u32, u32, u32);

/* export: 'printChar' */
void Z_platformZ_printChar(Z_platform_instance_t*, u32);

/* export: 'printString' */
void Z_platformZ_printString(Z_platform_instance_t*, u32);

/* export: 'printInt' */
void Z_platformZ_printInt(Z_platform_instance_t*, u32);

/* export: 'setTextColor' */
void Z_platformZ_setTextColor(Z_platform_instance_t*, u32);

/* export: 'setBackgroundColor' */
void Z_platformZ_setBackgroundColor(Z_platform_instance_t*, u32);

/* export: 'setCursorPosition' */
void Z_platformZ_setCursorPosition(Z_platform_instance_t*, u32, u32);

/* export: 'playNote' */
void Z_platformZ_playNote(Z_platform_instance_t*, u32, u32);

/* export: 'endFrame' */
void Z_platformZ_endFrame(Z_platform_instance_t*);

/* export: 'sndGes' */
f32 Z_platformZ_sndGes(Z_platform_instance_t*, u32);

#ifdef __cplusplus
}
#endif

#endif  /* PLATFORM_H_GENERATED_ */