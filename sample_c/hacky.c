#include <stdio.h>
#include <unistd.h>
#include <stdint.h>

#define TARGET_ADDRESS 0x1060

int main(int argc, const char* argv[]){
  uint16_t *ptr = (uint16_t*) TARGET_ADDRESS;

  printf("ptr is set to %p\n", ptr);

  for (int i = 0; i < 20; i++){
    //Writing some junk
    *ptr = 0xFF;
    ptr++;
  }

  printf("ptr now is at %p\n", ptr);

  return 0;
}
