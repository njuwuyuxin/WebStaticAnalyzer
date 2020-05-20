#include <iostream>
using namespace std;
enum Week {Mon,Tue,Wed};
enum Month {Jan,Feb,Mar,Apr,May,June,July};

void intTestDefault(){
	int b=0;
	Week week=Mon;
	switch(week){
		case Mon: b=1;break;
		case Tue: b=2;break;
		default: b=10;break;
	}
}

int TestSwitch(){
	int a=0;
	Week week=Mon;
	Month month=Jan;
	switch(a){
		case 1: cout<<1<<endl; break;
		case 2: cout<<2<<endl; break;
	}
	switch(week){
		case Mon: a=1;break;
		case Tue: a=2;break;
	}
	switch(month){
		case Jan: a=1;break;
		case Feb: a=2;break;
		case Mar: a=3;break;
		case May: a=5;break;
		case July: a=7;break;
	}
	switch(int i=1){
		case 1: cout<<1<<endl; break;
		case 2: cout<<2<<endl; break;
	}
	return 0;
}
