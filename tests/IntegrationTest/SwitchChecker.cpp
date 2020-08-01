#include "example.h"
#include <iostream>
enum WeekEnum {Mon,Tue,Wed};
enum MonthEnum {Jan,Feb,Mar,Apr,May,June,July};

void TestExprSwitch(){
	int i=0;
	int a=100;
	int b=3;
	switch(i++){
		case 0:a=200;break;
		case 1:a=300;break;
	}

	switch(i+=2){
		case 0:a=200;break;
		case 1:a=300;break;
	}

	switch(b+i/(b-i)){
		case 0:a=200;break;
		case 1:a=300;break;
	}
}

void intTestDefault(){
	int b=0;
	WeekEnum week=Mon;
	switch(week){
		case Mon: b=1;break;
		case Tue: b=2;break;
		default: b=10;break;
	}
}

int TestSwitch(){
	int a=0;
	WeekEnum week=Mon;
	MonthEnum month=Jan;
	Number num=One;
	switch(a){
		case 1: a=1; break;
		case 2: a=2; break;
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
		case 1: i=1; break;
		case 2: i=2; break;
	}
	switch(num){
		case One: a=100;break;
		case Three: a=200;break;
	}
	return 0;
}
