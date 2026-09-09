/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   heap.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: wabdi <wabdi@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/31 23:54:06 by wabdi             #+#    #+#             */
/*   Updated: 2026/09/09 01:38:34 by wabdi            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static void	heap_sift_up(t_heap *h, int i)
{
	int				parent;
	t_wait_entry	temp;

	while (i > 0)
	{
		parent = (i - 1) / 2;
		if (h->data[i].key < h->data[parent].key
			|| (h->data[i].key == h->data[parent].key
				&& h->data[i].coder_id < h->data[parent].coder_id))
		{
			temp = h->data[i];
			h->data[i] = h->data[parent];
			h->data[parent] = temp;
			i = parent;
		}
		else
			break ;
	}
}

static void	heap_sift_down(t_heap *h, int i)
{
	int				left;
	int				right;
	int				smallest;
	t_wait_entry	temp;

	while (1)
	{
		smallest = i;
		left = 2 * i + 1;
		right = 2 * i + 2;
		if (left < h->size && h->data[left].key < h->data[smallest].key)
			smallest = left;
		if (right < h->size && h->data[right].key < h->data[smallest].key)
			smallest = right;
		if (smallest == i)
			break ;
		temp = h->data[i];
		h->data[i] = h->data[smallest];
		h->data[smallest] = temp;
		i = smallest;
	}
}

int	heap_push(t_heap *h, int coder_id, long key)
{
	if (h->size >= h->capacity)
		return (-1);
	h->data[h->size].coder_id = coder_id;
	h->data[h->size].key = key;
	h->size++;
	heap_sift_up(h, h->size - 1);
	return (0);
}

int	heap_peek_front_id(t_heap *h)
{
	if (h->size == 0)
		return (-1);
	return (h->data[0].coder_id);
}

void	heap_remove_by_id(t_heap *h, int coder_id)
{
	int	i;
	int	parent;

	i = 0;
	while (i < h->size && h->data[i].coder_id != coder_id)
		i++;
	if (i == h->size)
		return ;
	h->size--;
	h->data[i] = h->data[h->size];
	if (i >= h->size)
		return ;
	parent = (i - 1) / 2;
	if (i > 0 && (h->data[i].key < h->data[parent].key
			|| (h->data[i].key == h->data[parent].key
				&& h->data[i].coder_id < h->data[parent].coder_id)))
		heap_sift_up(h, i);
	else
		heap_sift_down(h, i);
}
