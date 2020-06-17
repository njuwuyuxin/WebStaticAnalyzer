
#include <iostream>
using namespace std;
#ifdef A
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

    x=0;
    while(true)
    {
        if(x>=0)
            break;
    }
}

void TestWhile()
{
    int x = 1;
    while (x)
    {
        // cout << "Infinity Loop!Test" << endl;
    }

    while(true)
    {
        if(x < 0)
            break;
    }
}

void TestFor()
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
#endif

void TestNestedLoop()
{
    for(int i = 0;i<100;++i)
        for(int j = 0;j<10;++j)
        {}

    int x = 0;
    while(x<100)
    {
        int y = 10;
        while(y>0)
            y--;
        
        x++;
    }

    x= 0;
    while(x<100)
    {
        int y = 1;
        for(int i = y;i>y;i++)
        {
            while(i++)
            {}
        }
    }
}