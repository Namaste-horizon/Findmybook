#include "book.hpp"

treenode* createnode(int book_id,
                     const char* lib,
                     const char* title,
                     const char* author,
                     int total_copies) {
    treenode* newnode = static_cast<treenode*>(std::malloc(sizeof(treenode)));
    if (!newnode) {
        std::printf("Memory allocation failed\n");
        return nullptr;
    }

    newnode->book_id = book_id;
    std::strncpy(newnode->title, title, sizeof(newnode->title) - 1);
    newnode->title[sizeof(newnode->title) - 1] = '\0';
    std::strncpy(newnode->lib, lib, sizeof(newnode->lib) - 1);
    newnode->lib[sizeof(newnode->lib) - 1] = '\0';
    std::strncpy(newnode->author, author, sizeof(newnode->author) - 1);
    newnode->author[sizeof(newnode->author) - 1] = '\0';
    newnode->total_copies = total_copies;
    newnode->available_copies = total_copies;
    newnode->left = nullptr;
    newnode->right = nullptr;

    return newnode;
}

void view(treenode* root) {
    if (root == nullptr) {
        return;
    }

    std::printf("\nBook Id :%d\nTitle : %s\nAuthor : %s\nAvailable copies : %d \n",
                root->book_id,
                root->title,
                root->author,
                root->available_copies);
    view(root->left);
    view(root->right);
}

treenode* insert(treenode* root, treenode* newnode) {
    if (root == nullptr) {
        return newnode;
    }
    if (root->book_id == newnode->book_id) {
        std::free(newnode);
        return root;
    }
    if (root->book_id > newnode->book_id) {
        root->left = insert(root->left, newnode);
    } else {
        root->right = insert(root->right, newnode);
    }
    return root;
}

void edit(treenode* newnode,
          const char* author,
          const char* title,
          int available_copies,
          int total_copies) {
    std::strncpy(newnode->title, title, sizeof(newnode->title) - 1);
    newnode->title[sizeof(newnode->title) - 1] = '\0';
    std::strncpy(newnode->author, author, sizeof(newnode->author) - 1);
    newnode->author[sizeof(newnode->author) - 1] = '\0';
    newnode->total_copies = total_copies;
    newnode->available_copies = available_copies;
}

treenode* search_id(treenode* root, int book_id) {
    if (root == nullptr || root->book_id == book_id) {
        return root;
    }
    if (root->book_id > book_id) {
        return search_id(root->left, book_id);
    }
    return search_id(root->right, book_id);
}

int case_insensitive_cmp(const char* a, const char* b) {
    while (*a && *b) {
        if (std::tolower(static_cast<unsigned char>(*a)) !=
            std::tolower(static_cast<unsigned char>(*b))) {
            return 0;
        }
        a++;
        b++;
    }
    return *a == *b;
}

int search_string(treenode* root, const char* string) {
    if (root == nullptr) {
        return 0;
    }

    int found_count = 0;
    if (case_insensitive_cmp(root->title, string) ||
        case_insensitive_cmp(root->author, string)) {
        std::printf("\n      Book Id     : %d\n", root->book_id);
        std::printf("      Library     : %s\n", root->lib);
        std::printf("      Title       : %s\n", root->title);
        std::printf("      Author      : %s\n", root->author);
        std::printf(" Available Copies : %d\n", root->available_copies);
        found_count = 1;
    }
    found_count += search_string(root->left, string);
    found_count += search_string(root->right, string);
    return found_count;
}

treenode* findmin(treenode* root) {
    treenode* temp = root;
    while (temp && temp->left != nullptr) {
        temp = temp->left;
    }
    return temp;
}

treenode* deletenode(treenode* root, int book_id) {
    if (root == nullptr) {
        return nullptr;
    }
    if (root->book_id > book_id) {
        root->left = deletenode(root->left, book_id);
    } else if (root->book_id < book_id) {
        root->right = deletenode(root->right, book_id);
    } else {
        if (root->left == nullptr) {
            treenode* temp = root->right;
            std::free(root);
            return temp;
        }
        if (root->right == nullptr) {
            treenode* temp = root->left;
            std::free(root);
            return temp;
        }

        treenode* temp = findmin(root->right);
        root->book_id = temp->book_id;
        std::strncpy(root->lib, temp->lib, sizeof(root->lib) - 1);
        root->lib[sizeof(root->lib) - 1] = '\0';
        std::strncpy(root->title, temp->title, sizeof(root->title) - 1);
        root->title[sizeof(root->title) - 1] = '\0';
        std::strncpy(root->author, temp->author, sizeof(root->author) - 1);
        root->author[sizeof(root->author) - 1] = '\0';
        root->total_copies = temp->total_copies;
        root->available_copies = temp->available_copies;
        root->right = deletenode(root->right, temp->book_id);
    }
    return root;
}

void visit(treenode* root) {
    if (!root) {
        return;
    }
    std::printf("Book Id :%d\n", root->book_id);
    std::printf("Author Name :%s\n", root->author);
    std::printf("Book Title :%s\n", root->title);
    std::printf("Available Copies :%d\n\n", root->available_copies);
}

void visit_lib(treenode* root, const char* lib) {
    if (root == nullptr) {
        return;
    }
    if (case_insensitive_cmp(root->lib, lib)) {
        std::printf("\n      Book Id     : %d\n", root->book_id);
        std::printf("      Library     : %s\n", root->lib);
        std::printf("      Title       : %s\n", root->title);
        std::printf("      Author      : %s\n", root->author);
        std::printf(" Available Copies : %d\n", root->available_copies);
    }
    visit_lib(root->left, lib);
    visit_lib(root->right, lib);
}

void inorder(treenode* root) {
    if (root == nullptr) {
        return;
    }
    inorder(root->left);
    visit(root);
    inorder(root->right);
}

void free_bst(treenode* root) {
    if (!root) {
        return;
    }
    free_bst(root->left);
    free_bst(root->right);
    std::free(root);
}
