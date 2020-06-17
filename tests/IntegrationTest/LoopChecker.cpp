
#include <iostream>
using namespace std;

void NormalLoop()
{
    int x = 0;
    while(x>100)
    {
        // cout<<x<<endl;
        x++;
    }

    for(x= 0;x<100;x++)
    {    // cout<<x<<endl;
    }
}

void TestLoop()
{
    int x = 1;
    while (x)
    {
        // cout << "Infinity Loop!Test" << endl;
    }
}

void Test2()
{
    for(int i=0;;i++)
    {
        // cout<<"Test!"<<endl;
    }

    for(unsigned int i = 0;i>0;i++)
    {
        // cout<<"Test for range!"<<endl;
    }

    for(unsigned int j = 0;j<1;)
    {
        
    }
}
