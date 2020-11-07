/*
**	Testing main for wraloc
*/

#include "wraloc.h"

char		**tab_alloc(int x, int y)
{
	char	**tab;
	size_t	i;

	if (!(tab = (char **)malloc((y + 1) * sizeof(char *))))
		return (NULL);
	i = 0;
	while (i < x)
	{
		if (!(tab[i] = (char *)malloc(x * sizeof(char))))
			return (NULL);
		i++;
	}
	tab[i] = NULL;
	return (tab);
}

void		tab_free(char **tab)
{
	size_t	i;

	i = 0;
	while (tab && tab[i + 1])
	{
		free(tab[i]);
		tab[i] = NULL;
		i++;
	}
	if (tab)
	{
		free(tab);
		tab = NULL;
	}
}

void		f3(char ***tab)
{
	*tab = tab_alloc(5, 5);
}

void		f2(char ***tab)
{
	f3(tab);
}

void		f1(char ***tab)
{
	f2(tab);
}

int			main(void)
{
	char	**tab;

	f1(&tab);
	tab_free(tab);
	return (0);
}
