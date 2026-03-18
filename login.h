#ifndef LOGIN_H
#define LOGIN_H

#include <iostream>
#include <vector>
#include <fstream>
#include <algorithm>
#include <string>
#include <cstdint>

using namespace std;

class person {
protected:
    string fname;
    string lname;
    string email;
    string code;

public:
    person() = default;

    void inputbasic();
    void setdetails(const string& first_name, const string& last_name, const string& email_id, const string& role_code);

    string getfname() const;
    string getlname() const;
    string getemail() const;
    string getcode() const;

    void setfname(const string& first_name);
    void setlname(const string& last_name);
    void setemail(const string& email_id);
    void setcode(const string& role_code);

    void display() const;
};

class authsystem {
private:
    vector<person> adminlist;
    vector<person> userlist;
    vector<person> liblist;

    string getpassmasked(const string& prompt);
    string xorcrypt(const string& data);
    string readvalidpassword();

    int bsearchbyemail(vector<person>& people, const string& email);
    void loadlist(vector<person>& people, const string& filename);
    void savelist(const vector<person>& people, const string& filename);
    void storepassword(const string& role, const string& email, const string& pass);
    bool checkpassword(const string& role, const string& email, const string& pass);
    bool checkadminkey();
    void createaccount(vector<person>& people, const string& filename, const string& role, const string& code_prefix, const string& success_label, bool require_admin_key);
    void loginaccount(vector<person>& people, const string& role, const string& missing_message, bool require_admin_key);

public:
    void adminpanel();
    void userpanel();
    void libpanel();
    void menu();
};

#endif 
