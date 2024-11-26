#!/bin/bash

THEME="#74C7EC"
DB_FILE="xLog.db"

sqlite3 "$DB_FILE" <<EOF
CREATE TABLE IF NOT EXISTS sectors (
    id INTEGER PRIMARY KEY,
    name TEXT UNIQUE NOT NULL
);
EOF

gum spin --title "setting up $(gum style --foreground "$THEME" "xLog")" -- sleep 1.25
echo "$(gum style --foreground "$THEME" "xLog") successfully installed."
