use crate::{
    data::{SubmissionDeath, SubmissionMetadata},
    params::{AnalysisParams, ListParams},
};
use ::memcache::MemcacheError;
use async_trait::async_trait;
use bytes::Bytes;

pub mod database;
mod encode;
pub mod memcache;

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

#[derive(Debug)]
pub enum WrappedError {
    #[allow(unused)]
    Database(sqlx::Error),
    #[allow(unused)]
    Memcached(MemcacheError),
}
