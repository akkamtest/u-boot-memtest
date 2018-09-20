//#include <stdio.h>
//#include "stdint.h"

#include <common.h>
#include "memtest.h"

// Uncomment for debug
#define DEBUG_MEMTEST 1

static unsigned long long int SEED_A;
static unsigned long long int SEED_B;
static unsigned long long int SEED_C;
static unsigned long long int SEED_D;

unsigned long long int rand1 (unsigned char salt)
{
   static unsigned int a = 18000, b = 30903, c = 15333, d = 21041;

   SEED_A  = salt*a*(SEED_A &65535) + (SEED_A >>16);
   SEED_B  = salt*b*(SEED_B &65535) + (SEED_B >>16);
   SEED_C  = salt*c*(SEED_C &65535) + (SEED_C >>16);
   SEED_D  = salt*d*(SEED_D &65535) + (SEED_D >>16);

   return ((SEED_A <<48) + ((SEED_B &65535)<<32) + (SEED_C <<16) + (SEED_D &65535));
}

void reset_seed(void)
{
	unsigned long long int seed1 = 521288629;
	unsigned long long int seed2 = 362436069;
	unsigned long long int seed3 = 123456789;
	unsigned long long int seed4 = 917456120;
	
   SEED_A  = seed1;   
   SEED_B  = seed2;
   SEED_C  = seed3;   
   SEED_D  = seed4;
}

/*  Allows to visualize which test return the error, the faulty address and the difference between the expected value and the one read */
void error(unsigned long long int adr, unsigned long long int good, unsigned long long int bad, int test_num)
{
	printf ("TEST number: %d\n", test_num);
	printf ("	Faulty address: %llx\n", adr);
	printf ("		>Expected result: %llx\n", good);
	printf ("		>Obtained value : %llx\n", bad);	
}

#ifdef DEBUG_MEMTEST
	/*  Allows to visualize which test return the error, the faulty address and the difference between the expected value and the one read */
	void mtest_debug(int test_num, unsigned long long int adr, unsigned long long int value)
{
	printf ("TEST number: %d\n", test_num);
	printf ("	> Address: %llx\n", adr);
	printf ("	> Value  : %llx\n", value);	
}
#endif

/* Walking ones */
/* Test 0 [Address test, 8bits walking ones, no cache] */
// Start: Starting address of the test
// End  : Ending address of the test
// stop_after_err: 1 stop the test after an error / 0 let the test running
unsigned char addr_tst0(unsigned long long int *buf, unsigned long long int start, unsigned long long int end, unsigned char stop_after_err)
{
	unsigned char i;
	unsigned char *p;
	unsigned char *pe;
	unsigned char mask;

	int test_num = 1;

	/* Initialise tested memory range */
	p = (unsigned char *)start;
	pe = (unsigned char *)end;
	/* test each bit for all address */
	for (; p <= pe; p++) 
	{
		//mtest_debug(test_num, (unsigned long long int)p, *p);
		for (i = 0; i<8; i++) 
		{
			mask = 1<<i;
			*p &= mask;
			*p |= mask;
			if(*p != mask) 
			{
				error((unsigned long long int)p, mask, (unsigned long long int)*p, test_num);
				if (stop_after_err == 1)
				{
					return(1);	
				}
			}
		}
		//mtest_debug(test_num, (unsigned long long int)p, *p);
	}
	return(0);
}


unsigned char addr_tst1(unsigned long long int *buf, unsigned long long int start, unsigned long long int end, char stop_after_err)
{
	unsigned long long int *p;
	unsigned long long int *pe;
	int test_num = 2;	

	/* Initialise tested memory range */
	p = (unsigned long long int *)start;
	pe = (unsigned long long int *)end;
	mtest_debug(test_num, (unsigned long long int)p, *p);
	mtest_debug(test_num, (unsigned long long int)pe, *pe);
	/* Write each address with it's own address */	
	for (; p <= pe; p++) 
	{		
		*p = (unsigned long long int)p;
#ifdef DEBUG_MEMTEST
		mtest_debug(test_num, (unsigned long long int)p, *p);
#endif
	}

	/* Each address should have its own address */
	p = (unsigned long long int *)start;
	pe = (unsigned long long int *)end;
	for (; p <= pe; p++) 
	{
		
		if(*p != (unsigned long long int)p) 
		{
			error((unsigned long long int)p, (unsigned long long int)p, *p, test_num);
			if (stop_after_err == 1)
			{
				return(1);	
			}
		}
	}
	mtest_debug(0, 0, 0);
	return(0);
}
