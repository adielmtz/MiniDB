#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct BinaryTreeNode
{
    int64_t key;
    struct BinaryTreeNode *left;
    struct BinaryTreeNode *right;
} BinaryTreeNode;

typedef struct BinaryTree
{
    int64_t size;
    BinaryTreeNode *root;
} BinaryTree;

/**
 * Initializes a new SearchTree object.
 *
 * @param tree The tree to initialize (stack-allocated).
 */
void binarytree_init(BinaryTree *tree);

/**
 * Deallocates the memory used by the tree and its nodes.
 *
 * @param tree The tree to destroy.
 */
void binarytree_destroy(BinaryTree *tree);

/**
 * Returns true if the tree contains the given key.
 *
 * @param tree The tree where to search.
 * @param key The key to search.
 */
bool binarytree_contains(const BinaryTree *tree, int64_t key);

/**
 * Searches the tree and returns the node that matches the given key,
 * or NULL if the key was not found.
 *
 * @param tree The tree to where to search for the key.
 * @param key The key to search.
 *
 * @return The node that matches the key or NULL if the key was not found.
 */
BinaryTreeNode *binarytree_search(const BinaryTree *tree, int64_t key);

/**
 * Inserts a new key into the tree.
 *
 * @param tree The tree where to insert the key.
 * @param key The key to insert.
 *
 * @return The new node appended to the tree.
 */
BinaryTreeNode *binarytree_insert(BinaryTree *tree, int64_t key);

/**
 * Removes a key from the tree.
 *
 * @param tree The tree where to remove the key from.
 * @param key The key to remove.
 *
 * @return True if a key was found and successfully removed from the tree. Otherwise, false.
 */
bool binarytree_remove(BinaryTree *tree, int64_t key);
