#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int g(int x) {
  return x+3;
}

int f(int x) {
  return g(x);
}

void main(void) {
  printf("%d %d\n", f(8)+1, 13);
  unsigned int i = 0x00646c72;
//   unsigned int i = 40;
  printf("H%x Wo%s\n", 57616, &i);  //字符串存储本质就是ASCII码
  printf("x=%d y=%d\n", 3);
  exit(0);
}
