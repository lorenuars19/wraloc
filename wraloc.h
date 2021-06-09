#ifndef WRALOC_H
# define WRALOC_H

# include <stddef.h>

size_t						_WRALOC_NUM_ALLO_;
size_t						_WRALOC_NUM_FREE_;
char						_PRINTED;


# ifndef WRAP
#  define WRAP 1
# endif

# ifndef _LEAKS_ONLY_
#  define _LEAKS_ONLY_ 0
# endif

# ifndef _FD
#  define _FD 2
# endif

# if WRAP == 1

# include <execinfo.h>
# include <errno.h>
# include <string.h>
# include <stdlib.h>
# include <stdarg.h>
# include <stdio.h>
# include <unistd.h>

#  ifndef __APPLE__
#   include <malloc.h>
#  endif

# define CR "\x1b[0m"
# define CL_RD "\x1b[1;31m"
# define CL_GR "\x1b[1;32m"
# define CL_YE "\x1b[1;33m"
# define CL_BL "\x1b[1;34m"
# define CL_CY "\x1b[1;36m"

#define COLBG "\033[0;1;34;40m"
#define COLFG "\033[0;34;44m"
#define COLLK "\033[0;4;34;40m"
#define COLVR "\033[0;1;35;40m"

# define BT_BUF_SIZE 100
# define BUFSIZE 512

typedef unsigned char		_WRAP_t_byte;

typedef struct				mem_list
{
	size_t					id;
	void					*addr;
	size_t					size;
	_WRAP_t_byte			stat;
	char					*alloc_statrace;
	char					*freed_statrace;
	struct mem_list 		*next;
}							t_mem;

t_mem						*_WRALOC_MEM_LIST_;

static inline t_mem 				*_mem_new(void *addr, size_t size, _WRAP_t_byte stat)
{
	static size_t 			id = 'A';
	t_mem 					*head;

	if (!(head = (t_mem *)malloc(sizeof(t_mem))))
	{
		dprintf( _FD, "\n\n\n\n\nWRALOC ERROR : %d > %s\n\n\n\n\n", errno, strerror(errno));
		return (NULL);
	}
	head->id = id++;
	head->addr = addr;
	head->size = size;
	head->stat = stat;
	head->alloc_statrace = NULL;
	head->freed_statrace = NULL;
	head->next = NULL;
	return (head);
}

static inline t_mem				*_mem_append(t_mem **head, t_mem *new)
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

static inline void					_mem_del(t_mem *mem)
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
	if (mem)
	{
		free(mem);
		mem = NULL;
	}
}

static inline void				_mem_clear(t_mem **list)
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

static inline void				_mem_remove_by_addr(t_mem **head, void *addr)
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

static inline t_mem				*_mem_get_elem_by_addr(t_mem *head, void *addr)
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

static inline size_t			_mem_get_size(t_mem *head, void *addr)
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

static inline void				_mem_set_status(t_mem **head, void *addr, _WRAP_t_byte status)
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

static inline size_t			_mem_size(t_mem *list)
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

static inline void				_mem_print(t_mem *head)
{
	t_mem *tmp;

	tmp = head;
	if (!tmp)
	{
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
		dprintf( _FD, "%sADDR <%p> | SIZE %04lu | STATUS %s | ",
		(tmp->stat == 1) ? (CL_GR) : (CL_RD),tmp->addr, tmp->size, ((tmp->stat == 0) ? "Leaked" : "Freed "));
		if (tmp->id < 127)
		{
			dprintf( _FD, "ID %c ", (_WRAP_t_byte)tmp->id);
		}
		else
		{
			dprintf( _FD, "ID %04lu", tmp->id);
		}
		if (tmp->alloc_statrace)
		{
			dprintf( _FD, " : A %s ",tmp->alloc_statrace);
		}
		if (tmp->freed_statrace)
		{
			dprintf( _FD, "\033[35m F %s", tmp->freed_statrace);
		}
		tmp = tmp->next;
	dprintf( _FD, CR"\n");
	}
}

#  ifndef __APPLE__
static inline int	vasprintf(char **strp, const char *fmt, va_list ap)
{
	va_list ap1;
	size_t size;
	char *buffer;

	va_copy(ap1, ap);
	size = vsnprintf(NULL, 0, fmt, ap1) + 1;
	va_end(ap1);
	buffer = calloc(1, size);
	if (!buffer)
		return (-1);
	*strp = buffer;
	return vsnprintf(buffer, size, fmt, ap);
}

static inline int	asprintf(char **strp, const char *fmt, ...)
{
	int error;
	va_list ap;

	va_start(ap, fmt);
	error = vasprintf(strp, fmt, ap);
	va_end(ap);
	return error;
}
#  endif

# ifdef malloc
#  undef malloc
# endif

# ifdef free
#  undef free
# endif

# define _FORMAT "[%d]%s:%s()", line, file, func

static inline void			*_WRAPPED_malloc(size_t size, int line, const char *func, const char *file)
{
	void 					*ptr;
	t_mem					*tmp;

	if (!(ptr = malloc(size)))
	{
		dprintf( _FD, "\x1b[7;41m!!! !!! !!! !!! ALLOC FAILED !!! !!! !!! !!! \x1b[m\n");
		return (NULL);
	}
	_mem_append(&_WRALOC_MEM_LIST_, _mem_new(ptr, size, 0));
	_WRALOC_NUM_ALLO_++;
	tmp = _mem_get_elem_by_addr(_WRALOC_MEM_LIST_, ptr);
	dprintf( _FD, CL_GR"+A+ ALLO_NUM %04lu | ADDR <%p> | SIZE %04lu | ",
		_WRALOC_NUM_ALLO_, ptr, size);
	if (tmp && tmp->id < 127)
	{
		dprintf( _FD, "ID %c", (_WRAP_t_byte)tmp->id);
	}
	else if (tmp)
	{
		dprintf( _FD, "ID %04lu", tmp->id);
	}
	if (tmp)
	{
		if (asprintf(&(tmp->alloc_statrace), _FORMAT) < 0)
		{
			free(tmp->alloc_statrace);
			tmp->alloc_statrace = NULL;
		}
	}
	dprintf( _FD, " : %s",tmp->alloc_statrace);
	dprintf( _FD, CR "\n");
	return (ptr);
}

static inline void			_WRAPPED_free(void *ptr, int line, const char *func, const char *file)
{
	t_mem					*tmp;
	size_t					size;

	tmp = _mem_get_elem_by_addr(_WRALOC_MEM_LIST_, ptr);
	size = _mem_get_size(_WRALOC_MEM_LIST_, ptr);

	if (ptr)
	{
		dprintf( _FD, CL_BL "-F- FREE_NUM %04lu | ADDR <%p> | SIZE %04lu | ", _WRALOC_NUM_FREE_, ptr, size);
	}
	else
	{
		dprintf( _FD, CL_CY "-F- =-=-=-= ADDR <%p> ZERO SIZE FREE NULL POINTER =-=-=-=", ptr);
	}
	if (size && tmp && tmp->id < 127)
	{
		dprintf( _FD, "ID %c", (_WRAP_t_byte)tmp->id);
	}
	else if (size && tmp)
	{
		dprintf( _FD, "ID %04lu", tmp->id);
	}
	if (ptr && tmp)
	{
		if (asprintf(&(tmp->freed_statrace), _FORMAT) < 0)
		{
			free(tmp->freed_statrace);
			tmp->freed_statrace = NULL;
		}
	}
	if (tmp && tmp->freed_statrace)
	{
		dprintf( _FD, " : %s",tmp->freed_statrace);
	}
	else if (!ptr || !size)
	{
		dprintf( _FD, " : " _FORMAT);
	}
	if (ptr)
	{
		_WRALOC_NUM_FREE_++;
	}
		dprintf( _FD, CR "\n");
	_mem_set_status(&_WRALOC_MEM_LIST_, ptr, 1);
	free(ptr);
}

# define malloc(x) _WRAPPED_malloc(x, __LINE__, __FUNCTION__, __FILE__)
# define free(x) _WRAPPED_free(x, __LINE__, __FUNCTION__, __FILE__)

static inline void					_print_header(void)
{
dprintf( _FD,
""COLBG" "COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG" ""\n"
""COLBG"'"COLFG"#"COLFG"#"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG"'"COLFG"#"COLFG"#"COLBG":"COLBG"'"COLFG"#"COLFG"#"COLFG"#"COLFG"#"COLFG"#"COLFG"#"COLFG"#"COLFG"#"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG"'"COLFG"#"COLFG"#"COLFG"#"COLBG":"COLBG":"COLBG":"COLBG":"COLBG"'"COLFG"#"COLFG"#"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG"'"COLFG"#"COLFG"#"COLFG"#"COLFG"#"COLFG"#"COLFG"#"COLFG"#"COLBG":"COLBG":"COLBG":"COLBG"'"COLFG"#"COLFG"#"COLFG"#"COLFG"#"COLFG"#"COLFG"#"COLBG":"COLBG":""\n"
""COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG"'"COLFG"#"COLFG"#"COLBG":"COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG" "COLFG"#"COLFG"#"COLBG"."COLBG"."COLBG"."COLBG"."COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG":"COLBG":"COLBG"'"COLFG"#"COLFG"#"COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG":"COLBG":"COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG"'"COLFG"#"COLFG"#"COLBG"."COLBG"."COLBG"."COLBG"."COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG"'"COLFG"#"COLFG"#"COLBG"."COLBG"."COLBG"."COLBG" "COLFG"#"COLFG"#"COLBG":""\n"
""COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG":"COLBG":"COLBG":"COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG":"COLBG"'"COLFG"#"COLFG"#"COLBG":"COLBG"."COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG":"COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG":"COLBG":"COLBG":"COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG":"COLBG":"COLBG"."COLBG"."COLBG":"COLBG":""\n"
""COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG" "COLFG"#"COLFG"#"COLFG"#"COLFG"#"COLFG"#"COLFG"#"COLFG"#"COLFG"#"COLBG":"COLBG":"COLBG"'"COLFG"#"COLFG"#"COLBG":"COLBG":"COLBG":"COLBG"."COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG":"COLBG":"COLBG":"COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":""\n"
""COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG" "COLFG"#"COLFG"#"COLBG"."COLBG"."COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG":"COLBG":"COLBG" "COLFG"#"COLFG"#"COLFG"#"COLFG"#"COLFG"#"COLFG"#"COLFG"#"COLFG"#"COLFG"#"COLBG":"COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG":"COLBG":"COLBG":"COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":""\n"
""COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG":"COLBG"."COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG":"COLBG" "COLFG"#"COLFG"#"COLBG"."COLBG"."COLBG"."COLBG"."COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG":"COLBG":"COLBG":"COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG":"COLBG":"COLBG" "COLFG"#"COLFG"#"COLBG":""\n"
""COLBG"."COLBG" "COLFG"#"COLFG"#"COLFG"#"COLBG"."COLBG" "COLFG"#"COLFG"#"COLFG"#"COLBG":"COLBG":"COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG":"COLBG":"COLBG"."COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG":"COLBG":"COLBG":"COLBG" "COLFG"#"COLFG"#"COLBG":"COLBG" "COLFG"#"COLFG"#"COLFG"#"COLFG"#"COLFG"#"COLFG"#"COLFG"#"COLFG"#"COLBG":"COLBG"."COLBG" "COLFG"#"COLFG"#"COLFG"#"COLFG"#"COLFG"#"COLFG"#"COLFG"#"COLBG":"COLBG":"COLBG"."COLBG" "COLFG"#"COLFG"#"COLFG"#"COLFG"#"COLFG"#"COLFG"#"COLBG":"COLBG":""\n"
""COLBG":"COLBG"."COLBG"."COLBG"."COLBG":"COLBG":"COLBG"."COLBG"."COLBG"."COLBG":"COLBG":"COLBG":"COLBG"."COLBG"."COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG"."COLBG"."COLBG":"COLBG":"COLBG"."COLBG"."COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG"."COLBG"."COLBG":"COLBG":"COLBG"."COLBG"."COLBG"."COLBG"."COLBG"."COLBG"."COLBG"."COLBG"."COLBG":"COLBG":"COLBG":"COLBG"."COLBG"."COLBG"."COLBG"."COLBG"."COLBG"."COLBG"."COLBG":"COLBG":"COLBG":"COLBG":"COLBG"."COLBG"."COLBG"."COLBG"."COLBG"."COLBG"."COLBG":"COLBG":"COLBG":""\n"
""COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG" "COLVR"   WRALOC V3   " COLLK"https://github.com/lorenuars19/wraloc"COLBG" "COLBG" "COLBG":"COLBG":"COLBG":"COLBG":"COLBG":""\n"
""COLBG" "COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG":"COLBG" "
CR "\n");
}

static inline void					_print_LEAKS_ART(void)
{
dprintf(_FD,
CL_RD " ------------------------------------- " "\n"
CL_RD "| ............`    `................. |" "\n"
CL_RD "| MMMMMMMMMMMMMd:  `+mMMMMMMMMMMMMMMM |" "\n"
CL_RD "| MMMMMMMMMMMMMMMh.  `oMMMMMMMMMMMMMM |" "\n"
CL_RD "| MMMMMMMMMMMMMMh-  `+mMMMMMMMMMMMMMM |" "\n"
CL_RD "| MMMMMMMMMMMMMy`  `yMMMMMMMMMMMMMMMM |" "\n"
CL_RD "| MMMMMMMMMMMMMMh:`  /mMMMMMMMMMMMMMM |" "\n"
CL_RD "| yyyyyyyyyyyyyyyy/   `+yyyyyyyyyyyyy |" "\n"
CL_RD "|                  -`                 |" "\n"
CL_RD "|                `yNy.                |" "\n"
CL_RD "|               `hMMMd.               |" "\n"
CL_RD "|              `hMMMMMN-              |" "\n"
CL_RD "|              hMMMMMMMm`             |" "\n"
CL_RD "|             /MM     MMs             |" "\n"
CL_RD "|             hM LEAKS MN`            |" "\n"
CL_RD "|             yMM     MMm`            |" "\n"
CL_RD "|             `sNMMMMMNy.             |" "\n"
CL_RD "|                `....                |" "\n"
CL_RD " ------------------------------------- " CR "\n");
}

static inline void			_print_summary(int header)
{
	if (_WRALOC_MEM_LIST_)
	{
		char *color = CL_RD;
		if (_WRALOC_NUM_ALLO_ <= _WRALOC_NUM_FREE_)
		{
			color = CL_GR;
		}
		dprintf( _FD, "\n%s", color);
		dprintf( _FD, ".::::: Alloc less or equal to Free? :::::.");
		dprintf( _FD, CR"\n%s", color);
		dprintf( _FD, "::::: Alloc %08lu | Free %08lu :::::", _WRALOC_NUM_ALLO_, _WRALOC_NUM_FREE_);
		dprintf( _FD, CR"\n%s", color);
		if (_WRALOC_NUM_ALLO_ <= _WRALOC_NUM_FREE_)
		{
			dprintf( _FD, "'::::: O K : O K : O K : O K : O K ::::::'"CR"\n\n");
		}
		else
		{
			dprintf( _FD, "'::::::::::::: ! L E A K S ! ::::::::::::'"CR"\n\n");
			if (!header)
			{
				_print_LEAKS_ART();
			}
		}
		if (header)
		{
			_print_header();
		}
		dprintf( _FD, CR);
	}
}

# endif /* WRAP */

static inline void			_get_summary(void)
{

# if WRAP == 1
	_print_summary(1);
	_mem_print(_WRALOC_MEM_LIST_);
	_print_summary(0);
# endif
}



static inline void	constructor() __attribute__ ((constructor));
static inline void	destructor() __attribute__ ((destructor));

static inline void	constructor()
{
	_WRALOC_NUM_ALLO_ = 0;
	_WRALOC_NUM_FREE_ = 0;
	_PRINTED = 0;
}

static inline void	destructor()
{
# if WRAP == 1
	if (!_PRINTED)
	{
		_PRINTED = 1;
		_get_summary();
	}
	_mem_clear(&_WRALOC_MEM_LIST_);
# endif
}

#endif /* WRALLOC_H */
