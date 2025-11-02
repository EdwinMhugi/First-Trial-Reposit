#include <iostream>
#include <fstream>
#include <string>
#include <regex>
#include <ctime>
#include <filesystem>
#include <conio.h>
#include <limits>
#include <vector>
#include <sstream>

namespace fs = std::filesystem;

// ===== Structure =====
struct Student {
    std::string name;
    std::string regNo;
    std::string paycode;
    int tuition;
};

// ===== Class =====
class StudentPayment {
private:
    std::string name;
    std::string regNo;
    std::string paymentCode;
    int tuition;
    const int REQUIRED_TUITION = 4400000;

public:
    StudentPayment(const std::string& n, const std::string& reg, const std::string& pay, int t) {
        name = n;
        regNo = reg;
        paymentCode = pay;
        tuition = t;
    }

    std::string currentDateTime() const {
        time_t now = time(0);
        char buf[80];
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
        return std::string(buf);
    }

    static bool validatePaycode(const std::string &paycode) {
        return std::regex_match(paycode, std::regex("^[0-9]{10}$"));
    }

    static bool validateRegNo(const std::string &regNo) {
        return std::regex_match(regNo, std::regex("^[0-9]{2}/[0-9]+/[0-9]+/(D|W|DJ)/[0-9]{3}$"));
    }

    std::string determineCard(int totalPaid) const {
        double percent = (totalPaid * 100.0) / REQUIRED_TUITION;
        if (percent >= 100.0) return "Full Payment, FULL PAY MEAL CARD, ELIGIBLE FOR AN EXAM NUMBER";
        else if (percent >= 60.0) return "Meal Card (One Month)";
        else return "Tuition Average below 60%, Not Eligible for A Meal Card";
    }

    int getTotalPaid() const {
        std::ifstream file("StudentPayments.csv");
        if (!file.is_open()) return 0;

        std::string line;
        int totalPaid = 0;
        while (std::getline(file, line)) {
            std::stringstream ss(line);
            std::string paycode, name, regNo, tuitionStr, totalStr;
            std::getline(ss, paycode, ',');
            if (paycode == paymentCode) {
                // tuition paid is 4th column
                for (int i = 0; i < 3; ++i) std::getline(ss, name, ',');
                std::getline(ss, tuitionStr, ',');
                totalPaid += std::stoi(tuitionStr);
            }
        }
        file.close();
        return totalPaid;
    }

    void savePayment() const {
        int oldTotal = getTotalPaid();
        int newTotal = oldTotal + tuition;
        std::string cardType = determineCard(newTotal);

        std::ifstream checkFile("StudentPayments.csv");
        bool fileExists = checkFile.good();
        checkFile.close();

        std::ofstream file("StudentPayments.csv", std::ios::app);
        if (!fileExists) {
            file << "Paycode,Name,RegNo,TuitionPaid,TotalPaid,RemainingBalance,CardType,DateTime\n";
        }

        file << paymentCode << ","
             << name << ","
             << regNo << ","
             << tuition << ","
             << newTotal << ","
             << (REQUIRED_TUITION - newTotal) << ","
             << cardType << ","
             << currentDateTime() << "\n";

        file.close();
        std::cout << "✅ Payment saved successfully to CSV.\n";
    }
};

// ===== Password Input =====
std::string inputPassword() {
    std::string password;
    char ch;
    std::cout << "Enter Admin Password: ";
    while ((ch = _getch()) != '\r') {
        if (ch == '\b' && !password.empty()) {
            password.pop_back();
            std::cout << "\b \b";
        } else if (ch != '\b') {
            password.push_back(ch);
            std::cout << '*';
        }
    }
    std::cout << std::endl;
    return password;
}

// ===== Helper =====
bool findRecordByPaycode(const std::string &paycode, std::string &record) {
    std::ifstream file("StudentPayments.csv");
    if (!file.is_open()) return false;

    std::string line;
    while (std::getline(file, line)) {
        if (line.rfind(paycode + ",", 0) == 0) {
            record = line;
            file.close();
            return true;
        }
    }
    file.close();
    return false;
}

// ===== Student Menu =====
void studentMenu() {
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    int choice;
    do {
        std::cout << "\n===== Student/Parent Menu =====\n";
        std::cout << "1. Make Payment\n";
        std::cout << "2. View Payment History\n";
        std::cout << "3. View Registration Card\n";
        std::cout << "4. Back\n";
        std::cout << "Enter choice: ";
        std::cin >> choice;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        switch (choice) {
            case 1: {
                Student student;
                do {
                    std::cout << "Enter Paycode (10 digits): ";
                    std::getline(std::cin, student.paycode);
                    if (!StudentPayment::validatePaycode(student.paycode))
                        std::cout << "❌ Invalid Paycode!\n";
                } while (!StudentPayment::validatePaycode(student.paycode));

                std::string existingRecord;
                if (findRecordByPaycode(student.paycode, existingRecord)) {
                    std::stringstream ss(existingRecord);
                    std::string token;
                    std::getline(ss, token, ','); // paycode
                    std::getline(ss, student.name, ',');
                    std::getline(ss, student.regNo, ',');
                    std::cout << "✅ Record found for student: " << student.name << "\n";
                } else {
                    std::cout << "Enter Student Name: ";
                    std::getline(std::cin, student.name);

                    do {
                        std::cout << "Enter Registration Number (e.g., 24/1/370/D/040): ";
                        std::getline(std::cin, student.regNo);
                        if (!StudentPayment::validateRegNo(student.regNo))
                            std::cout << "❌ Invalid format!\n";
                    } while (!StudentPayment::validateRegNo(student.regNo));
                }

                do {
                    std::cout << "Enter Tuition to Pay (UGX): ";
                    std::cin >> student.tuition;
                    if (std::cin.fail() || student.tuition <= 0) {
                        std::cin.clear();
                        std::cin.ignore(1000, '\n');
                        student.tuition = -1;
                        std::cout << "❌ Tuition must be positive numbers only.\n";
                    }
                } while (student.tuition <= 0);
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

                StudentPayment sp(student.name, student.regNo, student.paycode, student.tuition);
                sp.savePayment();
                break;
            }

            case 2: {
                std::string paycode, record;
                std::cout << "Enter Paycode: ";
                std::getline(std::cin, paycode);
                if (findRecordByPaycode(paycode, record))
                    std::cout << "\n--- Payment Record ---\n" << record << "\n";
                else
                    std::cout << "❌ No payment records found.\n";
                break;
            }

            case 3: {
                std::string paycode, record;
                std::cout << "Enter Paycode: ";
                std::getline(std::cin, paycode);
                if (findRecordByPaycode(paycode, record)) {
                    std::stringstream ss(record);
                    std::string token;
                    int totalPaid = 0;
                    for (int i = 0; i < 4; ++i) std::getline(ss, token, ',');
                    totalPaid = std::stoi(token);
                    if (totalPaid < 0.6 * 4400000)
                        std::cout << "❌ Not eligible for a Registration Card.\n";
                    else
                        std::cout << "\n--- Registration Card ---\n" << record << "\n";
                } else std::cout << "❌ No record found.\n";
                break;
            }

            case 4: std::cout << "Returning to Access Point Menu...\n"; break;
            default: std::cout << "❌ Invalid choice.\n";
        }
    } while (choice != 4);
}

// ===== Update/Delete Admin =====
void updateStudentRecordByPaycode() {
    std::string paycode;
    std::cout << "Enter Paycode to update: ";
    std::getline(std::cin, paycode);

    std::ifstream file("StudentPayments.csv");
    if (!file.is_open()) { std::cout << "❌ No records found.\n"; return; }

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(file, line))
        lines.push_back(line);
    file.close();

    bool found = false;
    for (auto &l : lines) {
        if (l.rfind(paycode + ",", 0) == 0) {
            found = true;
            std::cout << "Enter updated Name: ";
            std::string newName;
            std::getline(std::cin, newName);

            std::stringstream ss(l);
            std::string tokens[8];
            for (int i = 0; i < 8; ++i) std::getline(ss, tokens[i], ',');
            tokens[1] = newName; // update name

            l = tokens[0];
            for (int i = 1; i < 8; ++i) l += "," + tokens[i];
            break;
        }
    }

    if (!found) { std::cout << "❌ Record not found.\n"; return; }

    std::ofstream out("StudentPayments.csv");
    for (const auto &l : lines)
        out << l << "\n";
    out.close();

    std::cout << "✅ Record updated successfully.\n";
}

void deleteStudentRecordByPaycode() {
    std::string paycode;
    std::cout << "Enter Paycode to delete: ";
    std::getline(std::cin, paycode);

    std::ifstream file("StudentPayments.csv");
    if (!file.is_open()) { std::cout << "❌ No records found.\n"; return; }

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(file, line))
        lines.push_back(line);
    file.close();

    std::ofstream out("StudentPayments.csv");
    bool deleted = false;
    for (const auto &l : lines) {
        if (l.rfind(paycode + ",", 0) != 0)
            out << l << "\n";
        else deleted = true;
    }
    out.close();

    if (deleted) std::cout << "✅ Record deleted successfully.\n";
    else std::cout << "❌ Record not found.\n";
}

// ===== Admin Menu =====
void adminMenu() {
    const std::string ADMIN_PASSWORD = "admin123";
    std::string enteredPassword = inputPassword();
    if (enteredPassword != ADMIN_PASSWORD) {
        std::cout << "❌ Wrong password.\n";
        return;
    }

    int choice;
    do {
        std::cout << "\n===== Admin Menu =====\n";
        std::cout << "1. Search Student Payments\n";
        std::cout << "2. View All Records\n";
        std::cout << "3. Update Student Record\n";
        std::cout << "4. Delete Student Record\n";
        std::cout << "5. Back\n";
        std::cout << "Enter choice: ";
        std::cin >> choice;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        switch (choice) {
            case 1: {
                std::string paycode, record;
                std::cout << "Enter Paycode: ";
                std::getline(std::cin, paycode);
                if (findRecordByPaycode(paycode, record))
                    std::cout << "\n" << record << "\n";
                else std::cout << "❌ No records found.\n";
                break;
            }

            case 2: {
                std::ifstream file("StudentPayments.csv");
                if (!file.is_open()) { std::cout << "❌ No records found.\n"; break; }
                std::string line;
                std::cout << "\n--- All Records ---\n";
                while (std::getline(file, line))
                    std::cout << line << "\n";
                file.close();
                break;
            }

            case 3: updateStudentRecordByPaycode(); break;
            case 4: deleteStudentRecordByPaycode(); break;
            case 5: std::cout << "Returning to main menu...\n"; break;
            default: std::cout << "❌ Invalid choice.\n";
        }
    } while (choice != 5);
}

// ===== Main =====
int main() {
    int mainChoice;
    do {
        std::cout << "\n===== Tuition Payment System =====\n";
        std::cout << "1. Student/Parent\n";
        std::cout << "2. Admin\n";
        std::cout << "3. Exit\n";
        std::cout << "Enter choice: ";
        std::cin >> mainChoice;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        switch (mainChoice) {
            case 1: studentMenu(); break;
            case 2: adminMenu(); break;
            case 3: std::cout << "Exiting program...\n"; break;
            default: std::cout << "❌ Invalid choice.\n";
        }
    } while (mainChoice != 3);

    return 0;
}
