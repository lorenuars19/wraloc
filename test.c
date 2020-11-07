#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "wraloc.h"
#define BT_BUF_SIZE 3

size_t	_hasto(char *s, char c)
{
	size_t	to;

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

char	*_jointo(char *s1, char *s2, char **tofree)
{
	char	*a;
	size_t	sl1;
	size_t	sl2;
	size_t	i;

	a = NULL;
	sl1 = _hasto(s1, '\0');
	sl2 = _hasto(s2, '\0');
	if (!(a = (char *)malloc((sl1 + sl2 + 1) * sizeof(char))))
	{
		if (tofree && *tofree)
			free(*tofree);
		return (NULL);
	}
	i = 0;
	while (sl1 && s1 && *s1 && *s1 != '\n')
		a[i++] = *s1++;
	while (sl2 && s2 && *s2 && *s2 != '\n')
		a[i++] = *s2++;
	a[i] = '\0';
	if (tofree && *tofree)
		free(*tofree);
	return (a);
}

int		_in_charset(char c, const char *set)
{
	while (set && *set)
	{
		if (*set == c)
			return (1);
		set++;
	}
	return (0);
}

char		*_trim_addr(const char *s, const char *set)
{
	char	*new;
	ssize_t	offset;
	ssize_t	len_cpy;
	ssize_t	i;

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

#define BUFSIZE 512

int			_parse_output(char *cmd, char **new)
{
	char	buf[BUFSIZE];
	FILE	*fp;

	if ((fp = popen(cmd, "r")) == NULL)
	{
		printf("Error opening pipe!\n");
		return (-1);
	}
	while (fgets(buf, BUFSIZE, fp) != NULL)
	{
		if (!(*new = _jointo(*new, buf, new)))
			return (-1);
	}
	if(pclose(fp))
	{
		printf("Command not found or exited with error status\n");
		return (-1);
	}
	return 0;
}

void		_get_stack_trace(void)
{
	int		nptrs;
	int		ret;
	void	*buffer[BT_BUF_SIZE];
	char	**strings;
	char	*tmp;
	char	*cmd;
	char	*stack_trace;

	nptrs = backtrace(buffer, BT_BUF_SIZE);
	strings = backtrace_symbols(buffer, nptrs);
	if (strings == NULL)
	{
		perror("backtrace_symbols");
		exit(EXIT_FAILURE);
	}
	for (int j = 0; j < nptrs; j++)
	{
		tmp = _trim_addr(strings[j], " ");
		if (!(cmd = _jointo("/usr/bin/addr2line -p -s -f -e ", tmp, &cmd)))
			return ;
		ret = _parse_output(cmd, &stack_trace);
		if (!(stack_trace = _jointo(stack_trace, " > ", &stack_trace)))
			return ;
		free(tmp);
	}
	printf("%s\n", stack_trace);
	free(stack_trace);
	free(strings);
}

void	myfunc2(void)
{
	_get_stack_trace();
}
void	myfunc(int ncalls)
{
	if (ncalls > 1)
		myfunc(ncalls - 1);
	else
		myfunc2();
}
int
main(int argc, char *argv[])
{
	if (argc != 2) {
		fprintf(stderr, "%s num-calls\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	myfunc(atoi(argv[1]));
	exit(EXIT_SUCCESS);
}
