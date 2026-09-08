use crate::{
    data::{SubmissionDeath, SubmissionMetadata},
    digest::{Sha1Result, sha1_digest},
    params::{AnalysisParams, ListParams},
};
use async_trait::async_trait;
use bytes::{BufMut, Bytes, BytesMut};
use hex::FromHex;
use memcache::{Client as MemcacheClient, FromMemcacheValue, MemcacheError, ToMemcacheValue};
use sqlx::{
    PgPool, Row,
    postgres::{PgPoolOptions, PgRow},
    query, raw_sql,
};
use std::{io::Write, iter::repeat, ops::Deref, time::Duration};

const DATA_FORMAT: u8 = 1;
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

fn create_binary_response_normal(deaths: &Vec<PgRow>) -> Bytes {
    const ITEM_LENGTH: usize = 4 + 4 + 2;
    let mut bytes = BytesMut::with_capacity(deaths.len() * ITEM_LENGTH + 1);
    bytes.put_u8(DATA_FORMAT);
    for death in deaths {
        bytes.put_f32_le(death.get::<f64, _>(0) as _); // x
        bytes.put_f32_le(death.get::<f64, _>(1) as _); // y
        bytes.put_u16_le(death.get::<i16, _>(2) as _); // percentage
    }
    bytes.freeze()
}

fn create_binary_response_platformer(deaths: &Vec<PgRow>) -> Bytes {
    const ITEM_LENGTH: usize = 4 + 4;
    let mut bytes = BytesMut::with_capacity(deaths.len() * ITEM_LENGTH + 1);
    bytes.put_u8(DATA_FORMAT);
    for death in deaths {
        bytes.put_f32_le(death.get::<f64, _>(0) as _); // x
        bytes.put_f32_le(death.get::<f64, _>(1) as _); // y
    }
    bytes.freeze()
}

fn create_binary_response_analysis(deaths: &Vec<PgRow>, salt: Option<&str>) -> Bytes {
    const ITEM_LENGTH: usize = 20 + 1 + 1 + 4 + 4 + 2;

    let mut bytes = BytesMut::with_capacity(deaths.len() * ITEM_LENGTH + 1);
    bytes.put_u8(DATA_FORMAT);
    for death in deaths {
        let userident: String = death.get(0);
        let salted_ui = if let Some(salt) = salt {
            sha1_digest(&format!("{userident}{salt}"))
        } else {
            Sha1Result::from_hex(userident).expect("Stored hash should be valid")
        };
        bytes.put_slice(&salted_ui); // userident
        bytes.put_u8(death.get::<i16, _>(1) as u8); // levelversion
        bytes.put_u8(if death.get::<bool, _>(2) { 1u8 } else { 0u8 }); // practice
        bytes.put_f32_le(death.get::<f64, _>(3) as f32); // x
        bytes.put_f32_le(death.get::<f64, _>(4) as f32); // y
        bytes.put_u16_le(death.get::<i16, _>(5) as u16); // percentage
    }
    bytes.freeze()
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
        Ok(create_binary_response_analysis(&deaths, Some(&salt)))
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
                        MEMCACHE_EXPIRATION.as_secs() as _,
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

#[cfg(test)]
mod test {
    use super::*;
    use sqlx::postgres::PgRow;

    #[test]
    fn test_create_binary_response_empty() {
        let deaths = vec![];
        let bytes = create_binary_response_normal(&deaths);
        assert_eq!(bytes.len(), 1);
        assert_eq!(bytes, Bytes::from(vec![DATA_FORMAT]));
        let bytes = create_binary_response_platformer(&deaths);
        assert_eq!(bytes.len(), 1);
        assert_eq!(bytes, Bytes::from(vec![DATA_FORMAT]));
        let bytes = create_binary_response_analysis(&deaths, None);
        assert_eq!(bytes.len(), 1);
        assert_eq!(bytes, Bytes::from(vec![DATA_FORMAT]));
    }

    #[sqlx::test]
    #[ignore]
    async fn test_create_binary_response_normal(pool: sqlx::PgPool) {
        let deaths = query("SELECT 5.3::FLOAT, 9.4::FLOAT, 1::SMALLINT;")
            .fetch_all(&pool)
            .await
            .unwrap();
        let bytes = create_binary_response_normal(&deaths);

        println!("{0:?}", bytes.to_vec());
        assert_eq!(bytes.len(), 1 + 4 + 4 + 2);
        assert_eq!(bytes[0], DATA_FORMAT, "Data Format should be 0x1");
        let death_slice = &bytes[1..];
        assert_eq!(death_slice[0x0..=0x3], f32::to_le_bytes(5.3), "x as LE float32");
        assert_eq!(death_slice[0x4..=0x7], f32::to_le_bytes(9.4), "y as LE float32");
        assert_eq!(death_slice[0x8..=0x9], u16::to_le_bytes(1), "percentage as LE uint16");
    }

    #[sqlx::test]
    #[ignore]
    async fn test_create_binary_response_platformer(pool: sqlx::PgPool) {
        let deaths = query("SELECT 5.3::FLOAT, 9.4::FLOAT;")
            .fetch_all(&pool)
            .await
            .unwrap();
        let bytes = create_binary_response_platformer(&deaths);
        println!("{0:?}", bytes.to_vec());

        assert_eq!(bytes.len(), 1 + 4 + 4);
        assert_eq!(bytes[0], DATA_FORMAT, "Data Format should be 0x1");
        let death_slice = &bytes[1..];
        assert_eq!(death_slice[0x0..=0x3], f32::to_le_bytes(5.3), "x as LE float32");
        assert_eq!(death_slice[0x4..=0x7], f32::to_le_bytes(9.4), "y as LE float32");
    }

    #[sqlx::test]
    #[ignore]
    async fn test_create_binary_response_analysis(pool: sqlx::PgPool) {
        let deaths = query(
            "SELECT '0123456789abcdef0123456789abcdef01234567'::CHAR(40), \
            42::SMALLINT, \
            true, \
            5.3::FLOAT, \
            9.4::FLOAT, \
            1::SMALLINT;"
        )
            .fetch_all(&pool)
            .await
            .unwrap();
        let bytes = create_binary_response_analysis(&deaths, None);
        println!("{0:?}", bytes.to_vec());

        assert_eq!(bytes.len(), 1 + 20 + 1 + 1 + 4 + 4 + 2);
        assert_eq!(bytes[0], DATA_FORMAT, "Data Format should be 0x1");
        let death_slice = &bytes[1..];
        assert_eq!(
            death_slice[0x0..=0x13],
            Sha1Result::from_hex("0123456789abcdef0123456789abcdef01234567").unwrap(),
            "userident stays intact"
        );
        assert_eq!(death_slice[0x14], 42, "level version as uint8");
        assert_eq!(death_slice[0x15], 1, "practice as bool");
        assert_eq!(death_slice[0x16..=0x19], f32::to_le_bytes(5.3), "x as LE float32");
        assert_eq!(death_slice[0x1a..=0x1d], f32::to_le_bytes(9.4), "y as LE float32");
        assert_eq!(death_slice[0x1e..=0x1f], u16::to_le_bytes(1), "percentage as LE uint16");
    }
}
