
#include <iostream>
using namespace std;

void TestLoop()
{
    int x = 1;
    while (1)
    {
        cout << "Infinity Loop!Test" << endl;
    }
}

void Test2()
{
    for(int i=0;;i++)
    {
        cout<<"Test!"<<endl;
    }
}
