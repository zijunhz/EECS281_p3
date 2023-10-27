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
#define min(a, b) ((a) > (b) ? (b) : (a))
#define max(a, b) ((a) < (b) ? (b) : (a))
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
    struct OnlyLessExecTs {
        bool operator()(const Trans* a, const uint64_t val) const {
            return a->execTs < val;
        }
        bool operator()(const uint64_t val, const Trans* a) const {
            return a->execTs > val;
        }
    };
    struct LessExecTsFirst {
        inline bool operator()(const Trans* a, const Trans* b) const {
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

void executeTransUpto(priority_queue<Trans*, vector<Trans*>, Trans::LessExecTsFirst>& pqExecTs, vector<Trans*>& executedTrans, uint64_t upperBound);

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
        vector<Trans*> executedTrans;
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
                        executeTransUpto(pqExecTs, executedTrans, tsu64);
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
                                cout << "Transaction placed at " << tsu64 << ": $" << amount << " from " << sender << " to " << receiver << " at " << execTsu64 << ".\n";
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
            executeTransUpto(pqExecTs, executedTrans, UINT64_MAX);
        }
        { /*=============================== READ QUERY LIST ===============================*/
            char qType;
            string arg1, arg2;
            while (cin >> qType) {
                if (qType == 'l' || qType == 'r') {
                    cin >> arg1 >> arg2;
                    uint64_t startTime = ts2u64(arg1), endTime = ts2u64(arg2);
                    // cout << qType << " " << arg1 << " " << arg2 << "\n";
                    auto itEnd = upper_bound(executedTrans.begin(), executedTrans.end(), endTime, Trans::OnlyLessExecTs());
                    if (qType == 'l') {
                        /*=============================== LIST TRANSACTIONS ===============================*/
                        uint32_t cnt = 0;
                        for (auto it = lower_bound(executedTrans.begin(), executedTrans.end(), startTime, Trans::OnlyLessExecTs()); it != itEnd; ++it, ++cnt) {
                            cout << (*it)->id << ": " << (*it)->sender->id << " sent " << (*it)->amount << ((*it)->amount == 1 ? " dollar to " : " dollars to ") << (*it)->receiver->id << " at " << (*it)->execTs << ".\n";
                        }
                        cout << "There were " << cnt << " transactions that were placed between time " << startTime << " to " << endTime << ".\n";
                    } else {
                        /*=============================== CALC REVENUE ===============================*/
                        uint32_t rev = 0;
                        for (auto it = lower_bound(executedTrans.begin(), executedTrans.end(), startTime, Trans::OnlyLessExecTs()); it != itEnd; ++it) {
                            auto transBeingExecuted = (*it);
                            uint32_t totalTransFee = min(max(10, transBeingExecuted->amount / 100ULL), 450ULL);
                            if (transBeingExecuted->execTs - transBeingExecuted->sender->regTime > 50000000000ULL)
                                totalTransFee = totalTransFee * 3 / 4;
                            rev += totalTransFee;
                        }
                        uint64_t totalInterval = endTime - startTime;
                        uint64_t year = totalInterval / 10000000000ULL;
                        uint64_t month = (totalInterval % 10000000000ULL) / 100000000ULL;
                        uint64_t day = (totalInterval % 100000000ULL) / 1000000ULL;
                        uint64_t hour = (totalInterval % 1000000ULL) / 10000ULL;
                        uint64_t minute = (totalInterval % 10000ULL) / 100ULL;
                        uint64_t second = (totalInterval % 100ULL);
                        cout << "281Bank has collected " << rev << " dollars in fees over";
                        if (year > 0)
                            cout << " " << year << " year" << (year > 1 ? "s" : "");
                        if (month > 0)
                            cout << " " << month << " month" << (month > 1 ? "s" : "");
                        if (day > 0)
                            cout << " " << day << " day" << (day > 1 ? "s" : "");
                        if (hour > 0)
                            cout << " " << hour << " hour" << (hour > 1 ? "s" : "");
                        if (minute > 0)
                            cout << " " << minute << " minute" << (minute > 1 ? "s" : "");
                        if (second > 0)
                            cout << " " << second << " second" << (second > 1 ? "s" : "");
                        cout << ".\n";
                    }
                } else {
                    cin >> arg1;
                    if (qType == 'h') {
                        /*=============================== CUSTOMER HISTORY ===============================*/
                        auto user = userid2user.find(arg1);
                        if (user == userid2user.end()) {
                            cout << "User " << arg1 << " does not exist.\n";
                        } else {
                            auto userPtr = user->second;
                            cout << "Customer " << arg1 << " account summary:\n"
                                 << "Balance: $" << userPtr->balance << "\n"
                                 << "Total # of transactions: " << userPtr->incomingTrans.size() + userPtr->outgoingTrans.size() << "\n"
                                 << "Incoming " << userPtr->incomingTrans.size() << ":\n";
                            for (auto trans : userPtr->incomingTrans) {
                                cout << trans->id << ": " << trans->sender->id << " sent " << trans->amount << (trans->amount == 1 ? " dollar to " : " dollars to ") << trans->receiver->id << " at " << trans->execTs << ".\n";
                            }
                            cout << "Outgoing " << userPtr->outgoingTrans.size() << ":\n";
                            for (auto trans : userPtr->outgoingTrans) {
                                cout << trans->id << ": " << trans->sender->id << " sent " << trans->amount << (trans->amount == 1 ? " dollar to " : " dollars to ") << trans->receiver->id << " at " << trans->execTs << ".\n";
                            }
                        }
                    } else {
                        /*=============================== SUMMARIZE DAY ===============================*/
                        uint64_t startTime = ts2u64(arg1) / 1000000ULL * 1000000ULL, endTime = startTime + 1000000ULL;
                        uint32_t cnt = 0, rev = 0;
                        cout << "Summary of [" << startTime << ", " << endTime << "):\n";
                        auto itEnd = upper_bound(executedTrans.begin(), executedTrans.end(), endTime, Trans::OnlyLessExecTs());
                        for (auto it = lower_bound(executedTrans.begin(), executedTrans.end(), startTime, Trans::OnlyLessExecTs()); it != itEnd; ++it, ++cnt) {
                            cout << (*it)->id << ": " << (*it)->sender->id << " sent " << (*it)->amount << ((*it)->amount == 1 ? " dollar to " : " dollars to ") << (*it)->receiver->id << " at " << (*it)->execTs << ".\n";
                            auto transBeingExecuted = (*it);
                            uint32_t totalTransFee = min(max(10, transBeingExecuted->amount / 100ULL), 450ULL);
                            if (transBeingExecuted->execTs - transBeingExecuted->sender->regTime > 50000000000ULL)
                                totalTransFee = totalTransFee * 3 / 4;
                            rev += totalTransFee;
                        }
                        cout << "There " << (cnt == 1 ? "was" : "were") << " a total of " << cnt << " transaction" << (cnt == 1 ? "" : "s") << ", 281Bank has collected " << rev << " dollars in fees.\n";
                    }
                    // cout << qType << " " << arg1 << "\n";
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

void executeTransUpto(priority_queue<Trans*, vector<Trans*>, Trans::LessExecTsFirst>& pqExecTs, vector<Trans*>& executedTrans, uint64_t upperBound) {
    while ((!pqExecTs.empty()) && (pqExecTs.top()->execTs < upperBound)) {
        auto transBeingExecuted = pqExecTs.top();
        auto sender = transBeingExecuted->sender;
        auto receiver = transBeingExecuted->receiver;
        uint32_t senderFee = transBeingExecuted->amount, receiverFee = 0;
        uint32_t totalTransFee = min(max(10, transBeingExecuted->amount / 100ULL), 450ULL);
        if (transBeingExecuted->execTs - sender->regTime > 50000000000ULL)
            totalTransFee = totalTransFee * 3 / 4;
        pqExecTs.pop();
        if (transBeingExecuted->type == 's') {
            receiverFee += totalTransFee / 2;
            senderFee += totalTransFee - receiverFee;
        } else {
            senderFee += totalTransFee;
        }
        if (senderFee > sender->balance || receiverFee > receiver->balance) {
            cout << "Insufficient funds to process transaction " << transBeingExecuted->id << ", discarding.\n";
        } else {
            cout << "Transaction executed at " << transBeingExecuted->execTs << ": $" << transBeingExecuted->amount << " from " << sender->id << " to " << receiver->id << ".\n";
            sender->balance -= senderFee;
            receiver->balance -= receiverFee;
            receiver->balance += transBeingExecuted->amount;
            executedTrans.push_back(transBeingExecuted);
            sender->outgoingTrans.push_back(transBeingExecuted);
            receiver->incomingTrans.push_back(transBeingExecuted);
        }
    }
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
