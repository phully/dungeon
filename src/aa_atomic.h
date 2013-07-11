#ifndef AA_ATOMIC_H
#define AA_ATOMIC_H

static inline int do_fetch_and_add(int *value, int add) 
{
	__asm__ volatile (

		"lock;"
		"    xaddl  %0, %1;   "

		: "+r" (add) : "m" (*value) : "cc", "memory");

	return add;
}

inline int atomic_increase(int *value)
{
	return do_fetch_and_add(value, 1); 
}

inline int atomic_decrease(int *value)
{
	return do_fetch_and_add(value, -1);
}


#endif