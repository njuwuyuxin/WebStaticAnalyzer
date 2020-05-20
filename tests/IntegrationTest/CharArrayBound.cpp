#include <cstdio>
#include <cstring>

void foo() {
  char str[10] = "example";

  str[7] = '\0';
  str[8] = '\0';
  7[str] = '\0';
  8[str] = '\0';
}

int TestCharArrayBound() {

  char str[10] = "example";

  for (int i = 0; i < sizeof(str); i++) {
    str[i] = 0;
  }

  foo();

  return 0;
}
