
#include <iostream>
using namespace std;
// #define NormalLoopTest
#ifdef NormalLoopTest
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

    while(true)
    {
        break;
    }

    for(int i = 0;i<100;++i)
    {
        break;
    }
}
#endif

// #define SimpleTest
#ifdef SimpleTest
void TestWhile()
{
    while (1)
    {
        // cout << "Infinity Loop!Test" << endl;
    }

    int x = 1;
    while(true)
    {
        if(x > 0)
            break;
    }

    while(x)
    {

    }
}

void TestFor()
{
    for(int i=0;;i++)
    {
        break;
    }

    for(unsigned int j = 0;j<1;)
    {

    }

    for(unsigned int i = 0;i>0;i++)
    {
        
    }

}
#endif

#ifdef CondExprTest
void TestLoop()
{  int i=0;
    for( i= 0;;i++)
    {
        if(i>10)//can this be detect as a condition expression ? Ans : Nope!!!
            break;
    }

i=0;
    while(true)
    {
        if(i>10)
            break;
    }
}
#endif


#define NestedLoop
#ifdef NestedLoop
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
#endif