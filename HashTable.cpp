//============================================================================
// Name        : HashTable.cpp
// Author      : Matt
// Version     : 1.0
// Copyright   : Copyright © 2023 SNHU COCE
// Description : Lab 4-2 Hash Table
//============================================================================

#include <algorithm> // std::remove in strToDouble
#include <climits> // UINT_MAX
#include <iostream>
#include <string> // atoi
#include <time.h> // clock
#include <vector>
#include <limits> // numeric limits
#include <cstdlib>  // atoi,atof
#include <iomanip> // fixed setprecision
#include <fstream> // file I/O

#include "CSVparser.hpp"

using namespace std;

//============================================================================
// Global definitions visible to all methods and classes
//============================================================================

const unsigned int DEFAULT_SIZE = 179;

// forward declarations
double strToDouble(string str, char ch);

// check if a number is prime for resizing new table
// positive int > 1 that only has two distinct positive divisors: 1 & itself
// ex: 2,3,5,7,11, etc
bool isPrime(unsigned int num)
{
    if (num <= 1) return false; // 0,1 aren't prime
    if (num <= 3) return true; // 2 and 3 are prime
    // next checking for numbers 4+, modulo, no remainder means not prime
    if (num % 2 == 0 || num % 3 == 0) return false;

    // starting at 5+, check 6i +- 1 up to sqrt(num)
    // ex: i+=6 means i walks 5,11,17,23
    for (unsigned int i = 5; i <= num / i; i += 6)
    {
        // for each i, test i and i+2 (6k-1 and 6k+1) any prime > 3 is one of those.
        if (num % i == 0 || num % (i + 2) == 0) return false;
    }
    return true;
}

// find next prime number >= num
unsigned int nextPrime(unsigned int num)
{
    if (num <= 2) return 2; // if it's 2
    // now for 3+, if it's even, increment it to make it odd.
    if (num % 2 == 0) num++;
    // while isPrime hasn't returned true, only check odd numbers.
    while (!isPrime(num))
    {
        num += 2; 
    }
    return num; // returns the next prime
}

// define a structure to hold bid information
struct Bid {
    string bidId; // unique identifier
    string title;
    string fund;
    double amount;
    Bid() {
        amount = 0.0;
    }
};

//============================================================================
// Hash Table class definition
//============================================================================

/**
 * Define a class containing data members and methods to
 * implement a hash table with chaining.
 */
class HashTable {

private:
    // Define structures to hold bids
    struct Node {
        Bid bid;
        unsigned int key;
        Node* next;
        // default constructor
        Node() {
            key = UINT_MAX;
            next = nullptr;
        }
        // initialize with a bid -- unused.
        Node(Bid aBid) : Node() {
            bid = aBid;
        }
        // initialize with a bid and a key
        Node(Bid aBid, unsigned int aKey) : Node(aBid) {
            key = aKey;
        }
    };

    vector<Node> nodes;
    unsigned int tableSize = DEFAULT_SIZE;

    unsigned int hash(int key) const;
    // method for auto resize utilizing chain length & collision count
    void checkAndResize(unsigned int chainLength, unsigned int collisionCount);

    
public:
    bool autoResize = true; // simple public toggle for menu

    HashTable();
    HashTable(unsigned int size);
    virtual ~HashTable();
    void Insert(const Bid& bid);
    void PrintAll() const;
    void Remove(const string& bidId);
    Bid Search(const string& bidId);
    //reused method for saving
    void SaveCSV(const string& path) const;
    // previously unused, now returns total items
    size_t Size() const
    {
        size_t elementCount = 0;
        for (unsigned int i = 0; i < tableSize; ++i)
        {
	        if (nodes[i].key != UINT_MAX)
	        {
                ++elementCount;
                for (Node* c = nodes[i].next; c; c = c->next)
                {
                    ++elementCount;
                }
	        }
        }
        return elementCount;
    }
};

/**
 * Default constructor
 * Creates a hash table with DEFAULT_SIZE (179) buckets.
 */
HashTable::HashTable() {
	// Initalize node structure by resizing tableSize -- which is set to DEFAULT_SIZE
	// this creates a vector with 179 node objects, keys set to UINT_MAX and next set to nullptr
    // UINT_MAX is used as a sentinel value to indicate an empty bucket
	nodes.resize(tableSize);
}

/**
 * Constructor for specifying size of the table
 * Use to improve efficiency of hashing algorithm
 * by reducing collisions without wasting memory.
 * NOTE: Currently not used in main()
 */
HashTable::HashTable(unsigned int size) {
    // invoke local tableSize to size with this->
	// create a vector with size node objects, keys set to UINT_MAX and next set to nullptr
	this->tableSize = size;
    // resize nodes size
	nodes.resize(tableSize);
}


/**
 * Destructor
 * Frees all dynamically allocated memory in the chains
 */
HashTable::~HashTable() {
	// loop through the nodes vector, for each bucket (node), check if it has a next pointer
	// if it does, traverse the linked list and delete each node
	// I can either use i < nodes.size or i < tableSize -- they are the same
    for (unsigned int i = 0; i < tableSize; i++)
    {
        // start with the first chained node (not the bucket)
        Node* current = nodes[i].next;
        // delete all linked nodes in the chain
        while (current != nullptr) {
            //store current node in a temp pointer
            Node* temp = current;
            // move to the next node before deleting
            current = current->next;
            // delete the previous node
            delete temp;
        }
    }
	// the commented code asked for this, but it's unnecessary since the vector will automatically be destroyed
   // nodes.erase(begin(nodes), end(nodes)); // nodes.clear() would be fine too.
}

/**
 * Calculate the hash value of a given key.
 * Note that key is specifically defined as
 * unsigned int to prevent undefined results
 * of a negative list index.
 *
 * @param key The key to hash
 * @return The calculated hash
 */
unsigned int HashTable::hash(int key) const {
	// modulo the key against the table size
	// I could alternatively utilize mid-square, multiplicative, or cryptographic hashing
	return key % tableSize;
}

/** 
 * Check if resize is needed and perform it
 * Uses the size constructor to create a new table
 */
void HashTable::checkAndResize(unsigned int chainLength, unsigned int collisionCount)
{
	// check resize conditions -- prevent recursive call during resize
    if (!autoResize) return;

    // resize if either condition is met
    if (chainLength >= 4 || collisionCount > tableSize / 3)
    {
        // determine reason for resize, chain length or collisions.
        string reason = (chainLength >= 4) ? "Chain length > 4" : "Excessive collisions.";
        unsigned int newSize = nextPrime(tableSize * 2);
        cout << "Auto resize (" << reason << "): changing " << tableSize << " to " << newSize << endl;

        // create new table using the size constructor
        HashTable* temp = new HashTable(newSize);
        temp->autoResize = false; // prevent recursive resizing during the transfer

        // transfer all bids, check if its empty, insert into temp
        for (unsigned int i = 0; i < tableSize; i++)
        {
            if (nodes[i].key != UINT_MAX)
            {
                temp->Insert(nodes[i].bid);
                Node* node = nodes[i].next;
                while (node != nullptr)
                {
                    temp->Insert(node->bid);
                    node = node->next;
                }
            }
        }

        // pointer swap the internals using std
        std::swap(nodes, temp->nodes);
        std::swap(tableSize, temp->tableSize);
        // clean up old table data
        delete temp;
        cout << "Resize complete\n";
    }
}





/**
 * Insert a bid
 *
 * @param bid The bid to insert
 */
void HashTable::Insert(const Bid& bid) {
	// convert the bidId to an integer using atoi, to use as the key
    // hash returns key modulo(%) tableSize
	unsigned int key = hash(atoi(bid.bidId.c_str()));
	// retrieve node/bucket using hash key
	Node* node = &nodes.at(key);

    // if the head bucket/node is empty
    // UINT_MAX is used for the default key constructor value. 
    if (node->key == UINT_MAX) 
    {
        // First bid in this bucket direct insert 
        node->key = key;
        node->bid = bid; // store the actual bid data
        node->next = nullptr;
        return;
    }

    // update existing bid
	if (node->bid.bidId == bid.bidId)
	{
        node->bid = bid;
        return;
	}
    // traverse chain
    unsigned int chainLength = 0; // counts nodes after head
    unsigned int collisionCount = 1; // collided with head at least once
    // while the next node isn't empty
    while (node->next != nullptr)
    {
        chainLength++;
        collisionCount++;
        node = node->next;
        if (node->bid.bidId == bid.bidId)
        {
            node->bid = bid;
            return;
        }
    }
    // add at end
    node->next = new Node(bid, key);
    chainLength++;

    // check if resize is needed
    checkAndResize(chainLength, collisionCount);
}

/**
 * Print all bids
 * Displays all bids in the hash table, showing which bucket they're in
 * Chained items are shown with " -- " as the prefix to indicate collision. 
 */
void HashTable::PrintAll() const {
    cout << fixed << setprecision(2);

    unsigned int totalItems = 0;
    unsigned int maxChain = 0;

    // iterate through all buckets
    for (unsigned int i = 0; i < tableSize; ++i)
    {
	    // if key isn't equal to UINT_MAX, meaning if it's not empty
        // only print non-empty buckets
        if (nodes[i].key != UINT_MAX)
        {
            cout << "Key " << i << ": ";
            // print the main node in the bucket
            // output key, bidId, title, amount, fund
            cout << nodes[i].bid.bidId << " | " << nodes[i].bid.title << " | "
                 << nodes[i].bid.amount << " | " << nodes[i].bid.fund << endl;
            totalItems++;

            // check if there are chained nodes (collisions)
            unsigned int chainLength = 0;
            Node* node = nodes[i].next;
            while (node != nullptr)
            {
	            // output chained bid info
                // -- prefix for chained
                cout << " -- " << i << ": ";
                cout << node->bid.bidId << " | " << node->bid.title << " | "
                     << node->bid.amount << " | " << node->bid.fund << endl;
                node = node->next;
                chainLength++;
                totalItems++;
            }
            if (chainLength > maxChain) maxChain = chainLength;
        }
    }

    // stats output
    cout << "There are " << totalItems << " items in " << tableSize << " buckets, the longest chain: "
        << maxChain << endl;
}

/**
 * Remove a bid
 * Removes a bid from the hash table, with case handling for bucket head and chain
 *
 * @param bidId The bid id to search for
 */
void HashTable::Remove(const string& bidId) {
    // calculate which bucket should contain the bid
    unsigned int key = hash(atoi(bidId.c_str()));
    // get the bucket
    Node* node = &nodes.at(key);

    // if this node contains the bid to remove and isn't empty
    // if the bid is in the bucket head / 1st position
	if (node->key != UINT_MAX && node->bid.bidId == bidId)
	{
		// if there's a chain, promote next node to head
        if (node->next != nullptr)
        {
	        // copy next node's data to current bucket head
            Node* temp = node->next;
            node->bid = temp->bid;
            node->next = temp->next;
            delete temp; // delete the now duplicate node
        }
        else
        {
	        // no chain, just clear this node and mark as empty
            node->key = UINT_MAX;
            node->bid = Bid(); // clear bid data with empty constructor
            node->next = nullptr;
        }
        return;
	}
    // search the chain for the bid to remove
    Node* prevNode = node;
    node = node->next;
    while (node != nullptr)
    {
	    if (node->bid.bidId == bidId)
	    {
		    // found, remove this node from the chain by updating pointers
            prevNode-> next = node->next;
            delete node;
            return;
	    }
        prevNode = node;
        node = node->next;
    }
}

/**
 * Search for the specified bidId
 * Returns the bid if found, or an empty bid if not found.
 * 
 * @param bidId The bid id to search for
 */
Bid HashTable::Search(const string& bidId) {

    Bid bid; //creates a local empty bid to return if search isn't found

    // calculate which bucket should contain this bid
    unsigned int key = hash(atoi(bidId.c_str()));
    //get the bucket
    Node* node = &nodes.at(key);

    // check if the bucket head contains our bid
    if (node->key != UINT_MAX && node->bid.bidId == bidId)
    {
        return node->bid;
    }
    // if bucket is not empty but bidId doesn't match, search the chain
    if (node->key != UINT_MAX)
    {
        node = node->next;
        while (node != nullptr)
        {
            // if the current node matches, return it
            if (node->bid.bidId == bidId)
            {
                return node->bid;
            }
            node = node->next;
        }
    }
    // if no entry found for the key, return empty bid
    return bid;
}

/**
 * Save the CSV file.
 */
void HashTable::SaveCSV(const string& path) const
// if title or fund ever contain commans, quotes, or newlines, this will break.
{
    std::ofstream file(path);
    if (!file)
    {
		cerr << "Error: could not open file " << path << " for writing.\n";
		return;
    }

    file << "Bid Id,Title,Fund,Amount\n"; // csv header
    file << std::fixed << std::setprecision(2);
	// iterate through all buckets
    for (unsigned int i = 0; i < tableSize; ++i)
    {
        if (nodes[i].key != UINT_MAX)
        {
            file << nodes[i].bid.bidId << "," << nodes[i].bid.title << ","
                 << nodes[i].bid.fund << "," << nodes[i].bid.amount << "\n";
            Node* node = nodes[i].next;
            while (node != nullptr)
            {
                file << node->bid.bidId << "," << node->bid.title << ","
                     << node->bid.fund << "," << node->bid.amount << "\n";
                node = node->next;
            }
        }
	}
}


//============================================================================
// Static methods used for testing
//============================================================================
// these can be changed to an unnamed namespace as well.
/**
 * Display the bid information to the console (std::out)
 *
 * @param bid struct containing the bid info
 */
static void displayBid(const Bid& bid) {
    cout << bid.bidId << ": " << bid.title << " | " << bid.amount << " | "
            << bid.fund << endl;
}

/**
 * Load a CSV file containing bids into a container
 *
 * @param csvPath the path to the CSV file to load
 * @return a container holding all the bids read
 */
static void loadBids(const string& csvPath, HashTable* hashTable) {
    cout << "Loading CSV file " << csvPath << endl;
   
    // initialize the CSV Parser using the given path
    csv::Parser file = csv::Parser(csvPath);

    // read and display header row - optional
    vector<string> header = file.getHeader();
    for (auto const& c : header) {
        cout << c << " | ";
    }
    cout << "" << endl;

    try {
        // loop to read rows of a CSV file
        for (unsigned int i = 0; i < file.rowCount(); i++) {

            // Create a data structure and add to the collection of bids
            Bid bid;
            bid.bidId = file[i][1];
            bid.title = file[i][0];
            bid.fund = file[i][8];
            bid.amount = strToDouble(file[i][4], '$');

            //cout << "Item: " << bid.title << ", Fund: " << bid.fund << ", Amount: " << bid.amount << endl;

            // push this bid to the end
            hashTable->Insert(bid);
        }
    } catch (csv::Error &e) {
        std::cerr << e.what() << std::endl;
    }
}

/**
 * Simple C function to convert a string to a double
 * after stripping out unwanted char
 *
 * credit: http://stackoverflow.com/a/24875936
 *
 * @param ch The character to strip out
 */
double strToDouble(string str, char ch) {
    str.erase(remove(str.begin(), str.end(), ch), str.end());
    return atof(str.c_str());
}

/**
 * The one and only main() method
 */
int main(int argc, char* argv[]) {

    // process command line arguments
    string csvPath, bidKey, searchId, removeId;

    switch (argc) {
    case 2:
        csvPath = argv[1];
        bidKey = "98223";
        break;
    case 3:
        csvPath = argv[1];
        bidKey = argv[2];
        break;
    default:
        csvPath = "eBid_Monthly_Sales.csv";
        bidKey = "98223";
    }

    // Define a timer variable
    clock_t ticks;

    // Define a hash table to hold all the bids -- default size (179) buckets
    HashTable* bidTable = new HashTable();
    Bid bid;
    
    
    int choice = 0;
    while (choice != 9) {
        cout << "Menu:" << endl;
        cout << "  1. Load Bids" << endl;
        cout << "  2. Display All Bids" << endl;
        cout << "  3. Find Bid" << endl;
        cout << "  4. Remove Bid" << endl;
        cout << "  5. Toggle Auto Resize (" << (bidTable->autoResize ? "ON" : "OFF") << ")" << endl;
		cout << "  6. Save Bids" << endl;
        cout << "  9. Exit" << endl;
        cout << "Enter choice: ";
        //cin >> choice;

        if (!(cin >> choice)) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            continue;
        }
        cin.ignore(numeric_limits<streamsize>::max(), '\n'); // flush newline left by >>


        switch (choice) {

        case 1: {
            // test file path being wrong or missing.
			cout << "Enter csv file path: file.csv for instance\n";
			cin >> csvPath;
            if (csvPath.empty()) {
                cout << "File path can't be empty, defaulting to eBid_Monthly_Sales.csv\n";
                csvPath = "eBid_Monthly_Sales.csv";
            }
        	/*ifstream test(csvPath);
			if (!test) {
				cout << "File not found, defaulting to eBid_Monthly_Sales.csv\n";
				csvPath = "eBid_Monthly_Sales.csv";
			}*/
            // instead of opening another stream, can just use try/catch

            // Initialize a timer variable before loading bids
            ticks = clock();
            // Complete the method call to load the bids
            //loadBids(csvPath, bidTable);
            try {
                loadBids(csvPath, bidTable);
            }
            catch (const csv::Error& e) {
                cout << "Failed to load " << csvPath
                    << ", defaulting to eBid_Monthly_Sales.csv\n";
                loadBids("eBid_Monthly_Sales.csv", bidTable);
            }
            // Calculate elapsed time and display result
            ticks = clock() - ticks; // current clock ticks minus starting clock ticks
            cout << "time: " << ticks << " clock ticks" << endl;
            cout << "time: " << ticks * 1.0 / CLOCKS_PER_SEC << " seconds" << endl;
            break;
        }

        case 2: {
            bidTable->PrintAll();
            break;
        }
        case 3: {
            cout << "Enter bid ID to search, for instance " << bidKey << ": \n";
            getline(cin, searchId);
            if (!searchId.empty()) bidKey = searchId;

            ticks = clock();
            bid = bidTable->Search(bidKey);
            ticks = clock() - ticks; // current clock ticks minus starting clock ticks

            if (!bid.bidId.empty()) {
                displayBid(bid);
            }
            else {
                cout << "Bid Id " << bidKey << " not found." << endl;
            }
            cout << "time: " << ticks << " clock ticks" << endl;
            cout << "time: " << ticks * 1.0 / CLOCKS_PER_SEC << " seconds" << endl;
            break;
        }
        case 4: {
            // request input and update bidKey accordingly
            cout << "Enter bid ID to remove " << bidKey;
            getline(cin, removeId);
            if (!removeId.empty()) bidKey = removeId;

            size_t before = bidTable->Size();
            bidTable->Remove(bidKey);
            size_t after = bidTable->Size();
            cout <<  ((after < before) ? "Removed " : "Not found ") << bidKey << "\n";
            //cout << "Removed bid " << bidKey << "\n";
            break;
        }
        case 5: {
                // case to handle menu resize being enabled or disabled
            bidTable->autoResize = !bidTable->autoResize;
            cout << "Auto resize " << (bidTable->autoResize ? "enabled" : "disabled") << endl;
            break;
        }
        case 6: {
            // case for saving bids to CSV
			cout << "Enter save file path (default: bids_saved.csv)\n";
			getline(cin, csvPath);
			if (csvPath.empty()) csvPath = "bids_saved.csv";

            ticks = clock();
            bidTable->SaveCSV(csvPath);
            ticks = clock() - ticks;

            cout << "Saved to " << csvPath << endl;
            cout << "time: " << ticks << " clock ticks" << endl;
            cout << "time: " << ticks * 1.0 / CLOCKS_PER_SEC << " seconds" << endl;
            break;
        }
            // added 9 for break since I also included a default for invalid input. 
        case 9:{ break; }
        default: {
            cout << "Invalid choice.\n";
            break;
        }
        }
    }

    cout << "Good bye." << endl;

    // clean up for new hashtable()
    delete bidTable;

    return 0;
}
