#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "ring_queue.h"

#include "common.h"

#undef  	DBG_ON
#undef  	FILE_NAME
#define 	DBG_ON  	(0x01)
#define 	FILE_NAME 	"ring_queue"



int ring_queue_init(ring_queue_t *queue, int buffer_size)
{
    queue->size = buffer_size;
    queue->flags = (char *)calloc(1,queue->size);
    if (queue->flags == NULL)
    {
        return -1;
    }
    queue->data = (void **)calloc(queue->size, sizeof(void*));
    if (queue->data == NULL)
    {
        free(queue->flags);
        return -1;
    }
    queue->head = 0;
    queue->tail = 0;
    memset(queue->flags, 0, queue->size);
    memset(queue->data, 0, queue->size * sizeof(void*));
    return 0;
}



int ring_queue_push(ring_queue_t *queue, void * ele)
{
    if (!(queue->num < queue->size))
    {
        return -1;
    }
    int cur_tail_index = queue->tail;
    char * cur_tail_flag_index = queue->flags + cur_tail_index; 
    while (!compare_and_swap(cur_tail_flag_index, 0, 1))
    {
        cur_tail_index = queue->tail;
        cur_tail_flag_index = queue->flags + cur_tail_index;
    }

    int update_tail_index = (cur_tail_index + 1) % queue->size;
	
    compare_and_swap(&queue->tail, cur_tail_index, update_tail_index);
    *(queue->data + cur_tail_index) = ele;
	
    fetch_and_add(cur_tail_flag_index, 1); 
    fetch_and_add(&queue->num, 1);
    return 0;
}


int  ring_queue_pop(ring_queue_t *queue, void **ele)
{
    if (!(queue->num > 0))
        return -1;
    int cur_head_index = queue->head;
    char * cur_head_flag_index = queue->flags + cur_head_index;

    while (!compare_and_swap(cur_head_flag_index, 2, 3)) 
    {
        cur_head_index = queue->head;
        cur_head_flag_index = queue->flags + cur_head_index;
    }

    int update_head_index = (cur_head_index + 1) % queue->size;
    compare_and_swap(&queue->head, cur_head_index, update_head_index);
    *ele = *(queue->data + cur_head_index);
	
    fetch_and_sub(cur_head_flag_index, 3);
    fetch_and_sub(&queue->num, 1);
    return 0;
}


