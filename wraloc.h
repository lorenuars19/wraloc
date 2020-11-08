#ifndef WRALOC_H
# define WRALOC_H

# include <stddef.h>
size_t						_WRALOC_NUM_ALLO_;
size_t						_WRALOC_NUM_FREE_;

# ifndef WRAP
#  define WRAP 1
# endif

# ifndef _FULL_TRACE_
#  define _FULL_TRACE_ 0
# endif

# ifndef _LEAKS_ONLY_
#  define _LEAKS_ONLY_ 0
# endif

# ifndef _STACK_OFFS_
#  define _STACK_OFFS_ 1
# endif

# if WRAP == 1

# include <execinfo.h>
# include <string.h>
# include <stdlib.h>
# include <stdio.h>

# define CR "\x1b[0m"
# define CL_RD "\x1b[1;31m"
# define CL_GR "\x1b[1;32m"
# define CL_YE "\x1b[1;33m"
# define CL_BL "\x1b[1;34m"

# define BT_BUF_SIZE 100
# define BUFSIZE 512

typedef unsigned char		_WRAP_t_byte;

typedef struct				mem_list
{
	size_t					id;
	void					*addr;
	size_t					size;
	_WRAP_t_byte					stat;
	char					*alloc_statrace;
	char					*freed_statrace;
	char					*alloc_fstatrace;
	char					*freed_fstatrace;
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
	while (sl1 && s1 && *s1)
		a[i++] = *s1++;
	while (sl2 && s2 && *s2)
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

char			*_trim(const char *s, const char *set, void *tofree)
{
	char		*new;
	ssize_t		slen;
	ssize_t		i;

	if (!s || !set[0])
		return (NULL);
	slen = strlen(s);
	i = 0;
	while (!(_in_charset(s[i], set)))
		i++;
	slen = i;
	if (slen < 0)
		return (NULL);
	if (!(new = (char *)malloc((slen + 1) * sizeof(char))))
		return (NULL);
	i = 0;
	while (i < slen)
	{
		if (s[i] != '\n')
			new[i] = s[i];
		i++;
	}
	new[i] = '\0';
	if (tofree)
		free(tofree);
	return (new);
}

int				_parse_output(char *cmd, char **new, int full)
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
		if (_hasto(buf, '?'))
		{
			return (2);
		}
		if (!full)
		{
			trimmed = _trim(buf, " ", trimmed);
			if (!(*new = _jointo(*new, trimmed, new)))
			{
				free(trimmed);
				trimmed = NULL;
				return (1);
			}
		free(trimmed);
		trimmed = NULL;
		}
		else
		{
			if (!(*new = _jointo(*new, buf, new)))
			{
				return (1);
			}
		}
	}
	if(pclose(fp))
	{
		return (1);
	}
	return 0;
}

char			*_get_stack_trace(int full)
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
	for (int j = _STACK_OFFS_; j < nptrs; j++)
	{
		tmp = _trim_addr(strings[j], " ");
		if (!(cmd = _jointo("/usr/bin/addr2line -p -f -e ", tmp, &cmd)))
		{
			return (NULL);
		}
		if (!(cmd = _jointo(cmd, " 2>&1 ", &cmd)))
		{
			return (NULL);
		}
		if ((ret = _parse_output(cmd, &stack_trace, full)))
		{
			if (ret != 2)
				return (NULL);
		}
		if (ret == 0)
		{
			if (!full)
			{
				if (!(stack_trace = _jointo(stack_trace, ((j > nptrs - 4) ? ("") : (" < ")), &stack_trace)))
				{
					return (NULL);
				}
			}
			else
			{
				if (!(stack_trace = _jointo(stack_trace, "\t", &stack_trace)))
				{
					return (NULL);
				}
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

static t_mem 				*_mem_new(void *addr, size_t size, _WRAP_t_byte stat)
{
	static size_t 			id = 'A';
	t_mem 					*head;

	if (!(head = (t_mem *)malloc(sizeof(t_mem))))
		return (NULL);
	head->id = id++;
	head->addr = addr;
	head->size = size;
	head->stat = stat;
	head->alloc_statrace = NULL;
	head->freed_statrace = NULL;
	head->alloc_fstatrace = NULL;
	head->freed_fstatrace = NULL;
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
	if (mem->alloc_statrace)
	{
		free(mem->alloc_statrace);
		mem->alloc_statrace = NULL;
	}
	if (mem->freed_statrace)
	{
		free(mem->freed_statrace);
		mem->freed_statrace = NULL;
	}
	if (mem->alloc_fstatrace)
	{
		free(mem->alloc_fstatrace);
		mem->alloc_fstatrace = NULL;
	}
	if (mem->freed_fstatrace)
	{
		free(mem->freed_fstatrace);
		mem->freed_fstatrace = NULL;
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

static void					_mem_remove_by_addr(t_mem **head, void *addr)
{
	t_mem 					*tmp;
	tmp = *head;
	while (tmp && tmp->next->addr != addr)
	{
		tmp = tmp->next;
	}
	tmp->next = tmp->next->next;
	tmp = tmp->next;
	tmp->next = NULL;
	_mem_del(tmp);
}

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

static void					_mem_set_status(t_mem **head, void *addr, _WRAP_t_byte status)
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
		while (tmp && _LEAKS_ONLY_ && tmp->stat == 1)
		{
			tmp = tmp->next;
			if (!tmp)
			{
				return ;
			}
		}
		printf("%sADDR <%p> | SIZE %08lu | STATUS %s | ",
		(tmp->stat == 1) ? (CL_GR) : (CL_RD),tmp->addr, tmp->size, ((tmp->stat == 0) ? "Leaked" : "Freed "));
		if (tmp->id < 127)
		{
			printf("ID %c ", (_WRAP_t_byte)tmp->id);
		}
		else
		{
			printf("ID %04lu", tmp->id);
		}
		if (tmp->stat == 0)
		{
			if (!tmp->alloc_fstatrace)
			{
				printf("\n"CL_RD"Wraloc REQUIRES 'addr2line(1)' which is a unix utility to be able to print the call stack"CR"\n");
			}
			else
			{
				printf(" : \n"CR"Allocated at : \n\t%s",tmp->alloc_fstatrace);
			}
		}
		tmp = tmp->next;
	printf(CR"\n");
	}
}

# ifdef malloc
#  undef malloc
# endif

# ifdef free
#  undef free
# endif

static inline void			*_WRAPPED_malloc(size_t size)
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
	printf(CL_GR"+A+ ALLO_NUM %08lu | ADDR <%p> | SIZE %08lu | ",
		_WRALOC_NUM_ALLO_, ptr, size);
	if (tmp && tmp->id < 127)
	{
		printf("ID %c", (_WRAP_t_byte)tmp->id);
	}
	else if (tmp)
	{
		printf("ID %08lu", tmp->id);
	}
	if (tmp && (tmp->alloc_statrace = _get_stack_trace(0)))
	{
		printf(CL_GR" : %s",tmp->alloc_statrace);
	}
	if (tmp && (tmp->alloc_fstatrace = _get_stack_trace(1)))
	{
		if (_FULL_TRACE_)
		{
			printf(CR"\nCall Stack : \n\t%s",tmp->alloc_fstatrace);
		}
	}
	printf(CR "\n");
	return (ptr);
}

static inline void			_WRAPPED_free(void *ptr)
{
	t_mem					*tmp;
	size_t 					id;

	tmp = _mem_get_elem_by_addr(_WRALOC_MEM_LIST_, ptr);
	printf(CL_BL "-F- FREE_NUM %08lu | ADDR <%p> | SIZE %08lu | ",
		_WRALOC_NUM_FREE_, ptr, _mem_get_size(_WRALOC_MEM_LIST_, ptr));
	if (tmp && tmp->id < 127)
	{
		printf("ID %c | ", (_WRAP_t_byte)tmp->id);
	}
	else if (tmp)
	{
		printf("ID %04lu | ", tmp->id);
	}
	if (tmp && (tmp->freed_statrace = _get_stack_trace(0)))
	{
		printf(" : %s",tmp->freed_statrace);
	}
	if (tmp && (tmp->freed_fstatrace = _get_stack_trace(1)))
	{
		if (_FULL_TRACE_)
		{
			printf(CR"\nCall Stack : \n\t%s",tmp->freed_fstatrace);
		}
	}
	printf(CR "\n");
	if (ptr)
	{
		_WRALOC_NUM_FREE_++;
	}
	_mem_set_status(&_WRALOC_MEM_LIST_, ptr, 1);
	free(ptr);
}

# define malloc(x) _WRAPPED_malloc(x)
# define free(x) _WRAPPED_free(x)


static inline void			_print_summary(void)
{
	if (_WRALOC_NUM_ALLO_ >= 0 && _WRALOC_NUM_FREE_ >= 0)
	{
		char *color = CL_RD;
		if (_WRALOC_NUM_ALLO_ <= _WRALOC_NUM_FREE_)
		{
			color = CL_GR;
		}
		printf("\n%s", color);
		printf(".:::: Alloc less or equal to Free ? ::::.");
		printf(CR"\n%s", color);
		printf("::::: Alloc %08lu  Free %08lu :::::", _WRALOC_NUM_ALLO_, _WRALOC_NUM_FREE_);
		printf(CR"\n%s", color);
		if (_WRALOC_NUM_ALLO_ <= _WRALOC_NUM_FREE_)
		{
			printf("'::::: O K : O K : O K : O K : O K :::::'");
		}
		else
		{
			printf("':::::::::::: ! L E A K S ! ::::::::::::'");
		}
		printf(CR"\n\n");
	}
}

# endif /* WRAP */

static inline void			_get_summary(void)
{

# if WRAP == 1
	_print_summary();
	_mem_print(_WRALOC_MEM_LIST_);
	_print_summary();
# endif
}

#define COLBG "\033[0;1;34;40m"
#define COLFG "\033[0;34;44m"
#define COLLK "\033[0;4;34;40m"
#define COLVR "\033[0;1;35;40m"


static inline void		__attribute__	((constructor))	constructor()
{
	_WRALOC_NUM_ALLO_ = 0;
	_WRALOC_NUM_FREE_ = 0;
# if WRAP == 1

printf(
""COLBG" "COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG" ""\n"
""COLBG"'"COLFG"#"COLFG"#"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG"'"COLFG"#"COLFG"#"COLBG":"COLBG"'"COLFG"#"COLFG"#"COLFG"#"COLFG"#"COLFG"#"COLFG"#"COLFG"#"COLFG"#"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG"'"COLFG"#"COLFG"#"COLFG"#"COLBG":"COLBG":"COLBG":"COLBG":"COLBG"'"COLFG"#"COLFG"#"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG"'"COLFG"#"COLFG"#"COLFG"#"COLFG"#"COLFG"#"COLFG"#"COLFG"#"COLBG":"COLBG":"COLBG":"COLBG"'"COLFG"#"COLFG"#"COLFG"#"COLFG"#"COLFG"#"COLFG"#"COLBG":"COLBG":""\n"
""COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG"'"COLFG"#"COLFG"#"COLBG":"COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG" "COLFG"#"COLFG"#"COLBG"."COLBG"."COLBG"."COLBG"."COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG":"COLBG":"COLBG"'"COLFG"#"COLFG"#"COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG":"COLBG":"COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG"'"COLFG"#"COLFG"#"COLBG"."COLBG"."COLBG"."COLBG"."COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG"'"COLFG"#"COLFG"#"COLBG"."COLBG"."COLBG"."COLBG" "COLFG"#"COLFG"#"COLBG":""\n"
""COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG":"COLBG":"COLBG":"COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG":"COLBG"'"COLFG"#"COLFG"#"COLBG":"COLBG"."COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG":"COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG":"COLBG":"COLBG":"COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG":"COLBG":"COLBG"."COLBG"."COLBG":"COLBG":""\n"
""COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG" "COLFG"#"COLFG"#"COLFG"#"COLFG"#"COLFG"#"COLFG"#"COLFG"#"COLFG"#"COLBG":"COLBG":"COLBG"'"COLFG"#"COLFG"#"COLBG":"COLBG":"COLBG":"COLBG"."COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG":"COLBG":"COLBG":"COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":""\n"
""COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG" "COLFG"#"COLFG"#"COLBG"."COLBG"."COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG":"COLBG":"COLBG" "COLFG"#"COLFG"#"COLFG"#"COLFG"#"COLFG"#"COLFG"#"COLFG"#"COLFG"#"COLFG"#"COLBG":"COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG":"COLBG":"COLBG":"COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":""\n"
""COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG":"COLBG"."COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG":"COLBG" "COLFG"#"COLFG"#"COLBG"."COLBG"."COLBG"."COLBG"."COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG":"COLBG":"COLBG":"COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG":"COLBG":"COLBG" "COLFG"#"COLFG"#"COLBG":""\n"
""COLBG"."COLBG" "COLFG"#"COLFG"#"COLFG"#"COLBG"."COLBG" "COLFG"#"COLFG"#"COLFG"#"COLBG":"COLBG":"COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG":"COLBG":"COLBG"."COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG":"COLBG":"COLBG":"COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG" "COLFG"#"COLFG"#"COLFG"#"COLFG"#"COLFG"#"COLFG"#"COLFG"#"COLFG"#"COLBG":"COLBG"."COLBG" "COLFG"#"COLFG"#"COLFG"#"COLFG"#"COLFG"#"COLFG"#"COLFG"#"COLBG":"COLBG":"COLBG"."COLBG" "COLFG"#"COLFG"#"COLFG"#"COLFG"#"COLFG"#"COLFG"#"COLBG":"COLBG":""\n"
""COLBG":"COLBG"."COLBG"."COLBG"."COLBG":"COLBG":"COLBG"."COLBG"."COLBG"."COLBG":"COLBG":"COLBG":"COLBG"."COLBG"."COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG"."COLBG"."COLBG":"COLBG":"COLBG"."COLBG"."COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG"."COLBG"."COLBG":"COLBG":"COLBG"."COLBG"."COLBG"."COLBG"."COLBG"."COLBG"."COLBG"."COLBG"."COLBG":"COLBG":"COLBG":"COLBG"."COLBG"."COLBG"."COLBG"."COLBG"."COLBG"."COLBG"."COLBG":"COLBG":"COLBG":"COLBG":"COLBG"."COLBG"."COLBG"."COLBG"."COLBG"."COLBG"."COLBG":"COLBG":"COLBG":""\n"
""COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG" "COLVR" WRALOC V2.3   "COLLK"https://github.com/lorenuars19/wraloc"COLBG" "COLBG" "COLBG":"COLBG":"COLBG":"COLBG":"COLBG":""\n"
""COLBG" "COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG" ""\n"
CR"\n");
# endif

}

static inline void		__attribute__	((destructor))	destructor()
{
# if WRAP == 1
	_get_summary();
	_mem_clear(&_WRALOC_MEM_LIST_);
# endif
}

#endif /* WRALLOC_H */
