#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
  printf("sizeof char %d\n", sizeof(char));
  printf("sizeof short %d\n", sizeof(short));
  printf("sizeof int %d\n", sizeof(int));
  printf("sizeof long %d\n", sizeof(long));
  printf("sizeof float %d\n", sizeof(float));
  printf("sizeof double %d\n", sizeof(double));

  printf("sizeof uint8_t %d\n", sizeof(uint8_t));
  printf("sizeof uint16_t %d\n", sizeof(uint16_t));
  printf("sizeof uint32_t %d\n", sizeof(uint32_t));
  printf("sizeof uint64_t %d\n", sizeof(uint64_t));

  printf("sizeof int8_t %d\n", sizeof(int8_t));
  printf("sizeof int16_t %d\n", sizeof(int16_t));
  printf("sizeof int32_t %d\n", sizeof(int32_t));
  printf("sizeof int64_t %d\n", sizeof(int64_t));

  return 0;
}
