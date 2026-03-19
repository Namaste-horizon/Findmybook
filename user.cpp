#include "user.hpp"

#include <iostream>

namespace {
Book* findBookById(std::vector<Book>& catalog, int bookId) {
    for (Book& book : catalog) {
        if (book.getid() == bookId) {
            return &book;
        }
    }
    return nullptr;
}

void printBookDetails(const Book& book) {
    std::cout << "Book ID: " << book.getid()
              << " | Name: " << book.getbook_name()
              << " | Author: " << book.getauthor_name()
              << " | Available: " << book.getavail_copies()
              << " | Library: " << book.getlib_name() << '\n';
}
}

User::User(const std::string& name,
           const std::string& email,
           const std::string& password)
    : user_id(id++),
      user_name(name),
      user_email(email),
      user_password(password) {}

int User::getuser_id() const {
    return user_id;
}

std::string User::getuser_name() const {
    return user_name;
}

std::string User::getuser_email() const {
    return user_email;
}

std::string User::getuser_password() const {
    return user_password;
}

void User::setuser_name(const std::string& name) {
    user_name = name;
}

void User::setuser_email(const std::string& email) {
    user_email = email;
}

void User::setuser_password(const std::string& password) {
    user_password = password;
}

void User::searchBookByName(const std::vector<Book>& catalog,
                            const std::string& query) const {
    bool found = false;
    std::cout << "\nSearch results for book name \"" << query << "\":\n";

    for (const Book& book : catalog) {
        if (book.getbook_name().find(query) != std::string::npos) {
            found = true;
            printBookDetails(book);
        }
    }

    if (!found) {
        std::cout << "No matching books found.\n";
    }
}

void User::searchBookByAuthor(const std::vector<Book>& catalog,
                              const std::string& author) const {
    bool found = false;
    std::cout << "\nSearch results for author \"" << author << "\":\n";

    for (const Book& book : catalog) {
        if (book.getauthor_name().find(author) != std::string::npos) {
            found = true;
            printBookDetails(book);
        }
    }

    if (!found) {
        std::cout << "No matching books found.\n";
    }
}

bool User::issueBookById(std::vector<Book>& catalog, int bookId) const {
    Book* book = findBookById(catalog, bookId);
    if (book == nullptr) {
        std::cout << "Book not found.\n";
        return false;
    }

    if (!book->issue_copy()) {
        std::cout << "No copies available for issue.\n";
        return false;
    }

    std::cout << "Book issued successfully: " << book->getbook_name() << '\n';
    return true;
}

bool User::returnBookById(std::vector<Book>& catalog, int bookId) const {
    Book* book = findBookById(catalog, bookId);
    if (book == nullptr) {
        std::cout << "Book not found.\n";
        return false;
    }

    if (!book->return_copy()) {
        std::cout << "All copies are already in the library.\n";
        return false;
    }

    std::cout << "Book returned successfully: " << book->getbook_name() << '\n';
    return true;
}

SuperUser::SuperUser(const std::string& name,
                     const std::string& email,
                     const std::string& password,
                     const std::string& lib)
    : User(name, email, password),
      lib_name(lib) {}

Book SuperUser::createBook(int totalCopies,
                           const std::string& bookName,
                           const std::string& authorName) const {
    return Book(totalCopies, bookName, authorName, lib_name);
}

void SuperUser::addBookToCatalog(std::vector<Book>& catalog,
                                 int totalCopies,
                                 const std::string& bookName,
                                 const std::string& authorName) const {
    catalog.push_back(createBook(totalCopies, bookName, authorName));
}

std::string SuperUser::getlib_name() const {
    return lib_name;
}

void SuperUser::setlib_name(const std::string& lib) {
    lib_name = lib;
}
