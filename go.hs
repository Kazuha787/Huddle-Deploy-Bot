{-# LANGUAGE DeriveGeneric #-}
{-# LANGUAGE OverloadedStrings #-}

module Main where

import Control.Monad (when)
import Data.Aeson (FromJSON, ToJSON, decode, encode)
import Data.Maybe (fromMaybe, isJust)
import Data.List (findIndex, sortBy)
import Data.Time (UTCTime, parseTimeM, formatTime, defaultTimeLocale, getCurrentTime)
import GHC.Generics (Generic)
import System.Directory (doesFileExist)
import System.IO (hPutStrLn, stderr)
import qualified Data.ByteString.Lazy as BL
import Options.Applicative
import Text.Printf (printf)

-- Task data type
data Task = Task
  { taskId :: Int
  , description :: String
  , completed :: Bool
  , createdAt :: UTCTime
  , dueDate :: Maybe UTCTime
  } deriving (Show, Generic)

instance ToJSON Task
instance FromJSON Task

-- TaskManager data type
data TaskManager = TaskManager
  { tasks :: [Task]
  , nextId :: Int
  , filePath :: FilePath
  }

-- CLI commands
data Command
  = Add String (Maybe String)
  | List
  | Complete Int
  | Delete Int
  | Clear
  deriving Show

-- CLI parser
parseCommand :: Parser Command
parseCommand = subparser
  ( command "add" (info parseAdd (progDesc "Add a new task"))
  <> command "list" (info (pure List) (progDesc "List all tasks"))
  <> command "complete" (info parseComplete (progDesc "Mark a task as complete"))
  <> command "delete" (info parseDelete (progDesc "Delete a task"))
  <> command "clear" (info (pure Clear) (progDesc "Clear all tasks"))
  )

parseAdd :: Parser Command
parseAdd = Add
  <$> argument str (metavar "DESCRIPTION" <> help "Task description")
  <*> optional (strOption
      ( long "due-date"
      <> short 'd'
      <> metavar "YYYY-MM-DD"
      <> help "Due date for the task"
      ))

parseComplete :: Parser Command
parseComplete = Complete
  <$> argument auto (metavar "ID" <> help "Task ID to complete")

parseDelete :: Parser Command
parseDelete = Delete
  <$> argument auto (metavar "ID" <> help "Task ID to delete")

-- Initialize TaskManager
newTaskManager :: FilePath -> TaskManager
newTaskManager fp = TaskManager
  { tasks = []
  , nextId = 1
  , filePath = fp
  }

-- Load tasks from JSON file
loadTasks :: FilePath -> IO TaskManager
loadTasks fp = do
  exists <- doesFileExist fp
  if exists
    then do
      contents <- BL.readFile fp
      case decode contents :: Maybe [Task] of
        Just ts -> do
          let maxId = if null ts then 0 else maximum (map taskId ts)
          return TaskManager { tasks = ts, nextId = maxId + 1, filePath = fp }
        Nothing -> do
          hPutStrLn stderr "Error: Could not parse tasks file."
          return $ newTaskManager fp
    else return $ newTaskManager fp

-- Save tasks to JSON file
saveTasks :: TaskManager -> IO ()
saveTasks tm = BL.writeFile (filePath tm) (encode $ tasks tm)

-- Add a task
addTask :: String -> Maybe String -> TaskManager -> IO TaskManager
addTask desc due tm = do
  dueTime <- case due of
    Just d -> parseTimeM True defaultTimeLocale "%Y-%m-%d" d :: IO (Maybe UTCTime)
    Nothing -> return Nothing
  currentTime <- getCurrentTime
  let task = Task
        { taskId = nextId tm
        , description = desc
        , completed = False
        , createdAt = currentTime
        , dueDate = dueTime
        }
      newTasks = task : tasks tm
      newManager = tm { tasks = newTasks, nextId = nextId tm + 1 }
  saveTasks newManager
  printf "Task added with ID %d\n" (taskId task)
  return newManager

-- List tasks
listTasks :: TaskManager -> IO ()
listTasks tm
  | null (tasks tm) = putStrLn "No tasks found."
  | otherwise = do
      putStrLn "\nTasks:"
      printf "%-5s %-30s %-10s %-20s %-20s\n" "ID" "Description" "Status" "Created At" "Due Date"
      putStrLn $ replicate 85 '-'
      mapM_ printTask (sortBy (\t1 t2 -> compare (taskId t1) (taskId t2)) $ tasks tm)
      putStrLn ""
  where
    printTask t = printf "%-5d %-30s %-10s %-20s %-20s\n"
      (taskId t)
      (description t)
      (if completed t then "Done" else "Pending")
      (formatTime defaultTimeLocale "%Y-%m-%d %H:%M" $ createdAt t)
      (fromMaybe "None" $ fmap (formatTime defaultTimeLocale "%Y-%m-%d") $ dueDate t)

-- Complete a task
completeTask :: Int -> TaskManager -> IO TaskManager
completeTask tid tm = do
  let (before, after) = break (\t -> taskId t == tid) (tasks tm)
  case after of
    (t:rest) -> do
      let newTasks = before ++ [t { completed = True }] ++ rest
          newManager = tm { tasks = newTasks }
      saveTasks newManager
      printf "Task %d marked as complete.\n" tid
      return newManager
    _ -> do
      printf "Task with ID %d not found.\n" tid
      return tm

-- Delete a task
deleteTask :: Int -> TaskManager -> IO TaskManager
deleteTask tid tm = do
  let (before, after) = break (\t -> taskId t == tid) (tasks tm)
  case after of
    (_:rest) -> do
      let newTasks = before ++ rest
          newManager = tm { tasks = newTasks }
      saveTasks newManager
      printf "Task %d deleted.\n" tid
      return newManager
    _ -> do
      printf "Task with ID %d not found.\n" tid
      return tm

-- Clear all tasks
clearTasks :: TaskManager -> IO TaskManager
clearTasks tm = do
  let newManager = tm { tasks = [], nextId = 1 }
  saveTasks newManager
  putStrLn "All tasks cleared."
  return newManager

-- Execute a command
executeCommand :: Command -> TaskManager -> IO TaskManager
executeCommand cmd tm = case cmd of
  Add desc due -> addTask desc due tm
  List -> listTasks tm >> return tm
  Complete tid -> completeTask tid tm
  Delete tid -> deleteTask tid tm
  Clear -> clearTasks tm

-- Main function
main :: IO ()
main = do
  cmd <- execParser $ info (parseCommand <**> helper)
    ( fullDesc
    <> progDesc "A simple CLI task manager"
    <> header "Task Manager - Manage your tasks from the command line" )
  tm <- loadTasks "tasks.json"
  _ <- executeCommand cmd tm
  return ()

-- Tests (using HUnit)
import Test.HUnit
import System.IO.Temp (withSystemTempFile)

testAddTask :: Test
testAddTask = TestCase $ withSystemTempFile "tasks.json" $ \fp _ -> do
  tm <- loadTasks fp
  tm' <- addTask "Test task" Nothing tm
  assertEqual "Task list length" 1 (length $ tasks tm')
  assertEqual "Task description" "Test task" (description $ head $ tasks tm')
  assertEqual "Next ID" 2 (nextId tm')

testCompleteTask :: Test
testCompleteTask = TestCase $ withSystemTempFile "tasks.json" $ \fp _ -> do
  tm <- loadTasks fp
  tm' <- addTask "Test task" Nothing tm
  tm'' <- completeTask 1 tm'
  assertEqual "Task completed" True (completed $ head $ tasks tm'')

testDeleteTask :: Test
testDeleteTask = TestCase $ withSystemTempFile "tasks.json" $ \fp _ -> do
  tm <- loadTasks fp
  tm' <- addTask "Test task" Nothing tm
  tm'' <- deleteTask 1 tm'
  assertEqual "Task list empty" 0 (length $ tasks tm'')

testClearTasks :: Test
testClearTasks = TestCase $ withSystemTempFile "tasks.json" $ \fp _ -> do
  tm <- loadTasks fp
  tm' <- addTask "Test task" Nothing tm
  tm'' <- addTask "Another task" Nothing tm'
  tm''' <- clearTasks tm''
  assertEqual "Task list empty" 0 (length $ tasks tm''')
  assertEqual "Next ID reset" 1 (nextId tm''')

testPersistence :: Test
testPersistence = TestCase $ withSystemTempFile "tasks.json" $ \fp _ -> do
  tm <- loadTasks fp
  tm' <- addTask "Persistent task" Nothing tm
  tm'' <- loadTasks fp
  assertEqual "Task persisted" 1 (length $ tasks tm'')
  assertEqual "Task description" "Persistent task" (description $ head $ tasks tm'')

tests :: Test
tests = TestList
  [ TestLabel "testAddTask" testAddTask
  , TestLabel "testCompleteTask" testCompleteTask
  , TestLabel "testDeleteTask" testDeleteTask
  , TestLabel "testClearTasks" testClearTasks
  , TestLabel "testPersistence" testPersistence
  ]

runTests :: IO ()
runTests = do
  counts <- runTestTT tests
  print counts
