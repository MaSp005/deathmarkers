use axum::{
    Router,
    extract::{Json, Query, State},
    http::{Response, StatusCode},
    routing::{get, post},
};
use std::{collections::HashMap, env, sync::Arc};
use tokio::net::TcpListener;

mod query;
use query::*;
mod fetch;
use fetch::*;
mod params;
use params::*;

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
) -> Response<String> {
    match ListParams::parse_from_query(&params) {
        Err(msg) =>build_err_response(msg),
        Ok(params) => {
            fetcher.fetch(params.query());

            let mut response: Response<String> = Response::default();
            *response.status_mut() = StatusCode::OK;
            response
        }
    }
}

async fn analysis(
    State(fetcher): State<Arc<dyn Fetcher + Sync + Send>>,
    Query(params): Query<HashMap<String, String>>,
) -> Response<String> {
    match AnalysisParams::parse_from_query(&params) {
        Err(msg) => build_err_response(msg),
        Ok(params) => {
            fetcher.fetch(params.query());

            let mut response: Response<String> = Response::default();
            *response.status_mut() = StatusCode::OK;
            response
        }
    }
}

async fn submit(
    State(fetcher): State<Arc<dyn Fetcher + Sync + Send>>,
    Query(params): Query<HashMap<String, String>>,
    Json(payload): Json<serde_json::Value>,
) -> Response<String> {
    dbg!(payload);

    let mut response: Response<String> = Response::default();
    *response.status_mut() = StatusCode::CREATED;
    return response;
}

fn build_err_response(msg: String) -> Response<String> {
    let mut response = Response::<String>::default();
    *response.status_mut() = StatusCode::BAD_REQUEST;
    response.body_mut().push_str(msg.as_str());
    response
}
