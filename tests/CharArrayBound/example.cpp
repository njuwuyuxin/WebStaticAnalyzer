#include <cstring>

int main()
{

    char str[8] = "example";

    for(int i = 0; i <= strlen(str); i++)
    {
        str[i] = str[i + 1];
    }

    return 0;
}
