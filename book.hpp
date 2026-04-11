#ifndef BOOK_HPP
#define BOOK_HPP

struct treenode {
    int book_id;
    char lib[128];
    char title[256];
    char author[256];
    int total_copies;
    int available_copies;
    treenode* left;
    treenode* right;
};

treenode* createnode(int book_id,
                     const char* lib,
                     const char* title,
                     const char* author,
                     int total_copies);
treenode* insert(treenode* root, treenode* node);
treenode* search_id(treenode* root, int book_id);
void edit(treenode* node,
          const char* author,
          const char* title,
          int available_copies,
          int total_copies);
treenode* deletenode(treenode* root, int book_id);
void free_bst(treenode* root);

#endif
