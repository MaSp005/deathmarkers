use std::iter::repeat;
use super::WrappedError;
use super::encode::*;
use super::Fetcher;
use crate::{
    data::{SubmissionDeath, SubmissionMetadata},
    params::{AnalysisParams, ListParams},
};
use async_trait::async_trait;
use bytes::Bytes;
use sqlx::{PgPool, postgres::PgPoolOptions, query, raw_sql};

const SCHEMA: &str = include_str!("../../schema.sql");

pub struct DatabaseFetcher {
    pool: PgPool,
    salt: bool,
}
impl DatabaseFetcher {
    pub async fn new(url: &String, salt: bool) -> Self {
        println!("Connecting to the database...");
        let pool = PgPoolOptions::new()
            .connect(&url)
            .await
            .expect("Failed to connect to DB");
        raw_sql(SCHEMA).execute(&pool).await.expect("Schema failed");
        println!("Connected to the database.");

        Self { pool, salt }
    }

    async fn fetch_list_normal(&self, q: ListParams) -> Result<Bytes, sqlx::Error> {
        let (qs, levelid) = q.query();
        let deaths = query(qs).bind(levelid).fetch_all(&self.pool).await?;
        Ok(create_binary_response_normal(&deaths))
    }

    async fn fetch_list_platformer(&self, q: ListParams) -> Result<Bytes, sqlx::Error> {
        let (qs, levelid) = q.query();
        let deaths = query(qs).bind(levelid).fetch_all(&self.pool).await?;
        Ok(create_binary_response_platformer(&deaths))
    }
}

#[async_trait]
impl Fetcher for DatabaseFetcher {
    async fn fetch_list(&self, q: ListParams) -> Result<Bytes, WrappedError> {
        if q.platformer {
            self.fetch_list_platformer(q)
                .await
                .map_err(|e| WrappedError::Database(e))
        } else {
            self.fetch_list_normal(q)
                .await
                .map_err(|e| WrappedError::Database(e))
        }
    }

    async fn fetch_analysis(&self, q: AnalysisParams) -> Result<Bytes, WrappedError> {
        let (qs, levelid) = q.query();
        let deaths = query(qs)
            .bind(levelid)
            .fetch_all(&self.pool)
            .await
            .map_err(|e| WrappedError::Database(e))?;
        let salt = rand::random_iter::<char>().take(10).collect::<String>();
        Ok(create_binary_response_analysis(
            &deaths,
            if self.salt { Some(&salt) } else { None },
        ))
    }

    async fn submit(
        &self,
        metadata: SubmissionMetadata<String>,
        deaths: Vec<SubmissionDeath>,
    ) -> Result<(), WrappedError> {
        let qs = metadata.get_insert_query();

        let userident: Vec<String> = repeat(metadata.userident.clone())
            .take(deaths.len())
            .collect();
        let levelid: Vec<i32> = repeat(metadata.level_id as i32)
            .take(deaths.len())
            .collect();
        let levelversion: Vec<i16> = repeat(metadata.levelversion as i16)
            .take(deaths.len())
            .collect();
        let practice: Vec<bool> = deaths.iter().map(|d| d.practice).collect();
        let x: Vec<f64> = deaths.iter().map(|d| d.x as f64).collect();
        let y: Vec<f64> = deaths.iter().map(|d| d.y as f64).collect();
        let percentage: Vec<i16> = deaths.iter().map(|d| d.percentage as i16).collect();

        match query(qs)
            .bind(userident)
            .bind(levelid)
            .bind(levelversion)
            .bind(practice)
            .bind(x)
            .bind(y)
            .bind(percentage)
            .execute(&self.pool)
            .await
        {
            Ok(_) => Ok(()),
            Err(e) => Err(WrappedError::Database(e)),
        }
    }
}
