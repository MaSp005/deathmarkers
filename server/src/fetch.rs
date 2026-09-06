use crate::{
    data::{SubmissionDeath, SubmissionMetadata},
    digest::sha1_digest,
    params::{AnalysisParams, ListParams},
};
use async_trait::async_trait;
use bytes::{BufMut, Bytes, BytesMut};
use sqlx::{Error, PgPool, Row, postgres::PgPoolOptions, query};

#[async_trait]
pub trait Fetcher {
    async fn fetch_list(&self, query: ListParams) -> Result<Bytes, Error>;
    async fn fetch_analysis(&self, query: AnalysisParams) -> Result<Bytes, Error>;
    async fn submit(
        &self,
        metadata: SubmissionMetadata<String>,
        deaths: Vec<SubmissionDeath>,
    ) -> Result<(), Error>;
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
        sqlx::migrate!()
            .run(&pool)
            .await
            .expect("Migrations failed");
        println!("Connected to the database.");

        Self { pool }
    }

    async fn fetch_list_normal(&self, q: ListParams) -> Result<Bytes, Error> {
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

    async fn fetch_list_platformer(&self, q: ListParams) -> Result<Bytes, Error> {
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
    async fn fetch_list(&self, q: ListParams) -> Result<Bytes, Error> {
        if q.platformer {
            self.fetch_list_platformer(q).await
        } else {
            self.fetch_list_normal(q).await
        }
    }

    async fn fetch_analysis(&self, q: AnalysisParams) -> Result<Bytes, Error> {
        // userident, levelversion, practice, x, y, percentage
        const ITEM_LENGTH: usize = 20 + 2 + 1 + 4 + 4 + 2;
        let (qs, levelid) = q.query();
        let deaths = query(qs).bind(levelid).fetch_all(&self.pool).await?;
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
    ) -> Result<(), Error> {
        todo!()
    }
}
