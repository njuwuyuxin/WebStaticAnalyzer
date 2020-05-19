#include <cstdio>
#include <cstring>

void foo() {
  char str[8] = "example";

  for (int i = 0; i <= 8; i++) {
    printf("%c", str[i]);
  }

  str[7] = '\0';
  str[8] = '\0';
  7[str] = '\0';
  8[str] = '\0';
}

int TestCharArrayBound() {

  char str[8] = "example";
  char str2[8] = "example";

  for (int i = 0; i <= strlen(str); i++) {
    str[i] = str[i + 1];
  }

  for (int i = 0; i <= 8; i++) {
    str2[i] = 0;
  }

  foo();

  return 0;
}
