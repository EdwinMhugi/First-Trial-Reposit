#include <iostream>
#include <fstream>
#include <string>
#include <regex>
#include <ctime>
#include <chrono>

// ===== Constants =====
const int REQUIRED_TUITION = 1000000; // Example: 1,000,000 UGX

// ===== Validation Functions =====
bool validatePaycode(const std::string &paycode) {
    return std::regex_match(paycode, std::regex("^[0-9]{10}$"));
}

bool validateRegNo(const std::string &regNo) {
    return std::regex_match(regNo, std::regex("^[0-9]{2}/[0-9]+/[0-9]+/(D|W|DJ)/[0-9]{3}$"));
}

// ===== Helper: Get Current Date/Time =====
std::string currentDateTime() {
    time_t now = time(0);
    char buf[80];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
    return std::string(buf);
}

// ===== Determine Card Type =====
std::string determineCard(int totalPaid) {
    double percent = (totalPaid * 100.0) / REQUIRED_TUITION;

    if (percent >= 100.0)
        return "Full Payment (Registration Card)";
    else if (percent >= 60.0)
        return "Meal Card (One Month)";
    else if (percent >= 50.0)
        return "Meal Card (One Week)";
    else
        return "Meal Card (Partial)";
}

// ===== Save Payment Record =====
void savePayment(const std::string &name, const std::string &regNo,
                 const std::string &paycode, int tuition, int totalPaid, const std::string &cardType) {
    std::ofstream file(name + "_" + paycode + ".txt", std::ios::app);
    if (file.is_open()) {
        file << "Name: " << name << "\n";
        file << "RegNo: " << regNo << "\n";
        file << "Paycode: " << paycode << "\n";
        file << "Tuition Paid: " << tuition << "\n";
        file << "Total Paid: " << totalPaid << "\n";
        file << "Remaining Balance: " << (REQUIRED_TUITION - totalPaid) << "\n";
        file << "Card Type: " << cardType << "\n";
        file << "Date/Time: " << currentDateTime() << "\n";
        file << "----------------------------------\n";
        file.close();
        std::cout << "✅ Payment saved successfully.\n";
    } else {
        std::cout << "❌ Error saving payment.\n";
    }
}

// ===== Get Total Tuition Paid =====
int getTotalPaid(const std::string &filename) {
    std::ifstream file(filename);
    if (!file.is_open()) return 0;

    std::string line;
    int totalPaid = 0;

    while (std::getline(file, line)) {
        if (line.rfind("Tuition Paid:", 0) == 0) {
            int amount = std::stoi(line.substr(13));
            totalPaid += amount;
        }
    }
    file.close();
    return totalPaid;
}

// ===== View Payment History with Timing =====
void viewPaymentHistory(const std::string &paycode, const std::string &name) {
    auto start = std::chrono::high_resolution_clock::now(); // start timing

    std::ifstream file(name + "_" + paycode + ".txt");
    if (!file.is_open()) {
        std::cout << "❌ No payment records found for this Paycode.\n";
        return;
    }

    std::cout << "\n--- Payment History ---\n";
    std::string line;
    while (std::getline(file, line)) {
        std::cout << line << "\n";
    }
    file.close();

    auto end = std::chrono::high_resolution_clock::now(); // end timing
    std::chrono::duration<double, std::milli> elapsed = end - start;
    std::cout << "⏱ Retrieval time: " << elapsed.count() << " ms\n";
}

// ===== View Registration Card with Timing =====
void viewRegistrationCard(const std::string &paycode, const std::string &name) {
    std::string filename = name + "_" + paycode + ".txt";
    int totalPaid = getTotalPaid(filename);

    if (totalPaid >= (0.6 * REQUIRED_TUITION)) {
        auto start = std::chrono::high_resolution_clock::now(); // start timing

        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cout << "❌ No record found.\n";
            return;
        }

        std::cout << "\n--- Registration Card ---\n";
        std::string line;
        while (std::getline(file, line)) {
            std::cout << line << "\n";
        }
        file.close();

        auto end = std::chrono::high_resolution_clock::now(); // end timing
        std::chrono::duration<double, std::milli> elapsed = end - start;
        std::cout << "⏱ Retrieval time: " << elapsed.count() << " ms\n";
    } else {
        std::cout << "❌ Not eligible for a Registration Card (must pay at least 60%).\n";
    }
}

// ===== Main Menu =====
int main() {
    int choice;

    do {
        std::cout << "\n===== Tuition Payment System =====\n";
        std::cout << "1. Make Payment\n";
        std::cout << "2. View Payment History\n";
        std::cout << "3. View Registration Card\n";
        std::cout << "4. Exit\n";
        std::cout << "Enter choice: ";
        std::cin >> choice;
        std::cin.ignore();

        switch (choice) {
            case 1: {
                std::string paycode, regNo, name;
                int tuition;

                // --- Paycode ---
                do {
                    std::cout << "Enter Paycode (10 digits): ";
                    std::getline(std::cin, paycode);
                    if (!validatePaycode(paycode)) {
                        std::cout << "❌ Invalid Paycode! Must be 10 digits.\n";
                    }
                } while (!validatePaycode(paycode));

                // --- RegNo ---
                do {
                    std::cout << "Enter Registration Number (e.g., 24/1/370/D/040): ";
                    std::getline(std::cin, regNo);
                    if (!validateRegNo(regNo)) {
                        std::cout << "❌ Invalid format! Use 24/1/370/D/040 (D/W/DJ + 3 digits).\n";
                    }
                } while (!validateRegNo(regNo));

                // --- Tuition ---
                do {
                    std::cout << "Enter Tuition to Pay: ";
                    std::cin >> tuition;

                    if (std::cin.fail()) {
                        std::cin.clear();
                        std::cin.ignore(1000, '\n');
                        tuition = -1;
                        std::cout << "❌ Invalid input! Numbers only.\n";
                    } else if (tuition <= 0) {
                        std::cout << "❌ Tuition must be positive.\n";
                    }
                } while (tuition <= 0);
                std::cin.ignore();

                // --- Name ---
                std::cout << "Enter Name: ";
                std::getline(std::cin, name);

                std::string filename = name + "_" + paycode + ".txt";
                int oldTotal = getTotalPaid(filename);
                int newTotal = oldTotal + tuition;

                if (newTotal > REQUIRED_TUITION) {
                    std::cout << "⚠️ You overpaid by " << (newTotal - REQUIRED_TUITION)
                              << ". This will still be recorded.\n";
                }

                std::string cardType = determineCard(newTotal);

                savePayment(name, regNo, paycode, tuition, newTotal, cardType);
                break;
            }

            case 2: {
                std::string paycode, name;
                std::cout << "Enter Paycode: ";
                std::getline(std::cin, paycode);
                std::cout << "Enter Name: ";
                std::getline(std::cin, name);

                viewPaymentHistory(paycode, name);
                break;
            }

            case 3: {
                std::string paycode, name;
                std::cout << "Enter Paycode: ";
                std::getline(std::cin, paycode);
                std::cout << "Enter Name: ";
                std::getline(std::cin, name);

                viewRegistrationCard(paycode, name);
                break;
            }

            case 4:
                std::cout << "Exiting program...\n";
                break;

            default:
                std::cout << "Invalid choice, try again.\n";
        }
    } while (choice != 4);

    return 0;
}
