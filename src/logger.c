/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   logger.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: wabdi <wabdi@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/27 23:42:53 by wabdi             #+#    #+#             */
/*   Updated: 2026/08/27 23:45:46 by wabdi            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

void	log_state(t_simulation *sim, int coder_id, char *msg)
{
	long	elapsed;

	elapsed = get_time_ms() - sim->start_time_ms;
	pthread_mutex_lock(&sim->log_lock);
	printf("%ld %d %s\n", elapsed, coder_id, msg);
	pthread_mutex_unlock(&sim->log_lock);
}
