#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>

#define	STDIN		0
#define	STDOUT		1
#define	STDERR		2

#define TYPE_END	3
#define TYPE_BREAK	4
#define	TYPE_PIPE	5

typedef struct	s_base
{
	char		**argv;
	int		size;
	int		type;
	int		fd[2];
	struct s_base	*prev;
	struct s_base	*next;
}		t_base;

int	ft_strlen(char *str)
{
	int	i;

	i = 0;
	if (!str)
		return (0);
	while (str[i])
		i++;
	return (i);
}

void	clean(t_base *ptr)
{
	int	i;
	t_base	*temp;

	while (ptr)
	{
		temp = ptr->next;
		i = 0;
		while (ptr->argv[i])
		{
			free(ptr->argv[i]);
			i++;
		}
		free(ptr->argv);
		free(ptr);
		ptr = temp;
	}

}

char	*ft_strdup(char *str)
{
	int	i;
	char	*result;

	i = ft_strlen(str);
	if (!(result = malloc((i + 1) * sizeof(char))))
		return (0);
	result[i] = '\0';
	while (--i >= 0)
		result[i] = str[i];
	return (result);
}

void	ft_pushback(t_base **ptr, t_base *new)
{
	t_base *temp;

	if (!(*ptr))
		*ptr = new;
	else
	{
		temp = *ptr;
		while (temp->next)
			temp = temp->next;
		temp->next = new;
		new->prev = temp;
	}
}


void	exit_fatal(void)
{
	write(STDERR, "error: fatal\n", 13);
	exit(1);
}

void	error_cd_1(void)
{
	write(STDERR, "error: cd: bad arguments\n", 25);
	exit(1);
}

void	error_cd_2(char *path)
{
	write(STDERR, "error: cd: cannot change directory to ", 38);
	write(STDERR, path, ft_strlen(path));
	write(STDERR, "\n", 1);
	exit(1);
}

void	error_execve(char *executable)
{
	write(STDERR, "error: cannot execute ", 22);
	write(STDERR, executable, ft_strlen(executable));
	write(STDERR, "\n", 1);
	exit(1);
}

int	av_size(char **av)
{
	int	i;

	i = 0;
	while (av[i] && strcmp(av[i], "|") != 0 && strcmp(av[i], ";") != 0)
		i++;
	return (i);
}

int	check_type(char *end)
{
	if (!end)
		return (TYPE_END);
	if (strcmp(end, ";") == 0)
		return (TYPE_BREAK);
	else if (strcmp(end, "|") == 0)
		return (TYPE_PIPE);
	return (0);
}

int	parse_av(t_base **ptr, char **av)
{
	int	size;
	t_base	*new;

	size = av_size(av);
	if (!(new = malloc(1 * sizeof(t_base))))
		exit_fatal();
	if (!(new->argv = malloc((size + 1) * sizeof(char *))))
		exit_fatal();
	new->size = size;
	new->type = check_type(av[size]);
	new->prev = NULL;
	new->next = NULL;
	new->argv[size] = 0;
	while (--size >= 0)
		new->argv[size] = ft_strdup(av[size]);
	ft_pushback(ptr, new);
	return (new->size);
}

void	exec_cmd(t_base *tmp, char **env)
{
	pid_t	pid;
	int	status;

	if (tmp->type == TYPE_PIPE)
	{
		if(pipe(tmp->fd))
			exit_fatal();
	}
	pid = fork();
	if (pid < 0)
		exit_fatal();
	if (pid == 0)
	{
		if (tmp->type == TYPE_PIPE && dup2(tmp->fd[1], STDOUT) < 0)
			exit_fatal();
		if ((tmp->prev && tmp->prev->type == TYPE_PIPE) && dup2(tmp->prev->fd[0], STDIN) < 0)
			exit_fatal();
		if (execve(tmp->argv[0], tmp->argv, env) == -1)
			error_execve(tmp->argv[0]);
		exit(0);
	}
	else
	{
		waitpid(pid, &status, 0);
		if (tmp->type == TYPE_PIPE)
			close(tmp->fd[1]);
		if (tmp->prev && tmp->prev->type == TYPE_PIPE)
			close(tmp->prev->fd[0]);
	}
}

void	exec_cmds(t_base *ptr, char **env)
{
	t_base	*tmp;

	tmp = ptr;
	while (tmp)
	{
		if (strcmp(tmp->argv[0], "cd") == 0)
		{
			if (tmp->size < 2)
				error_cd_1();
			else if (chdir(tmp->argv[1]) == -1)
				error_cd_2(tmp->argv[1]);
		}
		else
			exec_cmd(tmp, env);
		tmp = tmp->next;
	}
}





int	main(int ac, char **av, char **env)
{
	int	i;
	t_base	*ptr;

	i = 1;
	ptr = NULL;
	//system("lsof | wc -l");
	if (ac < 2)
		return (0);
	while (av[i])
	{
		if (strcmp(av[i], ";") == 0 || strcmp(av[i], "|") == 0)
		{
			i++;
			continue;
		}
		i += parse_av(&ptr, &av[i]);
		if (!av[i])
			break;
		else
			i++;
	}
	
	/* t_base *tmp;
	tmp = ptr;
	int	tool = 0;
	while(ptr)
	{
		if (!ptr)
			break;
		tool = 0;
		printf("COMMAND: ");
		while (ptr->argv[tool])
		{
			printf(" |%s|", ptr->argv[tool]);
			tool++;
		}
		printf("\nTYPE: %i", ptr->type);
		printf("	SIZE: %i\n\n", ptr->size);
		ptr = ptr->next;
	}
	ptr = tmp;
	(void)**env; */
	if (ptr)
		exec_cmds(ptr, env);
	//system("lsof | wc -l");
	clean(ptr);



	return (0);
}
