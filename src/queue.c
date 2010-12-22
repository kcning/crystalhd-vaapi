#include <stdlib.h>
#include "queue.h"

static queue_node_p __pool;

static inline
__attribute__ ((always_inline))
queue_node_p __queue_malloc()
{
	queue_node_p next_node = __pool;
	if ( NULL != next_node )
		__pool = __pool->next;
	else
		next_node = (queue_node_p)malloc(sizeof(queue_node));
	return next_node;
}

static inline
__attribute__ ((always_inline))
void __queue_free(queue_node_p node)
{
	if (NULL == node)
		return;
	node->next = __pool;
	__pool = node;
}

inline
queue_base_p queue_init( queue_base_p queue )
{
	queue->front = queue->rear = NULL;
}

inline
void queue_destroy( queue_base_p queue )
{
	while ( queue_peek( queue ) )
		queue_dequeue( queue );
}

inline
queue_node_p queue_enqueue( queue_base_p queue, void * const data /* in */ )
{
	queue_node_p new_node = __queue_malloc();
	if ( NULL == new_node )	// alloc failed
		return NULL;

	new_node->next = NULL;
	new_node->data = data;

	if ( queue->rear )
		queue->rear->next = new_node;
	queue->rear = new_node;

	if ( NULL == queue->front )
		queue->front = new_node;

	return queue_peek( queue );
}

inline
int queue_dequeue( queue_base_p queue )
{
	if ( queue_isempty( queue ) )
		return 0;
	
	// check if this is the last item in queue
	if ( queue->front == queue->rear )
		queue->rear = NULL;

	queue_node_p tmp = queue->front;
	queue->front = tmp->next;
	__queue_free(tmp);

	return 1;
}

inline
queue_node_p queue_peek( queue_base_p queue )
{
	return queue->rear;
	if ( NULL != queue->rear )
		return queue->rear->data;
	return NULL;
}

inline
int queue_isempty( queue_base_p queue )
{
	return NULL == queue->rear;
}
