#include <iostream>
using namespace std;

int x = -1;
unsigned int y = 1;
int z = x > y ? 1 : 0;

int negone() { 
    if(x > y)
      return -1; 
    else
      return 1;
}
unsigned int one() { 
    return 1; 
}

int TestCompare(){
    short int a = -2;
    long int b = 7;
    unsigned int c = 5;
    unsigned long long int d = 2;
    if(a > b){
        cout << "a>b" << endl;
    }
    if(a > c){
        cout << "a>c" << endl;
    }
    if(b > c){
        cout << "b>c" << endl;
    }
    if(-5 > 3){
        cout << "-5>3" << endl;
    }
    if(negone() > one()){
        cout << "-1>1" << endl;
    }
    if(a > d + c){
        cout << "a > d+c" << endl;
    }
    return 0;
}