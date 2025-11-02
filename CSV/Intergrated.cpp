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

    void savePayment() const {
        int oldTotal = getTotalPaid();
        int newTotal = oldTotal + tuition;
        std::string cardType = determineCard(newTotal);

        if (newTotal > REQUIRED_TUITION) {
            std::cout << "⚠️ Overpayment: " << (newTotal - REQUIRED_TUITION) 
                      << " will still be recorded.\n";
        }

        // Save to TXT
        std::ofstream file(paymentCode + "_" + name + ".txt", std::ios::app);
        if (file.is_open()) {
            file << "Name: " << name << "\n";
            file << "RegNo: " << regNo << "\n";
            file << "Paycode: " << paymentCode << "\n";
            file << "Tuition Paid: " << tuition << "\n";
            file << "Total Paid: " << newTotal << "\n";
            file << "Remaining Balance: " << (REQUIRED_TUITION - newTotal) << "\n";
            file << "Card Type: " << cardType << "\n";
            file << "Date/Time: " << currentDateTime() << "\n";
            file << "----------------------------------\n";
            file.close();
        } else {
            std::cout << "❌ Error saving payment.\n";
        }

        // Save to CSV (Admin use)
        std::ofstream csvFile("students.csv", std::ios::app);
        if (csvFile.is_open()) {
            csvFile << paymentCode << "," << name << "," << regNo << "," << tuition << "," 
                    << newTotal << "," << (REQUIRED_TUITION - newTotal) << "," 
                    << cardType << "," << currentDateTime() << "\n";
            csvFile.close();
        }
        std::cout << "✅ Payment saved successfully.\n";
    }

    int getTotalPaid() const {
        std::ifstream file(paymentCode + "_" + name + ".txt");
        if (!file.is_open()) return 0;

        std::string line;
        int totalPaid = 0;
        while (std::getline(file, line)) {
            if (line.rfind("Tuition Paid:", 0) == 0) {
                totalPaid += std::stoi(line.substr(13));
            }
        }
        file.close();
        return totalPaid;
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

// ===== Find File by Paycode =====
std::string findFileByPaycode(const std::string &paycode) {
    for (const auto &entry : fs::directory_iterator(".")) {
        std::string fname = entry.path().filename().string();
        if (fname.rfind(paycode + "_", 0) == 0 && fname.find(".txt") != std::string::npos) {
            return fname;
        }
    }
    return "";
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

                std::string fname = findFileByPaycode(student.paycode);
                if (!fname.empty()) {
                    student.name = fname.substr(fname.find("_") + 1);
                    student.name = student.name.substr(0, student.name.find(".txt"));
                    std::cout << "✅ Record found for student: " << student.name << "\n";
                    student.regNo = "N/A";
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
                    std::cout << "Enter Tuition to Pay: ";
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
                std::string paycode;
                std::cout << "Enter Paycode: ";
                std::getline(std::cin, paycode);
                std::string fname = findFileByPaycode(paycode);
                if (fname.empty()) std::cout << "❌ No payment records found.\n";
                else {
                    std::ifstream file(fname);
                    std::string line;
                    std::cout << "\n--- Payment History ---\n";
                    while (std::getline(file, line)) std::cout << line << "\n";
                    file.close();
                }
                break;
            }

            case 3: {
                std::string paycode;
                std::cout << "Enter Paycode: ";
                std::getline(std::cin, paycode);
                std::string fname = findFileByPaycode(paycode);
                if (fname.empty()) std::cout << "❌ No record found.\n";
                else {
                    std::ifstream file(fname);
                    std::string line;
                    int totalPaid = 0;
                    while (std::getline(file, line)) {
                        if (line.rfind("Tuition Paid:", 0) == 0)
                            totalPaid += std::stoi(line.substr(13));
                    }
                    file.close();
                    if (totalPaid < 0.6 * 4400000) {
                        std::cout << "❌ Not eligible for a Registration Card.\n";
                    } else {
                        std::ifstream file2(fname);
                        std::cout << "\n--- Registration Card ---\n";
                        while (std::getline(file2, line)) std::cout << line << "\n";
                        file2.close();
                    }
                }
                break;
            }

            case 4: std::cout << "Returning to Access Point Menu...\n"; break;
            default: std::cout << "❌ Invalid choice.\n";
        }
    } while (choice != 4);
}

// ===== Admin Functions (CSV) =====
void updateStudentCSVByPaycode() {
    std::string paycode;
    std::cout << "Enter Paycode of student to update: ";
    std::getline(std::cin, paycode);

    std::ifstream infile("students.csv");
    if (!infile.is_open()) {
        std::cout << "❌ CSV file not found.\n";
        return;
    }

    std::vector<std::string> lines;
    std::string line;
    bool found = false;

    while (std::getline(infile, line)) {
        std::stringstream ss(line);
        std::string token;
        std::getline(ss, token, ',');
        if (token == paycode) found = true;
        lines.push_back(line);
    }
    infile.close();

    if (!found) {
        std::cout << "❌ No record found in CSV.\n";
        return;
    }

    std::cout << "Which field do you want to update?\n";
    std::cout << "1. Name\n2. RegNo\n3. Paycode\n4. Tuition Paid\n";
    int choice;
    std::cin >> choice;
    std::cin.ignore();
    std::string newValue;
    std::cout << "Enter new value: ";
    std::getline(std::cin, newValue);

    for (auto &l : lines) {
        std::stringstream ss(l);
        std::string pay, name, reg, tuition, total, rem, card, dt;
        std::getline(ss, pay, ','); std::getline(ss, name, ','); std::getline(ss, reg, ',');
        std::getline(ss, tuition, ','); std::getline(ss, total, ','); std::getline(ss, rem, ',');
        std::getline(ss, card, ','); std::getline(ss, dt, ',');

        if (pay == paycode) {
            switch (choice) {
                case 1: name = newValue; break;
                case 2: reg = newValue; break;
                case 3: pay = newValue; break;
                case 4: tuition = newValue; break;
                default: std::cout << "Invalid option.\n"; return;
            }
            l = pay + "," + name + "," + reg + "," + tuition + "," + total + "," + rem + "," + card + "," + dt;
            break;
        }
    }

    std::ofstream outfile("students.csv");
    for (const auto &l : lines) outfile << l << "\n";
    outfile.close();

    std::cout << "✅ CSV Record updated successfully.\n";
}

void deleteStudentCSVByPaycode() {
    std::string paycode;
    std::cout << "Enter Paycode of student to delete: ";
    std::getline(std::cin, paycode);

    std::ifstream infile("students.csv");
    if (!infile.is_open()) {
        std::cout << "❌ CSV file not found.\n";
        return;
    }

    std::vector<std::string> lines;
    std::string line;
    bool found = false;
    while (std::getline(infile, line)) {
        std::stringstream ss(line);
        std::string token;
        std::getline(ss, token, ',');
        if (token == paycode) { found = true; continue; }
        lines.push_back(line);
    }
    infile.close();

    if (!found) { std::cout << "❌ No record found in CSV.\n"; return; }

    std::ofstream outfile("students.csv");
    for (const auto &l : lines) outfile << l << "\n";
    outfile.close();

    // Also remove TXT
    std::string txtFile = findFileByPaycode(paycode);
    if (!txtFile.empty()) fs::remove(txtFile);

    std::cout << "✅ Record deleted successfully from CSV and TXT.\n";
}

// ===== Admin Menu =====
void adminMenu() {
    const std::string ADMIN_PASSWORD = "admin123";
    std::string enteredPassword = inputPassword();
    if (enteredPassword != ADMIN_PASSWORD) {
        std::cout << "❌ Wrong password. Returning to main menu.\n";
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
                std::string paycode;
                std::cout << "Enter Paycode: ";
                std::getline(std::cin, paycode);
                std::ifstream file("students.csv");
                std::string line;
                bool found = false;
                while (std::getline(file, line)) {
                    std::stringstream ss(line);
                    std::string token;
                    std::getline(ss, token, ',');
                    if (token == paycode) { found = true; break; }
                }
                file.close();
                if (!found) std::cout << "❌ No records found.\n";
                else {
                    std::ifstream file("students.csv");
                    std::cout << "\n--- Student Payments (CSV) ---\n";
                    while (std::getline(file, line)) {
                        if (line.rfind(paycode, 0) == 0) std::cout << line << "\n";
                    }
                    file.close();
                }
                break;
            }

            case 2: {
                std::string line;
                std::ifstream file("students.csv");
                if (!file.is_open()) { std::cout << "❌ No CSV records found.\n"; break; }
                std::cout << "\n--- All Student Records (CSV) ---\n";
                while (std::getline(file,line)) std::cout << line << "\n";
                file.close();
                break;
            }

            case 3: updateStudentCSVByPaycode(); break;
            case 4: deleteStudentCSVByPaycode(); break;
            case 5: std::cout << "Returning to Access Point Menu...\n"; break;
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
