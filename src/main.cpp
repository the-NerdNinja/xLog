#include <bits/stdc++.h>
#include <sqlite3.h>
#include "init.h"
#include "dashboard.h"

using namespace std;
sqlite3* db = nullptr;


// --- Task operations ----------------------------------------------------
void add_task() {
  clear_screen(); cout<<"-- Add Task --\n";
  cout<<"Task name: "; string name; getline(cin,name);
  cout<<"Type (quick/session/grind): "; string type; getline(cin,type);
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

void delete_task(string name) {
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
  if(type=="quick"){ base_maj=10;base_min=5;} 
  else if(type=="session"){ base_maj=60;base_min=30;} 
  else if(type=="grind"){ base_maj=125;base_min=75;}

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

void complete_task(string name) {
  int tid=get_task_id_by_name(name);
  if(tid<0){ cout<<"Not found.\n";cin.get();return; }
  complete_task_by_id(tid);
  cout<<"Task completed!\n";
}

void grant_base_xp(const std::string& type, string maj_ele, string min_ele) {
  int maj_id = get_element_id_by_name(maj_ele);
  int min_id = get_element_id_by_name(min_ele);

  int base_maj = 0, base_min = 0;
  if      (type == "quick")   { base_maj = 10;  base_min = 5;  }
  else if (type == "session") { base_maj = 60;  base_min = 30; }
  else if (type == "grind")   { base_maj = 125; base_min = 75; }

  sqlite3_stmt* stmt;
  sqlite3_prepare_v2(db,
  "SELECT is_focus FROM elements WHERE id = ?", -1, &stmt, nullptr);
  sqlite3_bind_int(stmt, 1, maj_id);
  bool focus = (sqlite3_step(stmt) == SQLITE_ROW && sqlite3_column_int(stmt, 0) == 1);
  sqlite3_finalize(stmt);
  if(focus){ base_maj += (base_maj/10); base_min += (base_min/10);}

  // update major element
  sqlite3_prepare_v2(db,
    "UPDATE elements SET xp = xp + ? WHERE id = ?", -1, &stmt, nullptr);
  sqlite3_bind_int(stmt, 1, base_maj);
  sqlite3_bind_int(stmt, 2, maj_id);
  sqlite3_step(stmt);
  sqlite3_finalize(stmt);
  // update minor element
  sqlite3_prepare_v2(db,
    "UPDATE elements SET xp = xp + ? WHERE id = ?", -1, &stmt, nullptr);
  sqlite3_bind_int(stmt, 1, base_min);
  sqlite3_bind_int(stmt, 2, min_id);
  sqlite3_step(stmt);
  sqlite3_finalize(stmt);
}

void make_focus(string elem_name) {
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

// --- Enable Task --------------------------------------------------------
void enable_task(string name) {
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
void disable_task(string name) {
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
static void usage() {
  std::cout
    << "Usage: xlog <command> [args]\n"
    << "Commands:\n"
    << "  profile            (default if no args)\n"
    << "  today              Show today's tasks\n"
    << "  create             Add a new task\n"
    << "  delete <task_name>   Delete task by ID\n"
    << "  list               List all tasks\n"
    << "  enable <task_name>   Enable a task\n"
    << "  pause  <task_name>   Disable (pause) a task\n"
    << "  done   <name> [<name>...] Mark tasks done\n"
    << "  quick|session|grind <ele1> <ele2>  Grant quick XP\n"
    << "  info   <domain>    Show domain dashboard\n"
    << "  focus  <domain>    Set focus element\n";
}

int main(int argc, char* argv[]) {
  const char *home = getenv("HOME");
  char db_path[PATH_MAX];
  snprintf(db_path, sizeof(db_path), "%s/xLog/xLog.db", home);

  if (sqlite3_open(db_path, &db) != SQLITE_OK) {
    cerr<<"Error creating DB"<<endl;
    return -1;
  }

  init_db();

  if (get_int("SELECT COUNT(*) FROM domains") == 0)
    prompt_initial_setup();

  log_today_xp();
  if (argc == 1) {
    view_profile();
    sqlite3_close(db);
    return 0;
  }
  std::string cmd = argv[1];
  if (cmd == "profile") {
    view_profile();
  }
  else if (cmd == "today") {
    view_todays_tasks();
  }
  else if (cmd == "create") {
    add_task();
  }
  else if (cmd == "delete" && argc == 3) {
    delete_task(argv[2]);
  }
  else if (cmd == "list") {
    view_all_tasks();
  }
  else if (cmd == "enable" && argc == 3) {
    enable_task(argv[2]);
  }
  else if (cmd == "pause" && argc == 3) {
    disable_task(argv[2]);
  }
  else if (cmd == "done" && argc >= 3) {
    for (int i = 2; i < argc; ++i)
      complete_task(argv[i]);
  }
  else if (cmd == "info" && argc == 3) {
    view_domain(argv[2]);
  }
  else if (cmd == "focus" && argc == 3) {
    make_focus(argv[2]);
  }
  else if ((cmd == "Quick" or cmd == "Session" or cmd == "Grind") and argc==4){
    grant_base_xp(cmd, argv[2], argv[3]);
  }
  else {
    usage();
  }

  sqlite3_close(db);
  return 0;
}
// cmake --build build && ./build/xlog
