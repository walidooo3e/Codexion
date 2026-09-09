/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dongle.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: wabdi <wabdi@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/20 18:30:59 by wabdi             #+#    #+#             */
/*   Updated: 2026/09/09 01:58:13 by wabdi            ###   ########.fr       */
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
		if (dongles[i].heap)
		{
			free(dongles[i].heap->data);
			free(dongles[i].heap);
		}
		i++;
	}
	free(dongles);
}

static int	init_dongle_internals(t_dongle *d, long cap)
{
	d->heap = malloc(sizeof(t_heap));
	if (!d->heap)
		return (-1);
	d->heap->data = malloc(sizeof(t_wait_entry) * cap);
	if (!d->heap->data || pthread_mutex_init(&d->lock, NULL) != 0)
	{
		free(d->heap->data);
		free(d->heap);
		return (-1);
	}
	if (pthread_cond_init(&d->cond, NULL) != 0)
	{
		pthread_mutex_destroy(&d->lock);
		free(d->heap->data);
		free(d->heap);
		return (-1);
	}
	d->heap->size = 0;
	d->heap->capacity = (int)cap;
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
		sim->dongles[i].id = i;
		sim->dongles[i].in_use = false;
		sim->dongles[i].available_at_ms = now;
		sim->dongles[i].arrival_counter = 0;
		if (init_dongle_internals(&sim->dongles[i],
				sim->number_of_coders) != 0)
		{
			destroy_up_to(sim->dongles, i);
			return (-1);
		}
		i++;
	}
	return (0);
}

/* acquiring a dongle when it's free to use (not in use or cooldown)*/
bool	dongle_acquire(t_dongle *d, t_coder *c)
{
	long			key;
	struct timespec	deadline;

	pthread_mutex_lock(&d->lock);
	key = edf_next_key(c);
	if (c->sim->scheduler == SCHEDULER_FIFO)
		key = fifo_next_key(d);
	heap_push(d->heap, c->id, key);
	while (!sim_is_stopped(c->sim) && (d->in_use || get_time_ms()
			< d->available_at_ms || heap_peek_front_id(d->heap) != c->id))
	{
		if (!d->in_use && get_time_ms() < d->available_at_ms)
		{
			ms_to_timespec(d->available_at_ms, &deadline);
			pthread_cond_timedwait(&d->cond, &d->lock, &deadline);
		}
		else
			pthread_cond_wait(&d->cond, &d->lock);
	}
	heap_remove_by_id(d->heap, c->id);
	if (!sim_is_stopped(c->sim))
		d->in_use = true;
	pthread_mutex_unlock(&d->lock);
	return (!sim_is_stopped(c->sim));
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
