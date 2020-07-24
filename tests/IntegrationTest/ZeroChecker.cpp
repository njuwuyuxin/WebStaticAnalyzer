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
    return t2()+1;
}