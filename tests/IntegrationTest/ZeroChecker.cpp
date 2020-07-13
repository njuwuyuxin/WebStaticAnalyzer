int testZero1(){
	int a=2;
	int b=1;
	int c=a/b;
	a<<=b;
	a=~(~a);
	c=!(!c);
	return 0;
}

