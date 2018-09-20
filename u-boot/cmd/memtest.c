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

unsigned char movinv (int iter, ulong start, ulong end, unsigned char stop_after_err)
{
	int i;
	int test_num = 3;
	ulong *p, *pe;
	ulong p1 = 0xA5A5A5A5A5A5A5A5;
	
	/* Enable cache */
	icache_enable();
	dcache_enable();
	
	p = (ulong*)start;
	pe = (ulong*)end;
	
	/* Initialize memory with the initial pattern.  */
	for (; p <= pe; p++) 
	{
		*p = p1;
#ifdef DEBUG_MEMTEST
		mtest_debug(test_num, (ulong)p, *p);
#endif
	}
	
	/* Do moving inversions test. Check for initial pattern and then
	 * write the complement for each memory location. Test from bottom
	 * up and then from the top down.  */
	for (i=0; i<iter; i++) 
	{
		p = (ulong*)start;
		pe = (ulong*)end;
		for (; p <= pe; p++) 
		{
			if (*p != p1) 
			{
				error((unsigned long long int)p, p1, *p, test_num);
				if (stop_after_err == 1)
				{			
					return(1);	
				}
			}
			*p = ~p1;
#ifdef DEBUG_MEMTEST
		mtest_debug(test_num, (ulong)p, *p);
#endif
		}
		
		p = (ulong*)start;
		pe = (ulong*)end;
		do 
		{			
			if (*p != ~p1) 
			{
				error((unsigned long long int)p, ~p1, *p, test_num);
				if (stop_after_err == 1)
				{
					return(1);	
				}
			}
			*p = p1;
		} while (--p >= pe);
	}
	return(0);
}


unsigned char movinv_8bit (int iter, ulong start, ulong end, ulong stop_after_err)
{
	int i;
	int test_num = 4;
	unsigned char *p, *pe;
	unsigned char p1 = 0x0F;
	unsigned char p2 = ~p1;

	/* Enable cache */
	icache_enable();
	dcache_enable();
	
	p = (unsigned char*)start;
	pe = (unsigned char*)end;
	
	/* Initialize memory with the initial pattern.  */
	for (; p <= pe; p++)
	{
		*p = p1;
#ifdef DEBUG_MEMTEST
		mtest_debug(test_num, (ulong)p, *p);
#endif
	}
	
	/* Do moving inversions test. Check for initial pattern and then
	 * write the complement for each memory location. Test from bottom
	 * up and then from the top down.  */
	for (i=0; i<iter; i++) 
	{
		p = (unsigned char*)start;
		pe = (unsigned char*)end;

		for (; p <= pe; p++) 
		{
			if (*p != p1) 
			{
				error((ulong)p, p1, (unsigned char)*p, test_num);
				if (stop_after_err == 1)
				{
					return(1);	
				}
			}
			*p = p2;
#ifdef DEBUG_MEMTEST
		mtest_debug(test_num, (ulong)p, *p);
#endif
		}
		p = (unsigned char*)start;
		pe = (unsigned char*)end;
		do 
		{
			if (*p != p2) 
			{
				error((ulong)p, p2, (unsigned char)*p, test_num);
				if (stop_after_err == 1)
				{
					return(1);	
				}
			}
			*p = p1;
		} while (--p >= pe);
	}
	return(0);
}


unsigned char movinvr (int iter, ulong start, ulong end, unsigned char stop_after_err)
{
	int i;
	int test_num = 5;
	ulong *p, *pe;
	ulong p1;
	
	/* Enable cache */
	icache_enable();
	dcache_enable();
	
	/* Initialise random pattern */
	reset_seed();
	p1 = rand1(iter);

	/* Initialise tested memory range */
	p = (ulong*)start;
	pe = (ulong*)end;
	
	/* Initialize memory with the initial pattern */
	for (; p <= pe; p++) 
	{
		*p = p1;
#ifdef DEBUG_MEMTEST
		mtest_debug(test_num, (ulong)p, *p);
#endif	
	}

	/* Do moving inversions test. Check for initial pattern and then
	 * write the complement for each memory location. Test from bottom
	 * up and then from the top down.  */
	for (i=0; i<iter; i++) 
	{
		p = (ulong*)start;
		pe = (ulong*)end;

		for (; p <= pe; p++) 
		{
			if (*p != p1) 
			{
				error((ulong)p, p1, *p, test_num);
				if (stop_after_err == 1)
				{
					return(1);	
				}
			}
			*p = ~p1;
#ifdef DEBUG_MEMTEST
		mtest_debug(test_num, (ulong)p, *p);
#endif
		}

		pe = (ulong*)start;
		p = (ulong*)end;
		do 
		{
			if (*p != ~p1) 
			{
				error((ulong)p, ~p1, *p, test_num);
				if (stop_after_err == 1)
				{
					return(1);	
				}
			}
			*p = p1;
#ifdef DEBUG_MEMTEST
		mtest_debug(test_num, (ulong)p, *p);
#endif
		} while (--p >= pe);
	}
	return(0);
}


unsigned char movinv64(ulong start, ulong end, unsigned char stop_after_err)
{
	int k=0;
	ulong *p, *pe, pat, comp_pat, p1 = MEMTEST_PATTERN_64_A;
	int test_num = 7;
	ulong tab[64];
	unsigned char tab_compl = 0;
	
	/* Enable cache */
	icache_enable();
	dcache_enable();
	
	/* Initialise tested memory range */
	p = (ulong*)start;
	pe = (ulong*)end;

	/* Initialize memory with the initial pattern.  */
	k = 0;
	pat = p1;	
	while (p <= pe)
	{
		*p = pat;
#ifdef DEBUG_MEMTEST
		mtest_debug(test_num, (ulong)p, *p);
#endif		
		if (tab_compl == 0)
		{
			
			tab[k] = pat;
		}
		
		if (++k >= 64)
		{
			pat = p1;
			k = 0;
			tab_compl = 1;
		}
		else
		{
			pat = pat << 1;
		}
		p++;
	}


	/* Do moving inversions test. Check for initial pattern and then
	 * write the complement for each memory location. Test from bottom
	 * up and then from the top down.  */
	p = (ulong*)start;
	pe = (ulong*)end;
	k = 0;
	while (1)
	{			
		pat = tab[k];
		if (*p != pat)
		{
			error((unsigned long long int)p, pat, *p, test_num);
			if (stop_after_err == 1)
			{
				return(1);	
			}
		}
		comp_pat = ~pat;
		*p = comp_pat;
#ifdef DEBUG_MEMTEST
		mtest_debug(test_num, (ulong)p, *p);
#endif
		if (p >= pe)
		{
			break;
		}
		p++;

		if (++k >= 64)
		{
			k = 0;
		}

	}

	pe = (ulong*)start;
	p = (ulong*)end;
	while (1)
	{		
		pat = tab[k];
		comp_pat = ~pat;	
		if (*p != comp_pat)
		{
			error((unsigned long long int)p, comp_pat, *p, test_num+1);
			if (stop_after_err == 1)
			{
				return(1);	
			}
		}
		*p = pat;
		if (p <= pe)
		{
			break;
		}
		p--; 
		if (--k < 0)
		{
			k = 63;
		}
	}

	return(0);
}

unsigned char rand_seq(unsigned char iter_rand, ulong start, ulong end, unsigned char stop_after_err)
{
	int i;
	ulong *p, *pe, num;
	int test_num = 8;
	
	/* Enable cache */
	icache_enable();
	dcache_enable();
	
	reset_seed();
	
	/* Initialise tested memory range */
	p = (ulong*)start;
	pe = (ulong*)end;
	
	for (; p <= pe; p++) 
	{
		*p = rand1(iter_rand);
#ifdef DEBUG_MEMTEST
		mtest_debug(test_num, (ulong)p, *p);
#endif	
	}


	/* Do moving inversions test. Check for initial pattern and then
	 * write the complement for each memory location. */
	for (i=0; i<2; i++)
	{
		reset_seed();
		p = (ulong*)start;
		pe = (ulong*)end;
	
		for (; p <= pe; p++)
		{			
			num = rand1(iter_rand);
			if (i)
			{
				num = ~num;
			}
			if (*p != num)
			{
				error((ulong)p, num, *p, test_num);
				if (stop_after_err == 1)
				{
					return(1);	
				}
			}
			*p = ~num;
#ifdef DEBUG_MEMTEST
		mtest_debug(test_num, (ulong)p, *p);
#endif	
		}
	}
	return(0);
}
