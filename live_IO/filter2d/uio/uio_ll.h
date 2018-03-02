#ifndef SRC_UIO_LL_H_
#define SRC_UIO_LL_H_

#include <stdint.h>

#define UIO_MEM_SIZE 65536

void UioWrite32(uint8_t *uioMem, unsigned int offset, uint32_t data);
uint32_t UioRead32(uint8_t *uioMem, unsigned int offset);

#endif
