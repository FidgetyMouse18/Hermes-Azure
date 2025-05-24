#include "node_list.h"

sys_dlist_t node_list = SYS_DLIST_STATIC_INIT(&node_list);

int node_add(const uint8_t *uuid, const uint16_t timestamp)
{
    // struct node_node *current;
    // SYS_DLIST_FOR_EACH_CONTAINER(&node_list, current, node)
    // {
    //     if (memcmp(current->uuid, uuid, BLE_UUID_LEN) == 0)
    //     {
    //         return -EEXIST;
    //     }
    // }

    struct node_node *new_node = k_malloc(sizeof(struct node_node));
    if (!new_node)
    {
        return -ENOMEM;
    }

    memcpy(new_node->uuid, uuid, BLE_UUID_LEN);
    new_node->timestamp = timestamp;

    sys_dlist_append(&node_list, &new_node->node);
    return 0;
}

int node_remove(const uint8_t *uuid)
{
    struct node_node *current;
    SYS_DLIST_FOR_EACH_CONTAINER(&node_list, current, node)
    {
        if (memcmp(current->uuid, uuid, BLE_UUID_LEN) == 0)
        {
            sys_dlist_remove(&current->node);
            k_free(current);
            return 0;
        }
    }
    return -ENOENT;
}

int check_node(const uint8_t *uuid, const uint16_t timestamp)
{
    struct node_node *current;
    SYS_DLIST_FOR_EACH_CONTAINER(&node_list, current, node)
    {
        if (memcmp(current->uuid, uuid, BLE_UUID_LEN) == 0)
        {
            if (timestamp == current->timestamp)
            {
                return 0;
            }
            return -2;
        }
    }
    return -1;
}
