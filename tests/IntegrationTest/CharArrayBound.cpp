#include <cstdio>
#include <cstring>

int foo() {
  char str[10] = "example";
  int a = 3;
  int b = 4;

  (a + b)[str] = str[0];
  str[b * 2] = '\0';

  return b;
}

int TestCharArrayBound() {

  char str[10] = "example";

  for (int i = 0; i < sizeof(str); i++) {
    str[i] = 0;
  }

  str[foo()] = 0;

  return 0;
}
