#include "base.h"

void Test( int y1, int m1, int d1,
           int y2, int m2, int d2 )

  {
  _MMDD date1;
  date1.year = y1;
  date1.month = m1;
  date1.day = d1;
  _MMDD date2;
  date2.year = y2;
  date2.month = m2;
  date2.day = d2;

  int n = NumberOfDays( &date1, &date2 );

  printf( "Number of days from %04d-%02d-%02d to %04d-%02d-%02d is %d\n",
          y1, m1, d1, y2, m2, d2, n );

  int m = NumberOfMonths( &date1, &date2 );

  printf( "Number of months from %04d-%02d-%02d to %04d-%02d-%02d is %d\n",
          y1, m1, d1, y2, m2, d2, m );
  }

int main()
  {
  Test( 2022, 5, 26, 2022, 5, 26 );
  Test( 2022, 5, 26, 2023, 5, 26 );
  Test( 2022, 5, 26, 2025, 5, 26 );
  Test( 2022, 5, 26, 2025, 5, 1 );
  return 0;
  }
