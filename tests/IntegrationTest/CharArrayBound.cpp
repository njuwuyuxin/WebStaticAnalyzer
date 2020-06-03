#include <cstdio>
#include <cstring>

int foo() {
  char str[10] = "example";
  int i = 8;
  i = 9;

  str[7] = '\0';
  (str)[(8)] = '\0';
  7[str] = '\0';
  i[str] = '\0';

  return 0;
}

int TestCharArrayBound() {

  char str[10] = "example";

  for (int i = 0; i < sizeof(str); i++) {
    str[i] = 0;
  }

  str[foo()] = 0;

  return 0;
}
