int testZero2(){
	int a=2;
	int b=1;
	int c=a/b;
	int d = 3/0;
	int e=0;
	if(e&&a){
		c = a%b;
	}
	else{
		c=a+b;
	}
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
	int f=a>b?5:-5;
	return 0;
}
