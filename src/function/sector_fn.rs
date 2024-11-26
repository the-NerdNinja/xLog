use super::io::promt;
use rusqlite::{Connection, params};

#[derive(Debug)]
pub struct Sector {
    pub id: Option<u32>,
    pub name: String,
}

pub fn add_sector(conn: &Connection){
    let sect = promt("Enter Sector Name");
    
    conn.execute(
        "INSERT INTO sectors (name) VALUES (?1)",
        params![sect],
    ).expect("Could not insert sector");
}

pub fn show_sector(conn: &Connection) {
    let mut stmt = conn.prepare("SELECT id, name FROM sectors")
        .expect("failed query");

    let sector_iter = stmt.query_map([], |row| {
        Ok(Sector {
            id: row.get(0)?,
            name: row.get(1)?,
        })
    }).expect("row failed");

    println!("Sectors:");

    for sector in sector_iter {
        let sector = sector.expect("Failed to read sector");
        println!("ID: {}, Name: {}", sector.id.unwrap(), sector.name);
    }
}
