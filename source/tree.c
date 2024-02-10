/**
 * Grupo 65
 *    Vitória Zhu         Nº 56291
 *    Su Lishun           Nº 56375
 *    Erickson Cacondo    Nº 53653 
 */

#include <stdlib.h>
#include <string.h>
#include "data.h"
#include "tree-private.h"

/* Função para criar uma nova árvore tree vazia.
 * Em caso de erro retorna NULL.
 */
struct tree_t *tree_create() {
    struct tree_t* tree;
    int size = sizeof(struct tree_t);
    return NULL == (tree = malloc(size)) ? NULL : memset(tree, 0, size);
}

/* Função para libertar toda a memória ocupada por uma árvore.
 */
void tree_destroy(struct tree_t *tree) {
    if (NULL != tree) {
        tree_destroy(tree->left);
        tree_destroy(tree->right);
        treeElement_destroy(tree);
    }
}

/* Função para adicionar um par chave-valor à árvore.
 * Os dados de entrada desta função deverão ser copiados, ou seja, a
 * função vai *COPIAR* a key (string) e os dados para um novo espaço de
 * memória que tem de ser reservado. Se a key já existir na árvore,
 * a função tem de substituir a entrada existente pela nova, fazendo
 * a necessária gestão da memória para armazenar os novos dados.
 * Retorna 0 (ok) ou -1 em caso de erro.
 */
int tree_put(struct tree_t *tree, char *key, struct data_t *value) {
    if (NULL == tree || NULL == key) {
        return -1;
    }
    // create a new entry with the given key and value
    struct entry_t *newEntry = entry_create(strdup(key), data_dup(value));

    // check if the tree is empty
    if (NULL == tree->entry) {
        tree->entry = newEntry;
        return 0;
    }

    // find the place to put the entry
    struct tree_t *child = tree, *parent = NULL;
    do {
        parent = child;
        int cmp = entry_compare(newEntry, child->entry);
        if (0 > cmp) {
            child = child->left;
        } else if (0 < cmp) {
            child = child->right;
        } else {
            break;
        }
    } while (NULL != child);

    // put the entry
    int cmp = entry_compare(newEntry, parent->entry);
    if (0 > cmp) {
        parent->left = tree_create();
        parent->left->entry = newEntry;
    } else if (0 < cmp) {
        parent->right = tree_create();
        parent->right->entry = newEntry;
    } else {
        entry_destroy(parent->entry);
        parent->entry = newEntry;
    }
    return 0;
}

/* Função para obter da árvore o valor associado à chave key.
 * A função deve devolver uma cópia dos dados que terão de ser
 * libertados no contexto da função que chamou tree_get, ou seja, a
 * função aloca memória para armazenar uma *CÓPIA* dos dados da árvore,
 * retorna o endereço desta memória com a cópia dos dados, assumindo-se
 * que esta memória será depois libertada pelo programa que chamou
 * a função. Devolve NULL em caso de erro.
 */
struct data_t *tree_get(struct tree_t *tree, char *key) {
    if (NULL == tree || NULL == tree->entry) {
        return NULL;
    }

    struct data_t *ret = NULL;
    struct tree_t *curr = tree;
    // temporary entry for comparing keys
    struct entry_t *tempEntry = entry_create(strdup(key), NULL);

    // go down the tree to find the element
    do {
        int cmp = entry_compare(tempEntry, curr->entry);
        if (0 > cmp) {
            curr = curr->left;
        } else if (0 < cmp) {
            curr = curr->right;
        } else {
            ret = data_dup(curr->entry->value);
            break;
        }
    } while (NULL != curr);

    // delete temporary entry
    entry_destroy(tempEntry);

    return ret;
}

/* Função para remover um elemento da árvore, indicado pela chave key,
 * libertando toda a memória alocada na respetiva operação tree_put.
 * Retorna 0 (ok) ou -1 (key not found).
 */
int tree_del(struct tree_t *tree, char *key) {
    if (NULL == tree || NULL == tree->entry || NULL == key) {
        return -1;
    }

    struct tree_t *child = tree, *parent = NULL;
    // temporary entry for comparing keys
    struct entry_t *tempEntry = entry_create(strdup(key), NULL);

    // go down the tree to find the element
     do {
        int cmp = entry_compare(tempEntry, child->entry);
        if (0 > cmp) {
            parent = child;
            child = child->left;
        } else if (0 < cmp) {
            parent = child;
            child = child->right;
        } else {
            break;
        }
    } while (NULL != child);

    // delete the temporary entry
    entry_destroy(tempEntry);

    // key not found
    if (NULL == child) {
        return -1;
    }
    
    // the tree to be deleted has 0 or 1 child
    if (NULL == child->left || NULL == child->right) {
        // for keeping the existing child of the tree
        struct tree_t *temp = NULL == child->left ? child->right : child->left;

        // the tree to be deleted is the root
        if (NULL == parent) {
            entry_destroy(tree->entry);
            
            // the root has no children
            if (NULL == temp) {
                // make it an empty tree
                tree->entry = NULL;

            // the root has one child
            } else {
                // replace the root by its child
                tree->entry = entry_dup(temp->entry);
                tree->left = temp->left;
                tree->right = temp->right;
                treeElement_destroy(temp);
            }
        } else {
            // make parent point to the subtree of child
            // therefore we isolated the child tree 
            if (parent->left == child) {
                parent->left = temp;
            } else {
                parent->right = temp;
            }
            // delete the child
            treeElement_destroy(child);
        }
    } else {
        struct tree_t *temp_parent = child,
                       *temp_child = child->right; // this will be the element to replace the place of deleted tree
        // find the minimum entry in the right subtree of the child tree
        while (NULL != temp_child->left) {
            temp_parent = temp_child;
            temp_child = temp_child->left;
        }
        // isolate the found tree
        if (temp_parent != child) {
            temp_parent->left = temp_child->right;
        } else {
            temp_parent->right = temp_child->right;
        }
        // replace the entry of the tree being deleted by a copied entry of the isolated tree
        entry_destroy(child->entry);
        child->entry = entry_dup(temp_child->entry);
        // delete the isolated tree
        treeElement_destroy(temp_child);
    }
    return 0;
}

/* Função que devolve o número de elementos contidos na árvore.
 */
int tree_size(struct tree_t *tree) {
    if (NULL == tree || NULL == tree->entry) {
        return 0;
    }
    return 1 + tree_size(tree->left) + tree_size(tree->right);
}

/* Função que devolve a altura da árvore.
 */
int tree_height(struct tree_t *tree) {
    if (NULL == tree || NULL == tree->entry) {
        return 0;
    }
    return 1 + max(tree_height(tree->left), tree_height(tree->right));
}

/* Função que devolve um array de char* com a cópia de todas as keys da
 * árvore, colocando o último elemento do array com o valor NULL e
 * reservando toda a memória necessária. As keys devem vir ordenadas segundo a ordenação lexicográfica das mesmas.
 */
char **tree_get_keys(struct tree_t *tree) {
    int size = tree_size(tree);
    char **keys = calloc(size + 1, sizeof(char*));
    if (0 < size) {
        struct tree_t* trees [size]; // stack for keeping visited trees
        struct tree_t* curr = tree;
        int i = 0, // stack index
            j = 0; // array index
        while (j != size) {
            // inorder traversal
            while (NULL != curr) {
                trees[i++] = curr;
                curr = curr->left;
            }
            keys[j++] = strdup(trees[--i]->entry->key);
            curr = trees[i]->right;
        }
    }
    return keys;
}

/* Função que devolve um array de void* com a cópia de todas os values da
 * árvore, colocando o último elemento do array com o valor NULL e
 * reservando toda a memória necessária.
 */
void **tree_get_values(struct tree_t *tree) {
    int size = tree_size(tree);
    void **values = calloc(size + 1, sizeof(void*));
    if (0 < size) {
        struct tree_t* trees [size]; // stack for keeping visited trees
        struct tree_t* curr = tree;
        int i = 0, // stack index
            j = 0; // array index
        while (j != size) {
            // inorder traversal
            while (NULL != curr) {
                trees[i++] = curr;
                curr = curr->left;
            }
            values[j++] = data_dup(trees[--i]->entry->value);
            curr = trees[i]->right;
        }
    }
    return values;
}


/* Função que liberta toda a memória alocada por tree_get_keys().
 */
void tree_free_keys(char **keys) {
    if (NULL == keys) {
        return;
    }
    int i = 0;
    while (NULL != keys[i]) {
        free(keys[i++]);
    }
    free(keys);
}

/* Função que liberta toda a memória alocada por tree_get_values().
 */
void tree_free_values(void **values) {
    if (NULL == values) {
        return;
    }
    int i = 0;
    while (NULL != values[i]) {
        data_destroy(values[i++]);
    }
    free(values);
}

/* Função que devolve o maior de dois parâmetros.
 */
int max(int a, int b) {
    return a > b ? a : b;
}

/* Função que elimina um elemento do tipo struct tree_t, 
 * libertando a memória por ele ocupado.
 */
void treeElement_destroy(struct tree_t* tree) {
    if (NULL != tree) {
        entry_destroy(tree->entry);
        free(tree);
    }
}