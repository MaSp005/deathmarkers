use axum::{
    Router,
    extract::{Json, Query, State},
    http::{Response, StatusCode},
    routing::{get, post},
};
use std::{collections::HashMap, env};
use tokio::net::TcpListener;

mod params;
use params::*;

#[tokio::main]
async fn main() {
    let bind_addr = env::var("LISTEN_ADDRESS").unwrap_or(String::from("0.0.0.0:8048"));

    let app = Router::new()
        .route("/", get(root))
        .route("/list", get(list))
        .route("/analysis", get(analysis))
        .route("/submit", post(submit));

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
    Query(params): Query<HashMap<String, String>>,
) -> Response<String> {
    let params = dbg!(ListParams::parse_from_query(&params));
    if let Err(msg) = params {
        let mut response: Response<String> = Response::default();
        *response.status_mut() = StatusCode::BAD_REQUEST;
        response.body_mut().push_str(msg.as_str());
        return response;
    }

    let mut response: Response<String> = Response::default();
    *response.status_mut() = StatusCode::OK;
    return response;
}

async fn analysis(
    Query(params): Query<HashMap<String, String>>,
) -> Response<String> {
    let params = dbg!(AnalysisParams::parse_from_query(&params));
    if let Err(msg) = params {
        let mut response: Response<String> = Response::default();
        *response.status_mut() = StatusCode::BAD_REQUEST;
        response.body_mut().push_str(msg.as_str());
        return response;
    }

    let mut response: Response<String> = Response::default();
    *response.status_mut() = StatusCode::OK;
    return response;
}

async fn submit(
    Query(params): Query<HashMap<String, String>>,
    Json(payload): Json<serde_json::Value>,
) -> Response<String> {
    dbg!(payload);

    let mut response: Response<String> = Response::default();
    *response.status_mut() = StatusCode::CREATED;
    return response;
}
