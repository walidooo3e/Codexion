/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dongle.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: wabdi <wabdi@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/20 18:30:59 by wabdi             #+#    #+#             */
/*   Updated: 2026/08/22 17:09:42 by wabdi            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

/*
** Destroy dongles at the end of a normal run
** Or in the case of a failure to initiate all dongles correctly
*/
static void	destroy_up_to(t_dongle *dongles, int count)
{
	int	i;

	i = 0;
	while (i < count)
	{
		pthread_mutex_destroy(&dongles[i].lock);
		pthread_cond_destroy(&dongles[i].cond);
		i++;
	}
	free(dongles);
}

/*
** I Hate Norminette
*/
static int	init_single_dongle(t_dongle *dongle, int id, long now)
{
	dongle->id = id;
	dongle->in_use = false;
	dongle->available_at_ms = now;
	if (pthread_mutex_init(&dongle->lock, NULL) != 0)
		return (-1);
	if (pthread_cond_init(&dongle->cond, NULL) != 0)
	{
		pthread_mutex_destroy(&dongle->lock);
		return (-1);
	}
	return (0);
}

/*
** Allocates and initializes sim->dongles
** On failure, anything already initialized is cleanly
** rolled back before returning.
*/
int	dongle_init(t_simulation *sim)
{
	int		i;
	long	now;

	sim->dongles = malloc(sizeof(t_dongle) * sim->number_of_coders);
	if (!sim->dongles)
		return (-1);
	now = get_time_ms();
	i = 0;
	while (i < sim->number_of_coders)
	{
		if (init_single_dongle(&sim->dongles[i], i, now) != 0)
		{
			destroy_up_to(sim->dongles, i);
			return (-1);
		}
		i++;
	}
	return (0);
}

/*
** Public teardown, called once at the very end of the program.
*/
void	dongle_destroy(t_simulation *sim)
{
	destroy_up_to(sim->dongles, sim->number_of_coders);
}
