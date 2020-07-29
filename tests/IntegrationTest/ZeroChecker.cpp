static int
add_date_timedelta(int a, int b, int negate)
{
    int year = 1;
    int month = 2;
    int deltadays = 3;

    // seexp, 20, div/mod 0
    int bug_seexp = year % (month & 0x0); // seexp, warning

    return 0;
}