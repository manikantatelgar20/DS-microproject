
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <queue>
#include <list>
#include <stack>
#include <algorithm>
#include <limits>

using namespace std;

enum class Operation { Landing, Takeoff };

struct Aircraft {
    string id;
    Operation operation;
    int fuelPercent = 100;
    int waitingMinutes = 0;
    int normalPriority = 1;
    bool emergency = false;
    bool active = true;
    int priority = 0;
    int arrivalOrder = 0;
};

struct Runway {
    int id;
    bool available = true;
    string aircraftId = "";
};

struct Allocation {
    string aircraftId;
    int runwayId;
    int priority;
};

string operationName(Operation op) {
    return op == Operation::Landing ? "Landing" : "Take-off";
}

int calculatePriority(const Aircraft& a) {
    int score = a.emergency ? 1000 : 0;

    if (a.operation == Operation::Landing && a.fuelPercent <= 10)
        score += 500;
    else if (a.operation == Operation::Landing && a.fuelPercent <= 20)
        score += 200;

    score += a.normalPriority * 10;
    score += min(100, a.waitingMinutes / 5);

    return score;
}

struct PriorityCompare {
    bool operator()(const Aircraft& a, const Aircraft& b) const {
        if (a.priority != b.priority)
            return a.priority < b.priority;

        return a.arrivalOrder > b.arrivalOrder;
    }
};

class AirportScheduler {
private:
    vector<Aircraft> aircraft;
    vector<Runway> runways;
    queue<string> arrivalQueue;
    list<Allocation> history;
    stack<Allocation> allocationStack;

    int nextOrder = 1;

    int findAircraft(const string& id) const {
        for (int i = 0; i < (int)aircraft.size(); i++) {
            if (aircraft[i].id == id)
                return i;
        }
        return -1;
    }

    bool isAllocated(const string& id) const {
        for (const auto& r : runways) {
            if (r.aircraftId == id)
                return true;
        }
        return false;
    }

    int freeRunway() const {
        for (int i = 0; i < (int)runways.size(); i++) {
            if (runways[i].available && runways[i].aircraftId.empty())
                return i;
        }
        return -1;
    }

public:
    AirportScheduler() = default;

    static void clearInput() {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }

    // Add any positive number of runways.
    void addRunways() {
        int count;

        cout << "How many runways do you want to add? ";

        if (!(cin >> count) || count <= 0) {
            clearInput();
            cout << "Enter a positive number.\n";
            return;
        }

        for (int i = 0; i < count; i++) {
            int id = runways.empty() ? 1 : runways.back().id + 1;

            runways.push_back({id, true, ""});

            cout << "Runway " << id << " added.\n";
        }

        // Use newly added runways for waiting aircraft.
        allocateAutomatically();
    }

    // Add aircraft and automatically allocate if possible.
    void addAircraft() {
        Aircraft a{};

        cout << "\nAircraft ID: ";
        cin >> a.id;

        if (findAircraft(a.id) != -1) {
            cout << "Aircraft ID already exists. Use a unique ID.\n";
            return;
        }

        int op, emergency;

        cout << "Operation (1=Landing, 2=Take-off): ";

        if (!(cin >> op) || (op != 1 && op != 2)) {
            clearInput();
            cout << "Invalid operation.\n";
            return;
        }

        a.operation = (op == 1)
            ? Operation::Landing : Operation::Takeoff;

        cout << "Fuel percentage (0-100): ";

        if (!(cin >> a.fuelPercent) ||
            a.fuelPercent < 0 || a.fuelPercent > 100) {
            clearInput();
            cout << "Fuel must be between 0 and 100.\n";
            return;
        }

        cout << "Waiting time in minutes: ";

        if (!(cin >> a.waitingMinutes) || a.waitingMinutes < 0) {
            clearInput();
            cout << "Waiting time cannot be negative.\n";
            return;
        }

        cout << "Normal priority (1-5): ";

        if (!(cin >> a.normalPriority) ||
            a.normalPriority < 1 || a.normalPriority > 5) {
            clearInput();
            cout << "Priority must be between 1 and 5.\n";
            return;
        }

        cout << "Emergency? (1=Yes, 0=No): ";

        if (!(cin >> emergency) ||
            (emergency != 0 && emergency != 1)) {
            clearInput();
            cout << "Enter 1 or 0.\n";
            return;
        }

        a.emergency = emergency == 1;
        a.arrivalOrder = nextOrder++;
        a.priority = calculatePriority(a);

        aircraft.push_back(a);
        arrivalQueue.push(a.id);

        cout << "Aircraft registered. Priority: "
             << a.priority << "\n";

        allocateAutomatically();
    }

    // Priority queue chooses the highest-priority waiting aircraft.
    void allocateAutomatically() {
        priority_queue<Aircraft, vector<Aircraft>, PriorityCompare> waiting;

        for (const auto& a : aircraft) {
            if (a.active && !isAllocated(a.id)) {
                Aircraft candidate = a;
                candidate.priority = calculatePriority(candidate);
                waiting.push(candidate);
            }
        }

        if (waiting.empty()) {
            cout << "No aircraft waiting.\n";
            return;
        }

        int runwayIndex = freeRunway();

        if (runwayIndex == -1) {
            cout << "No available runway. Aircraft remain waiting.\n";
            return;
        }

        // Fill every free runway, always choosing the highest priority.
        while (runwayIndex != -1 && !waiting.empty()) {
            Aircraft chosen = waiting.top();
            waiting.pop();

            int idx = findAircraft(chosen.id);

            if (idx == -1 || !aircraft[idx].active ||
                isAllocated(chosen.id)) {
                runwayIndex = freeRunway();
                continue;
            }

            runways[runwayIndex].aircraftId = chosen.id;
            aircraft[idx].priority = chosen.priority;

            Allocation record{
                chosen.id,
                runways[runwayIndex].id,
                chosen.priority
            };

            history.push_back(record);
            allocationStack.push(record);

            cout << "\n*** AUTOMATIC ALLOCATION ***\n";
            cout << "Aircraft: " << chosen.id << "\n";
            cout << "Operation: " << operationName(chosen.operation) << "\n";
            cout << "Priority: " << chosen.priority << "\n";
            cout << "Runway: " << runways[runwayIndex].id << "\n";

            runwayIndex = freeRunway();
        }
    }

    // Display aircraft, runways, queue order and allocation history.
    void displayStatus() const {
        cout << "\n--- AIRCRAFT STATUS ---\n";

        if (aircraft.empty())
            cout << "No aircraft registered.\n";

        for (const auto& a : aircraft) {
            string status = isAllocated(a.id) ? "Allocated"
                : (a.active ? "Waiting" : "Completed");

            cout << a.id
                 << " | " << operationName(a.operation)
                 << " | Fuel: " << a.fuelPercent << "%"
                 << " | Waiting: " << a.waitingMinutes << " min"
                 << " | Priority: " << a.priority
                 << " | " << (a.emergency ? "Emergency" : "Normal")
                 << " | " << status << "\n";
        }

        cout << "\n--- RUNWAY STATUS ---\n";

        if (runways.empty())
            cout << "No runways added. Use option 1 first.\n";

        for (const auto& r : runways) {
            cout << "Runway " << r.id << ": ";

            if (!r.available)
                cout << "Unavailable";
            else if (!r.aircraftId.empty())
                cout << "Occupied by " << r.aircraftId;
            else
                cout << "Available";

            cout << "\n";
        }

        cout << "\n--- ALLOCATION HISTORY ---\n";

        if (history.empty())
            cout << "No allocations yet.\n";

        for (const auto& h : history) {
            cout << h.aircraftId
                 << " -> Runway " << h.runwayId
                 << " (priority " << h.priority << ")\n";
        }

        queue<string> copy = arrivalQueue;

        cout << "\nAircraft arrival order: ";

        if (copy.empty())
            cout << "None";

        while (!copy.empty()) {
            cout << copy.front() << " ";
            copy.pop();
        }

        cout << "\n";
    }

    // Release aircraft and automatically assign the next waiting aircraft.
    void releaseAircraft() {
        string id;

        cout << "\nAircraft ID to release: ";
        cin >> id;

        for (auto& r : runways) {
            if (r.aircraftId == id) {
                int runwayId = r.id;
                r.aircraftId.clear();

                int idx = findAircraft(id);

                if (idx != -1)
                    aircraft[idx].active = false;

                cout << "Aircraft " << id << " released.\n";
                cout << "Runway " << runwayId << " is now free.\n";

                cout << "Checking waiting aircraft...\n";
                allocateAutomatically();

                return;
            }
        }

        cout << "Aircraft is not currently allocated.\n";
    }

    // Linear search by aircraft ID.
    void searchAircraft() const {
        string id;

        cout << "\nAircraft ID to search: ";
        cin >> id;

        int idx = findAircraft(id);

        if (idx == -1) {
            cout << "Aircraft not found.\n";
            return;
        }

        const auto& a = aircraft[idx];

        cout << "\nAircraft found!\n";
        cout << "ID: " << a.id << "\n";
        cout << "Operation: " << operationName(a.operation) << "\n";
        cout << "Fuel: " << a.fuelPercent << "%\n";
        cout << "Waiting time: " << a.waitingMinutes << " minutes\n";
        cout << "Priority: " << a.priority << "\n";
        cout << "Status: "
             << (isAllocated(a.id) ? "Allocated"
                 : (a.active ? "Waiting" : "Completed")) << "\n";
    }
};

int main() {
    AirportScheduler scheduler;

    while (true) {
        cout << "\n====== AIRPORT RUNWAY SCHEDULER ======\n";
        cout << "1. Add runway(s)\n";
        cout << "2. Add aircraft (automatic allocation)\n";
        cout << "3. Display aircraft and runway status\n";
        cout << "4. Release aircraft (automatic next allocation)\n";
        cout << "5. Search aircraft by ID\n";
        cout << "0. Exit\n";
        cout << "Enter choice: ";

        int choice;

        if (!(cin >> choice)) {
            AirportScheduler::clearInput();
            cout << "Enter a numeric choice.\n";
            continue;
        }

        switch (choice) {
            case 1:
                scheduler.addRunways();
                break;

            case 2:
                scheduler.addAircraft();
                break;

            case 3:
                scheduler.displayStatus();
                break;

            case 4:
                scheduler.releaseAircraft();
                break;

            case 5:
                scheduler.searchAircraft();
                break;

            case 0:
                cout << "Scheduler closed.\n";
                return 0;

            default:
                cout << "Invalid choice.\n";
        }
    }
}
