#include "btree.h"
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
static BTreeNode *node_create(int64_t key, int64_t value)
{
    BTreeNode *node = malloc(sizeof(BTreeNode));
    if (!is_null(node)) {
        node->key = key;
        node->value = value;
        node->left = NULL;
        node->right = NULL;
    }

    return node;
}

void btree_init(BTree *tree)
{
    tree->size = 0;
    tree->root = NULL;
}

/**
 * Recursively destroys tree nodes.
 */
static void btree_node_destroy_recursive(BTreeNode *node)
{
    if (!is_null(node)) {
        btree_node_destroy_recursive(node->left);
        btree_node_destroy_recursive(node->right);
        free(node);
    }
}

void btree_destroy(BTree *tree)
{
    if (tree->size > 0) {
        btree_node_destroy_recursive(tree->root);
        tree->size = 0;
        tree->root = NULL;
    }
}

bool btree_contains(const BTree *tree, int64_t key)
{
    BTreeNode *current = tree->root;
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
static BTreeNode *btree_node_search_recursive(BTreeNode *node, int64_t key, BTreeNode **parent)
{
    BTreeNode *last = NULL;
    BTreeNode *current = node;

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

BTreeNode *btree_search(const BTree *tree, int64_t key)
{
    if (tree->size > 0) {
        return btree_node_search_recursive(tree->root, key, NULL);
    }

    return NULL;
}

/**
 * Recursively inserts a new node in the tree.
 */
static BTreeNode *btree_node_insert_recursive(BTreeNode *parent, int64_t key, int64_t value)
{
    BTreeNode **child = key < parent->key ? &parent->left : &parent->right;
    if (is_null(*child)) {
        *child = node_create(key, value);
        return *child;
    } else {
        return btree_node_insert_recursive(*child, key, value);
    }
}

BTreeNode *btree_insert(BTree *tree, int64_t key, int64_t value)
{
    BTreeNode *new_node = NULL;
    if (tree_is_empty(tree)) {
        assert(tree->root == NULL);
        new_node = node_create(key, value);
        tree->root = new_node;
    } else {
        new_node = btree_node_insert_recursive(tree->root, key, value);
    }

    if (!is_null(new_node)) {
        tree->size++;
    }

    return new_node;
}

/**
 * Finds the node that contains the smallest key.
 */
static BTreeNode *btree_node_search_successor(BTreeNode *node)
{
    while (!is_null(node->left)) {
        node = node->left;
    }

    return node;
}

/**
 * Recursively removes a node from the tree.
 */
static bool btree_node_remove_recursive(BTreeNode **root, int64_t key, int64_t *value)
{
    BTreeNode *parent = NULL;
    BTreeNode *current = btree_node_search_recursive(*root, key, &parent);

    if (is_null(current)) {
        return false;
    }

    if (!is_null(value)) {
        *value = current->value;
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
        BTreeNode *successor = btree_node_search_successor(current->right);
        int64_t subkey = successor->key;
        int64_t old_val = successor->value;
        btree_node_remove_recursive(root, subkey, NULL);
        current->key = subkey;
        current->value = old_val;
    } else {
        BTreeNode *child = !is_null(current->left) ? current->left : current->right;
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

bool btree_remove(BTree *tree, int64_t key, int64_t *address)
{
    if (!tree_is_empty(tree)) {
        if (btree_node_remove_recursive(&tree->root, key, address)) {
            tree->size--;
            assert(tree->size >= 0);
            return true;
        }
    }

    return false;
}
