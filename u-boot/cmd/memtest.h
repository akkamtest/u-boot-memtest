
#ifndef _MEMTEST_H_
#define _MEMTEST_H_

//typedef unsigned long ulong;


#define MEMTEST_LOWEST_ADDR		0x0000000000000000
#define MEMTEST_HIGHEST_ADDR	0xFFFFFFFFFFFFFFF8
#define MOD_SZ			   		20
#define MEMTEST_MOD_OFFSET 		0
#define MEMTEST_ITERATION  		2
#define MEMTEST_RAND_SEED_1		5
#define MEMTEST_RAND_SEED_2		17
#define MEMTEST_PATTERN_64_A	0xCAFEDECADECACAFE
#define MEMTEST_PATTERN_64_B	0xA5A5A5A5A5A5A5A5
#define MEMTEST_PATTERN_64_C	0x5A5A5A5A5A5A5A5A
#define IS_MEMTEST_1		 	0x0001
#define IS_MEMTEST_2		 	0x0002
#define IS_MEMTEST_3		 	0x0004
#define IS_MEMTEST_4		 	0x0008
#define IS_MEMTEST_5		 	0x0010
#define IS_MEMTEST_6		 	0x0020
#define IS_MEMTEST_7		 	0x0040
#define IS_MEMTEST_8		 	0x0080
#define IS_MEMTEST_9		 	0x0100
#define IS_MEMTEST_10		 	0x0200



unsigned char verify_start_param(ulong start);
unsigned char verify_end_param(ulong end);
unsigned char verify_stop_param(ulong stop);
unsigned long long int rand1 (unsigned char salt);
void reset_seed(void);
void error(ulong adr, ulong good, ulong bad, int test_num);
void mtest_debug(int test_num, ulong adr, ulong value);
unsigned char addr_tst1(ulong start, ulong end, unsigned char stop_after_err);
unsigned char addr_tst2(ulong start, ulong end, char stop_after_err);
unsigned char movinv (int iter, ulong start, ulong end, unsigned char stop_after_err);
unsigned char movinv_8bit (int iter, ulong start, ulong end, ulong stop_after_err);
unsigned char movinvr (int iter, ulong start, ulong end, unsigned char stop_after_err);
unsigned char movinv64(ulong start, ulong end, unsigned char stop_after_err);
unsigned char rand_seq(unsigned char iter_rand, ulong start, ulong end, unsigned char stop_after_err);
unsigned char modtst(int offset, int iter, ulong p1, ulong p2, ulong start, ulong end, unsigned char stop_after_err);
unsigned char bit_fade_fill(ulong p1, ulong start, ulong end, unsigned char stop_after_err);
unsigned char bit_fade_chk(ulong p1, ulong start, ulong end, unsigned char stop_after_err);
void wait (unsigned int sec);
#endif /* _TEST_H_ */
