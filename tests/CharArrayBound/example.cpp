#include <cstdio>
#include <cstring>

void foo() {
  char str3[8] = "example";

  for (int i = 0; i <= 8; i++) {
    printf("%c", str3[i]);
  }
}

int main() {

  char str[8] = "example";
  char str2[8] = "example";

  for (int i = 0; i <= strlen(str); i++) {
    str[i] = str[i + 1];
  }

  for (int i = 0; i <= 8; i++) {
    str2[i] = 0;
  }

  return 0;
}
