#pragma once

#include <bits/stdc++.h>
#include <sqlite3.h>
#include "utils.h"

extern sqlite3* db;
using namespace std;

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

void typePrint(const string& txt, useconds_t delayUs = 30000) {
  for (char c : txt) {
    cout << c << flush;
    usleep(delayUs);
  }
  cout << '\n';
}

void showIntro(const string& name) {
  vector<string> lines = {
    "System: \"Your Soul Chronicle is at 7%. Game Over.\"",
    name + ": \"No... this can't be the end!\"",
    "A blaze of white light tears through the battlefield.",
    "You tumble into a silent chamber before the Aevum Engine.",
    "The lever glows \"-4 Years\".",
    name + ": \"I will grow strong. I will learn. I will find allies who stand beside me.\"",
    name + ": \"And when I face him again... I will win.\"",
    "Your journey begins anew..."
  };

  int i=0;
  for (const auto& line : lines){
    typePrint(line);
    usleep(100000);
    cout<<endl;
    i++; if(i==2) cin.get();

  }
  cin.get();
}

// --- Initial setup ------------------------------------------------------
void prompt_initial_setup() {
  clear_screen();
  cout<<"Enter your name: ";
  string uname; getline(cin,uname);

  sqlite3_stmt* stmt;
  sqlite3_prepare_v2(db, "INSERT INTO user(id,name,created_at) VALUES(1,?,?)", -1, &stmt, nullptr);
  sqlite3_bind_text(stmt,1,uname.c_str(),-1,SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt,2,now_str().c_str(),-1,SQLITE_TRANSIENT);
  sqlite3_step(stmt); sqlite3_finalize(stmt);

  showIntro(uname);
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

  int did_elem = get_element_id_by_name("Discipline");
  sqlite3_prepare_v2(db,
    "INSERT INTO tasks(name,type,frequency,major_elem,minor_elem) VALUES('daily_login','Session',1,?,?)",
    -1,&stmt,nullptr);
  sqlite3_bind_int(stmt,1,did_elem);
  sqlite3_bind_int(stmt,2,did_elem);
  sqlite3_step(stmt); sqlite3_finalize(stmt);
}

