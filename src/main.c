/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: wabdi <wabdi@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/29 16:59:07 by wabdi             #+#    #+#             */
/*   Updated: 2026/09/09 01:52:29 by wabdi            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static int	sim_setup_all(t_simulation *sim)
{
	int	i;

	if (dongle_init(sim) == -1)
		return (-1);
	pthread_mutex_init(&sim->log_lock, NULL);
	pthread_mutex_init(&sim->stop_lock, NULL);
	pthread_mutex_init(&sim->state_lock, NULL);
	sim->stop = false;
	sim->start_time_ms = get_time_ms();
	sim->coders = malloc(sizeof(t_coder) * sim->number_of_coders);
	if (!sim->coders)
		return (-1);
	i = 0;
	while (i < sim->number_of_coders)
	{
		sim->coders[i].id = i + 1;
		sim->coders[i].compiles_done = 0;
		sim->coders[i].state = WAITING_FOR_DONGLES;
		sim->coders[i].last_compile_start_ms = sim->start_time_ms;
		sim->coders[i].left = &sim->dongles[i];
		sim->coders[i].right = &sim->dongles[(i + 1) % sim->number_of_coders];
		sim->coders[i].sim = sim;
		i++;
	}
	return (0);
}

static int	create_all_threads(t_simulation *sim)
{
	int	i;

	if (pthread_create(&sim->monitor_thread, NULL, monitor_routine, sim) != 0)
		return (-1);
	i = 0;
	while (i < sim->number_of_coders)
	{
		if (pthread_create(&sim->coders[i].thread, NULL,
				coder_routine, &sim->coders[i]) != 0)
		{
			sim_request_stop(sim);
			i--;
			while (i >= 0)
			{
				pthread_join(sim->coders[i].thread, NULL);
				i--;
			}
			pthread_join(sim->monitor_thread, NULL);
			return (-1);
		}
		i++;
	}
	return (0);
}

static void	join_all_threads(t_simulation *sim)
{
	int	i;

	i = 0;
	while (i < sim->number_of_coders)
	{
		pthread_join(sim->coders[i].thread, NULL);
		i++;
	}
	pthread_join(sim->monitor_thread, NULL);
}

static void	sim_cleanup(t_simulation *sim)
{
	destroy_up_to(sim->dongles, sim->number_of_coders);
	free(sim->coders);
	pthread_mutex_destroy(&sim->log_lock);
	pthread_mutex_destroy(&sim->stop_lock);
	pthread_mutex_destroy(&sim->state_lock);
}

int	main(int ac, char **av)
{
	t_simulation	sim;

	if (!parser(ac, av, &sim))
	{
		write(2, "Error: parsing failed.", 22);
		return (1);
	}
	if (sim_setup_all(&sim) != 0)
	{
		destroy_up_to(sim.dongles, sim.number_of_coders);
		write(2, "Error: setup failed.", 21);
		return (1);
	}
	if (create_all_threads(&sim) == 0)
		join_all_threads(&sim);
	sim_cleanup(&sim);
	return (0);
}
