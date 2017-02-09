/*! \file switch_list.h
  	\author Samson
	\date 2016-05-28
    \brief list header
*/
#ifndef SWITCH_LIST_H
#define SWITCH_LIST_H

SWITCH_BEGIN_EXTERN_C

typedef struct switch_list_node switch_list_node_t;
typedef struct switch_list switch_list_t;

// create a list with mutex
SWITCH_DECLARE(switch_status_t) switch_list_create(switch_list_t** list, switch_memory_pool_t *pool);

// push to [node] back, if node==NULL then add a new last node
SWITCH_DECLARE(switch_status_t) switch_list_pushback(switch_list_t* list, switch_list_node_t* node, void* data);

// push to [node] front, if node==NULL then add a new first node
SWITCH_DECLARE(switch_status_t) switch_list_pushfront(switch_list_t* list, switch_list_node_t* node, void* data);

// get the [node]'s next node, if node==NULL then get the first node
SWITCH_DECLARE(switch_status_t) switch_list_get_next(switch_list_t* list, switch_list_node_t* node, switch_list_node_t** next);

// remove [node], and ouput the [node]'s next node, if [next]==NULL then do not output
SWITCH_DECLARE(switch_status_t) switch_list_remove(switch_list_t* list, switch_list_node_t* node, switch_list_node_t** next);

// get [node]'s data
SWITCH_DECLARE(switch_status_t) switch_list_get_node_data(switch_list_t* list, switch_list_node_t* node, void** data);

// lock the [list]
SWITCH_DECLARE(switch_status_t) switch_list_lock(switch_list_t* list);

// unlock the [list]
SWITCH_DECLARE(switch_status_t) switch_list_unlock(switch_list_t* list);

SWITCH_END_EXTERN_C
#endif
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
