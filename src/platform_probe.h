#ifndef PLATFORM_PROBE_H
#define PLATFORM_PROBE_H
#include <stdint.h>
int platform_probe(void (*stage)(const char *,uint32_t));
#endif
