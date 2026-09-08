use std::{io::Write, ops::Deref, time::Duration};

use crate::data::{SubmissionDeath, SubmissionMetadata};
use crate::params::*;

use super::database::DatabaseFetcher;
use super::{Fetcher, WrappedError};
use async_trait::async_trait;
use bytes::Bytes;
use memcache::{Client as MemcacheClient, FromMemcacheValue, MemcacheError, ToMemcacheValue};

const MEMCACHE_EXPIRATION: Duration = Duration::from_mins(10);

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
