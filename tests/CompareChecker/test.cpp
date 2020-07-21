#include <iostream>
using namespace std;

typedef ssize_t Py_ssize_t;

int test(){
    Py_ssize_t size = 50;
    char ch = 'a';
    if(size > ch)
        return 0;
    else
        return 1;
}