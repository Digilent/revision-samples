#include "uio/uio_ll.h"

void UioWrite32(uint8_t *uioMem, unsigned int offset, uint32_t data)
{
  *((uint32_t*) (uioMem+offset)) = data;
}

uint32_t UioRead32(uint8_t *uioMem, unsigned int offset)
{
  return *((uint32_t*) (uioMem+offset));
}
