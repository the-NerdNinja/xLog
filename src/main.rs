mod function;
use rusqlite::Connection;
use function::sector_fn::{add_sector, show_sector};

fn main(){
    let conn = Connection::open("xLog.db")
        .expect("connection failed..");

    add_sector(&conn);
    add_sector(&conn);
    add_sector(&conn);
    add_sector(&conn);

    show_sector(&conn)

}
