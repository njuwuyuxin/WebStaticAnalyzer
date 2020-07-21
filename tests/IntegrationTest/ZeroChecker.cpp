int testZero2(int a, int b){
    return a+b;
}

int testZero1(){
	int a=2;
	int b=1; 
    int c;
    c=1/0;
    if(a>0){
        int d=2;
        if(b > 0){
            c=2/(b-1);
        }
    }
    else {
        int e=1;
    }
    try{
        c=1/0;
        throw 1;
    }
    catch(int i){
        c=1;
    }
    return 0;
}
