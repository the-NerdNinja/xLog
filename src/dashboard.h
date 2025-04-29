#pragma once

#include <bits/stdc++.h>
#include <sqlite3.h>
#include "utils.h"

extern sqlite3* db;
using namespace std;
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
void view_domain(string choice) {
    // Fetch domains
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, "SELECT id,name FROM domains ORDER BY id", -1, &stmt, nullptr);
    vector<pair<int, string>> domains;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        domains.emplace_back(sqlite3_column_int(stmt, 0), (const char*)sqlite3_column_text(stmt, 1));
    }
    sqlite3_finalize(stmt);

    // Select domain
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


