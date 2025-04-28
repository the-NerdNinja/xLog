#include <bits/stdc++.h>
#include <sqlite3.h>

using namespace std;
static sqlite3* db;

void clear_screen() {
  system("clear");
}

string now_str() {
  time_t t = time(nullptr);
  char buf[20];
  strftime(buf, sizeof(buf), "%Y-%m-%d", localtime(&t));
  return string(buf);
}

void check(int rc, char* err) {
  if (rc != SQLITE_OK) {
    cerr << "SQL error: " << (err ? err : "unknown") << endl;
    sqlite3_free(err);
    exit(1);
  }
}

// --- DB initialization --------------------------------------------------
void init_db() {
  char* err = nullptr;
  const char* sql =
    "PRAGMA foreign_keys = ON;"
    "CREATE TABLE IF NOT EXISTS user ("
    " id INTEGER PRIMARY KEY CHECK(id=1),"
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
    " UNIQUE(domain_id, name));"

    "CREATE TABLE IF NOT EXISTS tasks ("
    " id INTEGER PRIMARY KEY AUTOINCREMENT,"
    " name TEXT NOT NULL UNIQUE,"
    " type TEXT NOT NULL,"
    " frequency INTEGER NOT NULL,"
    " major_elem INTEGER NOT NULL REFERENCES elements(id),"
    " minor_elem INTEGER NOT NULL REFERENCES elements(id),"
    " last_done TEXT,"
    " streak INTEGER NOT NULL DEFAULT 0,"
    " active INTEGER NOT NULL DEFAULT 1);"

    "CREATE TABLE IF NOT EXISTS xp_log ("
    " id INTEGER PRIMARY KEY AUTOINCREMENT,"
    " date TEXT NOT NULL UNIQUE,"
    " profile_xp REAL NOT NULL,"
    " domain1_xp REAL NOT NULL,"
    " domain2_xp REAL NOT NULL,"
    " domain3_xp REAL NOT NULL,"
    " domain4_xp REAL NOT NULL);";

  int rc = sqlite3_exec(db, sql, nullptr, nullptr, &err);
  check(rc, err);
}

// --- Helpers ------------------------------------------------------------
int get_int(const string& q) {
  sqlite3_stmt* stmt;
  sqlite3_prepare_v2(db, q.c_str(), -1, &stmt, nullptr);
  int val = -1;
  if (sqlite3_step(stmt) == SQLITE_ROW) val = sqlite3_column_int(stmt, 0);
  sqlite3_finalize(stmt);
  return val;
}

double get_double(const string& q) {
  sqlite3_stmt* stmt;
  sqlite3_prepare_v2(db, q.c_str(), -1, &stmt, nullptr);
  double val = 0;
  if (sqlite3_step(stmt) == SQLITE_ROW) val = sqlite3_column_double(stmt, 0);
  sqlite3_finalize(stmt);
  return val;
}

int get_domain_id_by_name(const string& name) {
  sqlite3_stmt* stmt;
  sqlite3_prepare_v2(db, "SELECT id FROM domains WHERE name = ?", -1, &stmt, nullptr);
  sqlite3_bind_text(stmt,1,name.c_str(),-1,SQLITE_TRANSIENT);
  int id = -1;
  if (sqlite3_step(stmt) == SQLITE_ROW) id = sqlite3_column_int(stmt, 0);
  sqlite3_finalize(stmt);
  return id;
}

int get_element_id_by_name(const string& name) {
  sqlite3_stmt* stmt;
  sqlite3_prepare_v2(db, "SELECT id FROM elements WHERE name = ?", -1, &stmt, nullptr);
  sqlite3_bind_text(stmt,1,name.c_str(),-1,SQLITE_TRANSIENT);
  int id = -1;
  if (sqlite3_step(stmt) == SQLITE_ROW) id = sqlite3_column_int(stmt, 0);
  sqlite3_finalize(stmt);
  return id;
}

int get_task_id_by_name(const string& name) {
  sqlite3_stmt* stmt;
  sqlite3_prepare_v2(db, "SELECT id FROM tasks WHERE name = ?", -1, &stmt, nullptr);
  sqlite3_bind_text(stmt,1,name.c_str(),-1,SQLITE_TRANSIENT);
  int id = -1;
  if (sqlite3_step(stmt) == SQLITE_ROW) id = sqlite3_column_int(stmt, 0);
  sqlite3_finalize(stmt);
  return id;
}

string get_task_last_done(int tid) {
  sqlite3_stmt* stmt;
  sqlite3_prepare_v2(db, "SELECT last_done FROM tasks WHERE id = ?", -1, &stmt, nullptr);
  sqlite3_bind_int(stmt,1,tid);
  string s;
  if (sqlite3_step(stmt)==SQLITE_ROW && sqlite3_column_text(stmt,0))
    s = (const char*)sqlite3_column_text(stmt,0);
  sqlite3_finalize(stmt);
  return s;
}

vector<double> fetch_domain_xps() {
  vector<double> v(4);
  sqlite3_stmt* stmt;
  sqlite3_prepare_v2(db, "SELECT id FROM domains ORDER BY id LIMIT 4", -1, &stmt, nullptr);
  int i=0;
  while (sqlite3_step(stmt)==SQLITE_ROW && i<4) {
    int did = sqlite3_column_int(stmt,0);
    v[i++] = get_double("SELECT COALESCE(SUM(xp),0) FROM elements WHERE domain_id = " + to_string(did));
  }
  sqlite3_finalize(stmt);
  return v;
}

void insert_xp_log(const vector<double>& dx, double px) {
  sqlite3_stmt* stmt;
  sqlite3_prepare_v2(db,
    "INSERT INTO xp_log(date,profile_xp,domain1_xp,domain2_xp,domain3_xp,domain4_xp)"
    " VALUES(?,?,?,?,?,?)", -1, &stmt, nullptr);
  sqlite3_bind_text(stmt,1,now_str().c_str(),-1,SQLITE_TRANSIENT);
  sqlite3_bind_double(stmt,2,px);
  for (int i=0;i<4;i++) sqlite3_bind_double(stmt,3+i,dx[i]);
  sqlite3_step(stmt);
  sqlite3_finalize(stmt);
}

// --- Initial setup ------------------------------------------------------
void prompt_initial_setup() {
  clear_screen();
  cout<<"-- xLog : Initial Setup --\n";
  cout<<"Enter your name: ";
  string uname; getline(cin,uname);
  sqlite3_stmt* stmt;
  sqlite3_prepare_v2(db, "INSERT INTO user(id,name,created_at) VALUES(1,?,?)", -1, &stmt, nullptr);
  sqlite3_bind_text(stmt,1,uname.c_str(),-1,SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt,2,now_str().c_str(),-1,SQLITE_TRANSIENT);
  sqlite3_step(stmt); sqlite3_finalize(stmt);

  clear_screen(); cout<<"-- Create 4 domains & elements --\n";
  for (int d=0; d<4; ++d) {
    cout<<"Domain #"<<d+1<<" name: ";
    string dn; getline(cin,dn);
    sqlite3_prepare_v2(db, "INSERT INTO domains(name) VALUES(?)", -1, &stmt, nullptr);
    sqlite3_bind_text(stmt,1,dn.c_str(),-1,SQLITE_TRANSIENT);
    sqlite3_step(stmt); sqlite3_finalize(stmt);
    int did = get_domain_id_by_name(dn);
    for (int e=0; e<4; ++e) {
      cout<<"  Element #"<<e+1<<" for '"<<dn<<"': ";
      string en; getline(cin,en);
      sqlite3_prepare_v2(db,
        "INSERT INTO elements(domain_id,name) VALUES(?,?)", -1, &stmt, nullptr);
      sqlite3_bind_int(stmt,1,did);
      sqlite3_bind_text(stmt,2,en.c_str(),-1,SQLITE_TRANSIENT);
      sqlite3_step(stmt); sqlite3_finalize(stmt);
    }
  }

  int did_elem = get_element_id_by_name("discipline");
  sqlite3_prepare_v2(db,
    "INSERT INTO tasks(name,type,frequency,major_elem,minor_elem) VALUES('daily_login','Session',1,?,?)",
    -1,&stmt,nullptr);
  sqlite3_bind_int(stmt,1,did_elem);
  sqlite3_bind_int(stmt,2,did_elem);
  sqlite3_step(stmt); sqlite3_finalize(stmt);
}

// --- Task operations ----------------------------------------------------
void add_task() {
  clear_screen(); cout<<"-- Add Task --\n";
  cout<<"Task name: "; string name; getline(cin,name);
  cout<<"Type (Quick/Session/Grind): "; string type; getline(cin,type);
  cout<<"Frequency (days, 0=one-time): "; int freq; cin>>freq; cin.ignore();
  cout<<"Major element name: "; string maj; getline(cin,maj);
  cout<<"Minor element name: "; string minor_elem_name; getline(cin,minor_elem_name);
  int mi=get_element_id_by_name(maj), mn=get_element_id_by_name(minor_elem_name);
  if(mi<0||mn<0){ cout<<"Element not found.\n"; cin.get(); return;}  
  sqlite3_stmt* stmt;
  sqlite3_prepare_v2(db,
    "INSERT INTO tasks(name,type,frequency,major_elem,minor_elem)"
    " VALUES(?,?,?,?,?)", -1, &stmt, nullptr);
  sqlite3_bind_text(stmt,1,name.c_str(),-1,SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt,2,type.c_str(),-1,SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt,3,freq);
  sqlite3_bind_int(stmt,4,mi);
  sqlite3_bind_int(stmt,5,mn);
  sqlite3_step(stmt); sqlite3_finalize(stmt);
}

void delete_task() {
  clear_screen(); cout<<"-- Delete Task --\n";
  cout<<"Task name: "; string name; getline(cin,name);
  int tid=get_task_id_by_name(name);
  if(tid<0){ cout<<"Not found.\n"; cin.get(); return; }
  sqlite3_stmt* stmt;
  sqlite3_prepare_v2(db, "DELETE FROM tasks WHERE id=?", -1, &stmt, nullptr);
  sqlite3_bind_int(stmt,1,tid);
  sqlite3_step(stmt); sqlite3_finalize(stmt);
  cout<<"Deleted.\n"; cin.get();
}

void view_todays_tasks() {
  clear_screen(); cout<<"-- Today's Tasks --\n";
  sqlite3_stmt* stmt;
  sqlite3_prepare_v2(db,
    "SELECT name,type FROM tasks WHERE active=1 AND "
    "(last_done IS NULL OR (frequency>0 AND date('now','localtime')>=date(last_done,'+'||frequency||' days')))"
    , -1, &stmt, nullptr);
  while(sqlite3_step(stmt)==SQLITE_ROW)
    cout<<"- "<<(const char*)sqlite3_column_text(stmt,0)
        <<" ("<<(const char*)sqlite3_column_text(stmt,1)<<")\n";
  sqlite3_finalize(stmt);
  cin.get();
}

void complete_task_by_id(int tid) {
  sqlite3_stmt* stmt;
  sqlite3_prepare_v2(db,
    "SELECT type,major_elem,minor_elem,streak,frequency,last_done"
    " FROM tasks WHERE id=?", -1, &stmt, nullptr);
  sqlite3_bind_int(stmt,1,tid); sqlite3_step(stmt);
  string type=(char*)sqlite3_column_text(stmt,0);
  int maj=sqlite3_column_int(stmt,1);
  int minr=sqlite3_column_int(stmt,2);
  int streak=sqlite3_column_int(stmt,3);
  int freq=sqlite3_column_int(stmt,4);
  string last = (char*)sqlite3_column_text(stmt,5) ?: string();
  sqlite3_finalize(stmt);

  int base_maj=0, base_min=0;
  if(type=="Quick"){ base_maj=10;base_min=5;} 
  else if(type=="Session"){ base_maj=60;base_min=30;} 
  else if(type=="Grind"){ base_maj=125;base_min=75;}

  double maj_xp=base_maj, min_xp=base_min;
  sqlite3_prepare_v2(db,
  "SELECT is_focus FROM elements WHERE id = ?", -1, &stmt, nullptr);
  sqlite3_bind_int(stmt, 1, maj);
  bool focus = (sqlite3_step(stmt) == SQLITE_ROW && sqlite3_column_int(stmt, 0) == 1);
  sqlite3_finalize(stmt);
  if(focus){ maj_xp*=1.1; min_xp*=1.1; }

  int pct=std::min(streak,20);
  maj_xp*=(1+pct/100.0); min_xp*=(1+pct/100.0);

  if(!last.empty()&&freq>0) {
    sqlite3_prepare_v2(db,
      "SELECT date(?, '+'||?||' days')", -1, &stmt, nullptr);
    sqlite3_bind_text(stmt,1,last.c_str(),-1,SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt,2,freq);
    if(sqlite3_step(stmt)==SQLITE_ROW) {
      string due=(char*)sqlite3_column_text(stmt,0);
      if(now_str()>due) { maj_xp*=0.6; min_xp*=0.6; }
    }
    sqlite3_finalize(stmt);
  }

  int imaj=int(round(maj_xp)), imin=int(round(min_xp));
  sqlite3_prepare_v2(db, "UPDATE elements SET xp=xp+? WHERE id=?", -1, &stmt, nullptr);
  sqlite3_bind_int(stmt,1,imaj); sqlite3_bind_int(stmt,2,maj);
  sqlite3_step(stmt); sqlite3_finalize(stmt);
  sqlite3_prepare_v2(db, "UPDATE elements SET xp=xp+? WHERE id=?", -1, &stmt, nullptr);
  sqlite3_bind_int(stmt,1,imin); sqlite3_bind_int(stmt,2,minr);
  sqlite3_step(stmt); sqlite3_finalize(stmt);

  sqlite3_prepare_v2(db,
    "UPDATE tasks SET last_done=?,streak=streak+1 WHERE id=?", -1, &stmt, nullptr);
  sqlite3_bind_text(stmt,1,now_str().c_str(),-1,SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt,2,tid);
  sqlite3_step(stmt); sqlite3_finalize(stmt);
}

void complete_task() {
  clear_screen(); cout<<"-- Complete Task --\n";
  cout<<"Task name: "; string name; getline(cin,name);
  int tid=get_task_id_by_name(name);
  if(tid<0){ cout<<"Not found.\n";cin.get();return; }
  complete_task_by_id(tid);
  cout<<"Task completed!\n"; cin.get();
}

void make_focus() {
  clear_screen();
  cout << "-- Make Focus Element --\n";
  cout << "Element name: ";
  string elem_name;
  getline(cin, elem_name);

  int eid = get_element_id_by_name(elem_name);
  if (eid < 0) {
    cout << "Element not found.\n"; cin.get(); return;
  }

  // Find domain_id of this element
  sqlite3_stmt* stmt;
  int dom_id = -1;
  sqlite3_prepare_v2(db, "SELECT domain_id FROM elements WHERE id = ?", -1, &stmt, nullptr);
  sqlite3_bind_int(stmt, 1, eid);
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    dom_id = sqlite3_column_int(stmt, 0);
  }
  sqlite3_finalize(stmt);

  if (dom_id == -1) {
    cout << "Domain not found.\n"; cin.get(); return;
  }

  // Unset previous focus in this domain
  sqlite3_prepare_v2(db, "UPDATE elements SET is_focus = 0 WHERE domain_id = ?", -1, &stmt, nullptr);
  sqlite3_bind_int(stmt, 1, dom_id);
  sqlite3_step(stmt);
  sqlite3_finalize(stmt);

  // Set this element as focus
  sqlite3_prepare_v2(db, "UPDATE elements SET is_focus = 1 WHERE id = ?", -1, &stmt, nullptr);
  sqlite3_bind_int(stmt, 1, eid);
  sqlite3_step(stmt);
  sqlite3_finalize(stmt);

  cout << "Focus updated successfully.\n";
  cin.get();
}


// --- Daily log ----------------------------------------------------------
void log_today_xp() {
  string today = now_str();
  sqlite3_stmt* stmt;
  sqlite3_prepare_v2(db, "SELECT 1 FROM xp_log WHERE date = ?", -1, &stmt, nullptr);
  sqlite3_bind_text(stmt,1,today.c_str(),-1,SQLITE_TRANSIENT);
  if(sqlite3_step(stmt)==SQLITE_ROW){ sqlite3_finalize(stmt); return; }
  sqlite3_finalize(stmt);

  int dlt = get_task_id_by_name("daily_login");
  if(dlt>=0) complete_task_by_id(dlt);

  auto dx = fetch_domain_xps();
  double prod=1;
  const double xp_max=109500.0;
  for(double x:dx) prod*=x;
  double px = pow(prod,1.0/4);
  insert_xp_log(dx,px);
}

// --- Profile & domain view ---------------------------------------------
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
  double domain_xps[4] = {0};
  sqlite3_stmt* stmt_dom;
  sqlite3_prepare_v2(db,
    "SELECT id, name FROM domains ORDER BY id",
    -1, &stmt_dom, nullptr);

  int idx = 0;
  while (sqlite3_step(stmt_dom) == SQLITE_ROW && idx < 4) {
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
  for (int i = 0; i < 4; ++i) product *= domain_xps[i];
  double profile_xp = pow(product, 1.0/4);

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
  while (sqlite3_step(stmt_dom) == SQLITE_ROW && idx < 4) {
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

  struct Elem { std::string name; double xp; bool is_focus; };
  std::vector<Elem> elems;
  double sum_xp = 0.0, xp_max = 109500.0;

  sqlite3_prepare_v2(db,
    "SELECT name, xp, is_focus FROM elements WHERE domain_id = ?",
    -1, &stmt, nullptr);
  sqlite3_bind_int(stmt, 1, dom_id);
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    Elem e;
    e.name = (const char*)sqlite3_column_text(stmt, 0);
    e.xp = sqlite3_column_double(stmt, 1);
    e.is_focus = sqlite3_column_int(stmt, 2) == 1;
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
    std::cout << colors[lvl] << "\033[1m";
    if (e.is_focus)
      std::cout << "\033[4m"; // underline
    std::cout << std::left << std::setw(12) << e.name << "\033[0m";
    std::cout << colors[lvl]
      << " : " << std::fixed << std::setprecision(2) << e.xp
      << "\033[0m\n";
  }

  std::cin.get();
}

// --- Enable Task --------------------------------------------------------
void enable_task() {
  clear_screen();
  cout << "-- Enable Task --\n";
  cout << "Task name: ";
  string name; getline(cin, name);
  int tid = get_task_id_by_name(name);
  if (tid < 0) {
    cout << "Task not found.\n";
  } else {
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db,
      "UPDATE tasks SET active = 1 WHERE id = ?",
      -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, tid);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    cout << "Task enabled.\n";
  }
  cin.get();
}

// --- Disable Task -------------------------------------------------------
void disable_task() {
  clear_screen();
  cout << "-- Disable Task --\n";
  cout << "Task name: ";
  string name; getline(cin, name);
  int tid = get_task_id_by_name(name);
  if (tid < 0) {
    cout << "Task not found.\n";
  } else {
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db,
      "UPDATE tasks SET active = 0 WHERE id = ?",
      -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, tid);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    cout << "Task disabled.\n";
  }
  cin.get();
}

// --- View All Tasks -----------------------------------------------------
void view_all_tasks() {
  clear_screen();
  cout << "-- All Tasks --\n";
  sqlite3_stmt* stmt;
  sqlite3_prepare_v2(db,
    "SELECT name, type, frequency, last_done, streak, active "
    "FROM tasks ORDER BY id",
    -1, &stmt, nullptr);
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    string name      = (const char*)sqlite3_column_text(stmt, 0);
    string type      = (const char*)sqlite3_column_text(stmt, 1);
    int freq         = sqlite3_column_int(stmt, 2);
    const char* ld   = (const char*)sqlite3_column_text(stmt, 3);
    int streak       = sqlite3_column_int(stmt, 4);
    int active       = sqlite3_column_int(stmt, 5);
    cout << "- " << name
         << " [" << type << "]"
         << " freq=" << freq
         << " last_done=" << (ld ? ld : "never")
         << " streak=" << streak
         << " " << (active ? "ENABLED" : "DISABLED")
         << "\n";
  }
  sqlite3_finalize(stmt);
  cin.get();
}

// ---  Menu -------------------------------------------------------
void menu() {
  while (true) {
    clear_screen();
    cout << "== Main Menu ==\n"
         << "1. Add Task\n"
         << "2. Delete Task\n"
         << "3. View Today's Tasks\n"
         << "4. Complete Task\n"
         << "5. View Profile\n"
         << "6. View Domain Details\n"
         << "7. Make Focus Element\n"
         << "8. View All Tasks\n"
         << "9. Enable Task\n"
         << "10. Disable Task\n"
         << "0. Exit\n> ";
    int ch; cin >> ch; cin.ignore();
    if      (ch == 1)  add_task();
    else if (ch == 2)  delete_task();
    else if (ch == 3)  view_todays_tasks();
    else if (ch == 4)  complete_task();
    else if (ch == 5)  view_profile();
    else if (ch == 6)  view_domain();
    else if (ch == 7)  make_focus();
    else if (ch == 8)  view_all_tasks();
    else if (ch == 9)  enable_task();
    else if (ch == 10) disable_task();
    else break;
  }
}

int main() {
  sqlite3_open("xLog.db", &db);
  init_db();
  if(get_int("SELECT COUNT(*) FROM domains") == 0) prompt_initial_setup();
  log_today_xp();
  menu();
  sqlite3_close(db);
  return 0;
}

// cmake --build build && ./build/xlog
