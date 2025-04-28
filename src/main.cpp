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
// ANSI styles
static constexpr const char* BOLD    = "\033[1m";
static constexpr const char* ITALIC  = "\033[3m";
static constexpr const char* RESET   = "\033[0m";
static constexpr const char* DIM     = "\033[90m";

// Rank labels and colors
enum { ROOKIE, EXPLORER, CRAFTER, STRATEGIST, EXPERT, ARCHITECT, ELITE, MASTER, LEGEND };
static constexpr const char* RANK_NAMES[9] = {
    "Rookie", "Explorer", "Crafter", "Strategist",
    "Expert", "Architect", "Elite", "Master", "Legend"
};
static constexpr const char* COLORS[9] = {
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

// Badge dimensions (odd height) for minimal design
template<int H>
struct BadgeSize { static constexpr int HEIGHT = H; static constexpr int WIDTH = H * 2; };
using Badge = BadgeSize<11>;
static constexpr double XP_MAX = 109500.0;

void clear_screen();

// Pad or truncate to width
static string padRight(const string& s, int width) {
    if ((int)s.size() >= width) return s.substr(0, width);
    return s + string(width - s.size(), ' ');
}

// Minimalistic badge using + - | border and centered text
static vector<string> buildBadge(const string& text, const char* color) {
    vector<string> b(Badge::HEIGHT);
    int w = Badge::WIDTH;
    int innerW = w - 2;
    int midH = Badge::HEIGHT / 2;
    for (int y = 0; y < Badge::HEIGHT; ++y) {
        if (y == 0 || y == Badge::HEIGHT - 1) {
            b[y] = string(color) + "+" + string(innerW, '-') + "+" + RESET;
        } else if (y == midH) {
            string t = text;
            if ((int)t.size() > innerW) t = t.substr(0, innerW);
            int pad = innerW - t.size();
            int lp = pad / 2, rp = pad - lp;
            b[y] = string(color) + "|" + string(lp, ' ') + t + string(rp, ' ') + "|" + RESET;
        } else {
            b[y] = string(color) + "|" + string(innerW, ' ') + "|" + RESET;
        }
    }
    return b;
}

// Build a simple progress bar
static string buildProgressBar(double frac, int len, const char* color) {
    int filled = int(frac * len + 0.5);
    string bar = string(filled, '=') + string(len - filled, '-');
    return string(color) + "[" + bar + "] " + to_string(int(frac * 100)) + "%" + RESET;
}

// Enhanced profile dashboard
void view_profile() {
    clear_screen();
    // Load domain names and XP
    vector<string> domain_names(4);
    double domain_xps[4] = {};
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, "SELECT id,name FROM domains ORDER BY id", -1, &stmt, nullptr);
    int idx = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && idx < 4) {
        int id = sqlite3_column_int(stmt, 0);
        domain_names[idx] = (const char*)sqlite3_column_text(stmt, 1);
        sqlite3_stmt* sum;
        sqlite3_prepare_v2(db, "SELECT COALESCE(SUM(xp),0) FROM elements WHERE domain_id=?", -1, &sum, nullptr);
        sqlite3_bind_int(sum, 1, id);
        if (sqlite3_step(sum) == SQLITE_ROW) domain_xps[idx] = sqlite3_column_double(sum, 0);
        sqlite3_finalize(sum);
        idx++;
    }
    sqlite3_finalize(stmt);

    // Compute profile XP, level, progress fraction
    double prod = 1;
    for (double x : domain_xps) prod *= x;
    double profile_xp = pow(prod, 1.0 / 4);
    double lvlF = sqrt(profile_xp / XP_MAX) * 8.0;
    int lvl = min(8, max(0, int(lvlF)));
    double frac = (lvl < 8 ? lvlF - lvl : 1.0);
    const char* color = COLORS[lvl];
    const char* rank = RANK_NAMES[lvl];

    // Fetch user name and days left
    sqlite3_prepare_v2(db,
        "SELECT name, CAST(julianday(date(created_at,'+4 years'))-julianday('now','localtime') AS INTEGER)"
        " FROM user WHERE id=1", -1, &stmt, nullptr);
    string user;
    int daysLeft = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        user = (const char*)sqlite3_column_text(stmt, 0);
        daysLeft = sqlite3_column_int(stmt, 1);
    }
    sqlite3_finalize(stmt);

    // Build badge and info lines
    auto badge = buildBadge(rank, color);
    vector<string> info(Badge::HEIGHT);
    info[0] = string(color) + BOLD + rank + " " + ITALIC + user + RESET;
    info[1] = string(DIM) + string(31, '-') + RESET;
    info[2] = string("XP      : ") + color + to_string(int(profile_xp)) + RESET;
    info[3] = string("Time    : ") + to_string(daysLeft);
    info[4] = string(BOLD) + "Lvl:" + RESET + " " + buildProgressBar(frac, 20, color);
    for (int i = 0; i < 4; ++i) {
        info[6 + i] = padRight(domain_names[i], 12) + " : " + color + to_string(int(domain_xps[i])) + RESET;
    }

    // Render side-by-side
    for (int y = 0; y < Badge::HEIGHT; ++y) {
        cout << badge[y] << "  " << info[y] << "\n";
    }
    cin.get();
}

// Enhanced domain dashboard
void view_domain() {
    clear_screen();
    // Fetch domains
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, "SELECT id,name FROM domains ORDER BY id", -1, &stmt, nullptr);
    vector<pair<int, string>> domains;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        domains.emplace_back(sqlite3_column_int(stmt, 0), (const char*)sqlite3_column_text(stmt, 1));
    }
    sqlite3_finalize(stmt);

    // Select domain
    cout << "\nEnter domain name: ";
    string choice; getline(cin, choice);
    int did = -1;
    for (auto& d : domains) if (d.second == choice) did = d.first;
    if (did < 0) { cout << "Domain not found.\n"; cin.get(); return; }

    // Fetch user name
    sqlite3_prepare_v2(db, "SELECT name FROM user WHERE id=1", -1, &stmt, nullptr);
    string user;
    if (sqlite3_step(stmt) == SQLITE_ROW) user = (const char*)sqlite3_column_text(stmt, 0);
    sqlite3_finalize(stmt);

    // Fetch elements and total XP
    struct Elem { string name; double xp; bool focus; };
    vector<Elem> elems;
    double totalXP = 0;
    sqlite3_prepare_v2(db, "SELECT name,xp,is_focus FROM elements WHERE domain_id=?", -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, did);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Elem e{(const char*)sqlite3_column_text(stmt, 0), sqlite3_column_double(stmt, 1), sqlite3_column_int(stmt, 2) == 1};
        elems.push_back(e);
        totalXP += e.xp;
    }
    sqlite3_finalize(stmt);
    sort(elems.begin(), elems.end(), [](auto& a, auto& b){ return a.xp > b.xp; });

    // Compute domain progress
    double lvlF = sqrt(totalXP / XP_MAX) * 8.0;
    int lvl = min(8, max(0, int(lvlF)));
    double frac = (lvl < 8 ? lvlF - lvl : 1.0);
    const char* color = COLORS[lvl];

    // Build badge and info
    auto badge = buildBadge(choice, color);
    vector<string> info(Badge::HEIGHT);
    info[0] = string(color) + BOLD + ITALIC + user + RESET;
    info[1] = string(DIM) + string(31, '-') + RESET;
    info[2] = string(color) + BOLD + choice + " : " + to_string(int(totalXP)) + RESET;
    info[4] = string(BOLD) + "Lvl:" + RESET + " " + buildProgressBar(frac, 20, color);
    for (int i = 0; i < 4 && i < (int)elems.size(); ++i) {
        string label = padRight(elems[i].name, 12);
        if (elems[i].focus) label = string(ITALIC) + label + RESET;
        info[6 + i] = label + " : " + color + to_string(int(elems[i].xp)) + RESET;
    }

    // Render side-by-side
    for (int y = 0; y < Badge::HEIGHT; ++y) {
        cout << badge[y] << "  " << info[y] << "\n";
    }
    cin.get();
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
