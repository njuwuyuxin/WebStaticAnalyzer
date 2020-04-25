#include <stdio.h>

static int gx = 0;

void *inc_1(void *x_void_ptr)
{

    /* increment x to 100 */
    int *x_ptr = (int *)x_void_ptr;
    for(int i = 0; i<100; i++)
    {
        ++(*x_ptr);
        gx++;
    }
 
    printf("x increment finished\n");

    /* the function must return something - NULL will do */
    return NULL;

}

/* this function is run by the second thread */
void *inc_x(void *x_void_ptr)
{

    /* increment x to 100 */
    int *x_ptr = (int *)x_void_ptr;
    for(int i = 0; i<100; i++)
    {
        ++(*x_ptr);
        gx++;
    }
    printf("x increment finished\n");
    return NULL;
}

int main()
{

    int x = 0, y = 0;

    /* show the initial values of x and y */
    printf("x: %d, y: %d\n", x, y);


    inc_x(&x);

    for(int i = 0; i<100; i++)
    {
        x++;
        gx++;
    }

    printf("x increment finished\n");

    return 0;
}
