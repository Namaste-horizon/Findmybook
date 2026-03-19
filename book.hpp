#ifndef BOOK_HPP
#define BOOK_HPP

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>

struct treenode {
    int book_id;
    char title[50];
    char author[50];
    char lib[50];
    int total_copies;
    int available_copies;
    treenode* left;
    treenode* right;
};

void view(treenode* root);

int case_insensitive_cmp(const char* a, const char* b);

treenode* createnode(int book_id,
                     const char* lib,
                     const char* title,
                     const char* author,
                     int total_copies);

treenode* insert(treenode* root, treenode* newnode);

int search_string(treenode* root, const char* string);

treenode* search_id(treenode* root, int book_id);

treenode* deletenode(treenode* root, int book_id);

void edit(treenode* root,
          const char* author,
          const char* title,
          int available_copies,
          int total_copies);

treenode* findmin(treenode* root);

void inorder(treenode* root);

void visit(treenode* node);

void visit_lib(treenode* node, const char* lib);

void free_bst(treenode* root);

#endif
