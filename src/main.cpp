#include <bits/stdc++.h>
#include <sqlite3.h>

using namespace std;
static sqlite3* db;

void clear_screen() {
  std::system("clear");
}

std::string now_str() {
  time_t t = time(nullptr);
  char buf[20];
  strftime(buf, sizeof(buf), "%Y-%m-%d", localtime(&t));
  return std::string(buf);
}

void check(int rc, char* err) {
  if (rc != SQLITE_OK) {
    std::cerr << "SQL error: " << (err ? err : "unknown") << std::endl;
    sqlite3_free(err);
    exit(1);
  }
}

void init_db() {
  char* err = nullptr;
  const char* sql =
    "PRAGMA foreign_keys = ON;"

    "CREATE TABLE IF NOT EXISTS user ("
    " id INTEGER PRIMARY KEY CHECK (id = 1),"
    " name TEXT NOT NULL,"
    " created_at TEXT NOT NULL);"

    "CREATE TABLE IF NOT EXISTS domains ("
    " id INTEGER PRIMARY KEY AUTOINCREMENT,"
    " name TEXT NOT NULL UNIQUE);"

    "CREATE TABLE IF NOT EXISTS elements ("
    " id INTEGER PRIMARY KEY AUTOINCREMENT,"
    " domain_id INTEGER NOT NULL REFERENCES domains(id) ON DELETE CASCADE,"
    " name TEXT NOT NULL,"
    " is_focus INTEGER NOT NULL DEFAULT 0,"
    " xp INTEGER NOT NULL DEFAULT 0,"
    " UNIQUE(domain_id,name));"

    "CREATE UNIQUE INDEX IF NOT EXISTS one_focus_per_domain "
    "ON elements(domain_id) WHERE is_focus = 1;"

    "CREATE TABLE IF NOT EXISTS tasks ("
    " id INTEGER PRIMARY KEY AUTOINCREMENT,"
    " name TEXT NOT NULL UNIQUE,"
    " type TEXT NOT NULL,"
    " frequency INTEGER NOT NULL,"
    " major_elem INTEGER NOT NULL REFERENCES elements(id),"
    " minor_elem INTEGER NOT NULL REFERENCES elements(id),"
    " last_done TEXT,"
    " streak INTEGER NOT NULL DEFAULT 0);";

  int rc = sqlite3_exec(db, sql, nullptr, nullptr, &err);
  check(rc, err);
}

int count_rows(const std::string& tbl) {
  sqlite3_stmt* stmt;
  int count = 0;
  std::string q = "SELECT COUNT(*) FROM " + tbl;
  sqlite3_prepare_v2(db, q.c_str(), -1, &stmt, nullptr);
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    count = sqlite3_column_int(stmt, 0);
  }
  sqlite3_finalize(stmt);
  return count;
}

void prompt_initial_setup() {
  clear_screen();
  std::cout << "-- xLog : Initial Setup --\n";

  std::cout << "Enter your name: ";
  std::string user_name;
  std::getline(std::cin, user_name);

  sqlite3_stmt* stmt;
  sqlite3_prepare_v2(db, "INSERT INTO user(id, name, created_at) VALUES(1, ?, ?)", -1, &stmt, nullptr);
  sqlite3_bind_text(stmt, 1, user_name.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 2, now_str().c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_step(stmt);
  sqlite3_finalize(stmt);

  clear_screen();
  std::cout << "-- Create 5 domains --\n";
  std::string name;
  for (int i = 0; i < 5; ++i) {
    std::cout << "Enter name for domain #" << i+1 << ": ";
    std::getline(std::cin, name);
    sqlite3_prepare_v2(db, "INSERT INTO domains(name) VALUES(?)", -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
  }
}
void view_profile() {
  clear_screen();
  std::cout << "-- Profile Details --\n\n";

  // Rank names & colors
  const char* rank_names[] = {
    "Rookie", "Explorer", "Crafter", "Strategist",
    "Expert", "Architect", "Elite", "Master", "Legend"
  };
  const char* colors[] = {
    "\033[97m", // Rookie White
    "\033[90m", // Explorer Gray
    "\033[93m", // Crafter Yellow
    "\033[91m", // Strategist Orange
    "\033[92m", // Expert Green
    "\033[94m", // Architect Blue
    "\033[95m", // Elite Purple
    "\033[31m", // Master Red
    "\033[30m"  // Legend Black
  };
  const double xp_max = 109500.0;

  double domain_xps[5] = {0};
  sqlite3_stmt* stmt;
  sqlite3_prepare_v2(db, "SELECT id FROM domains ORDER BY id", -1, &stmt, nullptr);
  int idx = 0;
  while (sqlite3_step(stmt) == SQLITE_ROW && idx < 5) {
    int dom_id = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt); // reuse for inner query

    sqlite3_prepare_v2(db,
      "SELECT COALESCE(SUM(xp), 0) "
      "FROM elements WHERE domain_id = ?",
      -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, dom_id);
    sqlite3_step(stmt);
    domain_xps[idx++] = sqlite3_column_double(stmt, 0);
  }
  sqlite3_finalize(stmt);

  double product = 1;
  for (int i = 0; i < 5; ++i) product *= domain_xps[i];
  double profile_xp = pow(product, 1.0/5);

  int profile_level = std::min(8,
    std::max(0, int(sqrt(profile_xp/xp_max)*8.0))
  );

  sqlite3_prepare_v2(db,
  "SELECT name, CAST(julianday(created_at, '+4 years') - julianday('now') AS INTEGER) "
  "AS days_left FROM user WHERE id=1",
  -1, &stmt, nullptr);
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    std::string user_name = (const char*)sqlite3_column_text(stmt, 0);
    int days_left = sqlite3_column_int(stmt, 1);
    std::cout
      << colors[profile_level] << "\033[1m"
      << rank_names[profile_level] << " " << user_name
      << "\033[0m\n"
      << colors[profile_level]
      << std::left << std::setw(12) << "XP" << " : "
      << std::fixed << std::setprecision(0) << profile_xp
      << "\033[0m\n" 
      << "Days Left: "
      << days_left
      << "\n\n"; 
  }
  sqlite3_finalize(stmt);

  sqlite3_prepare_v2(db, "SELECT name FROM domains ORDER BY id", -1, &stmt, nullptr);
  idx = 0;
  while (sqlite3_step(stmt) == SQLITE_ROW && idx < 5) {
    std::string name = (const char*)sqlite3_column_text(stmt, 0);
    double xp = domain_xps[idx++];
    int dom_level = std::min(8,
      std::max(0, int(sqrt(xp/xp_max)*8.0))
    );
    std::cout
      << colors[dom_level] << "\033[1m"
      << std::left << std::setw(12) << name
      << "\033[0m"
      << colors[dom_level]
      << " : " << std::fixed << std::setprecision(0) << xp
      << "\033[0m\n";
  }
  sqlite3_finalize(stmt);

  std::cin.get();
}

int main(int argc, char* argv[]) {
  sqlite3_open("xlog.db", &db);
  init_db();

  if (count_rows("domains") == 0) prompt_initial_setup();
  view_profile();
}

// cmake --build build && ./build/xlog
