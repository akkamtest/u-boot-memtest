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
void error(ulong adr, ulong good, ulong bad, int test_num)
{
	printf ("TEST number: %d\n", test_num);
	printf ("	Faulty address: %08lx\n", adr);
	printf ("		>Expected result: %08lx\n", good);
	printf ("		>Obtained value : %08lx\n", bad);	
}

#ifdef DEBUG_MEMTEST
	/*  Allows to visualize which test return the error, the faulty address and the difference between the expected value and the one read */
	void mtest_debug(int test_num, ulong adr, ulong value)
{
	printf ("TEST number: %d\n", test_num);
	printf ("	> Address: %08lx\n", adr);
	printf ("	> Value  : %08lx\n", value);	
}
#endif

/* Walking ones */
/* Test 0 [Address test, 8bits walking ones, no cache] */
// Start: Starting address of the test
// End  : Ending address of the test
// stop_after_err: 1 stop the test after an error / 0 let the test running
unsigned char addr_tst1(ulong start, ulong end, unsigned char stop_after_err)
{
	unsigned char i, mask, *p, *pe;
	int test_num = 1;

	/* Initialise tested memory range */
	p = (unsigned char *)start;
	pe = (unsigned char *)end;	
		
	/* Disable cache */
	icache_disable();
	dcache_disable();
	
	/* test each bit for all address */
	for (; p <= pe; p++) 
	{
		//mtest_debug(test_num, (unsigned long long int)p, *p);
		for (i = 0; i<8; i++) 
		{
			mask = 1<<i;
			*p &= mask;
			*p |= mask;
#ifdef DEBUG_MEMTEST
			mtest_debug(test_num, (ulong)p, *p);
#endif
			if(*p != mask) 
			{
				error((ulong)p, mask, (ulong)*p, test_num);
				if (stop_after_err == 1)
				{
					return(1);	
				}
			}
		}
	}
	return(0);
}

unsigned char addr_tst2(ulong start, ulong end, char stop_after_err)
{
	ulong *p, *pe;
	int test_num = 2;	

	/* Initialise tested memory range */
	p = (ulong*)start;
	pe = (ulong*)end;
	
	/* Write each address with it's own address */	
	for (; p <= pe; p++) 
	{		
		*p = (ulong)p;
#ifdef DEBUG_MEMTEST
		mtest_debug(test_num, (ulong)p, *p);
#endif
	}

	/* Each address should have its own address */
	p = (ulong *)start;
	pe = (ulong *)end;
	for (; p <= pe; p++) 
	{		
		if(*p != (ulong)p) 
		{
			error((ulong)p, (ulong)p, *p, test_num);
			if (stop_after_err == 1)
			{
				return(1);	
			}
		}
	}
	return(0);
}
