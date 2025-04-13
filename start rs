use serde::{Deserialize, Serialize};
use std::fs::{File, OpenOptions};
use std::io::{self, Read, Write};
use std::path::Path;
use chrono::{DateTime, Utc};
use clap::{Parser, Subcommand};

// Command-line arguments parsing with Clap
#[derive(Parser)]
#[command(name = "Task Manager")]
#[command(about = "A simple CLI task manager", long_about = None)]
struct Cli {
    #[command(subcommand)]
    command: Commands,
}

// Subcommands for different operations
#[derive(Subcommand)]
enum Commands {
    /// Add a new task
    Add {
        /// Task description
        description: String,
        /// Optional due date (format: YYYY-MM-DD)
        #[arg(short, long)]
        due_date: Option<String>,
    },
    /// List all tasks
    List,
    /// Mark a task as complete
    Complete {
        /// Task ID
        id: usize,
    },
    /// Delete a task
    Delete {
        /// Task ID
        id: usize,
    },
    /// Clear all tasks
    Clear,
}

// Task structure
#[derive(Serialize, Deserialize, Clone)]
struct Task {
    id: usize,
    description: String,
    completed: bool,
    created_at: DateTime<Utc>,
    due_date: Option<DateTime<Utc>>,
}

// TaskManager to handle task operations
struct TaskManager {
    tasks: Vec<Task>,
    next_id: usize,
    file_path: String,
}

impl TaskManager {
    // Initialize TaskManager by loading tasks from file or creating new
    fn new(file_path: &str) -> io::Result<Self> {
        let mut manager = TaskManager {
            tasks: Vec::new(),
            next_id: 1,
            file_path: file_path.to_string(),
        };
        if Path::new(file_path).exists() {
            manager.load_tasks()?;
        }
        Ok(manager)
    }

    // Load tasks from JSON file
    fn load_tasks(&mut self) -> io::Result<()> {
        let mut file = File::open(&self.file_path)?;
        let mut contents = String::new();
        file.read_to_string(&mut contents)?;
        if contents.trim().is_empty() {
            self.tasks = Vec::new();
        } else {
            self.tasks = serde_json::from_str(&contents)?;
            // Update next_id based on loaded tasks
            self.next_id = self.tasks.iter().map(|t| t.id).max().unwrap_or(0) + 1;
        }
        Ok(())
    }

    // Save tasks to JSON file
    fn save_tasks(&self) -> io::Result<()> {
        let mut file = OpenOptions::new()
            .write(true)
            .create(true)
            .truncate(true)
            .open(&self.file_path)?;
        let json = serde_json::to_string_pretty(&self.tasks)?;
        file.write_all(json.as_bytes())?;
        Ok(())
    }

    // Add a new task
    fn add_task(&mut self, description: String, due_date: Option<String>) -> io::Result<()> {
        let due_date = due_date.and_then(|date| {
            chrono::NaiveDate::parse_from_str(&date, "%Y-%m-%d")
                .ok()
                .map(|d| DateTime::<Utc>::from_utc(d.and_hms_opt(0, 0, 0).unwrap(), Utc))
        });

        let task = Task {
            id: self.next_id,
            description,
            completed: false,
            created_at: Utc::now(),
            due_date,
        };
        self.tasks.push(task);
        self.next_id += 1;
        self.save_tasks()?;
        println!("Task added with ID {}", self.next_id - 1);
        Ok(())
    }

    // List all tasks
    fn list_tasks(&self) {
        if self.tasks.is_empty() {
            println!("No tasks found.");
            return;
        }
        println!("\nTasks:");
        println!("{:<5} {:<30} {:<10} {:<20} {:<20}", "ID", "Description", "Status", "Created At", "Due Date");
        println!("{}", "-".repeat(85));
        for task in &self.tasks {
            let status = if task.completed { "Done" } else { "Pending" };
            let created_at = task.created_at.format("%Y-%m-%d %H:%M");
            let due_date = task
                .due_date
                .map_or("None".to_string(), |d| d.format("%Y-%m-%d").to_string());
            println!(
                "{:<5} {:<30} {:<10} {:<20} {:<20}",
                task.id, task.description, status, created_at, due_date
            );
        }
        println!();
    }

    // Mark a task as complete
    fn complete_task(&mut self, id: usize) -> io::Result<()> {
        if let Some(task) = self.tasks.iter_mut().find(|t| t.id == id) {
            task.completed = true;
            self.save_tasks()?;
            println!("Task {} marked as complete.", id);
        } else {
            println!("Task with ID {} not found.", id);
        }
        Ok(())
    }

    // Delete a task
    fn delete_task(&mut self, id: usize) -> io::Result<()> {
        if let Some(index) = self.tasks.iter().position(|t| t.id == id) {
            self.tasks.remove(index);
            self.save_tasks()?;
            println!("Task {} deleted.", id);
        } else {
            println!("Task with ID {} not found.", id);
        }
        Ok(())
    }

    // Clear all tasks
    fn clear_tasks(&mut self) -> io::Result<()> {
        self.tasks.clear();
        self.next_id = 1;
        self.save_tasks()?;
        println!("All tasks cleared.");
        Ok(())
    }
}

// Main function
fn main() -> io::Result<()> {
    let cli = Cli::parse();
    let mut manager = TaskManager::new("tasks.json")?;

    match cli.command {
        Commands::Add { description, due_date } => {
            manager.add_task(description, due_date)?;
        }
        Commands::List => {
            manager.list_tasks();
        }
        Commands::Complete { id } => {
            manager.complete_task(id)?;
        }
        Commands::Delete { id } => {
            manager.delete_task(id)?;
        }
        Commands::Clear => {
            manager.clear_tasks()?;
        }
    }

    Ok(())
}

// Unit tests
#[cfg(test)]
mod tests {
    use super::*;
    use std::fs;
    use tempfile::NamedTempFile;

    #[test]
    fn test_add_task() -> io::Result<()> {
        let file = NamedTempFile::new()?;
        let file_path = file.path().to_str().unwrap();
        let mut manager = TaskManager::new(file_path)?;
        
        manager.add_task("Test task".to_string(), None)?;
        assert_eq!(manager.tasks.len(), 1);
        assert_eq!(manager.tasks[0].description, "Test task");
        assert_eq!(manager.tasks[0].completed, false);
        assert_eq!(manager.next_id, 2);
        
        Ok(())
    }

    #[test]
    fn test_complete_task() -> io::Result<()> {
        let file = NamedTempFile::new()?;
        let file_path = file.path().to_str().unwrap();
        let mut manager = TaskManager::new(file_path)?;
        
        manager.add_task("Test task".to_string(), None)?;
        manager.complete_task(1)?;
        assert_eq!(manager.tasks[0].completed, true);
        
        Ok(())
    }

    #[test]
    fn test_delete_task() -> io::Result<()> {
        let file = NamedTempFile::new()?;
        let file_path = file.path().to_str().unwrap();
        let mut manager = TaskManager::new(file_path)?;
        
        manager.add_task("Test task".to_string(), None)?;
        manager.delete_task(1)?;
        assert_eq!(manager.tasks.len(), 0);
        
        Ok(())
    }

    #[test]
    fn test_clear_tasks() -> io::Result<()> {
        let file = NamedTempFile::new()?;
        let file_path = file.path().to_str().unwrap();
        let mut manager = TaskManager::new(file_path)?;
        
        manager.add_task("Test task 1".to_string(), None)?;
        manager.add_task("Test task 2".to_string(), None)?;
        manager.clear_tasks()?;
        assert_eq!(manager.tasks.len(), 0);
        assert_eq!(manager.next_id, 1);
        
        Ok(())
    }

    #[test]
    fn test_persistence() -> io::Result<()> {
        let file = NamedTempFile::new()?;
        let file_path = file.path().to_str().unwrap();
        {
            let mut manager = TaskManager::new(file_path)?;
            manager.add_task("Persistent task".to_string(), None)?;
        }
        let manager = TaskManager::new(file_path)?;
        assert_eq!(manager.tasks.len(), 1);
        assert_eq!(manager.tasks[0].description, "Persistent task");
        
        Ok(())
    }
}
