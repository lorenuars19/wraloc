#ifndef WRALOC_H
# define WRALOC_H

# include <stddef.h>
size_t						_WRALOC_NUM_ALLO_;
size_t						_WRALOC_NUM_FREE_;

# ifndef WRAP
#  define WRAP 1
# endif

# if WRAP == 1

# include <execinfo.h>
# include <string.h>
# include <stdlib.h>
# include <stdio.h>

# define CR "\x1b[m"
# define CL_RD "\x1b[41m"
# define CL_GR "\x1b[42m"
# define CL_YE "\x1b[43m"
# define CL_BL "\x1b[44m"

#define BT_BUF_SIZE 100
#define BUFSIZE 512

typedef unsigned char		t_byte;

typedef struct				mem_list
{
	size_t					id;
	void					*addr;
	size_t					size;
	t_byte					stat;
	char					*stack_trace;
	struct mem_list 		*next;
}							t_mem;

t_mem						*_WRALOC_MEM_LIST_;

size_t			_hasto(char *s, char c)
{
	size_t		to;

	to = 0;
	while (s && s[to])
	{
		if (s[to] == c)
			return (to + 1);
		to++;
	}
	if (s && s[to] == c)
		return (to + 1);
	return (0);
}

char			*_jointo(char *s1, char *s2, char **tofree)
{
	char		*a;
	size_t		sl1;
	size_t		sl2;
	size_t		i;

	a = NULL;
	sl1 = _hasto(s1, '\0');
	sl2 = _hasto(s2, '\0');
	if (!(a = (char *)malloc((sl1 + sl2 + 1) * sizeof(char))))
	{
		if (tofree != NULL && *tofree != NULL)
			free(*tofree);
		return (NULL);
	}
	i = 0;
	while (sl1 && s1 && *s1 && *s1 != '\n')
		a[i++] = *s1++;
	while (sl2 && s2 && *s2 && *s2 != '\n')
		a[i++] = *s2++;
	a[i] = '\0';
	if (tofree != NULL && *tofree != NULL)
		free(*tofree);
	return (a);
}

int				_in_charset(char c, const char *set)
{
	while (set && *set)
	{
		if (*set == c)
			return (1);
		set++;
	}
	return (0);
}

char			*_trim_addr(const char *s, const char *set)
{
	char		*new;
	ssize_t		offset;
	ssize_t		len_cpy;
	ssize_t		i;

	if (!s || !set[0])
		return (NULL);
	len_cpy = strlen(s);
	while (!(_in_charset(s[len_cpy], set)))
		len_cpy--;
	if (len_cpy < 0)
		return (NULL);
	if (!(new = (char *)malloc((len_cpy + 1) * sizeof(char))))
		return (NULL);
	i = 0;
	while (i < len_cpy)
	{
		if (_in_charset(s[i], "()"))
			new[i] = ' ';
		else
			new[i] = s[i];
		i++;
	}
	new[i] = '\0';
	return (new);
}
char			*_trim(const char *s, const char *set)
{
	char		*new;
	ssize_t		offset;
	ssize_t		slen;
	ssize_t		i;
	ssize_t		j;

	if (!s || !set[0])
		return (NULL);
	slen = strlen(s);
	i = 0;
	while (!(_in_charset(s[i], set)))
		i++;

	j = i;
	while(!(_in_charset(s[j], ":")))
		j++;
	slen = j - i;
	printf("slen %ld | i %ld | j %ld| j - i %ld\n", slen, i, j, j - i);
	if (slen < 0)
		return (NULL);
	if (!(new = (char *)malloc((slen + 1) * sizeof(char))))
		return (NULL);
	i = 0;
	while (i < j - slen)
	{
		new[i] = s[i];
		i++;
	}
	offset = j - i;
	while (i < slen)
	{
		new[i] = s[offset + i];
		i++;
	}
	new[i] = '\0';
	return (new);
}

int				_parse_output(char *cmd, char **new)
{
	char		buf[BUFSIZE];
	FILE		*fp;
	char		*trimmed = NULL;

	if ((fp = popen(cmd, "r")) == NULL)
	{
		return (1);
	}
	while (fgets(buf, BUFSIZE, fp) != NULL)
	{
		trimmed = _trim(buf, " ");
		if (_hasto(buf, '?'))
		{
			return (2);
		}
		if (!(*new = _jointo(*new, trimmed, new)))
		{
			free(trimmed);
			return (1);
		}
		free(trimmed);

	}
	if(pclose(fp))
	{
		return (1);
	}
	return 0;
}

char			*_get_stack_trace(void)
{
	int			nptrs = 0;
	int			ret = 0;
	void		*buffer[BT_BUF_SIZE];
	char		**strings = NULL;
	char		*tmp = NULL;
	char		*cmd = NULL;
	char		*stack_trace = NULL;

	nptrs = backtrace(buffer, BT_BUF_SIZE);
	strings = backtrace_symbols(buffer, nptrs);
	if (strings == NULL)
	{
		free(strings);
		return (NULL);
	}
	for (int j = 4; j < nptrs; j++)
	{
		tmp = _trim_addr(strings[j], " ");
		if (!(cmd = _jointo("/usr/bin/addr2line -p -s -f -e ", tmp, &cmd)))
		{
			return (NULL);
		}
		if ((ret = _parse_output(cmd, &stack_trace)))
		{
			if (ret != 2)
				return (NULL);
		}
		if (ret != 2)
		{
			if (!(stack_trace = _jointo(stack_trace, ((j > nptrs - 4) ? ("") : (" < ")), &stack_trace)))
			{
				return (NULL);
			}
		}
		free(tmp);
		tmp = NULL;
		free(cmd);
		cmd = NULL;
	}
	free(strings);
	return (stack_trace);
}

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
	head->stack_trace = _get_stack_trace();
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

	if (mem->stack_trace)
	{
		free(mem->stack_trace);
		mem->stack_trace = NULL;
	}
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

t_mem						*_mem_get_elem_by_addr(t_mem *head, void *addr)
{
	t_mem 					*tmp;

	tmp = head;
	while (tmp && tmp->addr != addr)
	{
		tmp = tmp->next;
	}
	if (tmp && tmp->addr == addr)
	{
		return (tmp);
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

static void					_mem_print(t_mem *head)
{
	t_mem *tmp;

	tmp = head;
	if (!tmp)
	{
		printf("\n"CL_RD"_WRALOC_MEM_LIST_NULL_"CR);
		return;
	}
	while (tmp)
	{
		if (tmp->id < 127)
		{
			printf("%sID %4c", ((tmp->stat == 1) ? CL_GR : CL_RD), (t_byte)tmp->id);
		}
		else
		{
			printf("%sID %04lu", ((tmp->stat == 1) ? CL_GR : CL_RD), tmp->id);
		}
		printf(CR"    ADDR <%p>    SIZE %04lu    STATUS %-20s      "CR"\n",
		tmp->addr, tmp->size, ((tmp->stat == 0) ? CL_RD"Leaked"CR : CL_GR"Freed"CR));
		tmp = tmp->next;
	}
	printf(CL_BL CR"\n\n");
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
	t_mem					*tmp;
	size_t 					id;

	if (!(ptr = malloc(size)))
	{
		printf("\x1b[7;41m!!! !!! !!! !!! ALLOC FAILED !!! !!! !!! !!! \x1b[m\n");
		return (NULL);
	}
	_mem_append(&_WRALOC_MEM_LIST_, _mem_new(ptr, size, 0));
	_WRALOC_NUM_ALLO_++;
	tmp = _mem_get_elem_by_addr(_WRALOC_MEM_LIST_, ptr);
	printf(CL_GR"+A+ ALLO_NUM %08lu | ADDR <%p> | SIZE %04lu | ",
		_WRALOC_NUM_ALLO_, ptr, size);
	if (tmp->id < 127)
	{
		printf("ID %c | ", (t_byte)tmp->id);
	}
	else
	{
		printf("ID %08lu | ", tmp->id);
	}
	printf(CL_GR"source : %s",tmp->stack_trace);
	printf(CR "\n");

	return (ptr);
}

static inline void			_myfree(void *ptr)
{
	t_mem					*tmp;
	size_t 					id;

	tmp = _mem_get_elem_by_addr(_WRALOC_MEM_LIST_, ptr);
	printf(CL_RD "-F- FREE_NUM %08lu | ADDR <%p> | SIZE %04lu | ",
		_WRALOC_NUM_FREE_, ptr, _mem_get_size(_WRALOC_MEM_LIST_, ptr));
	if (tmp->id < 127)
	{
		printf("ID %c | ", (t_byte)tmp->id);
	}
	else
	{
		printf("ID %04lu | ", tmp->id);
	}
	printf(CL_RD"source : %s",tmp->stack_trace);
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
		{
			color = CL_GR;
		}
		printf("\n%s", color);
		printf("\t===== Alloc less or equal to Free ? =====");
		printf(CR"\n%s", color);
		printf("\t===== Alloc %08lu  Free %08lu =====", _WRALOC_NUM_ALLO_, _WRALOC_NUM_FREE_);
		printf(CR"\n%s", color);
		if (_WRALOC_NUM_ALLO_ <= _WRALOC_NUM_FREE_)
		{
			printf("\t====== O K = O K = O K = O K = O K ======");
		}
		else
		{
			printf("\t============= ! L E A K S ! =============");
		}
		printf(CR"\n");
		_mem_print(_WRALOC_MEM_LIST_);
	}
# endif
}

static inline void		__attribute__	((constructor))	ctor()
{
	_WRALOC_NUM_ALLO_ = 0;
	_WRALOC_NUM_FREE_ = 0;
}

static inline void		__attribute__	((destructor))	dtor()
{
# if WRAP == 1
	_get_summary();
	_mem_clear(&_WRALOC_MEM_LIST_);
# endif
}

#endif /* WRALLOC_H */
