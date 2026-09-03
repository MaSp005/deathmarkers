use std::{collections::HashMap, fmt::Debug};

use serde_json::Value;

use crate::data::{SHA1_LENGTH, SubmissionDeath};

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

fn is_sha1_string(s: &str) -> bool {
    s.len() == SHA1_LENGTH * 2
        && s.chars().all(|c| match c {
            '0'..'9' | 'a'..'f' | 'A'..'F' => true,
            _ => false,
        })
}

fn get_query(platformer: bool, practice: bool) -> &'static str {
    match (platformer, practice) {
        (false, false) => "SELECT x, y FROM format1 WHERE levelid = $1 AND practice = false;",
        (false, true) => "SELECT x, y FROM format1 WHERE levelid = $1;",
        (true, false) => {
            "SELECT x, y, percentage FROM format1 WHERE levelid = $1 AND percentage < 101 AND practice = false;"
        }
        (true, true) => {
            "SELECT x, y, percentage FROM format1 WHERE levelid = $1 AND percentage < 101;"
        }
    }
}

const ANALYSIS_QUERY: &'static str = "SELECT userident,levelversion,practice,x,y,percentage \
    FROM format1 WHERE levelid = $1;";

#[derive(Debug)]
pub enum ResponseType {
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
pub struct ListParams {
    pub level_id: u32,
    pub platformer: bool,
    pub practice: bool,
    pub response: ResponseType,
}
impl ListParams {
    pub fn parse_from_query(query: &HashMap<String, String>) -> Result<ListParams, String> {
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
    pub fn query(&self) -> (&'static str, i64) {
        (
            get_query(self.platformer, self.practice),
            self.level_id as i64,
        )
    }
}

#[derive(Debug)]
pub struct AnalysisParams {
    pub level_id: u32,
    pub response: ResponseType,
}
impl AnalysisParams {
    pub fn parse_from_query(query: &HashMap<String, String>) -> Result<AnalysisParams, String> {
        let level_id = parse_level_id_from_param(query.get("levelid"));
        let response = ResponseType::parse_from_param(query.get("response"));

        join_errs(&level_id, &response).and_then(|_| {
            Ok(AnalysisParams {
                level_id: level_id.unwrap(),
                response: response.unwrap(),
            })
        })
    }
    pub fn query(&self) -> (&'static str, i64) {
        (ANALYSIS_QUERY, self.level_id as i64)
    }
}

pub struct SubmissionPayload {}
impl SubmissionPayload {
    pub fn parse(
        params: HashMap<String, String>,
        payload: Value,
    ) -> Result<Vec<SubmissionDeath>, String> {
        let levelid: Result<u64, String> = match params.get("levelid") {
            Some(l) => l
                .parse()
                .map_err(|_| "levelid incorrectly formatted".to_owned()),
            None => {
                let body_lid = payload.get("levelid");
                if body_lid.is_none() {
                    Err("levelid not provided as parameter nor in body".to_owned())
                } else {
                    match body_lid.unwrap() {
                        Value::Number(v) => match v.as_u64() {
                            Some(u) => Ok(u),
                            None => return Err("levelid must be a positive integer".to_owned()),
                        },
                        _ => return Err("levelid must be a positive integer".to_owned()),
                    }
                }
            }
        };
        let format: Result<u8, String> = match payload.get("format") {
            None => return Err("format not provided".to_owned()),
            Some(f) => match f {
                Value::Number(v) => match v.as_u64() {
                    Some(u) => Ok(u as u8),
                    None => return Err("levelid must be a positive integer".to_owned()),
                },
                _ => return Err("levelid must be a positive integer".to_owned()),
            },
        };
        let version: Result<u8, String> = match payload.get("levelversion") {
            None => Ok(0),
            Some(v) => match v {
                Value::Number(v) => match v.as_u64() {
                    Some(u) => Ok(u as u8),
                    None => return Err("levelversion must be a positive integer".to_owned()),
                },
                _ => return Err("levelversion must be a positive integer".to_owned()),
            },
        };
        let userident: Result<String, String> = match payload.get("userident") {
            Some(u) => match u {
                Value::String(s) => {
                    if is_sha1_string(s) {
                        Ok(s.to_string())
                    } else {
                        Err("userident incorrectly formatted".to_owned())
                    }
                }
                _ => Err("userident must be transmitted as a string".to_owned()),
            },
            None => todo!(),
        };

        let mut errs = join_errs(&levelid, &format);
        errs = join_errs(&errs, &version);
        errs = join_errs(&errs, &userident);

        let deaths: Vec<SubmissionDeath> = vec![];
        errs.and(Ok(deaths))
    }
}
