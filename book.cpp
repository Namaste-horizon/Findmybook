#include "book.hpp"

#include <cstring>

namespace {

void copy_text(char* destination, size_t size, const char* source) {
    if (size == 0) {
        return;
    }
    std::strncpy(destination, source == nullptr ? "" : source, size - 1);
    destination[size - 1] = '\0';
}

treenode* min_node(treenode* node) {
    treenode* current = node;
    while (current != nullptr && current->left != nullptr) {
        current = current->left;
    }
    return current;
}

} 

treenode* createnode(int book_id,
                     const char* lib,
                     const char* title,
                     const char* author,
                     int total_copies) {
    treenode* node = new treenode{};
    node->book_id = book_id;
    copy_text(node->lib, sizeof(node->lib), lib);
    copy_text(node->title, sizeof(node->title), title);
    copy_text(node->author, sizeof(node->author), author);
    node->total_copies = total_copies < 1 ? 1 : total_copies;
    node->available_copies = node->total_copies;
    node->left = nullptr;
    node->right = nullptr;
    return node;
}

treenode* insert(treenode* root, treenode* node) {
    if (root == nullptr) {
        return node;
    }
    if (node->book_id < root->book_id) {
        root->left = insert(root->left, node);
    } else if (node->book_id > root->book_id) {
        root->right = insert(root->right, node);
    } else {
        delete node;
    }
    return root;
}

treenode* search_id(treenode* root, int book_id) {
    if (root == nullptr || root->book_id == book_id) {
        return root;
    }
    if (book_id < root->book_id) {
        return search_id(root->left, book_id);
    }
    return search_id(root->right, book_id);
}

void edit(treenode* node,
          const char* author,
          const char* title,
          int available_copies,
          int total_copies) {
    if (node == nullptr) {
        return;
    }
    copy_text(node->author, sizeof(node->author), author);
    copy_text(node->title, sizeof(node->title), title);
    node->total_copies = total_copies < 1 ? 1 : total_copies;
    if (available_copies < 0) {
        available_copies = 0;
    }
    if (available_copies > node->total_copies) {
        available_copies = node->total_copies;
    }
    node->available_copies = available_copies;
}

treenode* deletenode(treenode* root, int book_id) {
    if (root == nullptr) {
        return nullptr;
    }

    if (book_id < root->book_id) {
        root->left = deletenode(root->left, book_id);
        return root;
    }
    if (book_id > root->book_id) {
        root->right = deletenode(root->right, book_id);
        return root;
    }

    if (root->left == nullptr) {
        treenode* right = root->right;
        delete root;
        return right;
    }
    if (root->right == nullptr) {
        treenode* left = root->left;
        delete root;
        return left;
    }

    treenode* successor = min_node(root->right);
    root->book_id = successor->book_id;
    copy_text(root->lib, sizeof(root->lib), successor->lib);
    copy_text(root->title, sizeof(root->title), successor->title);
    copy_text(root->author, sizeof(root->author), successor->author);
    root->total_copies = successor->total_copies;
    root->available_copies = successor->available_copies;
    root->right = deletenode(root->right, successor->book_id);
    return root;
}

void free_bst(treenode* root) {
    if (root == nullptr) {
        return;
    }
    free_bst(root->left);
    free_bst(root->right);
    delete root;
}
