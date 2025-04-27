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

int get_domain_id_by_name(const std::string& name) {
  sqlite3_stmt* stmt;
  sqlite3_prepare_v2(db, "SELECT id FROM domains WHERE name = ?", -1, &stmt, nullptr);
  sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
  int id = -1;
  if (sqlite3_step(stmt) == SQLITE_ROW) id = sqlite3_column_int(stmt, 0);
  sqlite3_finalize(stmt);
  return id;
}

int get_element_id_by_name(const std::string& name) {
  sqlite3_stmt* stmt;
  sqlite3_prepare_v2(db, "SELECT id FROM elements WHERE name = ?", -1, &stmt, nullptr);
  sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
  int id = -1;
  if (sqlite3_step(stmt) == SQLITE_ROW) id = sqlite3_column_int(stmt, 0);
  sqlite3_finalize(stmt);
  return id;
}

int get_task_id_by_name(const std::string& name) {
  sqlite3_stmt* stmt;
  sqlite3_prepare_v2(db, "SELECT id FROM tasks WHERE name = ?", -1, &stmt, nullptr);
  sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
  int id = -1;
  if (sqlite3_step(stmt) == SQLITE_ROW) id = sqlite3_column_int(stmt, 0);
  sqlite3_finalize(stmt);
  return id;
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

  // --- 1) Load and sum each domain's XP from elements.xp ---
  double domain_xps[5] = {0};
  sqlite3_stmt* stmt_dom;
  sqlite3_prepare_v2(db,
    "SELECT id, name FROM domains ORDER BY id",
    -1, &stmt_dom, nullptr);

  int idx = 0;
  while (sqlite3_step(stmt_dom) == SQLITE_ROW && idx < 5) {
    int dom_id = sqlite3_column_int(stmt_dom, 0);

    // sum xp for this domain
    sqlite3_stmt* stmt_sum;
    sqlite3_prepare_v2(db,
      "SELECT COALESCE(SUM(xp), 0) FROM elements WHERE domain_id = ?",
      -1, &stmt_sum, nullptr);
    sqlite3_bind_int(stmt_sum, 1, dom_id);
    if (sqlite3_step(stmt_sum) == SQLITE_ROW) {
      domain_xps[idx] = sqlite3_column_double(stmt_sum, 0);
    }
    sqlite3_finalize(stmt_sum);
    idx++;
  }
  sqlite3_finalize(stmt_dom);

  // --- 2) Compute profile XP (geometric mean) ---
  double product = 1;
  for (int i = 0; i < 5; ++i) product *= domain_xps[i];
  double profile_xp = pow(product, 1.0/5);

  // --- 3) Determine profile level ---
  int profile_level = std::min(8,
    std::max(0, int(sqrt(profile_xp/xp_max)*8.0))
  );

  // --- 4) Show user name, XP & days left ---
  sqlite3_prepare_v2(db,
  "SELECT name, "
  "       CAST( "
  "         julianday(date(created_at, '+4 years'))"
  "         - julianday(date('now', 'localtime'))"
  "       AS INTEGER) AS days_left "
  "  FROM user WHERE id=1",
  -1, &stmt_dom, nullptr);

  if (sqlite3_step(stmt_dom) == SQLITE_ROW) {
    std::string user_name = (const char*)sqlite3_column_text(stmt_dom, 0);
    int days_left = sqlite3_column_int(stmt_dom, 1);

    std::cout
      << colors[profile_level] << "\033[1m"
      << rank_names[profile_level] << " " << user_name
      << "\033[0m\n"
      << colors[profile_level]
      << std::left << std::setw(12) << "XP" << " : "
      << std::fixed << std::setprecision(0) << profile_xp
      << "\033[0m\n"
      << "Days Left: " << days_left << "\n\n";
  }
  sqlite3_finalize(stmt_dom);

  // --- 5) List each domain, color & align ---
  sqlite3_prepare_v2(db, "SELECT name FROM domains ORDER BY id", -1, &stmt_dom, nullptr);
  idx = 0;
  while (sqlite3_step(stmt_dom) == SQLITE_ROW && idx < 5) {
    std::string name = (const char*)sqlite3_column_text(stmt_dom, 0);
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
  sqlite3_finalize(stmt_dom);

  std::cin.get();
}

void add_element() {
  clear_screen();
  std::cout << "-- Add Element --\n";
  std::string domain_name;
  std::cout << "Domain name: ";
  std::getline(std::cin, domain_name);
  int domain_id = get_domain_id_by_name(domain_name);
  if (domain_id == -1) { std::cout << "Domain not found.\n"; std::cin.get(); return; }

  sqlite3_stmt* stmt;
  sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM elements WHERE domain_id = ?", -1, &stmt, nullptr);
  sqlite3_bind_int(stmt,1,domain_id);
  sqlite3_step(stmt);
  int cnt = sqlite3_column_int(stmt,0);
  sqlite3_finalize(stmt);
  if (cnt >= 4) {
    std::cout << "Domain already has 4 elements.\n";
    std::cin.get(); return;
  }

  std::string name;
  std::cout << "Element name: "; std::getline(std::cin,name);
  sqlite3_prepare_v2(db, "INSERT INTO elements(domain_id,name) VALUES(?,?)", -1, &stmt, nullptr);
  sqlite3_bind_int(stmt,1,domain_id);
  sqlite3_bind_text(stmt,2,name.c_str(),-1,SQLITE_TRANSIENT);
  sqlite3_step(stmt);
  sqlite3_finalize(stmt);
}

void add_task() {
  clear_screen();
  std::cout << "-- Add Task --\n";
  std::string name, type, maj_name, min_name;
  int freq;
  std::cout << "Task name: "; std::getline(std::cin,name);
  std::cout << "Type (Quick/Session/Grind): "; std::getline(std::cin,type);
  std::cout << "Frequency (days, 0=one-time): "; std::cin >> freq;
  std::cin.ignore();
  std::cout << "Major element name: "; std::getline(std::cin,maj_name);
  std::cout << "Minor element name: "; std::getline(std::cin,min_name);
  int maj = get_element_id_by_name(maj_name);
  int min = get_element_id_by_name(min_name);
  if (maj == -1 || min == -1) { std::cout << "Element(s) not found.\n"; std::cin.get(); return; }

  sqlite3_stmt* stmt;
  sqlite3_prepare_v2(db, "INSERT INTO tasks(name,type,frequency,major_elem,minor_elem) VALUES(?,?,?,?,?)", -1, &stmt, nullptr);
  sqlite3_bind_text(stmt,1,name.c_str(),-1,SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt,2,type.c_str(),-1,SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt,3,freq);
  sqlite3_bind_int(stmt,4,maj);
  sqlite3_bind_int(stmt,5,min);
  sqlite3_step(stmt);
  sqlite3_finalize(stmt);
}

void view_todays_tasks() {
  clear_screen();
  std::cout << "-- Today's Tasks --\n";
  sqlite3_stmt* stmt;
  sqlite3_prepare_v2(db,
    "SELECT name, type FROM tasks WHERE "
    "  last_done IS NULL"
    "  OR (frequency > 0"
    "      AND date('now','localtime')"
    "          >= date(last_done, '+' || frequency || ' days')"
    "     )",
    -1, &stmt, nullptr);

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    std::cout << "- " << sqlite3_column_text(stmt,0)
              << " (" << sqlite3_column_text(stmt,1) << ")\n";
  }
  sqlite3_finalize(stmt);
  std::cin.get();
}

void complete_task() {
  clear_screen();
  std::cout << "-- Complete Task --\n";
  std::string task_name;
  std::cout << "Task name: ";
  std::getline(std::cin, task_name);
  int task_id = get_task_id_by_name(task_name);
  if (task_id == -1) {
    std::cout << "Task not found.\n";
    std::cin.get();
    return;
  }

  sqlite3_stmt* stmt;
  sqlite3_prepare_v2(db,
    "SELECT type, major_elem, minor_elem, streak FROM tasks WHERE id = ?",
    -1, &stmt, nullptr);
  sqlite3_bind_int(stmt, 1, task_id);
  sqlite3_step(stmt);
  std::string type = (const char*)sqlite3_column_text(stmt, 0);
  int maj = sqlite3_column_int(stmt, 1);
  int min = sqlite3_column_int(stmt, 2);
  int streak = sqlite3_column_int(stmt, 3);
  sqlite3_finalize(stmt);

  int maj_xp = 0, min_xp = 0;
  if (type == "Quick")     { maj_xp = 10;  min_xp = 5;   }
  else if (type == "Session") { maj_xp = 60;  min_xp = 30;  }
  else if (type == "Grind")   { maj_xp = 125; min_xp = 75;  }

  maj_xp += 2 * (streak + 1);
  min_xp += 1 * (streak + 1);

  sqlite3_prepare_v2(db,
    "UPDATE elements SET xp = xp + ? WHERE id = ?",
    -1, &stmt, nullptr);
  sqlite3_bind_int(stmt, 1, maj_xp);
  sqlite3_bind_int(stmt, 2, maj);
  sqlite3_step(stmt);
  sqlite3_finalize(stmt);

  sqlite3_prepare_v2(db,
    "UPDATE elements SET xp = xp + ? WHERE id = ?",
    -1, &stmt, nullptr);
  sqlite3_bind_int(stmt, 1, min_xp);
  sqlite3_bind_int(stmt, 2, min);
  sqlite3_step(stmt);
  sqlite3_finalize(stmt);

  sqlite3_prepare_v2(db,
    "UPDATE tasks SET last_done = ?, streak = streak + 1 WHERE id = ?",
    -1, &stmt, nullptr);
  sqlite3_bind_text(stmt, 1, now_str().c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt, 2, task_id);
  sqlite3_step(stmt);
  sqlite3_finalize(stmt);

  std::cout << "Task completed!\n";
  std::cin.get();
}

void view_domain() {
  clear_screen();
  sqlite3_stmt* stmt;
  sqlite3_prepare_v2(db,
    "SELECT id, name FROM domains ORDER BY id",
    -1, &stmt, nullptr);
  std::cout << "Available domains:\n";
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    std::cout << "- " << (const char*)sqlite3_column_text(stmt, 1) << "\n";
  }
  sqlite3_finalize(stmt);

  std::cout << "\nEnter domain name: ";
  std::string domain_name;
  std::getline(std::cin, domain_name);
  int dom_id = -1;
  sqlite3_prepare_v2(db,
    "SELECT id FROM domains WHERE name = ?",
    -1, &stmt, nullptr);
  sqlite3_bind_text(stmt, 1, domain_name.c_str(), -1, SQLITE_TRANSIENT);
  if (sqlite3_step(stmt) == SQLITE_ROW)
    dom_id = sqlite3_column_int(stmt, 0);
  sqlite3_finalize(stmt);

  if (dom_id == -1) {
    std::cout << "Domain not found.\n";
    std::cin.get();
    return;
  }

  struct Elem { std::string name; double xp; };
  std::vector<Elem> elems;
  double sum_xp = 0.0, xp_max = 109500.0;

  sqlite3_prepare_v2(db,
    "SELECT name, xp FROM elements WHERE domain_id = ?",
    -1, &stmt, nullptr);
  sqlite3_bind_int(stmt, 1, dom_id);
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    Elem e;
    e.name = (const char*)sqlite3_column_text(stmt, 0);
    e.xp   = sqlite3_column_double(stmt, 1);
    sum_xp += e.xp;
    elems.push_back(e);
  }
  sqlite3_finalize(stmt);

  int domain_level = std::min(8,
    std::max(0, int(std::sqrt(sum_xp / xp_max) * 8.0))
  );

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

  std::cout
    << colors[domain_level] << "\033[1m"
    << " " << domain_name
    << " (XP: " << std::fixed << std::setprecision(2) << sum_xp << ")"
    << "\033[0m\n\n";

  for (auto& e : elems) {
    int lvl = std::min(8,
      std::max(0, int(std::sqrt(e.xp / xp_max) * 8.0))
    );
    std::cout
      << colors[lvl] << "\033[1m"
      << std::left << std::setw(12) << e.name
      << "\033[0m"
      << colors[lvl]
      << " : " << std::fixed << std::setprecision(2) << e.xp
      << "\033[0m\n";
  }

  std::cin.get();
}

void menu() {
  while (true) {
    clear_screen();
    std::cout << "== Main Menu ==\n"
              << "1. Add Element\n"
              << "2. Add Task\n"
              << "3. View Today's Tasks\n"
              << "4. Complete Task\n"
              << "5. View Profile Details\n"
              << "6. View Domain Details\n"
              << "0. Exit\n"
              << "> ";
    int ch;
    std::cin >> ch;
    std::cin.ignore();
    if      (ch == 1) add_element();
    else if (ch == 2) add_task();
    else if (ch == 3) view_todays_tasks();
    else if (ch == 4) complete_task();
    else if (ch == 5) view_profile();
    else if (ch == 6) view_domain();
    else break;
  }
}

int main() {
  sqlite3_open("xLog.db", &db);
  init_db();
  if (count_rows("domains") == 0) prompt_initial_setup();
  menu();
  sqlite3_close(db);
  return 0;
}

// cmake --build build && ./build/xlog
