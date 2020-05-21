#ifndef WRALOC_H
# define WRALOC_H

size_t						_WRALOC_NUM_ALLO_;
size_t						_WRALOC_NUM_FREE_;

# ifndef WRAP
#  define WRAP 1
# endif

# if WRAP == 1

# include <stdlib.h>
# include <stdio.h>

# define CR "\x1b[m"
# define CL_RD "\x1b[41m"
# define CL_GR "\x1b[42m"
# define CL_BL "\x1b[44m"

typedef unsigned char		t_byte;

typedef struct				mem_list
{		
	size_t					id;
	void					*addr;
	size_t					size;
	t_byte					stat;
	struct mem_list 		*next;
}							t_mem;

t_mem						*_WRALOC_MEM_LIST_;

static t_mem 				*_mem_new(void *addr, size_t size, t_byte stat)
{
	static size_t 			id = 'A';
	t_mem 					*head;

	if (!(head = (t_mem *)malloc(sizeof(t_mem))))
		return (NULL);
	head->id = id++;
	head->addr = addr;
	head->size = size;
	head->stat = stat;
	head->next = NULL;
	return (head);
}

static t_mem				*_mem_append(t_mem **head, t_mem *new)
{
	t_mem 					*tmp;

	tmp = NULL;
	if (!new)
		return (NULL);
	if (*head)
	{
		tmp = *head;
		while (new && tmp && tmp->next)
		{
			tmp = tmp->next;
		}
		tmp->next = new;
	}
	else if (head)
	{
		*head = new;
	}
	return (new);
}

static void					_mem_del(t_mem *mem)
{
	if (mem)
	{
			free(mem);
			mem = NULL;
	}
}

static void					_mem_clear(t_mem **list)
{
	t_mem					*tmp;
	
	while (list && *list)
	{
		tmp = (*list)->next;
		_mem_del(*list);
		*list = tmp;
	}
	_WRALOC_NUM_ALLO_ = 0;
	_WRALOC_NUM_FREE_ = 0;
}

/*
** static void					_mem_remove_by_addr(t_mem **head, void *addr)
** {
** 	t_mem 					*tmp;
** 
** 	tmp = *head;
** 	while (tmp && tmp->next->addr != addr)
** 	{
** 		tmp = tmp->next;
** 	}
** 	tmp->next = tmp->next->next;
** 	tmp = tmp->next;
** 	tmp->next = NULL;
** 	_mem_del(tmp);
** }
*/

static size_t				_mem_get_id(t_mem *head, void *addr)
{
	t_mem 					*tmp;

	tmp = head;
	while (tmp && tmp->addr != addr)
	{
		tmp = tmp->next;
	}
	if (tmp && tmp->addr == addr)
	{
		return (tmp->id);
	}
	return (0);
}

static size_t				_mem_get_size(t_mem *head, void *addr)
{
	t_mem 					*tmp;

	tmp = head;
	while (tmp && tmp->addr != addr)
	{
		tmp = tmp->next;
	}
	if (tmp && tmp->addr == addr)
	{
		return (tmp->size);
	}
	return (0);
}

static void					_mem_set_status(t_mem **head, void *addr, t_byte status)
{
	t_mem 					*tmp;

	tmp = *head;
	while (tmp)
	{
		if (tmp && tmp->addr == addr)
		{
			tmp->stat = status;
		}
		tmp = tmp->next;
	}
	
}

static size_t				_mem_size(t_mem *list)
{
	size_t	size;

	size = 0;
	while (list)
	{
		list = list->next;
		size++;
	}
	return (size);
}

# define R_BORDER CL_BL" |"CR
# define L_BORDER CL_BL"| "CR
# define H_BORDER CL_BL"-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --"CR

static void					_mem_print(t_mem *head)
{
	t_mem *tmp;

	tmp = head;
	if (!tmp)
	{
		printf("\n"CL_RD"_WRALOC_MEM_LIST_NULL_"CR);
		return;
	}
	printf(CL_BL H_BORDER CR"\n");
	printf(CL_BL L_BORDER"WRALOC MEM List of size [%04lu] :                    "
	"                        "R_BORDER CR"\n", _mem_size(tmp));
	while (tmp)
	{
		if (tmp->id < 127)
		{
			printf(CL_BL L_BORDER"  ID %4c", (t_byte)tmp->id);
		}
		else
		{
			printf(CL_BL L_BORDER"  ID %04lu", tmp->id);
		}
		printf("    ADDR <%p>    SIZE %04lu    STATUS %-20s      "R_BORDER CR"\n",
		tmp->addr, tmp->size, ((tmp->stat == 0) ? CL_RD"Allocated"CR: CL_GR"Freed"CR));
		tmp = tmp->next;
	}
	printf(CL_BL H_BORDER CR"\n\n");
}

# ifdef malloc
#  undef malloc
# endif

# ifdef free
#  undef free
# endif

static inline void			*_mymalloc(size_t size)
{
	void 					*ptr;
	size_t 					id;

	if (!(ptr = malloc(size)))
	{
		printf("\x1b[7;41m!!! !!! !!! !!! ALLOC FAILED !!! !!! !!! !!! \x1b[m\n");
		return (NULL);
	}
	_mem_append(&_WRALOC_MEM_LIST_, _mem_new(ptr, size, 0));
	_WRALOC_NUM_ALLO_++;
	id = _mem_get_id(_WRALOC_MEM_LIST_, ptr);
	printf(CL_GR"+++ +++ +++ ALLO_NUM %04lu    ADDR <%p>    SIZE %04lu",
		_WRALOC_NUM_ALLO_, ptr, size);
	if (id < 127)
	{
		printf("    ID %c", (t_byte)id);
	}
	else
	{
		printf("    ID %04lu", id);
	}
	printf(CR "\n");

	return (ptr);
}

static inline void			_myfree(void *ptr)
{
	size_t 					id;

	id = _mem_get_id(_WRALOC_MEM_LIST_, ptr);
	printf(CL_RD "--- --- --- FREE_NUM %04lu    ADDR <%p>    SIZE %04lu",
		   _WRALOC_NUM_FREE_, ptr, _mem_get_size(_WRALOC_MEM_LIST_, ptr));
	if (id < 127)
	{
		printf("    ID %c", (t_byte)id);
	}
	else
	{
		printf("    ID %04lu", id);
	}
	printf(CR "\n");
	if (ptr)
	{
		_WRALOC_NUM_FREE_++;
	}
	_mem_set_status(&_WRALOC_MEM_LIST_, ptr, 1);
	free(ptr);
}

# define malloc(x) _mymalloc(x)
# define free(x) _myfree(x)

# endif /* WRAP */

static inline void			_get_summary(void)
{
# if WRAP == 1
	if (_WRALOC_NUM_ALLO_ && _WRALOC_NUM_FREE_)
	{
		char *color = CL_RD;
		if (_WRALOC_NUM_ALLO_ <= _WRALOC_NUM_FREE_)
			color = CL_GR;
		printf("\n%s", color);
		printf("== == == == == == == == == ==================== == == == == == == == == == == ==");
		printf(CR"\n%s", color);
		printf("== == == == == == == == == ==  A %03lu  F %03lu  == == == == == == == == == == == ==", _WRALOC_NUM_ALLO_, _WRALOC_NUM_FREE_);
		printf(CR"\n%s", color);
		printf("== == == == == == == == == ==================== == == == == == == == == == == ==");
		printf(CR"\n");
		_mem_print(_WRALOC_MEM_LIST_);
	}
# endif
}

static inline void 			__attribute__ ((constructor)) ctor()
{
	_WRALOC_NUM_ALLO_ = 0;
	_WRALOC_NUM_FREE_ = 0;
}

static inline void 			__attribute__ ((destructor))  dtor() 
{
# if WRAP == 1
	_get_summary();
	_mem_clear(&_WRALOC_MEM_LIST_);
# endif
}

#endif /* WRALLOC_H */
