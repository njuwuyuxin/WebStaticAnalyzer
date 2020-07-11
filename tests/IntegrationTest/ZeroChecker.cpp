int testZero1(){
	int a=2;
	int b=0;
	int c=a/b;
	int d = 3/0;
	return 0;
}

int testZero2(){
	int a=1;
	int b=0;
	int c;
	int e=0;
	int f=1;
	if(e&&a){
		c = a%b;
	}
	else{
		c=a+b;
	}
	if(f&&a){
		f=a/b;
	}
	else{
		f=a+b;
	}
	return 0;
}

int testZero3(){
	int a=1;
	int b=0;
	int e=2;
	switch(b){
		case 0:{
			e=a/b;
		}break;
		case 1:case 2:{
			e=1;
		}break;
		default:{
			e=3;
		}break;
	}
	a=b/e;
	return 0;
}
