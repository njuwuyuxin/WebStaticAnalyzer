
#include <iostream>
using namespace std;

void TestLoop()
{
    int x = 1;
    while (x)
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

    for(unsigned int i = 0;i>0;i++)
    {
        cout<<"Test for range!"<<endl;
    }
}
