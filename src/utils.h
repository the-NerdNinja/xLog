#pragma once

#include <bits/stdc++.h>
#include <sqlite3.h>
using namespace std;

extern sqlite3* db;

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


