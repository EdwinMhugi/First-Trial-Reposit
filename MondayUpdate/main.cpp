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
#include <iomanip>

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
    StudentPayment(const std::string& n, const std::string& reg, const std::string& pay, int t)
        : name(n), regNo(reg), paymentCode(pay), tuition(t) {}

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

    // ===== Safe numeric parsing =====
    static int extractNumber(const std::string &line) {
        std::string digits;
        for (char c : line)
            if (std::isdigit(c)) digits += c;
        return digits.empty() ? 0 : std::stoi(digits);
    }

    // ===== TXT: get total paid =====
   int getTotalFromTXT() const {
    std::string recordFile;
    for (const auto &entry : fs::directory_iterator(".")) {
        std::string fname = entry.path().filename().string();
        if (fname.rfind(paymentCode + "_", 0) == 0 && fname.find(".txt") != std::string::npos) {
            recordFile = fname;
            break;
        }
    }

    if (recordFile.empty()) return 0; // file not found

    std::ifstream file(recordFile);
    if (!file.is_open()) return 0;

    std::string line;
    int totalPaid = 0;

    while (std::getline(file, line)) {
        if (line.rfind("Tuition Paid:",0) != std::string::npos) {
            std::string digits;
            for (char c : line)
                if (isdigit(c)) digits += c;
            if (!digits.empty()) totalPaid += std::stoi(digits); // sum all payments
        }
    }

    file.close();
    return totalPaid;
}


    void savePayment() const {
        // --- Update CSV ---
        int oldTotalCSV = getTotalPaidCSV();
        int newTotalCSV = oldTotalCSV + tuition;
        std::string cardTypeCSV = determineCard(newTotalCSV);

        std::ifstream checkCSV("StudentPayments.csv");
        bool fileExists = checkCSV.good();
        checkCSV.close();

        std::ofstream csv("StudentPayments.csv", std::ios::app);
        if (!fileExists)
            csv << "Paycode,Name,RegNo,TuitionPaid,TotalPaid,RemainingBalance,CardType,DateTime\n";
        csv << paymentCode << "," << name << "," << regNo << "," << tuition << "," 
            << newTotalCSV << "," << (REQUIRED_TUITION - newTotalCSV) << ","
            << cardTypeCSV << "," << currentDateTime() << "\n";
        csv.close();

        // --- Update TXT ---
        std::string filename = paymentCode + "_" + name + ".txt";
        int oldTotalTXT = getTotalFromTXT();
        int newTotalTXT = oldTotalTXT + tuition;
        std::string cardTypeTXT = determineCard(newTotalTXT);

        std::ofstream txt(filename, std::ios::app);
        txt << "Name: " << name << "\n";
        txt << "RegNo: " << regNo << "\n";
        txt << "Paycode: " << paymentCode << "\n";
        txt << "Tuition Paid: " << tuition << "\n";
        txt << "Total Tuition Paid: " << newTotalTXT << "\n";
        txt << "Remaining Balance: " << (REQUIRED_TUITION - newTotalTXT) << "\n";
        txt << "Meal Card Eligibility: " << cardTypeTXT << "\n";
        txt << "Date/Time: " << currentDateTime() << "\n";
        txt << "----------------------------------\n";
        txt.close();

        std::cout << "✅ Payment saved successfully.\n";
    }

    int getTotalPaidCSV() const {
        std::ifstream file("StudentPayments.csv");
        if (!file.is_open()) return 0;
        std::string line;
        int totalPaid = 0;
        while (std::getline(file, line)) {
            std::stringstream ss(line);
            std::string token;
            std::getline(ss, token, ','); // paycode
            if (token == paymentCode) {
                for (int i = 0; i < 4; ++i) std::getline(ss, token, ','); // skip to TotalPaid
                totalPaid = std::stoi(token);
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

// ===== CSV Search =====
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
                std::string filename = paycode + "_";
                std::string recordFile;
                for (const auto &entry : fs::directory_iterator(".")) {
                    std::string fname = entry.path().filename().string();
                    if (fname.rfind(paycode + "_", 0) == 0 && fname.find(".txt") != std::string::npos) {
                        recordFile = fname;
                        break;
                    }
                }
                if (recordFile.empty()) { std::cout << "❌ No payment records found.\n"; break; }
                std::ifstream file(recordFile);
                std::string line;
                std::cout << "\n--- Payment History ---\n";
                while (std::getline(file, line)) std::cout << line << "\n";
                file.close();
                break;
            }

            case 3: {
                std::string paycode;
                std::cout << "Enter Paycode: ";
                std::getline(std::cin, paycode);
                std::string recordFile;
                for (const auto &entry : fs::directory_iterator(".")) {
                    std::string fname = entry.path().filename().string();
                    if (fname.rfind(paycode + "_", 0) == 0 && fname.find(".txt") != std::string::npos) {
                        recordFile = fname;
                        break;
                    }
                }
                if (recordFile.empty()) { std::cout << "❌ No record found.\n"; break; }
                StudentPayment sp("", "", paycode, 0);
                int totalPaid = sp.getTotalFromTXT();
                int balance = 4400000 - totalPaid;
                double percent = (totalPaid * 100.0) / 4400000;
                std::string cardType = sp.determineCard(totalPaid);

                std::ifstream file(recordFile);
                std::string name, regNo, line;
                std::getline(file, line); name = line.substr(6); // Name: 
                std::getline(file, line); regNo = line.substr(7); // RegNo: 
                std::cout << "\n--- Registration Card ---\n";
                std::cout << "Name: " << name << "\n";
                std::cout << "RegNo: " << regNo << "\n";
                std::cout << "Paycode: " << paycode << "\n";
                std::cout << "Total Tuition Paid: " << totalPaid << "\n";
                std::cout << "Remaining Balance: " << balance << "\n";
                std::cout << "Percentage Paid: " << std::fixed << std::setprecision(2) << percent << "%\n";
                std::cout << "Meal Card Eligibility: " << cardType << "\n";
                file.close();
                break;
            }

            case 4: std::cout << "Returning to Access Point Menu...\n"; break;
            default: std::cout << "❌ Invalid choice.\n";
        }
    } while (choice != 4);
}

// ===== Admin Functions =====
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
// ===== Helper: format a CSV line =====
void printFormattedRecord(const std::string &line) {
    std::stringstream ss(line);
    std::string paycode, name, regNo, tuitionPaid, totalPaid, remaining, rest;

    std::getline(ss, paycode, ',');
    std::getline(ss, name, ',');
    std::getline(ss, regNo, ',');
    std::getline(ss, tuitionPaid, ',');
    std::getline(ss, totalPaid, ',');
    std::getline(ss, remaining, ',');
    std::getline(ss, rest);

    // Split CardType and DateTime (last comma in rest)
    size_t lastComma = rest.rfind(',');
    std::string cardType = rest.substr(0, lastComma);
    std::string dateTime = rest.substr(lastComma + 1);

    std::cout << "Name: " << name
              << " | RegNo: " << regNo
              << " | Paycode: " << paycode
              << " | Tuition Paid: " << tuitionPaid
              << " | Total Paid: " << totalPaid
              << " | Remaining: " << remaining
              << " | Card Type: " << cardType
              << " | Date: " << dateTime << "\n";
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
                std::string paycode;
                std::cout << "Enter Paycode: ";
                std::getline(std::cin, paycode);

                std::ifstream file("StudentPayments.csv");
                if (!file.is_open()) { std::cout << "❌ No records found.\n"; break; }

                std::string line;
                bool found = false;
                while (std::getline(file, line)) {
                    if (line.rfind(paycode + ",", 0) == 0) {
                        printFormattedRecord(line);
                        found = true;
                        break;
                    }
                }
                if (!found) std::cout << "❌ No records found.\n";
                file.close();
                break;
            }

            case 2: {
                std::ifstream file("StudentPayments.csv");
                if (!file.is_open()) { std::cout << "❌ No records found.\n"; break; }

                std::string line;
                std::getline(file, line); // skip header
                while (std::getline(file, line)) {
                    printFormattedRecord(line);
                }
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
        std::cout << "1. Student/Parent\n2. Admin\n3. Exit\n";
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
