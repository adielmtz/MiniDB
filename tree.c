#include "tree.h"
#include <assert.h>
#include <stdlib.h>

#ifndef is_null
#define is_null(ptr) ((ptr) == NULL)
#endif

#define tree_is_empty(tree) ((tree)->size == 0)
#define node_is_leaf(node) (is_null((node)->left) && is_null((node)->right))
#define node_is_branch(node) (!is_null((node)->left) && !is_null((node)->right))

/**
 * Creates a new empty node.
 */
static BinaryTreeNode *node_create(int64_t key, int64_t address)
{
    BinaryTreeNode *node = malloc(sizeof(BinaryTreeNode));
    if (!is_null(node)) {
        node->key = key;
        node->address = address;
        node->left = NULL;
        node->right = NULL;
    }

    return node;
}

void binarytree_init(BinaryTree *tree)
{
    tree->size = 0;
    tree->root = NULL;
}

/**
 * Recursively destroys tree nodes.
 */
static void node_destroy_recurse(BinaryTreeNode *node)
{
    if (!is_null(node)) {
        node_destroy_recurse(node->left);
        node_destroy_recurse(node->right);
        free(node);
    }
}

void binarytree_destroy(BinaryTree *tree)
{
    if (tree->size > 0) {
        node_destroy_recurse(tree->root);
        tree->size = 0;
        tree->root = NULL;
    }
}

bool binarytree_contains(const BinaryTree *tree, int64_t key)
{
    BinaryTreeNode *current = tree->root;
    while (!is_null(current)) {
        if (current->key == key) {
            return true;
        }

        current = key < current->key ? current->left : current->right;
    }

    return false;
}

/**
 * Recursively searches for a node that matches the given key.
 */
static BinaryTreeNode *node_search_recurse(BinaryTreeNode *node, int64_t key, BinaryTreeNode **parent)
{
    BinaryTreeNode *last = NULL;
    BinaryTreeNode *current = node;

    while (current != NULL && current->key != key) {
        last = current;
        if (key < current->key) {
            current = current->left;
        } else {
            current = current->right;
        }
    }

    if (!is_null(parent)) {
        *parent = last;
    }

    return current;
}

BinaryTreeNode *binarytree_search(const BinaryTree *tree, int64_t key)
{
    if (tree->size > 0) {
        return node_search_recurse(tree->root, key, NULL);
    }

    return NULL;
}

/**
 * Recursively inserts a new node in the tree.
 */
static BinaryTreeNode *tree_insert_node_recurse(BinaryTreeNode *parent, int64_t key, int64_t address)
{
    BinaryTreeNode **child = key < parent->key ? &parent->left : &parent->right;
    if (is_null(*child)) {
        *child = node_create(key, address);
        return *child;
    } else {
        return tree_insert_node_recurse(*child, key, address);
    }
}

BinaryTreeNode *binarytree_insert(BinaryTree *tree, int64_t key, int64_t address)
{
    BinaryTreeNode *new_node = NULL;
    if (tree_is_empty(tree)) {
        assert(tree->root == NULL);
        new_node = node_create(key, address);
        tree->root = new_node;
    } else {
        new_node = tree_insert_node_recurse(tree->root, key, address);
    }

    if (!is_null(new_node)) {
        tree->size++;
    }

    return new_node;
}

/**
 * Finds the node that contains the smallest key.
 */
static BinaryTreeNode *tree_search_successor_node(BinaryTreeNode *node)
{
    while (!is_null(node->left)) {
        node = node->left;
    }

    return node;
}

/**
 * Recursively removes a node from the tree.
 */
static bool tree_remove_node_recurse(BinaryTreeNode **root, int64_t key, int64_t *address)
{
    BinaryTreeNode *parent = NULL;
    BinaryTreeNode *current = node_search_recurse(*root, key, &parent);

    if (is_null(current)) {
        return false;
    }

    if (!is_null(address)) {
        *address = current->address;
    }

    if (node_is_leaf(current)) {
        if (current != *root) {
            if (current == parent->left) {
                parent->left = NULL;
            } else {
                parent->right = NULL;
            }
        } else {
            *root = NULL;
        }

        free(current);
    } else if (node_is_branch(current)) {
        BinaryTreeNode *successor = tree_search_successor_node(current->right);
        int64_t subkey = successor->key;
        int64_t subaddress = successor->address;
        tree_remove_node_recurse(root, subkey, NULL);
        current->key = subkey;
        current->address = subaddress;
    } else {
        BinaryTreeNode *child = !is_null(current->left) ? current->left : current->right;
        if (current != *root) {
            if (current == parent->left) {
                parent->left = child;
            } else {
                parent->right = child;
            }
        } else {
            *root = child;
        }

        free(current);
    }

    return true;
}

bool binarytree_remove(BinaryTree *tree, int64_t key, int64_t *address)
{
    if (!tree_is_empty(tree)) {
        if (tree_remove_node_recurse(&tree->root, key, address)) {
            tree->size--;
            assert(tree->size >= 0);
            return true;
        }
    }

    return false;
}
