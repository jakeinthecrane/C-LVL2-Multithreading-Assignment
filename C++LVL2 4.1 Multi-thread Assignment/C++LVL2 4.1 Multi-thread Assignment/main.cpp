#include <iostream>
#include <stdexcept>
#include <string>  
#include <vector>
#include <sstream>
#include <fstream>
#include <memory>    
#include <thread>    // Directive for thread creation 
#include <atomic>    // Atomic directive to flag to thread safety on save status. 
#include <chrono>    // Provides the tools to work with clocks, durations, time points.  

using namespace std;

void displayInstructions()
{
    cout << "Personal Expense Tracker to keep you organized and well-balanced!" << endl;
    cout << "Log your expenses by categorizing them and adding the amount. "
        << "Type 'done' when ready for a summary.\n" << endl;
}

struct Expense
{
    string category;
    double amount = 0.0;
};

class ExpenseTracker
{
private:
    vector<Expense> expenses;

    void loadExpensesFromFile(const string& filename)
    {
        ifstream file(filename);
        if (file.is_open())
        {
            Expense expense;
            while (file >> expense.category >> expense.amount)
            {
                expenses.push_back(expense);
            }
            cout << "Welcome back! Your list of expenses is stored in 'expenses.txt'.  Guide yourself there whenever you may want to see your progress."
                << " Continue to add below:\n";
        }
        else
        {
            cout << "No existing expense file found. Starting fresh.\n";
        }
    }

    void saveExpensesToFile(const string& filename) const
    {
        ofstream file(filename);
        for (const auto& expense : expenses)
        {
            file << expense.category << " " << expense.amount << "\n";
        }
        cout << "Expenses saved to file.\n";
    }

public:
    /* Atomic flag to track save completion  */
    mutable std::atomic<bool> saveCompleted{ false };

    ExpenseTracker(const string& filename)
    {
        loadExpensesFromFile(filename);
    }

    void addExpense(const string& category, const string& inputAmount)
    {
        double amount;
        stringstream ss(inputAmount);
        if (!(ss >> amount))
        {
            throw invalid_argument("Error: Invalid input. Please enter a numeric value for the amount.");
        }
        if (amount < 0)
        {
            throw out_of_range("Error: Expense amount cannot be negative.");
        }

        Expense expense{ category, amount };
        expenses.push_back(expense);
        cout << "Added expense: " << expense.category << " - $" << expense.amount << endl;
    }

    void searchExpense(const string& searchCategory) const
    {
        bool found = false;
        for (const Expense& expense : expenses)
        {
            if (expense.category == searchCategory)
            {
                cout << "- " << expense.category << ": $" << expense.amount << endl;
                found = true;
            }
        }
        if (!found)
        {
            cout << "No expenses found in category: " << searchCategory << endl;
        }
    }

    void displayExpenses() const
    {
        if (expenses.empty())
        {
            cout << "No expenses recorded yet." << endl;
            return;
        }
        cout << "\nRecorded Expenses:" << endl;
        for (const auto& expense : expenses)
        {
            cout << "- " << expense.category << ": $" << expense.amount << endl;
        }
    }

    void calculateTotal() const
    {
        if (expenses.empty())
        {
            throw runtime_error("Error: No expenses recorded. Please add expenses before calculating the total.");
        }

        // Setting up threads with a constant integer variable named numThreads, initialized to 4 threads. 
        const int numThreads = 4;
        size_t expensesPerThread = expenses.size() / numThreads; //this function calculates expenses.size divided by numThreads to keep a balanced workload. We're using size_t for the container compatability.  Doens't allow negative numbers for better data integrity.  
        vector<thread> threads; // thread vector named threads created for dynamic storage of theads when "threads.emplace_back(sumExpenses, i, start, end)" make the call
        vector<double> partialSums(numThreads, 0.0);  // vector for doubles named partialSums storing the sum of expenses which take 2 parameters; the thread and expense.

        /* Lambda function for thread execution.Set to auto to deduce the lambda function for cleaner UX.sumExpenses is the name of the 
        lambda function */
        auto sumExpenses = [&](int threadIndex, size_t start, size_t end) // & capture list is used to reference the expenses and partial sums vectors through use of reference.
            {                 // Lambda function takes 3 parameters to keep track of the threads declaring int threadIndex, and unsigned ints start and end to the 
                double sum = 0.0; // variable function named sum set to 0
                for (size_t i = start; i < end; ++i)  // lambda condition for loop iterating each partial thread
                {
                    sum += expenses[i].amount;  // here I'm going to implement the shorthand version of this function as opposed to "sum = sum + expenses[i].amount;" for better UX..
                }
                partialSums[threadIndex] = sum; // all indices are accumulated in their respective threads and stored in partialSums vector.  
            };

        // Launch threads.  Thread index is represented by i. Each thread is assigned a specific range of work. 
        for (int i = 0; i < numThreads; ++i) // for loop iterating each thread beginning at 0.  Named numThreads.
        {   /* Follows is the formula that divides the work amongst the threads. Here we create the work division amongst the threads.  Each thread running a start and end per thread creation then a call for a new thread reloading the lambda function. */
            size_t start = i * expenses.size() / numThreads;  // start index with formula to calculate the thread division amongst the threads created.  i is multiplied by expense.size() then divided by numThreads to find the value of what each thread will execute so that each thread has a balanced load. 
            size_t end = (i + 1) * expenses.size() / numThreads; // end index with finished formula to complete the end result assigning what this thread in particular thread should work on. 
            threads.emplace_back(sumExpenses, i, start, end); // threads vector calls epmplace_back operation passing sumExpenses, i, start, end arguments and restarting the loop creating a new thread dynamically and avoids manual creation. . 
        }  // ie:say I had 18 expense entries; start: 0*18/ 4 = 0,end: 1*18/ 4 = 4,  start: 1*18/ 4 = 4, end: 2* 18/ 4 = 8 (not 9 because the formula is calculated using integer division. Whats left over is truncated and not rounded.)
           // These threads will run as 0,1,2,3 for the first thread, and 4, 5, 6, 7 for the second and so fourth dividing the threads accordingly to create a balanced execution. Truncation ensures contiguous ranges without gaps or overlaps. 
           
        // The calling thread, Main thread is blocked here until all working threads represented by t finish execution before proceeding in main
        for (auto& t : threads) //for loop iterating over each auto type t reference thread object in the threads vector.. 
        {
            t.join(); // member function of the thread class join() function is called by t object to wait for all concurrent operations for data integrity.
        }             // (I used the for loop to avoid iterating each thread manually in the function body... a dynamic iteration approach..)

        // Combine partial sums from all threads to get the total
        double total = 0.0; // total variable set to 0
        for (const auto& sum : partialSums) //
        {
            total = total + sum; // function to add sum total to overall total... "total += sum" is the compound assignment operator for shorthand.
        }

        cout << "\nTotal spending: $" << total << endl; // console out to let user know their overall expense total. 
    }

    void saveToFileAsync(const string& filename) const // is executed in a separate thread while the main thread continues its work.  Saving in the background.
    {
        // Reset the saveCompleted flag before starting save operation to ensure the program's state reality and will flag true once the detached thread completes the save.
        saveCompleted = false;

        // Create a new thread to save expenses without blocking the main thread
        thread saveThread([this, &filename]() // we create an object named saveThread passing the 'capture this' pointer and filename variable by reference.
            {
            this->saveExpensesToFile(filename); // logic execution 
            saveCompleted = true;  // Marks flag to save as completed to verify good data integrity. 
            });

        saveThread.detach(); // Detach thread to allow it to run independently.  It saves in the background. Main thread runs without waiting for save to complete. 
        cout << "Expenses are being saved in the background.\n";
    }
};

int main()
{
    displayInstructions();

    string filename = "expenses.txt";

    
    unique_ptr<ExpenseTracker> tracker = make_unique<ExpenseTracker>(filename);

    bool success = true;

    try
    {
        string category, amount;
        while (true)
        {
            cout << "\nEnter an expense category (or type 'SEARCH' to look up a category. "
                << "Otherwise 'DONE' to finish): ";
            getline(cin, category);
            if (category == "DONE") break;
            if (category == "SEARCH")
            {
                cout << "Enter category to search: ";
                string searchCategory;
                getline(cin, searchCategory);
                tracker->searchExpense(searchCategory);
                continue;
            }

            cout << "Enter the amount spent on " << category << ": $";
            cin >> amount;
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            tracker->addExpense(category, amount);
        }

        tracker->displayExpenses();
        tracker->calculateTotal(); // This function is now multithreaded function..
    }
    catch (const invalid_argument& e)
    {
        cout << e.what() << endl;
        success = false;
    }
    catch (const out_of_range& e)
    {
        cout << e.what() << endl;
        success = false;
    }
    catch (const runtime_error& e)
    {
        cout << e.what() << endl;
        success = false;
    }

    if (success)
    {
        tracker->saveToFileAsync(filename); // Save expenses asynchronously ensures main thread waits for the asynchronous save operation to finish

        // While loop with a condition to wait until the save operation is complete before exiting. 
        while (!tracker->saveCompleted.load())
        {
            this_thread::sleep_for(chrono::milliseconds(50));  // added a small delay to avoid busy-waiting that can drag the CPU
        }

        cout << "\nGreat job on staying on top of your finances!" << endl;
    }

    // `tracker` goes out of scope here, and the ExpenseTracker object it points to is automatically deleted
    return 0;
}
