#include <iostream>
#include <cstdio>
#include <fstream>
#include <cstring>
#include <string>
#include <sstream>
#include <ctime>
#include <map>
#include <unordered_map>
#include <vector>


struct TArgs {
    const char* input_path = nullptr;
    const char* output_path = nullptr;
    bool print = false;
    int stats = 10;
    int window = 0;
    int from = -2147483648;
    int to = 2147483647;
};

struct LogEntry {
    std::string remote_addr;
    std::string local_time;
    std::string request;
    int status;
    int bytes_send;
    int local_time_timestamp;
};


time_t convertToTimestamp(std::string& local_time) {
    std::unordered_map<std::string, int> month_map = {
        {"Jan", 0}, {"Feb", 1}, {"Mar", 2}, {"Apr", 3},
        {"May", 4}, {"Jun", 5}, {"Jul", 6}, {"Aug", 7},
        {"Sep", 8}, {"Oct", 9}, {"Nov", 10}, {"Dec", 11}
    };

    std::tm tm = {};
    std::istringstream ss(local_time.substr(0, 20));

    std::string day, month, year, hour, minute, second;
    std::getline(ss, day, '/');
    std::getline(ss, month, '/');
    std::getline(ss, year, ':');
    std::getline(ss, hour, ':');
    std::getline(ss, minute, ':');
    std::getline(ss, second, ' ');

    tm.tm_mday = std::stoi(day);
    tm.tm_mon = month_map[month];
    tm.tm_year = std::stoi(year) - 1900;
    tm.tm_hour = std::stoi(hour);
    tm.tm_min = std::stoi(minute);
    tm.tm_sec = std::stoi(second);

    time_t timestamp = mktime(&tm);

    return timestamp;
}


bool CheckLog(TArgs* args, LogEntry* entry) {
    if (entry->status >= 500) {
        if ((args->from <= entry->local_time_timestamp) && (entry->local_time_timestamp <= args->to)) {
            return true;
        }
    }
    return false;
}


void PrintLog(TArgs* args, std::string line) {
    if (args->print) {
        std::cout << line << std::endl;
    }
    return;
}

void LogToParams(LogEntry* entry, std::string line) {

    std::istringstream stream(line);
    std::string unused;

    stream >> entry->remote_addr;
    stream >> unused >> unused;
    std::getline(stream, unused, '[');
    std::getline(stream, entry->local_time, ']');
    stream.ignore();


    std::getline(stream, entry->request, '"');
    std::getline(stream, entry->request, '"');
    stream >> entry->status;
    stream >> entry->bytes_send;
    entry->local_time_timestamp = convertToTimestamp(entry->local_time);;

    return;
}

void ParserEqualsTerminal(char* arg, TArgs* args) {
    if ((strncmp(arg, "-o=", 3) == 0)) {
        args->output_path = arg + 3;
    }
    else if ((strncmp(arg, "--output=", 9) == 0)) {
        args->output_path = arg + 9;
    }
    else if (strncmp(arg, "-s=", 3) == 0) {
        args->stats = std::stoi(arg + 3);
    }
    else if ((strncmp(arg, "--stats=", 8) == 0)) {
        args->stats = std::stoi(arg + 8);
    }
    else if (strncmp(arg, "-w=", 3) == 0) {
        args->window = std::stoi(arg + 3);
    }
    else if ((strncmp(arg, "--window=", 9) == 0)) {
        args->window = std::stoi(arg + 9);
    }
    else if (strncmp(arg, "-f=", 3) == 0) {
        args->from = std::stoi(arg + 3);
    }
    else if ((strncmp(arg, "--from=", 7) == 0)) {
        args->from = std::stoi(arg + 7);
    }
    else if (strcmp(arg, "-e=") == 0) {
        args->to = std::stoi(arg + 3);
    }
    else if ((strncmp(arg, "--to=", 5) == 0)) {
        args->to = std::stoi(arg + 5);
    }
    else {
        args->input_path = arg;
    }
}


void ParserTerminal(int argc, char** argv, TArgs* args) {
    for (int i = 0; i < argc; ++i) {
        if ((strcmp(argv[i], "-o") == 0) || (strcmp(argv[i], "--output") == 0)) {
            args->output_path = argv[i + 1];
            ++i;
        }
        else if ((strcmp(argv[i], "-p") == 0) || (strcmp(argv[i], "--print") == 0)) {
            args->print = true;
            //std::cout<<args->print;
        }
        else if ((strcmp(argv[i], "-s") == 0) || (strcmp(argv[i], "--stats") == 0)) {
            args->stats = std::stoi(argv[i + 1]);
            ++i;
        }
        else if ((strcmp(argv[i], "-w") == 0) || (strcmp(argv[i], "--window") == 0)) {
            args->window = std::stoi(argv[i + 1]);
            ++i;
        }
        else if ((strcmp(argv[i], "-f") == 0) || (strcmp(argv[i], "--from") == 0)) {
            args->from = std::stoi(argv[i + 1]);
            ++i;
        }
        else if ((strcmp(argv[i], "-e") == 0) || (strcmp(argv[i], "--to") == 0)) {
            args->to = std::stoi(argv[i + 1]);
            ++i;
        }
        else {
            ParserEqualsTerminal(argv[i], args);
        }
    }
}


void AddToStats(LogEntry* entry, TArgs* args, std::unordered_map<std::string, int>& request_count) {
    if (args->stats > 0) {
        if (request_count.count(entry->request) > 0) {
            request_count[entry->request]++;
        }
        else {
            request_count[entry->request] = 1;
        }
    }
}

void StatsFunction(TArgs* args, std::unordered_map<std::string, int>* request_count) {
    std::vector<std::pair<int, std::string>> sorted_requests;
    for (const std::pair<std::string, int>& item : *request_count) {
        sorted_requests.push_back(std::make_pair(item.second, item.first));
    }
    std::sort(sorted_requests.rbegin(), sorted_requests.rend());
    for (int i = 0; i < args->stats && i < sorted_requests.size(); ++i) {
        std::cout << sorted_requests[i].first << " - " << sorted_requests[i].second << std::endl;
    }
}

std::string timestampToReadable(time_t timestamp) {
    struct tm* timeinfo;
    timeinfo = std::localtime(&timestamp);

    char buffer[80];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
    return std::string(buffer);
}

void WindowFunction(std::vector<int>& timestamps, TArgs* args) {

    int max_count = 0;
    int start_time = 0;
    int end_time = 0;

    int left = 0;

    for (int right = 0; right < timestamps.size(); ++right) {
        while (timestamps[right] - timestamps[left] > args->window) {
            ++left;
        }
        int count = right - left + 1;
        if (count > max_count) {
            max_count = count;
            start_time = timestamps[left];
            end_time = timestamps[right];
        }
    }

    std::cout << "MAX requests: " << max_count << std::endl;
    std::cout << "START of window: " << timestampToReadable(start_time) << std::endl;
    std::cout << "END of window: " << timestampToReadable(end_time) << std::endl;

}


void FileReadline(std::ifstream* fileread, std::ofstream* filewrite, TArgs* args) {
    std::string line;
    LogEntry entry;
    std::unordered_map<std::string, int> request_count;

    std::vector<int> timestamps;

    while (std::getline(*fileread, line)) {
        LogToParams(&entry, line);
        if (CheckLog(args, &entry)) {
            PrintLog(args, line);
            *filewrite << line << '\n';
            AddToStats(&entry, args, request_count);
        }
        timestamps.push_back(entry.local_time_timestamp);

    }
    if (args->stats > 0) {
        StatsFunction(args, &request_count);
    }
    if (args->window > 0) {
        WindowFunction(timestamps, args);
    }
    return;
}


void WorkingWithFile(TArgs* args) {
    if ((args->input_path != nullptr) && (args->output_path != nullptr)) {
        std::ifstream fileread(args->input_path);
        std::ofstream filewrite(args->output_path);
        if (!fileread.is_open()) {
            std::cout << "Error opening " << args->input_path << '\n';
            return;
        }
        if (!filewrite.is_open()) {
            std::cout << "Error opening " << args->output_path << '\n';
            return;
        }
        else {
            FileReadline(&fileread, &filewrite, args);
        }
    }
    else {
        std::cout << "You did not provide a file name" << '\n';
        return;
    }
}


int main(int argc, char** argv) {
    TArgs args;
    ParserTerminal(argc, argv, &args);
    WorkingWithFile(&args);
}
