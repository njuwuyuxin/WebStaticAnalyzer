#include "checkers/CheckerManager.h"

CheckerManager::CheckerManager(Config* conf){
    configure = conf;
}

void CheckerManager::add_checker(BasicChecker* c){
    checkers.push_back(c);
}

void CheckerManager::check_all(){
    ofstream process_file("time.txt",ios::app);
    if (!process_file.is_open()) {
        cerr << "can't open time.txt\n";
        return;
    }

    auto enable = configure->getOptionBlock("CheckerEnable");

    for(auto checker:checkers){
        if (enable.find(checker->name)->second == "true") {
            process_file << "Starting " + checker->name + " check" << endl;
            clock_t start, end;
            start = clock();

            checker->check();
        
            end = clock();
            unsigned sec = unsigned((end - start) / CLOCKS_PER_SEC);
            unsigned min = sec / 60;
            process_file << "Time: " << min << "min" << sec % 60 << "sec" << endl;
            process_file << "End of " + checker->name + " "
                            "check\n-----------------------------------------------"
                            "------------"
                        << endl;
        }
    }
    process_file.close();
}