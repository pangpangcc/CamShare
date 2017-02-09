/*! \file switch_list.c
  	\author Samson
	\date 2016-05-28
    \brief list header
*/

#include <switch.h>
#include <switch_list.h>

struct switch_list_node {
	void* data;
	struct switch_list_node* next;
};

struct switch_list {
	switch_list_node_t* head;
	switch_list_node_t* last;
	switch_mutex_t* mutex;
	switch_queue_t* free_node_queue;
	switch_memory_pool_t* pool;
};

// get list node
static switch_list_node_t* get_list_node(switch_list_t* list)
{
	switch_list_node_t* node = NULL;
	if (SWITCH_STATUS_SUCCESS != switch_queue_trypop(list->free_node_queue, (void**)&node)) {
		node = (switch_list_node_t*)switch_core_alloc(list->pool, sizeof(switch_list_node_t));
		memset(node, 0, sizeof(switch_list_node_t));
	}
	return node;
}

// recycle node
static void recycle_list_node(switch_list_t* list, switch_list_node_t* node)
{
	memset(node, 0, sizeof(switch_list_node_t));
	switch_queue_trypush(list->free_node_queue, node);
}

// create a list with mutex
SWITCH_DECLARE(switch_status_t) switch_list_create(switch_list_t** list, switch_memory_pool_t *pool)
{
	// create list
	*list = (switch_list_t*)switch_core_alloc(pool, sizeof(switch_list_t));
	memset(*list, 0, sizeof(switch_list_t));

	// set param
	(*list)->pool = pool;

	// create mutex
	switch_mutex_init(&(*list)->mutex, SWITCH_MUTEX_NESTED, (*list)->pool);

	// create free queue
	switch_queue_create(&(*list)->free_node_queue, SWITCH_CORE_QUEUE_LEN, (*list)->pool);

	return SWITCH_STATUS_SUCCESS;
}

// push to [node] back, if node==NULL then add a new last node
SWITCH_DECLARE(switch_status_t) switch_list_pushback(switch_list_t* list, switch_list_node_t* node, void* data)
{
	switch_list_node_t* newnode = get_list_node(list);
	newnode->data = data;

	if (NULL == list->head) {
		list->head = newnode;
		list->last = list->head;
	}
	else if (NULL == node || list->last == node) {
		list->last->next = newnode;
		list->last = newnode;
	}
	else {
		switch_list_node_t* tmp = node->next;
		node->next = newnode;
		newnode->next = tmp;
	}

	return SWITCH_STATUS_SUCCESS;
}

// push to [node] front, if node==NULL then add a new first node
SWITCH_DECLARE(switch_status_t) switch_list_pushfront(switch_list_t* list, switch_list_node_t* node, void* data)
{
	switch_status_t result = SWITCH_STATUS_SUCCESS;
	switch_list_node_t* newnode = get_list_node(list);
	newnode->data = data;

	if (NULL == list->head || NULL == node) {
		newnode->next = list->head;
		list->head = newnode;
	}
	else {
		switch_list_node_t* prevnode = NULL;
		for (prevnode = list->head;
			NULL != prevnode->next && node != prevnode->next;
			prevnode = prevnode->next);

		if (node == prevnode->next) {
			switch_list_node_t* temp = prevnode->next;
			prevnode->next = newnode;
			newnode->next = temp;
		}
		else {
			result = SWITCH_STATUS_FALSE;
		}
	}

	return result;
}

// get the [node]'s next node, if node==NULL then get the first node
SWITCH_DECLARE(switch_status_t) switch_list_get_next(switch_list_t* list, switch_list_node_t* node, switch_list_node_t** next)
{
	if (NULL != node) {
		*next = node->next;
	}
	else {
		*next = list->head;
	}
	return SWITCH_STATUS_SUCCESS;
}

// remove [node], and ouput the [node]'s next node, if [next]==NULL then do not output
SWITCH_DECLARE(switch_status_t) switch_list_remove(switch_list_t* list, switch_list_node_t* node, switch_list_node_t** next)
{
	switch_status_t result = SWITCH_STATUS_SUCCESS;

	if (node == list->head) {
		list->head = list->head->next;
		recycle_list_node(list, node);

		if (NULL != next) {
			*next = list->head;
		}
	}
	else {
		switch_list_node_t* prevnode = NULL;
		for (prevnode = list->head;
			NULL != prevnode->next && node != prevnode->next;
			prevnode = prevnode->next);

		if (node == prevnode->next) {
			if (node == list->last) {
				list->last = prevnode;
			}

			prevnode->next = node->next;
			recycle_list_node(list, node);

			if (NULL != next) {
				*next = prevnode->next;
			}
		}
		else {
			result = SWITCH_STATUS_FALSE;
		}
	}

	return result;
}

// get [node]'s data
SWITCH_DECLARE(switch_status_t) switch_list_get_node_data(switch_list_t* list, switch_list_node_t* node, void** data)
{
	*data = node->data;
	return SWITCH_STATUS_SUCCESS;
}

// lock the [list]
SWITCH_DECLARE(switch_status_t) switch_list_lock(switch_list_t* list)
{
	switch_mutex_lock(list->mutex);
	return SWITCH_STATUS_SUCCESS;
}

// unlock the [list]
SWITCH_DECLARE(switch_status_t) switch_list_unlock(switch_list_t* list)
{
	switch_mutex_unlock(list->mutex);
	return SWITCH_STATUS_SUCCESS;
}

/* For Emacs:
 * Local Variables:
 * mode:c
 * indent-tabs-mode:t
 * tab-width:4
 * c-basic-offset:4
 * End:
 * For VIM:
 * vim:set softtabstop=4 shiftwidth=4 tabstop=4 noet:
 */
