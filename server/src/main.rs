use axum::{
    Router,
    body::Bytes,
    extract::{Json, Query, State},
    http::{Response, StatusCode},
    routing::{get, post},
};
use std::{collections::HashMap, env, sync::Arc};
use tokio::net::TcpListener;

mod data;
mod fetch;
use fetch::*;
mod params;
use params::*;

use crate::data::AnalysisDeath;

#[tokio::main]
async fn main() {
    let bind_addr = env::var("LISTEN_ADDRESS").unwrap_or(String::from("0.0.0.0:8048"));

    let fetcher: Arc<dyn Fetcher + Sync + Send> = Arc::new(DatabaseFetcher::new().await);

    let app = Router::new()
        .route("/", get(root))
        .route("/list", get(list))
        .route("/analysis", get(analysis))
        .route("/submit", post(submit))
        .with_state(fetcher);

    let listener = TcpListener::bind(bind_addr)
        .await
        .expect("Failed to bind the TCP listener. Is LISTEN_ADDRESS correctly formatted?");
    axum::serve(listener, app).await.unwrap();
}

async fn root() -> &'static str {
    "This is the DeathMarkers Server!\n\
    If you're seeing this, the root path is not being shadowed by a reverse proxy \
    to serve some human-readable web page or redirect you elsewhere.\n\
    If you're the admin of this server, you should do that!"
}

async fn list(
    State(fetcher): State<Arc<dyn Fetcher + Sync + Send>>,
    Query(params): Query<HashMap<String, String>>,
) -> Result<Bytes, (StatusCode, String)> {
    match ListParams::parse_from_query(&params) {
        Err(msg) => Err((StatusCode::BAD_REQUEST, msg)),
        Ok(params) => {
            // let q = DMQuery::List(params);
            let data = if params.platformer {
                fetcher.fetch_list_platformer(params).await
            } else {
                fetcher.fetch_list(params).await
            };

            todo!()
        }
    }
}

async fn analysis(
    State(fetcher): State<Arc<dyn Fetcher + Sync + Send>>,
    Query(params): Query<HashMap<String, String>>,
) -> Result<Bytes, (StatusCode, String)> {
    match AnalysisParams::parse_from_query(&params) {
        Err(msg) => Err((StatusCode::BAD_REQUEST, msg)),
        Ok(params) => {
            // let q = DMQuery::List(params);
            let data = fetcher.fetch_analysis(params).await;

            todo!()
        }
    }
}

async fn submit(
    State(fetcher): State<Arc<dyn Fetcher + Sync + Send>>,
    Query(params): Query<HashMap<String, String>>,
    Json(payload): Json<serde_json::Value>,
) -> Result<StatusCode, (StatusCode, &'static str)> {
    let deaths: Vec<AnalysisDeath> = vec![];
    let result = fetcher.submit(deaths);
    if result.await.is_err() {
        Err((
            StatusCode::INTERNAL_SERVER_ERROR,
            "Error writing to the database. May be due to wrongly formatted input. Try again.",
        ))
    } else {
        Ok(StatusCode::CREATED)
    }
}
