use crate::{
    data::{SubmissionDeath, SubmissionMetadata},
    digest::sha1_digest,
    params::{AnalysisParams, ListParams},
};
use async_trait::async_trait;
use bytes::{BufMut, Bytes, BytesMut};
use memcache::{Client as MemcacheClient, FromMemcacheValue, MemcacheError, ToMemcacheValue};
use sqlx::{
    PgPool, Row,
    postgres::{PgPoolOptions, PgRow},
    query, raw_sql,
};
use std::{io::Write, iter::repeat, ops::Deref, time::Duration};

const MEMCACHE_EXPIRATION: Duration = Duration::from_mins(10);
const SCHEMA: &str = include_str!("../schema.sql");

#[async_trait]
pub trait Fetcher {
    async fn fetch_list(&self, query: ListParams) -> Result<Bytes, WrappedError>;
    async fn fetch_analysis(&self, query: AnalysisParams) -> Result<Bytes, WrappedError>;
    async fn submit(
        &self,
        metadata: SubmissionMetadata<String>,
        deaths: Vec<SubmissionDeath>,
    ) -> Result<(), WrappedError>;
}

pub struct DatabaseFetcher {
    pool: PgPool,
}
impl DatabaseFetcher {
    pub async fn new(url: &String) -> Self {
        println!("Connecting to the database...");
        let pool = PgPoolOptions::new()
            .connect(&url)
            .await
            .expect("Failed to connect to DB");
        raw_sql(SCHEMA).execute(&pool).await.expect("Schema failed");
        println!("Connected to the database.");

        Self { pool }
    }

    async fn fetch_list_normal(&self, q: ListParams) -> Result<Bytes, sqlx::Error> {
        // x, y, percentage
        const ITEM_LENGTH: usize = 4 + 4 + 2;
        assert_eq!(q.platformer, false);
        let (qs, levelid) = q.query();
        let deaths = query(qs).bind(levelid).fetch_all(&self.pool).await?;
        let mut bytes = BytesMut::with_capacity(deaths.len() * ITEM_LENGTH + 1);
        bytes.put_u8(1);
        for death in deaths {
            bytes.put_f32(death.get::<f64, usize>(0) as f32); // x
            bytes.put_f32(death.get::<f64, usize>(1) as f32); // y
            bytes.put_u16(death.get::<i16, usize>(2) as u16); // percentage
        }
        Ok(bytes.freeze())
    }

    async fn fetch_list_platformer(&self, q: ListParams) -> Result<Bytes, sqlx::Error> {
        // x, y
        const ITEM_LENGTH: usize = 4 + 4;
        assert_eq!(q.platformer, true);
        let (qs, levelid) = q.query();
        let deaths = query(qs).bind(levelid).fetch_all(&self.pool).await?;
        let mut bytes = BytesMut::with_capacity(deaths.len() * ITEM_LENGTH + 1);
        bytes.put_u8(1);
        for death in deaths {
            bytes.put_f32(death.get::<f64, usize>(0) as f32); // x
            bytes.put_f32(death.get::<f64, usize>(1) as f32); // y
        }
        Ok(bytes.freeze())
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
        // userident, levelversion, practice, x, y, percentage
        const ITEM_LENGTH: usize = 20 + 2 + 1 + 4 + 4 + 2;
        let (qs, levelid) = q.query();
        let deaths = query(qs)
            .bind(levelid)
            .fetch_all(&self.pool)
            .await
            .map_err(|e| WrappedError::Database(e))?;
        let salt = rand::random_iter::<char>().take(10).collect::<String>();
        let mut bytes = BytesMut::with_capacity(deaths.len() * ITEM_LENGTH + 1);
        bytes.put_u8(1);
        for death in deaths {
            let userident: String = death.get(0);
            let salted_ui = sha1_digest(&format!("{userident}_{salt}"));
            bytes.put_slice(&salted_ui); // userident
            bytes.put_u16(death.get::<i16, usize>(1) as u16); // levelversion
            bytes.put_u8(if death.get::<bool, usize>(2) {
                1u8
            } else {
                0u8
            }); // practice
            bytes.put_f32(death.get::<f64, usize>(3) as f32); // x
            bytes.put_f32(death.get::<f64, usize>(4) as f32); // y
            bytes.put_u16(death.get::<i16, usize>(5) as u16); // percentage
        }
        Ok(bytes.freeze())
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

pub struct MemcachedFetcher {
    memcached: MemcacheClient,
    upstream: DatabaseFetcher,
}
impl MemcachedFetcher {
    pub fn new(mc_url: String, upstream: DatabaseFetcher) -> Self {
        println!("Connecting to memcached...");
        let mc = memcache::connect(vec![mc_url])
            .expect("Failed to connect to Memcached. Is the URL valid?");
        println!("Connected to memcached.");
        Self {
            memcached: mc,
            upstream,
        }
    }
}

#[derive(Debug)]
pub enum WrappedError {
    #[allow(unused)]
    Database(sqlx::Error),
    #[allow(unused)]
    Memcached(MemcacheError),
}

struct McBytesWrapper(Bytes);

impl Into<Bytes> for McBytesWrapper {
    fn into(self) -> Bytes {
        self.0
    }
}
impl From<Bytes> for McBytesWrapper {
    fn from(value: Bytes) -> Self {
        Self(value)
    }
}
impl Deref for McBytesWrapper {
    type Target = Bytes;

    fn deref(&self) -> &Self::Target {
        &self.0
    }
}

impl<W: Write> ToMemcacheValue<W> for McBytesWrapper {
    fn get_flags(&self) -> u32 {
        0
    }

    fn get_length(&self) -> usize {
        self.len()
    }

    fn write_to(&self, stream: &mut W) -> std::io::Result<()> {
        stream.write_all(self)
    }
}
impl FromMemcacheValue for McBytesWrapper {
    fn from_memcache_value(bytes: Vec<u8>, _flags: u32) -> Result<Self, MemcacheError> {
        Ok(Self(Bytes::from(bytes)))
    }
}

#[async_trait]
impl Fetcher for MemcachedFetcher {
    async fn fetch_list(&self, q: ListParams) -> Result<Bytes, WrappedError> {
        let key = q.get_key();
        let cached = self
            .memcached
            .get::<McBytesWrapper>(&key)
            .map_err(|e| WrappedError::Memcached(e))?;

        match cached {
            Some(data) => Ok(data.into()),
            None => {
                let fetched = self.upstream.fetch_list(q).await?;
                let _ = self
                    .memcached
                    .set::<McBytesWrapper>(
                        &key,
                        McBytesWrapper(fetched.clone()),
                        MEMCACHE_EXPIRATION.as_secs() as u32,
                    )
                    .map_err(|e| println!("Error setting memcached key {key}: {e}"));
                Ok(fetched.into())
            }
        }
    }

    async fn fetch_analysis(&self, q: AnalysisParams) -> Result<Bytes, WrappedError> {
        let key = q.get_key();
        let cached = self
            .memcached
            .get::<McBytesWrapper>(&key)
            .map_err(|e| WrappedError::Memcached(e))?;

        match cached {
            Some(data) => Ok(data.into()),
            None => {
                let fetched = self.upstream.fetch_analysis(q).await?;
                let _ = self
                    .memcached
                    .set::<McBytesWrapper>(
                        &key,
                        McBytesWrapper(fetched.clone()),
                        MEMCACHE_EXPIRATION.as_secs() as u32,
                    )
                    .map_err(|e| println!("Error setting memcached key {key}: {e}"));
                Ok(fetched.into())
            }
        }
    }

    async fn submit(
        &self,
        metadata: SubmissionMetadata<String>,
        deaths: Vec<SubmissionDeath>,
    ) -> Result<(), WrappedError> {
        self.upstream.submit(metadata, deaths).await
    }
}
