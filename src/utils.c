/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: wabdi <wabdi@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/23 00:55:38 by wabdi             #+#    #+#             */
/*   Updated: 2026/08/23 00:59:06 by wabdi            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

/* get the time of the day in seconds and micro seconds */
long	get_time_ms(void)
{
	struct timeval	tv;

	gettimeofday(&tv, NULL);
	return ((tv.tv_sec * 1000L) + (tv.tv_usec / 1000L));
}

/* convert time from micro seconds to nanoseconds */
void	ms_to_timespec(long ms, struct timespec *ts)
{
	ts->tv_sec = ms / 1000;
	ts->tv_nsec = (ms % 1000) * 1000000L;
}
