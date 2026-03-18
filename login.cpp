#include "login.h"

#include <sstream>

namespace {
const string admin_login_key = "admin@123";
}

void person::inputbasic() {
    cout << "first name: " << flush;
    getline(cin, fname);

    cout << "last name: " << flush;
    getline(cin, lname);

    while (true) {
        cout << "email id: " << flush;
        getline(cin, email);

        size_t at_position = email.find('@');
        size_t dot_position = string::npos;
        if (at_position != string::npos) {
            dot_position = email.find('.', at_position);
        }

        if (at_position != string::npos &&
            dot_position != string::npos &&
            at_position < dot_position) {
            break;
        }

        cout << "invalid email format, try again\n";
    }
}

string person::getfname() const {
    return fname;
}

string person::getlname() const {
    return lname;
}

string person::getemail() const {
    return email;
}

string person::getcode() const {
    return code;
}

void person::setfname(const string& first_name) {
    fname = first_name;
}

void person::setlname(const string& last_name) {
    lname = last_name;
}

void person::setemail(const string& email_id) {
    email = email_id;
}

void person::setcode(const string& role_code) {
    code = role_code;
}

void person::setdetails(const string& first_name, const string& last_name, const string& email_id, const string& role_code) {
    fname = first_name;
    lname = last_name;
    email = email_id;
    code = role_code;
}

void person::display() const {
    cout << fname << " " << lname << " | " << email << " | " << code << "\n";
}

string authsystem::xorcrypt(const string& data) {
    const string key = "libraryxorkey";
    string encrypted_text(data.size(), '\0');

    for (size_t i = 0; i < data.size(); ++i) {
        encrypted_text[i] = data[i] ^ key[i % key.size()];
    }

    return encrypted_text;
}

string authsystem::getpassmasked(const string& prompt) {
    cout << prompt << flush;
    string pass;
    getline(cin, pass);
    return pass;
}

string authsystem::readvalidpassword() {
    string pass;

    while (true) {
        pass = getpassmasked("choose password (min 8): ");
        if (pass.size() < 8) {
            cout << "password too short\n";
            continue;
        }

        string confirm_pass = getpassmasked("retype password: ");
        if (pass != confirm_pass) {
            cout << "passwords do not match\n";
            continue;
        }

        break;
    }

    return pass;
}

bool authsystem::checkadminkey() {
    string entered_admin_key = getpassmasked("admin key: ");
    if (entered_admin_key != admin_login_key) {
        cout << "invalid admin key\n";
        return false;
    }

    return true;
}

void authsystem::loadlist(vector<person>& people, const string& filename) {
    people.clear();

    ifstream input_file(filename);
    if (!input_file.is_open()) {
        return;
    }

    string line;
    while (getline(input_file, line)) {
        if (line.empty()) {
            continue;
        }

        stringstream line_stream(line);
        string first_name;
        string last_name;
        string email_id;
        string role_code;

        if (!getline(line_stream, first_name, ',')) {
            continue;
        }
        if (!getline(line_stream, last_name, ',')) {
            continue;
        }
        if (!getline(line_stream, email_id, ',')) {
            continue;
        }
        if (!getline(line_stream, role_code)) {
            continue;
        }

        person current_person;
        current_person.setdetails(first_name, last_name, email_id, role_code);
        people.push_back(current_person);
    }
}

void authsystem::savelist(const vector<person>& people, const string& filename) {
    ofstream output_file(filename, ios::trunc);
    if (!output_file.is_open()) {
        return;
    }

    for (const person& current_person : people) {
        output_file << current_person.getfname() << ','
                    << current_person.getlname() << ','
                    << current_person.getemail() << ','
                    << current_person.getcode() << '\n';
    }
}

int authsystem::bsearchbyemail(vector<person>& people, const string& email) {
    sort(people.begin(), people.end(),
         [](const person& first_person, const person& second_person) {
             return first_person.getemail() < second_person.getemail();
         });

    int left = 0;
    int right = static_cast<int>(people.size()) - 1;

    while (left <= right) {
        int middle = left + (right - left) / 2;

        if (people[middle].getemail() == email) {
            return middle;
        }

        if (people[middle].getemail() < email) {
            left = middle + 1;
        } else {
            right = middle - 1;
        }
    }

    return -1;
}

void authsystem::storepassword(const string& role, const string& email, const string& pass) {
    string encrypted_password = xorcrypt(pass);
    string role_email_key = role + "|" + email;

    ofstream password_file("rolepass.bin", ios::binary | ios::app);
    if (!password_file.is_open()) {
        return;
    }

    uint32_t key_length = static_cast<uint32_t>(role_email_key.size());
    uint32_t password_length = static_cast<uint32_t>(encrypted_password.size());

    password_file.write(reinterpret_cast<const char*>(&key_length), sizeof(key_length));
    password_file.write(role_email_key.data(), key_length);
    password_file.write(reinterpret_cast<const char*>(&password_length), sizeof(password_length));
    password_file.write(encrypted_password.data(), password_length);
}

bool authsystem::checkpassword(const string& role, const string& email, const string& pass) {
    string encrypted_input = xorcrypt(pass);
    string role_email_key = role + "|" + email;

    ifstream password_file("rolepass.bin", ios::binary);
    if (!password_file.is_open()) {
        return false;
    }

    while (password_file.peek() != EOF) {
        uint32_t stored_key_length = 0;
        if (!password_file.read(reinterpret_cast<char*>(&stored_key_length), sizeof(stored_key_length))) {
            break;
        }

        string stored_key(stored_key_length, '\0');
        password_file.read(&stored_key[0], stored_key_length);

        uint32_t stored_password_length = 0;
        password_file.read(reinterpret_cast<char*>(&stored_password_length), sizeof(stored_password_length));

        string stored_password(stored_password_length, '\0');
        password_file.read(&stored_password[0], stored_password_length);

        bool same_user = (stored_key == role_email_key);
        bool same_password = (stored_password == encrypted_input);

        if (same_user && same_password) {
            return true;
        }
    }

    return false;
}

void authsystem::createaccount(vector<person>& people,
                               const string& filename,
                               const string& role,
                               const string& code_prefix,
                               const string& success_label,
                               bool require_admin_key) {
    if (require_admin_key && !checkadminkey()) {
        return;
    }

    person new_person;
    new_person.inputbasic();

    if (bsearchbyemail(people, new_person.getemail()) != -1) {
        cout << "email already registered\n";
        return;
    }

    new_person.setcode(code_prefix + to_string(people.size() + 1));

    string pass = readvalidpassword();

    people.push_back(new_person);
    savelist(people, filename);
    storepassword(role, new_person.getemail(), pass);

    cout << success_label << " created with code " << new_person.getcode() << '\n';
}

void authsystem::loginaccount(vector<person>& people,
                              const string& role,
                              const string& missing_message,
                              bool require_admin_key) {
    cout << "email: " << flush;
    string email;
    getline(cin, email);

    if (bsearchbyemail(people, email) == -1) {
        cout << missing_message << '\n';
        return;
    }

    if (require_admin_key) {
        if (!checkadminkey()) {
            return;
        }
    }

    string pass = getpassmasked("password: ");
    if (checkpassword(role, email, pass)) {
        cout << "login success\n";
    } else {
        cout << "invalid credentials\n";
    }
}

void authsystem::adminpanel() {
    loadlist(adminlist, "admin.csv");

    cout << "admin panel (create/login)\n";
    cout << "1 create admin\n2 login\n0 back\nchoose: " << flush;

    string choice;
    getline(cin, choice);

    if (choice == "1") {
        createaccount(adminlist, "admin.csv", "admin", "a", "admin", true);
    } else if (choice == "2") {
        loginaccount(adminlist, "admin", "admin account not found", true);
    } else if (choice == "0") {
        return;
    } else {
        cout << "invalid choice\n";
    }
}

void authsystem::userpanel() {
    loadlist(userlist, "user.csv");

    cout << "user panel (create/login)\n";
    cout << "1 create user\n2 login\n0 back\nchoose: " << flush;

    string choice;
    getline(cin, choice);

    if (choice == "1") {
        createaccount(userlist, "user.csv", "user", "u", "user", false);
    } else if (choice == "2") {
        loginaccount(userlist, "user", "user account not found", false);
    } else if (choice == "0") {
        return;
    } else {
        cout << "invalid choice\n";
    }
}

void authsystem::libpanel() {
    loadlist(liblist, "lib.csv");

    cout << "library manager panel (create/login)\n";
    cout << "1 create lib\n2 login\n0 back\nchoose: " << flush;

    string choice;
    getline(cin, choice);

    if (choice == "1") {
        createaccount(liblist, "lib.csv", "lib", "l", "lib manager", false);
    } else if (choice == "2") {
        loginaccount(liblist, "lib", "library manager account not found", false);
    } else if (choice == "0") {
        return;
    } else {
        cout << "invalid choice\n";
    }
}

void authsystem::menu() {
    while (true) {
        cout << "\n1 admin\n2 user\n3 library\n0 exit\nchoose: " << flush;

        string choice;
        getline(cin, choice);

        if (choice == "1") {
            adminpanel();
        } else if (choice == "2") {
            userpanel();
        } else if (choice == "3") {
            libpanel();
        } else if (choice == "0") {
            break;
        } else {
            cout << "invalid choice\n";
        }
    }
}
