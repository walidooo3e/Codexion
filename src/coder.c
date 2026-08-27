/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   coder.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: wabdi <wabdi@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/27 01:19:54 by wabdi             #+#    #+#             */
/*   Updated: 2026/08/27 23:42:37 by wabdi            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static bool	acquire_pair(t_coder *c, t_dongle *first, t_dongle *second)
{
	if (!dongle_acquire(first, c->sim))
		return (false);
	log_state(c->sim, c->id, "has taken a dongle");
	if (!dongle_acquire(second, c->sim))
	{
		dongle_release(first, c->sim->dongle_cooldown);
		return (false);
	}
	log_state(c->sim, c->id, "has taken a dongle");
	return (true);
}

static bool	coder_acquire_both(t_coder *c)
{
	if (c->right == c->left)
	{
		while (!sim_is_stopped(c->sim))
			usleep(1000);
		return (false);
	}
	if (c->left->id < c->right->id)
		return (acquire_pair(c, c->left, c->right));
	return (acquire_pair(c, c->right, c->left));
}

static void	coder_compile(t_coder *c)
{
	t_simulation	*sim;

	sim = c->sim;
	pthread_mutex_lock(&sim->state_lock);
	c->state = COMPILING;
	c->last_compile_start_ms = get_time_ms();
	pthread_mutex_unlock(&sim->state_lock);
	log_state(c->sim, c->id, "is compiling");
	usleep(c->sim->time_to_compile * 1000);
	dongle_release(c->left, sim->dongle_cooldown);
	dongle_release(c->right, sim->dongle_cooldown);
	pthread_mutex_lock(&sim->state_lock);
	c->compiles_done++;
	pthread_mutex_unlock(&sim->state_lock);
}

static void	coder_debug_refactor(t_coder *c)
{
	t_simulation	*sim;

	sim = c->sim;
	pthread_mutex_lock(&sim->state_lock);
	c->state = DEBUGGING;
	pthread_mutex_unlock(&sim->state_lock);
	log_state(c->sim, c->id, "is debugging");
	usleep(sim->time_to_debug * 1000);
	pthread_mutex_lock(&sim->state_lock);
	c->state = REFACTORING;
	pthread_mutex_unlock(&sim->state_lock);
	log_state(c->sim, c->id, "is refactoring");
	usleep(sim->time_to_refactor * 1000);
}

void	*coder_routine(void *arg)
{
	t_coder			*c;
	t_simulation	*sim;

	c = (t_coder *)arg;
	sim = c->sim;
	while (!sim_is_stopped(sim))
	{
		pthread_mutex_lock(&sim->state_lock);
		c->state = WAITING_FOR_DONGLES;
		pthread_mutex_unlock(&sim->state_lock);
		if (coder_acquire_both(c))
		{
			coder_compile(c);
			coder_debug_refactor(c);
		}
		else
			break ;
	}
	return (NULL);
}
