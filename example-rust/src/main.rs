extern crate serde_json;

use serde_json::json;


fn main() {
    let value = json!({
        "hello": true
    });
    println!("{:?}", value);
}
