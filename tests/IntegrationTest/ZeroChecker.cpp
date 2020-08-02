int testZero2(){
    return 0;
}

int testZero(){
    int a=1;
    int b=2;
    int c=1/(2+2/3-2);
    int d=0;
    if(d>0){
        a = 1/0;
    }
    else{
        b=a/0;
    }
    d = 1/((a++)-1);
    d = 1/(a-2);
    int e = 1/testZero2();
    try{
        a=1/0;
        throw 1;
    }
    catch(int i){

    }
    return 0;
}