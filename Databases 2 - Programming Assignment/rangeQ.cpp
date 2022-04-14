#include "stdio.h"
#include "string.h"
#include "time.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <algorithm>
#include <bits/stdc++.h>

using namespace std;

static int max_dim = 0;
static int block_size = -1;
static int counter_ss = 0;
static int counter_kd = 0;

/**
 * @brief Class used to store each node in kd-tree and my-tree
 * 
 */
struct Node{

    vector<vector<double>> data;
    bool leaf;
    double split;
    int dim;
    Node* left;
    Node* right;

    Node() : split(0), dim(0), left(nullptr), right(nullptr), leaf(false) {}

};


/**
 * @brief Used to load a database from file into memory. Sorts database on each dimension.
 * 
 * @param database - Relative path to file to read database from. File must have records in the following format:
 *                   [x,y,z,.....]
 *                   A value for each dimension, delimited by a space
 * @param db - The 2-D array to fill with records
 */
void read_db_file(string database, vector<vector<double>>* db){

    ifstream file;
    file.open(database);

    if(!file.is_open()){
        cout << "FILE FAILED TO OPEN" << endl;
        return;
    }

    string l = "";
    while(getline(file, l, '\n')){
        vector<double> touple;
        char* line = const_cast<char*>(l.c_str());
        char* token = strtok(line, ",");
        while(token != NULL){
            double record = stod(token);
            touple.push_back(record);
            token = strtok(NULL, ",");
        }
        db->push_back(touple);
    }
    cout << "Sorting data..." << endl;
    sort(db->begin(), db->end());
    cout << "Done!" << endl;
}

/**
 * @brief Used to load queries from file into memory. We could've loaded each query when needed in order to save memory,
 *        but since memory is no issue in this situation, it is easier to access a 2-D array of preloaded queries.
 * 
 * @param query - Relative path to file to read queries from. File must have queries in the following format:
 *                [x_min x_max y_min y_max z_min ...]
 *                A min and max value for each dimension, delimited by a space
 *                File must specify a range for each dimension that exists in the database file
 * @param q - The 2-D array to fill with queries
 */
void read_q_file(string query, vector<vector<double>>* q){

    ifstream file;
    file.open(query);

    if(!file.is_open()){
        cout << "FILE FAILED TO OPEN" << endl;
        return;
    }

    string l = "";
    while(getline(file, l, '\n')){
        vector<double> touple;
        char* line = const_cast<char*>(l.c_str());
        char* token = strtok(line, " ");
        while(token != NULL){
            double record = stod(token);
            touple.push_back(record);
            token = strtok(NULL, " ");
        }
        q->push_back(touple);
    }
    cout << "Done!" << endl;
}

/**
 * @brief Used to print up to the k-th element of a 2-D array
 *          [DB or Query arrays]
 * 
 * @param k - Print up to k-th element
 * @param v - 2-D array
 */
void print(int k, vector<vector<double>>* v){
    for(int i = 0; i < k; i++){
       for(int j = 0; j < v->at(i).size(); j++){
           cout << v->at(i)[j] << " ";
       }
       cout << endl;
   }
}

/**
 * @brief Used to print a single element from a 2-D array
 *          [DB or Query arrays]
 *        Used for reporting a single record that satisfies query
 * 
 * @param i - Index of record you want printed
 * @param v - 2-D array
 */
void print_i(int i, vector<vector<double>>* v){
    for(int j = 0; j < v->at(i).size(); j++){
        cout << v->at(i)[j] << " ";
    }
    cout << endl;
}

/**
 * @brief Used to only report valid records from a node that is found to contain relevant records
 *        by search(). Nodes that are identified to contain relevant records may contain records
 *        that shouldn't be reported. 
 * 
 * @param node - The node to pull data from
 * @param query - The query to check data against
 * @param r - Vector which will allow for query results to be sorted and compared
 */
void print_within_range(Node* node, vector<double>* query, vector<vector<double>>* r){
    for(int i = 0; i < node->data.size(); i++){
        int dim_start = 0;
        int dim_end = 1;
        for(int j = 0; j < node->data[i].size(); j++){
                //Any dimension doesn't fall within range, disregard record
                if(node->data[i][j] < query->at(dim_start) || node->data[i][j] > query->at(dim_end)){
                    break;
                }
                //You made it here on the last element, report record
                if(j == node->data[i].size()-1){
                    counter_kd++;
                    r->push_back(node->data[i]);
                }
                //You made it here and you aren't on the last element, iterate query to look at next element
                dim_start += 2;
                dim_end += 2;
        }
    }
}

/**
 * @brief This is a helper function to find_min_and_max. Builds an array of values from a k dimensional
 *        database
 * 
 * @param data - Database of records
 * @param v - Predefined vector to fill with values from data
 * @param dim - Dimension to build v on
 */
void build_array_on_k_dim(vector<vector<double>>* data, vector<double>* v, int dim){  
    for(int i = 0; i < data->size(); i++){
        v->push_back(data->at(i)[dim]);
    }
}

/**
 * @brief Allows to calculate a smarter splitting point for the kd tree and finds the 
 *        splitting point and splitting dimension for my tree.
 *        
 * @param data - Records
 * @param p - Predefined pair to store min and max values
 * @param dim - The dimension to find the min and max on
 */
void find_min_and_max(vector<vector<double>>* data, pair<double,double>* p, int dim){
    vector<double> vals;
    build_array_on_k_dim(data, &vals, dim);

    p->first = *min_element(vals.begin(), vals.end());
    p->second = *max_element(vals.begin(), vals.end());
}

/**
 * @brief Adds records to either left or right split depending on a precalculated mean
 * 
 * @param db - Database
 * @param left - Left split of database
 * @param right - Right split of database
 * @param dim - Dimension to split on
 * @param mean - Dictates if records fall on left or right split
 */
void get_data(vector<vector<double>>* db, vector<vector<double>>* left, vector<vector<double>>* right, int dim, double mean){
    for(int i = 0; i < db->size(); i++){
        if(db->at(i)[dim] < mean){
            left->push_back(db->at(i));
        } else {
            right->push_back(db->at(i));
        }
    }
}

/**
 * @brief Main recursive loop that builds the kd-tree
 * 
 * @param data - Database of records
 * @param dim - Dimension that node is splitting records on
 * @return Node* - Created and populated Node
 */
Node* insert(vector<vector<double>>* data, int dim){
    //Create new node
    Node* node = new Node();
    //When the data size is within block size, node is leaf and return it
    if(data->size() <= block_size){
        node->leaf = true;
        node->data = *data;
        return node;
    }

    if(dim > max_dim){
        dim = 0;
    }

    node->dim = dim;
    //Find mean of the dimension to find a good split
    pair<double,double> range;
    find_min_and_max(data, &range, dim);
    node->split = (int)(range.first+range.second)/2;

    vector<vector<double>> left;
    vector<vector<double>> right;

    get_data(data, &left, &right, dim, node->split);

    node->left = insert(&left, dim+1);
    node->right = insert(&right, dim+1);

    return node;

}

/**
 * @brief Used to call recursive insert. Returns the build kd-tree
 * 
 * @param db - Database of records
 * @return Node* - Root node of kd-tree
 */
Node* kd_tree(vector<vector<double>>* db){
    
    max_dim = db->at(0).size()-1;
    return insert(db, 0);

}

/**
 * @brief Searches kd tree and my tree for the records that are relevant to the query
 * 
 * @param node - Current node in recursive search
 * @param query - Current query
 * @param r - Vector used to add relevant records to. Allows for sorting and reporting
 */
void search_recursive(Node* node, vector<double>* query, vector<vector<double>>* r){

    if(!node){
        return;
    }
    
    if(node->leaf){
        print_within_range(node, query, r);
    }

    int curr_dim = node->dim;
    int split = node->split;
    int min = query->at(curr_dim*2);
    int max = query->at(curr_dim*2+1);

    if(split > min){
        search_recursive(node->left, query, r);
    }

    if(split <= max){
        search_recursive(node->right, query, r);
    }
}

/**
 * @brief Used to call search_recursive and format outputs. Sorts reported records so that comparison
 *        is easy
 * 
 * @param root - Root node of tree
 * @param q - Full list of queries
 */
void search(Node* root, vector<vector<double>>* q){
    vector<vector<double>> results;
    for(int k = 0; k < q->size(); k++){
        print_i(k, q);
        cout << "==============================" << endl;
        search_recursive(root, &q->at(k), &results);
        sort(results.begin(), results.end());
        print(results.size(), &results);
        results.clear();
        cout << "<<<<<<<<<<<<<<<>>>>>>>>>>>>>>>" << endl;
        cout << endl;
    }
    cout << "RECORDS REPORTED: " << counter_kd << endl;
}

/**
 * @brief Used for sequentially searching database for a given set of queries. Each query will be printed before the search is started.
 *        Records that satisfy the query will be printed below the query line by line. ====== shows where the query output starts. 
 *        <<<>>> shows where the query output ends.
 * 
 * @param db - Database 2-D array to run queries on
 * @param q - Queries 2-D array to run against the database
 */
void sequential_search(vector<vector<double>>* db, vector<vector<double>>* q){
    //For every query
    for(int k = 0; k < q->size(); k++){
        print_i(k, q);
        cout << "==============================" << endl;
        //For every record in db
        for(int i = 0; i < db->size(); i++){
            //First point is out of range, don't even bother with the rest of the points since db is sorted
            if(db->at(i)[0] > q->at(k)[1]){
                break;
            }
            int dim_start = 0;
            int dim_end = 1;
            //For every element in record
            for(int j = 0; j < db->at(i).size(); j++){
                //Any dimension doesn't fall within range, disreguard record
                if(db->at(i)[j] < q->at(k)[dim_start] || db->at(i)[j] > q->at(k)[dim_end]){
                    break;
                }
                //You made it here on the last element, report record
                if(j == db->at(i).size()-1){
                    counter_ss++;
                    print_i(i, db);
                }
                //You made it here and you aren't on the last element, iterate query to look at next element
                dim_start += 2;
                dim_end += 2;
            }
        }
        cout << "<<<<<<<<<<<<<<<>>>>>>>>>>>>>>>" << endl;
        cout << endl;
    }
    cout << "RECORDS REPORTED: " << counter_ss << endl;
}

/**
 * @brief Main recursive loop that builds the my-tree
 * 
 * @param data - Database of records
 * @return Node* - Created and populated Node
 */
Node* my_insert(vector<vector<double>>* data){

    Node* node = new Node();
    
    if(data->size() <= block_size){
        node->leaf = true;
        node->data = *data;
        return node;
    }
    
    int curr_dim = 0;
    int split_dim;
    double range = -1;
    int mean = -1;
    pair<double,double> p;
    while(curr_dim <= max_dim){
        find_min_and_max(data, &p, curr_dim);
        if((p.second-p.first) > range){
            range = p.second-p.first;
            mean = (p.first+p.second)/2;
            split_dim = curr_dim;
        }
        curr_dim++;
    }

    node->split = mean;
    node->dim = split_dim;

    vector<vector<double>> left;
    vector<vector<double>> right;

    get_data(data, &left, &right, split_dim, node->split);

    node->left = my_insert(&left);
    node->right = my_insert(&right);

    return node;

}

/**
 * @brief Used to call recursive insert. Returns the build my-tree
 * 
 * @param db - Database of records
 * @return Node* - Root node of kd-tree
 */
Node* my_tree(vector<vector<double>>* db){

    max_dim = db->at(0).size()-1;
    return my_insert(db);

}


int main(int argc, char *argv[]){
    
    if(argc < 4){
        cout << "NOT ENOUGH ARGUMENTS" << endl;
        return -1;
    }
    
    int choice = stoi(argv[1]);
    string database = argv[2];
    string queries = argv[3];
    if(argc == 5){
        block_size = stoi(argv[4]);
    }

    if((choice == 1 || choice == 2) && block_size == -1){
        cout << "MUST ENTER A BLOCK SIZE" << endl;
        return -1;
    }
    
    vector<vector<double>> db;
    vector<vector<double>> q;

    cout << "Creating database in memory..." << endl;
    read_db_file(database, &db);
    cout << "Creating queries in memory..." << endl;
    read_q_file(queries, &q);

    clock_t time;
    Node* root;

    switch(choice){
        case 0:
            cout << "Executing queries..." << endl;
            time = clock();
            sequential_search(&db, &q);
            printf("Time taken: %.2f\n", (double)(clock() - time)/CLOCKS_PER_SEC);
            cout << "Sequential search | Queries complete" << endl;
            break;
        case 1:
            cout << "Building Kd-tree..." << endl;
            root = kd_tree(&db);
            cout << "Kd-tree complete" << endl;
            cout << "Executing queries..." << endl;
            time = clock();
            search(root, &q);
            printf("Time taken: %.2f\n", (double)(clock() - time)/CLOCKS_PER_SEC);
            cout << "Kd-tree | Queries complete" << endl;
            break;
        case 2:
            cout << "Building My-Tree..." << endl;
            root = my_tree(&db);
            cout << "My-tree complete" << endl;
            cout << "Executing queries..." << endl;
            time = clock();
            search(root, &q);
            printf("Time taken: %.2f\n", (double)(clock() - time)/CLOCKS_PER_SEC);
            cout << "My-tree | Queries complete" << endl;
            break;
        default:
            cout << "OPTION NOT RECOGNIZED - ENTER A VALID OPTION [0,1,2]" << endl;
    }

    return 0;
}