use axum::{
    Router,
    extract::{Json, Query},
    http::{Response, StatusCode},
    routing::{get, post},
};
use std::{collections::HashMap, env, fmt::Debug};
use tokio::net::TcpListener;

#[tokio::main]
async fn main() {
    let bind_addr = env::var("LISTEN_ADDRESS").unwrap_or(String::from("0.0.0.0:8048"));

    let app = Router::new()
        .route("/list", get(list))
        .route("/analysis", get(analysis))
        .route("/submit", post(submit));

    let listener = TcpListener::bind(bind_addr)
        .await
        .expect("Failed to bind the TCP listener. Is LISTEN_ADDRESS correctly formatted?");
    axum::serve(listener, app).await.unwrap();
}

fn parse_level_id_from_param(val: Option<&String>) -> Result<u32, String> {
    match val {
        None => Err(String::from("levelid parameter not provided")),
        Some(s) => match s.parse::<u32>() {
            Err(_) => Err(String::from("Invalid levelid parameter")),
            Ok(s) => Ok(s),
        },
    }
}

fn parse_bool_from_param(
    val: Option<&String>,
    default: Option<bool>,
    name: &str,
) -> Result<bool, String> {
    match (match val {
        None => return default.ok_or(format!("{name} parameter not provided")),
        Some(s) => s,
    })
    .as_str()
    {
        "true" => Ok(true),
        "false" => Ok(false),
        _other => Err(format!("Invalid {name} parameter")),
    }
}

fn join_errs(a: &Result<impl Debug, String>, b: &Result<impl Debug, String>) -> Result<(), String> {
    match (a.is_err(), b.is_err()) {
        (false, false) => Ok(()),
        (false, true) => Err(b.as_ref().unwrap_err().clone()),
        (true, false) => Err(a.as_ref().unwrap_err().clone()),
        (true, true) => Err(a.as_ref().unwrap_err().clone() + "\n" + &b.as_ref().unwrap_err()),
    }
}

#[derive(Debug)]
enum ResponseType {
    CSV,
    Binary,
}
impl ResponseType {
    fn parse_from_param(val: Option<&String>) -> Result<ResponseType, String> {
        let val = match val {
            None => return Ok(ResponseType::CSV),
            Some(s) => s,
        };

        match val.as_str() {
            "csv" => Ok(ResponseType::CSV),
            "bin" => Ok(ResponseType::Binary),
            _other => Err(format!("Invalid response parameter")),
        }
    }
}

#[derive(Debug)]
struct ListParams {
    level_id: u32,
    platformer: bool,
    practice: bool,
    response: ResponseType,
}
impl ListParams {
    fn parse_from_query(query: &HashMap<String, String>) -> Result<ListParams, String> {
        let level_id = parse_level_id_from_param(query.get("levelid"));
        let platformer = parse_bool_from_param(query.get("platformer"), None, "platformer");
        let response = ResponseType::parse_from_param(query.get("response"));
        let practice = parse_bool_from_param(query.get("practice"), Some(true), "practice");

        let mut errs = join_errs(&level_id, &platformer);
        errs = join_errs(&errs, &response);
        errs = join_errs(&errs, &practice);

        errs.and_then(|_| {
            Ok(ListParams {
                level_id: level_id.unwrap(),
                platformer: platformer.unwrap(),
                practice: practice.unwrap(),
                response: response.unwrap(),
            })
        })
    }
}

#[derive(Debug)]
struct AnalysisParams {
    level_id: u32,
    response: ResponseType,
}
impl AnalysisParams {
    fn parse_from_query(query: &HashMap<String, String>) -> Result<AnalysisParams, String> {
        let level_id = parse_level_id_from_param(query.get("levelid"));
        let response = ResponseType::parse_from_param(query.get("response"));

        join_errs(&level_id, &response).and_then(|_| {
            Ok(AnalysisParams {
                level_id: level_id.unwrap(),
                response: response.unwrap(),
            })
        })
    }
}

async fn list(Query(params): Query<HashMap<String, String>>) -> Response<String> {
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

async fn analysis(Query(params): Query<HashMap<String, String>>) -> Response<String> {
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

async fn submit(Json(payload): Json<serde_json::Value>) -> Response<String> {
    dbg!(payload);

    let mut response: Response<String> = Response::default();
    *response.status_mut() = StatusCode::CREATED;
    return response;
}
