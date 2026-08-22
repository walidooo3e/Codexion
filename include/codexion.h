/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   codexion.h                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: wabdi <wabdi@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/22 16:55:50 by wabdi             #+#    #+#             */
/*   Updated: 2026/08/22 16:58:15 by wabdi            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CODEXION_H
# define CODEXION_H

# include <pthread.h>
# include <sys/time.h>
# include <stdbool.h>
# include <stdlib.h>

typedef enum e_coder_state
{
	WAITING_FOR_DONGLES,
	COMPILING,
	DEBUGGING,
	REFACTORING,
	BURNED_OUT
}	t_coder_state;

typedef struct s_dongle
{
	int				id;
	bool			in_use;
	long			available_at_ms;
	pthread_mutex_t	lock;
	pthread_cond_t	cond;
}	t_dongle;

typedef struct s_coder
{
	int					id;
	int					compiles_done;
	t_coder_state		state;
	long				last_compile_start_ms;
	t_dongle			*left;
	t_dongle			*right;
	pthread_t			thread;
	struct s_simulation	*sim;
}	t_coder;

typedef enum e_scheduler
{
	SCHEDULER_FIFO,
	SCHEDULER_EDF
}	t_scheduler;

typedef struct s_simulation
{
	int				number_of_coders;
	long			time_to_burnout;
	long			time_to_compile;
	long			time_to_debug;
	long			time_to_refactor;
	int				number_of_compiles_required;
	long			dongle_cooldown;
	t_scheduler		scheduler;
	t_coder			*coders;
	t_dongle		*dongles;
	pthread_mutex_t	log_lock;
	pthread_mutex_t	stop_lock;
	bool			stop;
	long			start_time_ms;
	pthread_t		monitor_thread;
}	t_simulation;

/* utils.c */
long	get_time_ms(void);

/* dongle.c */
int		dongle_init(t_simulation *sim);
void	dongle_destroy(t_simulation *sim);

#endif