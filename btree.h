#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct BTreeNode
{
    int64_t key;
    int64_t value;
    struct BTreeNode *left;
    struct BTreeNode *right;
} BTreeNode;

typedef struct BTree
{
    int64_t size;
    BTreeNode *root;
} BTree;

/**
 * Initializes a new BTree.
 *
 * @param tree The tree to initialize (stack-allocated).
 */
void btree_init(BTree *tree);

/**
 * Deallocates the memory used by the tree and its nodes.
 *
 * @param tree The tree to destroy.
 */
void btree_destroy(BTree *tree);

/**
 * Returns true if the tree contains the given key.
 *
 * @param tree The tree where to search.
 * @param key The key to search.
 */
bool btree_contains(const BTree *tree, int64_t key);

/**
 * Searches the tree and returns the node that matches the given key,
 * or NULL if the key was not found.
 *
 * @param tree The tree to where to search for the key.
 * @param key The key to search.
 *
 * @return The node that matches the key or NULL if the key was not found.
 */
BTreeNode *btree_search(const BTree *tree, int64_t key);

/**
 * Inserts a new key into the tree.
 *
 * @param tree The tree where to insert the key.
 * @param key The key to insert.
 * @param value The value of the node.
 *
 * @return The new node appended to the tree.
 */
BTreeNode *btree_insert(BTree *tree, int64_t key, int64_t value);

/**
 * Removes a key from the tree.
 *
 * @param tree The tree where to remove the key from.
 * @param key The key to remove.
 * @param old_value If not NULL, contains the value stored in the removed node.
 *
 * @return True if a key was found and successfully removed from the tree. Otherwise, false.
 */
bool btree_remove(BTree *tree, int64_t key, int64_t *old_value);
