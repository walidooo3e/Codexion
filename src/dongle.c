/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dongle.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: wabdi <wabdi@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/20 18:30:59 by wabdi             #+#    #+#             */
/*   Updated: 2026/08/27 01:16:13 by wabdi            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

/*
** Destroy dongles at the end of a normal run
** Or in the case of a failure to initiate all dongles correctly
*/
void	destroy_up_to(t_dongle *dongles, int count)
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

/* acquiring a dongle when it's free to use (not in use or cooldown)*/
bool	dongle_acquire(t_dongle *d, t_simulation *sim)
{
	struct timespec	deadline;

	pthread_mutex_lock(&d->lock);
	while (!sim_is_stopped(sim)
		&& (d->in_use || get_time_ms() < d->available_at_ms))
	{
		if (d->in_use)
			pthread_cond_wait(&d->cond, &d->lock);
		else
		{
			ms_to_timespec(d->available_at_ms, &deadline);
			pthread_cond_timedwait(&d->cond, &d->lock, &deadline);
		}
	}
	if (sim_is_stopped(sim))
	{
		pthread_mutex_unlock(&d->lock);
		return (false);
	}
	d->in_use = true;
	pthread_mutex_unlock(&d->lock);
	return (true);
}

/* releasing a dongle when a coder is done with it */
void	dongle_release(t_dongle *d, long cooldown_ms)
{
	pthread_mutex_lock(&d->lock);
	d->in_use = false;
	d->available_at_ms = get_time_ms() + cooldown_ms;
	pthread_cond_broadcast(&d->cond);
	pthread_mutex_unlock(&d->lock);
}
