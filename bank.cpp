// Project Identifier: 292F24D17A4455C1B5133EDD8C7CEAA0C9570A98
#include <getopt.h>
#include <algorithm>
#include <deque>
#include <fstream>
#include <iostream>
#include <map>
#include <queue>
#include <set>
#include <sstream>
#include <string>
#include <vector>
// #define DEBUG_REG
// #define DEBUG_LOG
// #define DEBUG_OUT
using namespace std;

uint64_t ts2u64(string str);
uint32_t ip2u32(const string& ip);
string u322ip(uint32_t ip);

class ErrorType {
   public:
    string what;
    ErrorType(const string& what = "")
        : what(what) {}
};
class Trans;
class User;
class Bank;

/*=============================== TRANS TYPE ===============================*/

class Trans {
   public:
    User* sender;
    User* receiver;
    uint32_t amount;
    uint64_t execTs;
    char type;
    uint32_t id;
    Trans()
        : sender(nullptr), receiver(nullptr), amount(0), execTs(0), type('\0'), id(0) {}
    Trans(User* sender, User* receiver, uint32_t amount, uint64_t execTs, char type, uint32_t id)
        : sender(sender), receiver(receiver), amount(amount), execTs(execTs), type(type), id(id) {}

    struct LessExecTsFirst {
        inline bool operator()(const Trans* a, const Trans* b) {
            return (a->execTs > b->execTs) || (a->execTs == b->execTs && a->id > b->id);
        }
    };
};

/*=============================== USER TYPE ===============================*/

class User {
   public:
    // the followings should be set in the reg stage
    uint64_t regTime;
    string id;
    uint32_t pin;
    uint32_t balance;
    // the followings should be set in the transaction stage
    set<uint32_t> ip;
    vector<Trans*> incomingTrans;
    vector<Trans*> outgoingTrans;
    User()
        : regTime(0), id(""), pin(0), balance(0){};
    User(uint64_t regTime, const string& id, uint32_t pin, uint32_t balance)
        : regTime(regTime), id(id), pin(pin), balance(balance) {}
    void print() const {
        cout << "\tUser:\t" << id << "\t" << regTime << "\t" << pin << "\t" << balance << "\n";
    }
    void printIp() const {
        cout << "\t" << id << "'s ip: ";
        for (auto temp : ip)
            cout << u322ip(temp)
                 << " ";
        cout << "\n";
    }
};

int main(int argc, char** argv) {
    try {
        ios_base::sync_with_stdio(false);
        cin.tie(0);
        cout.tie(0);
        bool isVerbose = false;
        ifstream regFile;

        {  // command line args
            int optionIdx = 0;
            int option = 0;
            opterr = 0;
            struct option longOpts[] = {{"help", no_argument, nullptr, 'h'},
                                        {"verbose", no_argument, nullptr, 'v'},
                                        {"file", required_argument, nullptr, 'f'}};
            while ((option = getopt_long(argc, argv, "hvf:", longOpts, &optionIdx)) != -1) {
                switch (option) {
                    case 'h':
                        cout << "I'm too lazy to write help. Check out https://eecs281staff.github.io/p3-281bank/\n";
                        return 0;
                    case 'v':
                        isVerbose = true;
                        break;
                    case 'f':
                        regFile = ifstream(string(optarg));
                        break;
                    default:
                        break;
                }
            }
        }

        /*=============================== THE BANK ===============================*/
        deque<User> users;
        deque<Trans> trans;
        unordered_map<string, User*> userid2user;
        priority_queue<Trans*, vector<Trans*>, Trans::LessExecTsFirst> pqExecTs;
        uint32_t transId = 0;

        { /*=============================== READ REG FILE ===============================*/
            string ts, id, pin, balance;
            while (!regFile.eof()) {
                getline(regFile, ts, '|');
                if (ts == "" || ts == "\n")
                    break;
                getline(regFile, id, '|');
                getline(regFile, pin, '|');
                getline(regFile, balance);
                // add user
                users.push_back(User(ts2u64(ts), id, uint32_t(stoul(pin)), uint32_t(stoul(balance))));
                userid2user[id] = &users.back();
            }
#ifdef DEBUG_REG
            for (auto& user : users) {
                user.print();
                userid2user[user.id]->print();
            }
#endif
        }

        { /*=============================== READ OPERATIONS ===============================*/
            string first;
            while (cin >> first) {
                if (islower(first[0])) {
                    if (first[0] == 'l') {
                        /*===== log in user =====*/
                        string id, ip;
                        uint32_t pin;
                        cin >> id >> pin >> ip;
                        auto user = userid2user.find(id);
                        if (user != userid2user.end() && user->second->pin == pin) {
                            user->second->ip.insert(ip2u32(ip));
                            if (isVerbose) {
                                cout << "User " << id << " logged in.\n";
                            }
#ifdef DEBUG_LOG
                            user->second->printIp();
#endif
                        } else if (isVerbose) {
                            cout << "Failed to log in " << id << ".\n";
                        }
                        getline(cin, first);
                    } else if (first[0] == 'o') {
                        /*===== log out user =====*/
                        string id, ip;
                        cin >> id >> ip;
                        auto user = userid2user.find(id);
                        if (user != userid2user.end()) {
                            auto ipIt = user->second->ip.find(ip2u32(ip));
                            if (ipIt != user->second->ip.end()) {
                                user->second->ip.erase(ipIt);
                                if (isVerbose)
                                    cout << "User " << id << " logged out.\n";
#ifdef DEBUG_OUT
                                user->second->printIp();
#endif
                            } else if (isVerbose) {
                                cout << "Failed to log out " << id << ".\n";
                            }
                        } else if (isVerbose) {
                            cout << "Failed to log out " << id << ".\n";
                        }
                        getline(cin, first);
                    } else if (first[0] == 'p') {
                        /*===== place transaction =====*/
                        string ts, ip, sender, receiver, execTs;
                        uint32_t amount;
                        char transType;
                        cin >> ts >> ip >> sender >> receiver >> amount >> execTs >> transType;
                        uint64_t tsu64 = ts2u64(ts), execTsu64 = ts2u64(execTs);
                        auto senderIt = userid2user.find(sender);
                        auto receiverIt = userid2user.find(receiver);
                        if (execTsu64 - tsu64 > 3000000ULL) {
                            cout << "Select a time less than three days in the future.\n";
                        } else if (senderIt == userid2user.end()) {
                            cout << "Sender " << sender << " does not exist.\n";
                        } else if (receiverIt == userid2user.end()) {
                            cout << "Recipient " << receiver << " does not exist.\n";
                        } else {
                            auto senderPtr = senderIt->second;
                            auto receiverPtr = receiverIt->second;
                            uint64_t execTsu64 = ts2u64(execTs);
                            if (execTsu64 < senderPtr->regTime || execTsu64 < receiverPtr->regTime) {
                                cout << "At the time of execution, sender and/or recipient have not registered.\n";
                            } else if (senderPtr->ip.empty()) {
                                cout << "Sender " << sender << " is not logged in.\n";
                            } else if (senderPtr->ip.find(ip2u32(ip)) == senderPtr->ip.end()) {
                                cout << "Fraudulent transaction detected, aborting request.\n";
                            } else {
                                cout << "Transaction placed at " << ts << ": $" << amount << " from " << sender << " to " << receiver << " at " << execTs << "\n";
                                trans.push_back(Trans(senderPtr, receiverPtr, amount, execTsu64, transType, transId++));
                                pqExecTs.push(&trans.back());
                            }
                        }
                        getline(cin, first);
                    }
                } else if (first[0] == '#') {
                    string temp;
                    getline(cin, temp);
                } else if (first[0] == '$') {
                    getline(cin, first);
                    break;
                }
            }
        }
        { /*=============================== READ QUERY LIST ===============================*/
            char qType;
            string arg1, arg2;
            while (cin >> qType) {
                if (qType != 'h') {
                    cin >> arg1 >> arg2;
                    cout << qType << " " << arg1 << " " << arg2 << "\n";
                } else {
                    cin >> arg1;
                    cout << qType << " " << arg1 << "\n";
                }
            }
        }
    } catch (ErrorType& err) {
        cout << err.what << endl;
        return 1;
    } catch (std::runtime_error& e) {
        cerr << e.what() << endl;
        return 1;
    }
    return 0;
}

uint64_t ts2u64(string str) {
    str.erase(remove(str.begin(), str.end(), ':'), str.end());
    return stoull(str);
}

uint32_t ip2u32(const string& ip) {
    uint32_t num = 0, val = 0;
    for (size_t i = 0; i < ip.size(); ++i) {
        if (ip[i] == '.') {
            num = (num << 8) | val;
            val = 0;
        } else {
            val = val * 10 + (ip[i] - '0');
        }
    }
    num = (num << 8) | val;
    return num;
}

string u322ip(uint32_t ip) {
    stringstream ss;
    for (int i = 3; i >= 0; i--) {
        ss << ((ip >> (8 * i)) & 0xFF);
        if (i > 0)
            ss << ".";
    }
    return ss.str();
}

/*
change time stamp to number uint64_t *100ULL

place time will be increasing, but not execution time

getline(regFile,str,'|')

In the case of shared fees, the recipient must have enough money in
their account to pay the fee before they can receive the transfer.

Minimum fee: 10 USD
1% fee of the transaction amount (integer division)
Maximum fee: 450 USD

get a new transaction id (strictly increasing counter)

sender with more than > 5 years from the given execution date of transaction will
deduct 25% on transaction fees
the 25% deduction will be a deduction on the overall transaction fee.

the sender pays the larger share of the fee

If someone sent 1 dollar to someone else, you should say 1 dollar, not 1 dollars.

There was a total of 1 transaction
*/
