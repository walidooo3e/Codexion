/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   monitor.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: wabdi <wabdi@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/28 00:26:45 by wabdi             #+#    #+#             */
/*   Updated: 2026/08/29 16:50:47 by wabdi            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static bool	monitor_check_burnout(t_simulation *sim)
{
    int			i;
	long		now;
	long		deadline;

	i = 0;
	now = 0;
	deadline = 0;
	while (i < sim-> number_of_coders)
	{
		now = get_time_ms();
		pthread_mutex_lock(&sim->state_lock);
		deadline = now - (sim->coders[i].last_compile_start_ms);
		if (deadline >= sim->time_to_burnout)
		{
			sim->coders[i].state = BURNED_OUT;
			pthread_mutex_unlock(&sim->state_lock);
			log_state(sim, sim->coders[i].id, "burned out");
			sim_request_stop(sim);
			return true;
		}
		pthread_mutex_unlock(&sim->state_lock);

		i++;
	}
	return false;
}

static bool	monitor_check_success(t_simulation *sim)
{
	int	i;

	i = 0;
	while (i < sim->number_of_coders)
	{
		pthread_mutex_lock(&sim->state_lock);
		if (sim->coders[i].compiles_done < sim->number_of_compiles_required)
		{
			pthread_mutex_unlock(&sim->state_lock);
			return (false);
		}
		pthread_mutex_unlock(&sim->state_lock);
		i++;
	}
	sim_request_stop(sim);
	return (true);
}

void	*monitor_routine(void *arg)
{
	t_simulation	*sim;
	
	sim = (t_simulation *)arg;
	while(!sim_is_stopped(sim))
	{
		if (monitor_check_burnout(sim))
			break;
		else if (monitor_check_success(sim))
			break;
		usleep(1000);
	}
	return (NULL);
}