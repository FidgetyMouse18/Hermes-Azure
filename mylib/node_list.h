#ifndef NODE_LIST_H
#define NODE_LIST_H

#include <zephyr/sys/dlist.h>
#include <zephyr/kernel.h>
#include <stdint.h>
#include <zephyr/shell/shell.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define BLE_UUID_LEN 4

struct node_node
{
    sys_dnode_t node;
    uint8_t uuid[BLE_UUID_LEN];
    uint16_t timestamp;
};

extern sys_dlist_t node_list;
int node_add(const uint8_t *uuid, const uint16_t timestamp);
int node_remove(const uint8_t *uuid);
int check_node(const uint8_t *uuid, const uint16_t timestamp);

#endif