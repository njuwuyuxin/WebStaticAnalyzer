#include <iostream>
using namespace std;
enum Week {Mon,Tue,Wed};
enum Month {Jan,Feb,Mar};


int main(){
	int a=0;
	Week week;
	switch(a){
		case 1: cout<<1<<endl; break;
		case 2: cout<<2<<endl; break;
	}
	// switch(week){
	// 	case Mon: a=1;break;
	// 	case Tue: a=2;break;
	// }
	// switch(int i=1){
	// 	case 1: cout<<1<<endl; break;
	// 	case 2: cout<<2<<endl; break;
	// }
	return 0;
}
