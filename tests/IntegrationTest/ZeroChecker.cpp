int t1();
int t2();
int t3();
int t4();
int t5();

int t1(){
    return t2()+t3();
}

int t2(){
    return t4();
}

int t3(){
   return t4();
}

int t4(){
    return t5();
}

int t5(){
    int a = 0;
    int b = 1/a;
    int c = 1/(-a+1-1);
    int d, e;
    d = 1;
    e = 2;
    return t2()+1;
}