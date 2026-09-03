use crate::{
    data::AnalysisDeath,
    params::{AnalysisParams, ListParams},
};
use async_trait::async_trait;
use axum::body::Bytes;
use sqlx::{Error, PgPool, postgres::PgPoolOptions, query};
use std::env;

#[async_trait]
pub trait Fetcher {
    async fn fetch_list(&self, query: ListParams) -> Result<Bytes, Error>;
    async fn fetch_list_platformer(&self, query: ListParams) -> Result<Bytes, Error>;
    async fn fetch_analysis(&self, query: AnalysisParams) -> Result<Bytes, Error>;
    async fn submit(&self, deaths: Vec<AnalysisDeath>) -> Result<(), Error>;
}

pub struct DatabaseFetcher {
    pool: PgPool,
}
impl DatabaseFetcher {
    pub async fn new() -> DatabaseFetcher {
        let db_url = env::var("DATABASE_URL").expect("DATABASE_URL must be set");

        println!("Connecting to the database...");
        let pool = PgPoolOptions::new()
            .connect(&db_url)
            .await
            .expect("Failed to connect to DB");
        sqlx::migrate!()
            .run(&pool)
            .await
            .expect("Migrations failed");
        println!("Connected to the database.");

        DatabaseFetcher { pool }
    }
}
#[async_trait]
impl Fetcher for DatabaseFetcher {
    async fn fetch_list(&self, q: ListParams) -> Result<Bytes, Error> {
        assert_eq!(q.platformer, false);
        let (qs, levelid) = q.query();
        // stream.map(|d| Box::new(NormalDeath {}))
        // self.pool;
        // query(qs).bind(levelid).map(converter(&q)).fetch(&self.pool)
        let deaths = query(qs).bind(levelid).fetch_all(&self.pool).await;
        if let Err(e) = deaths {
            return Err(e);
        };
        // Ok(deaths.iter().flat_map(|r: &PgRow| slice::));
        todo!()
        // query(qs).bind(levelid).try_map(f);
    }
    async fn fetch_list_platformer(&self, q: ListParams) -> Result<Bytes, Error> {
        let (qs, levelid) = q.query();
        todo!()
    }
    async fn fetch_analysis(&self, q: AnalysisParams) -> Result<Bytes, Error> {
        let (qs, levelid) = q.query();
        todo!()
    }
    async fn submit(&self, deaths: Vec<AnalysisDeath>) -> Result<(), Error> {
        todo!()
    }
}
