/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parsing.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: wabdi <wabdi@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/22 15:53:17 by wabdi             #+#    #+#             */
/*   Updated: 2026/08/27 01:16:34 by wabdi            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static int	is_digit(char c)
{
	return (c >= '0' && c <= '9');
}

static int	is_valid_number(char *str)
{
	int	i;
	int	result;

	i = 0;
	result = 0;
	while (str[i])
	{
		if (is_digit(str[i]))
			result = result * 10 + (str[i] - '0');
		else
			return (-1);
		i++;
	}
	return (result);
}

static void	build_targets(t_simulation *sim, long *targets[7])
{
	targets[0] = &sim->number_of_coders;
	targets[1] = &sim->time_to_burnout;
	targets[2] = &sim->time_to_compile;
	targets[3] = &sim->time_to_debug;
	targets[4] = &sim->time_to_refactor;
	targets[5] = &sim->number_of_compiles_required;
	targets[6] = &sim->dongle_cooldown;
}

int	parser(int ac, char **av, t_simulation *sim)
{
	int		i;
	int		to_parse;
	long	*targets[7];

	if (ac != 9)
		return (0);
	i = 1;
	build_targets(sim, targets);
	while (i <= 7)
	{
		to_parse = is_valid_number(av[i]);
		if (to_parse == -1)
			return (0);
		*targets[i - 1] = to_parse;
		i++;
	}
	if (strcmp(av[i], "fifo") == 0)
		sim->scheduler = SCHEDULER_FIFO;
	else if (strcmp(av[i], "edf") == 0)
		sim->scheduler = SCHEDULER_EDF;
	else
		return (0);
	return (1);
}
