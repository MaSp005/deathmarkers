use sqlx::{PgPool, postgres::PgPoolOptions};
use std::env;

pub trait Fetcher {
    fn fetch(&self, query: (&'static str, i64));
}

pub struct DatabaseFetcher {
    pool: PgPool,
}
impl DatabaseFetcher {
    pub async fn new() -> DatabaseFetcher {
        let db_url = env::var("DATABASE_URL").expect("DATABASE_URL must be set");

        let pool = PgPoolOptions::new()
            .connect(&db_url)
            .await
            .expect("Failed to connect to DB");
        sqlx::migrate!()
            .run(&pool)
            .await
            .expect("Migrations failed");

        DatabaseFetcher { pool }
    }
}
impl Fetcher for DatabaseFetcher {
    fn fetch(&self, query: (&'static str, i64)) {
        todo!()
    }
}
