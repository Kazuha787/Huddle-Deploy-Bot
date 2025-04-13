#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <optional>
#include <algorithm>
#include <iomanip>
#include <chrono>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <cxxopts.hpp>
#include <date/date.h>

// Using declarations for clarity
using json = nlohmann::json;
using namespace std::chrono;
using namespace date;
namespace fs = std::filesystem;

// Enum for task priority
enum class Priority { Low, Medium, High };

// Convert Priority to string
std::string priority_to_string(Priority p) {
    switch (p) {
        case Priority::Low: return "Low";
        case Priority::Medium: return "Medium";
        case Priority::High: return "High";
        default: return "Unknown";
    }
}

// Task class with priority and category
class Task {
public:
    int id;
    std::string description;
    bool completed;
    Priority priority;
    std::string category;
    system_clock::time_point created_at;
    std::optional<system_clock::time_point> due_date;

    Task(int id, std::string desc, Priority pri = Priority::Medium, std::string cat = "General", bool comp = false)
        : id(id), description(std::move(desc)), completed(comp), priority(pri), category(std::move(cat)),
          created_at(system_clock::now()) {}

    // JSON serialization
    void to_json(json& j) const {
        j = json{
            {"id", id},
            {"description", description},
            {"completed", completed},
            {"priority", priority_to_string(priority)},
            {"category", category},
            {"created_at", format_time(created_at)},
            {"due_date", due_date ? format_time(*due_date) : nullptr}
        };
    }

    void from_json(const json& j) {
        id = j.at("id").get<int>();
        description = j.at("description").get<std::string>();
        completed = j.at("completed").get<bool>();
        std::string pri_str = j.at("priority").get<std::string>();
        priority = (pri_str == "Low") ? Priority::Low : (pri_str == "High") ? Priority::High : Priority::Medium;
        category = j.at("category").get<std::string>();
        created_at = parse_time(j.at("created_at").get<std::string>());
        if (j.at("due_date").is_null()) {
            due_date = std::nullopt;
        } else {
            due_date = parse_time(j.at("due_date").get<std::string>());
        }
    }

private:
    // Format time point to string
    static std::string format_time(const system_clock::time_point& tp) {
        return date::format("%Y-%m-%d %H:%M:%S", tp);
    }

    // Parse string to time point
    static system_clock::time_point parse_time(const std::string& s) {
        std::istringstream iss(s);
        system_clock::time_point tp;
        iss >> date::parse("%Y-%m-%d %H:%M:%S", tp);
        return tp;
    }
};

// TaskManager class with enhanced functionality
class TaskManager {
public:
    TaskManager(const std::string& fp) : file_path(fp), next_id(1) {
        load_tasks();
    }

    // Add a new task with priority and category
    void add_task(const std::string& desc, const std::optional<std::string>& due,
                  Priority pri, const std::string& cat) {
        std::optional<system_clock::time_point> due_date;
        if (due) {
            std::istringstream iss(*due);
            system_clock::time_point tp;
            iss >> date::parse("%Y-%m-%d", tp);
            if (!iss.fail()) {
                due_date = tp;
            } else {
                std::cerr << "Warning: Invalid due date format, ignoring due date.\n";
            }
        }

        tasks.emplace_back(next_id, desc, pri, cat);
        if (due_date) {
            tasks.back().due_date = due_date;
        }
        std::cout << "Task added with ID " << next_id << "\n";
        next_id++;
        save_tasks();
    }

    // List tasks with sorting option
    void list_tasks(const std::string& sort_by) const {
        if (tasks.empty()) {
            std::cout << "No tasks found.\n";
            return;
        }

        std::vector<Task> sorted_tasks = tasks;
        if (sort_by == "priority") {
            std::sort(sorted_tasks.begin(), sorted_tasks.end(),
                [](const Task& a, const Task& b) {
                    return static_cast<int>(a.priority) > static_cast<int>(b.priority);
                });
        } else if (sort_by == "due_date") {
            std::sort(sorted_tasks.begin(), sorted_tasks.end(),
                [](const Task& a, const Task& b) {
                    if (!a.due_date && !b.due_date) return a.id < b.id;
                    if (!a.due_date) return false;
                    if (!b.due_date) return true;
                    return *a.due_date < *b.due_date;
                });
        }

        std::cout << "\nTasks:\n";
        std::cout << std::left << std::setw(5) << "ID"
                  << std::setw(30) << "Description"
                  << std::setw(10) << "Status"
                  << std::setw(10) << "Priority"
                  << std::setw(15) << "Category"
                  << std::setw(20) << "Created At"
                  << std::setw(20) << "Due Date" << "\n";
        std::cout << std::string(110, '-') << "\n";

        for (const auto& task : sorted_tasks) {
            std::cout << std::left << std::setw(5) << task.id
                      << std::setw(30) << task.description
                      << std::setw(10) << (task.completed ? "Done" : "Pending")
                      << std::setw(10) << priority_to_string(task.priority)
                      << std::setw(15) << task.category
                      << std::setw(20) << date::format("%Y-%m-%d %H:%M", task.created_at)
                      << std::setw(20) << (task.due_date ? date::format("%Y-%m-%d", *task.due_date) : "None")
                      << "\n";
        }
        std::cout << "\n";
    }

    // Mark a task as complete
    void complete_task(int id) {
        auto it = std::find_if(tasks.begin(), tasks.end(), [id](const Task& t) { return t.id == id; });
        if (it != tasks.end()) {
            it->completed = true;
            std::cout << "Task " << id << " marked as complete.\n";
            save_tasks();
        } else {
            std::cout << "Task with ID " << id << " not found.\n";
        }
    }

    // Delete a task
    void delete_task(int id) {
        auto it = std::find_if(tasks.begin(), tasks.end(), [id](const Task& t) { return t.id == id; });
        if (it != tasks.end()) {
            tasks.erase(it);
            std::cout << "Task " << id << " deleted.\n";
            save_tasks();
        } else {
            std::cout << "Task with ID " << id << " not found.\n";
        }
    }

    // Clear all tasks
    void clear_tasks() {
        tasks.clear();
        next_id = 1;
        std::cout << "All tasks cleared.\n";
        save_tasks();
    }

private:
    std::vector<Task> tasks;
    int next_id;
    std::string file_path;

    // Load tasks from JSON file
    void load_tasks() {
        std::ifstream file(file_path);
        if (!file.is_open()) {
            return;
        }

        json j;
        try {
            file >> j;
            tasks = j.get<std::vector<Task>>();
            if (!tasks.empty()) {
                next_id = std::max_element(tasks.begin(), tasks.end(),
                    [](const Task& a, const Task& b) { return a.id < b.id; })->id + 1;
            }
        } catch (const json::exception& e) {
            std::cerr << "Error parsing tasks file: " << e.what() << "\n";
            tasks.clear();
        }
    }

    // Save tasks with backup
    void save_tasks() const {
        // Create backup if file exists
        if (fs::exists(file_path)) {
            fs::copy_file(file_path, file_path + ".bak", fs::copy_options::overwrite_existing);
        }

        std::ofstream file(file_path);
        if (!file.is_open()) {
            std::cerr << "Error: Could not open tasks file for writing.\n";
            return;
        }
        json j = tasks;
        file << std::setw(4) << j << std::endl;
    }
};

// CLI parsing
cxxopts::Options setup_options() {
    cxxopts::Options options("TaskManager", "A CLI tool to manage tasks with priorities and categories.");
    options.add_options()
        ("c,command", "Command (add|list|complete|delete|clear)", cxxopts::value<std::string>())
        ("d,description", "Task description", cxxopts::value<std::string>()->default_value(""))
        ("due-date", "Due date (YYYY-MM-DD)", cxxopts::value<std::string>()->default_value(""))
        ("p,priority", "Priority (low|medium|high)", cxxopts::value<std::string>()->default_value("medium"))
        ("category", "Task category", cxxopts::value<std::string>()->default_value("General"))
        ("s,sort-by", "Sort list by (id|priority|due_date)", cxxopts::value<std::string>()->default_value("id"))
        ("i,id", "Task ID", cxxopts::value<int>()->default_value("0"))
        ("h,help", "Print usage");
    return options;
}

// Parse priority from string
Priority parse_priority(const std::string& p) {
    if (p == "low") return Priority::Low;
    if (p == "high") return Priority::High;
    return Priority::Medium;
}

// Main function
int main(int argc, char* argv[]) {
    auto options = setup_options();
    try {
        auto result = options.parse(argc, argv);

        if (result.count("help") || !result.count("command")) {
            std::cout << options.help() << std::endl;
            return 0;
        }

        TaskManager manager("tasks.json");
        const auto command = result["command"].as<std::string>();

        if (command == "add") {
            const auto desc = result["description"].as<std::string>();
            if (desc.empty()) {
                std::cerr << "Error: Description required for add command.\n";
                return 1;
            }
            const auto due = result["due-date"].as<std::string>();
            const auto pri = parse_priority(result["priority"].as<std::string>());
            const auto cat = result["category"].as<std::string>();
            manager.add_task(desc, due.empty() ? std::nullopt : std::make_optional(due), pri, cat);
        } else if (command == "list") {
            const auto sort_by = result["sort-by"].as<std::string>();
            manager.list_tasks(sort_by);
        } else if (command == "complete") {
            const auto id = result["id"].as<int>();
            if (id <= 0) {
                std::cerr << "Error: Valid ID required for complete command.\n";
                return 1;
            }
            manager.complete_task(id);
        } else if (command == "delete") {
            const auto id = result["id"].as<int>();
            if (id <= 0) {
                std::cerr << "Error: Valid ID required for delete command.\n";
                return 1;
            }
            manager.delete_task(id);
        } else if (command == "clear") {
            manager.clear_tasks();
        } else {
            std::cerr << "Unknown command: " << command << "\n";
            std::cout << options.help() << std::endl;
            return 1;
        }
    } catch (const cxxopts::OptionException& e) {
        std::cerr << "Error parsing options: " << e.what() << "\n";
        std::cout << options.help() << std::endl;
        return 1;
    }

    return 0;
}

// Tests
void run_tests() {
    TaskManager tm("test_tasks.json");
    std::cout << "Running tests...\n";

    // Test 1: Add task
    tm.add_task("Test task", std::nullopt, Priority::Medium, "General");
    if (tm.tasks.size() != 1 || tm.tasks[0].description != "Test task") {
        std::cerr << "Test 1 failed: Add task\n";
        return;
    }

    // Test 2: Complete task
    tm.complete_task(1);
    if (!tm.tasks[0].completed) {
        std::cerr << "Test 2 failed: Complete task\n";
        return;
    }

    // Test 3: Add task with priority and category
    tm.add_task("High priority task", std::nullopt, Priority::High, "Work");
    if (tm.tasks.size() != 2 || tm.tasks[1].priority != Priority::High || tm.tasks[1].category != "Work") {
        std::cerr << "Test 3 failed: Add task with priority and category\n";
        return;
    }

    // Test 4: Delete task
    tm.delete_task(1);
    if (tm.tasks.size() != 1 || tm.tasks[0].id != 2) {
        std::cerr << "Test 4 failed: Delete task\n";
        return;
    }

    // Test 5: Clear tasks
    tm.clear_tasks();
    if (!tm.tasks.empty() || tm.next_id != 1) {
        std::cerr << "Test 5 failed: Clear tasks\n";
        return;
    }

    // Test 6: Persistence
    tm.add_task("Persistent task", std::nullopt, Priority::Low, "Personal");
    TaskManager tm2("test_tasks.json");
    if (tm2.tasks.size() != 1 || tm2.tasks[0].description != "Persistent task") {
        std::cerr << "Test 6 failed: Persistence\n";
        return;
    }

    std::cout << "All tests passed.\n";
}
