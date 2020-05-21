#ifndef WRALOC_H
# define WRALOC_H

# include <stdlib.h>
# include <stdio.h>

size_t	_WRALOC_NUM_ALLO_;
size_t	_WRALOC_NUM_FREE_;

#if WRAP == 1

# ifdef malloc
#  undef malloc
# endif

# ifdef free
#  undef free
# endif

static inline void _myfree(void *ptr);
static inline void *_mymalloc(size_t size);
static inline void _get_summary(void);

static inline void	*_mymalloc(size_t size)
{
	void *ptr;
	if (!(ptr = malloc(size)) /* malloc fails, exit */
	{
		printf("\x1b[7;41m!!! !!! !!! !!! ALLOC FAILED !!! !!! !!! !!! \x1b[m\n");
		exit(1);
	}
	printf_("\x1b[42m+++ ++_+ +++ +++ +++ +++ +++ +++ +++ ALLOC_NUM %04lu <%2p>\x1b[0m\n", _WRALOC_NUM_ALLO_, ptr);
	_WRALOC_NUM_ALLO_++;
	return (ptr);
}

static inline void	_myfree(void *ptr)
{
	printf("\x1b[41m--- --- --- --- --- --- --- --- --- FREE_NUM %04lu <%2p>\x1b[0m\n", _WRALOC_NUM_FREE_, ptr);
	if (ptr) /* do not count NULL */
		_WRALOC_NUM_FREE_++;
	free(ptr);
}

# define malloc(x) _mymalloc(x)
# define free(x) _myfree(x)

#endif /* WRAP */

static inline void	_get_summary(void)
{
	printf("\x1b[7;52m");
	printf("=== === === === === === === === === === === =====================\n");
	printf("=== === === === === === === === === === === === A %04lu F %04lu ===\n", _WRALOC_NUM_ALLO_, _WRALOC_NUM_FREE_);
	printf("=== === === === === === === === === === === =====================");
	printf("\x1b[0m\n");
}

static inline void __attribute__ ((constructor)) ctor()
{
	_WRALOC_NUM_ALLO_ = 0;
	_WRALOC_NUM_FREE_ = 0;
}

static inline void __attribute__ ((destructor))  dtor() 
{
	_get_summary();
}

#endif /* WRALLOC_H */

