#include <iostream>
#include <sqlite3.h>

int main(int argc, char* argv[]) {
  sqlite3* db;
  int rc = sqlite3_open("xlog.db", &db);
  if (rc) {
    std::cerr << "Err0r: " << sqlite3_errmsg(db) << std::endl;
    return rc;
  }
  std::cout << "Database Online..!" << std::endl;
  sqlite3_close(db);
}

// cmake --build build && ./build/xlog
