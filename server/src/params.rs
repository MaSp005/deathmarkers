use crate::{data::*, digest::*};
use serde_json::Value;
use std::{collections::HashMap, fmt::Debug, ops::Deref};

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

fn submission_from_json(value: impl Deref<Target = Value>) -> Result<SubmissionDeath, String> {
    let practice = value.get("practice").map(|p| p.as_bool()).flatten();
    let x = value.get("x").map(|p| p.as_f64()).flatten();
    let y = value.get("y").map(|p| p.as_f64()).flatten();
    let percentage = value.get("percentage").map(|p| p.as_u64()).flatten();

    if x.is_some() && y.is_some() && percentage.is_some() {
        Ok(SubmissionDeath::new(
            practice.unwrap_or(false),
            x.unwrap() as f32,
            y.unwrap() as f32,
            percentage.unwrap() as i16,
        ))
    } else {
        Err("x, y or percentage not passed correctly.".to_owned())
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

fn get_query(platformer: bool, practice: bool) -> &'static str {
    match (platformer, practice) {
        (false, false) => {
            "SELECT x, y, percentage FROM format1 WHERE levelid = $1 AND practice = false;"
        }
        (false, true) => "SELECT x, y, percentage FROM format1 WHERE levelid = $1;",
        (true, false) => {
            "SELECT x, y FROM format1 WHERE levelid = $1 AND percentage < 101 AND practice = false;"
        }
        (true, true) => "SELECT x, y FROM format1 WHERE levelid = $1 AND percentage < 101;",
    }
}

const ANALYSIS_QUERY: &'static str = "SELECT userident,levelversion,practice,x,y,percentage \
    FROM format1 WHERE levelid = $1;";

#[derive(Debug, PartialEq)]
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

#[derive(Debug, PartialEq)]
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

#[derive(Debug, PartialEq)]
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

#[derive(Debug, PartialEq)]
pub struct SubmissionPayload(pub SubmissionMetadata<String>, pub Vec<SubmissionDeath>);
impl SubmissionPayload {
    pub fn parse(
        params: HashMap<String, String>,
        payload: Value,
    ) -> Result<SubmissionPayload, String> {
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
            Some(u) => {
                if let Some(s) = u.as_str()
                    && is_sha1(s)
                {
                    Ok(String::from(s))
                } else {
                    Err("userident incorrectly formatted".to_owned())
                }
            }

            None => {
                let playername = payload.get("playername").map(|p| p.as_str()).flatten();
                let userid = payload.get("userid").map(|i| i.as_u64()).flatten();
                if playername.and(userid).is_none() {
                    Err(
                        "If userident is not provided, playername and userid must be \
                            provided as string and positive integer, respectively."
                            .to_owned(),
                    )
                } else if let Ok(levelid) = levelid {
                    Ok(stringify_digest(make_userident(
                        playername.unwrap(),
                        userid.unwrap(),
                        levelid,
                    )))
                } else {
                    Ok("".to_owned())
                }
            }
        };

        let deaths: Result<Vec<SubmissionDeath>, String> =
            if let Some(array) = payload.get("deaths").map(|d| d.as_array()).flatten() {
                let mut deaths = Vec::with_capacity(array.len());
                let mut first_err = Ok(());
                for (i, value) in array.iter().enumerate() {
                    match submission_from_json(value) {
                        Ok(d) => {
                            deaths.push(d);
                        }
                        Err(e) => {
                            first_err = Err(format!("Error in deaths[{i}]: {e}"));
                            break;
                        }
                    }
                }
                first_err.and(Ok(deaths))
            } else {
                submission_from_json(&payload).map(|d| vec![d])
            };

        let mut errs = join_errs(&levelid, &format);
        errs = join_errs(&errs, &version);
        errs = join_errs(&errs, &userident);
        errs = join_errs(&errs, &deaths);

        errs.and_then(|_| {
            Ok(SubmissionPayload(
                SubmissionMetadata::new(
                    levelid.unwrap() as u32,
                    format.unwrap(),
                    version.unwrap(),
                    userident.unwrap(),
                ),
                deaths.unwrap(),
            ))
        })
    }
}

#[cfg(test)]
mod test {
    use super::*;
    use serde_json::json;

    const NULL_SHA1_STRING: &str = "0000000000000000000000000000000000000000";

    #[test]
    fn test_parse_level_id_from_param() {
        assert!(parse_level_id_from_param(None).is_err());
        assert!(parse_level_id_from_param(Some(&"".to_owned())).is_err());
        assert!(parse_level_id_from_param(Some(&"a".to_owned())).is_err());
        assert!(parse_level_id_from_param(Some(&"-1".to_owned())).is_err());
        assert!(parse_level_id_from_param(Some(&"1.1".to_owned())).is_err());
        assert_eq!(parse_level_id_from_param(Some(&"1".to_owned())), Ok(1));
    }

    #[test]
    fn test_parse_bool_from_param() {
        assert!(parse_bool_from_param(None, None, "").is_err());
        assert_eq!(parse_bool_from_param(None, Some(true), ""), Ok(true));
        assert_eq!(parse_bool_from_param(None, Some(false), ""), Ok(false));
        assert!(parse_bool_from_param(Some(&"".to_owned()), None, "").is_err());
        assert!(parse_bool_from_param(Some(&"".to_owned()), Some(true), "").is_err());
        assert!(parse_bool_from_param(Some(&"".to_owned()), Some(false), "").is_err());
        assert!(parse_bool_from_param(Some(&"true or false".to_owned()), None, "").is_err());
        assert!(parse_bool_from_param(Some(&"true or false".to_owned()), Some(true), "").is_err());
        assert!(parse_bool_from_param(Some(&"true or false".to_owned()), Some(false), "").is_err());
        assert!(parse_bool_from_param(Some(&"1".to_owned()), None, "").is_err());
        assert!(parse_bool_from_param(Some(&"1".to_owned()), Some(true), "").is_err());
        assert!(parse_bool_from_param(Some(&"1".to_owned()), Some(false), "").is_err());
        assert!(parse_bool_from_param(Some(&"TRUE".to_owned()), None, "").is_err());
        assert!(parse_bool_from_param(Some(&"TRUE".to_owned()), Some(true), "").is_err());
        assert!(parse_bool_from_param(Some(&"TRUE".to_owned()), Some(false), "").is_err());
        assert_eq!(
            parse_bool_from_param(Some(&"true".to_owned()), None, ""),
            Ok(true)
        );
        assert_eq!(
            parse_bool_from_param(Some(&"true".to_owned()), Some(true), ""),
            Ok(true)
        );
        assert_eq!(
            parse_bool_from_param(Some(&"true".to_owned()), Some(false), ""),
            Ok(true)
        );
        assert_eq!(
            parse_bool_from_param(Some(&"false".to_owned()), None, ""),
            Ok(false)
        );
        assert_eq!(
            parse_bool_from_param(Some(&"false".to_owned()), Some(true), ""),
            Ok(false)
        );
        assert_eq!(
            parse_bool_from_param(Some(&"false".to_owned()), Some(false), ""),
            Ok(false)
        );
    }

    #[test]
    fn test_submission_from_json() {
        assert!(submission_from_json(&json!({})).is_err());
        assert_eq!(
            submission_from_json(&json!({"x": 1.0, "y": 1.0, "percentage": 5})),
            Ok(SubmissionDeath::new(false, 1.0, 1.0, 5))
        );
        assert_eq!(
            submission_from_json(&json!({"practice": true, "x": 1.0, "y": 1.0, "percentage": 5})),
            Ok(SubmissionDeath::new(true, 1.0, 1.0, 5))
        );
        assert!(submission_from_json(&json!({"y": 1.0, "percentage": 5})).is_err());
        assert!(submission_from_json(&json!({"x": 1.0, "percentage": 5})).is_err());
        assert!(submission_from_json(&json!({"x": 1.0, "y": 1.0})).is_err());
        assert!(submission_from_json(&json!({"x": 1.0, "y": 1.0, "percentage": -1})).is_err());
        assert!(submission_from_json(&json!({"x": 1.0, "y": 1.0, "percentage": 1.0})).is_err());
        assert!(submission_from_json(&json!({"x": "1", "y": "1", "percentage": "1"})).is_err());
        assert!(submission_from_json(&json!({"x": true, "y": true, "percentage": true})).is_err());
        assert!(submission_from_json(&json!({"x": [], "y": [], "percentage": []})).is_err());
        assert!(submission_from_json(&json!({"x": {}, "y": {}, "percentage": {}})).is_err());
        assert_eq!(
            submission_from_json(
                &json!({"something_else": {}, "x": 1.0, "y": 1.0, "percentage": 5})
            ),
            Ok(SubmissionDeath::new(false, 1.0, 1.0, 5))
        );
    }

    #[test]
    fn test_join_errs() {
        assert_eq!(join_errs(&Ok(()), &Ok(())), Ok(()));
        assert_eq!(
            join_errs(&Err::<(), String>("a".to_owned()), &Ok(())),
            Err("a".to_owned())
        );
        assert_eq!(
            join_errs(&Ok(()), &Err::<(), String>("b".to_owned())),
            Err("b".to_owned())
        );
        assert_eq!(
            join_errs(
                &Err::<(), String>("a".to_owned()),
                &Err::<(), String>("b".to_owned())
            ),
            Err("a\nb".to_owned())
        );
    }

    #[test]
    fn test_get_query() {
        let norm_nopr = get_query(false, false);
        let norm_pr = get_query(false, true);
        let pl_nopr = get_query(true, false);
        let pl_pr = get_query(true, true);

        assert_eq!(norm_nopr.contains("practice = false"), true);
        assert_eq!(norm_pr.contains("practice = false"), false);
        assert_eq!(pl_nopr.contains("practice = false"), true);
        assert_eq!(pl_pr.contains("practice = false"), false);

        assert_eq!(norm_nopr.contains("percentage FROM"), true);
        assert_eq!(norm_pr.contains("percentage FROM"), true);
        assert_eq!(pl_nopr.contains("percentage FROM"), false);
        assert_eq!(pl_pr.contains("percentage FROM"), false);

        assert_eq!(norm_nopr.contains("percentage < 101"), false);
        assert_eq!(norm_pr.contains("percentage < 101"), false);
        assert_eq!(pl_nopr.contains("percentage < 101"), true);
        assert_eq!(pl_pr.contains("percentage < 101"), true);
    }

    #[test]
    fn test_parse_response_type() {
        assert_eq!(ResponseType::parse_from_param(None), Ok(ResponseType::CSV));
        assert_eq!(
            ResponseType::parse_from_param(Some(&"csv".to_owned())),
            Ok(ResponseType::CSV)
        );
        assert_eq!(
            ResponseType::parse_from_param(Some(&"bin".to_owned())),
            Ok(ResponseType::Binary)
        );
        assert!(ResponseType::parse_from_param(Some(&"other".to_owned())).is_err());
    }

    #[test]
    fn test_parse_list_params() {
        assert!(ListParams::parse_from_query(&HashMap::from([])).is_err());

        assert_eq!(
            ListParams::parse_from_query(&HashMap::from([
                ("levelid".to_owned(), "1".to_owned()),
                ("platformer".to_owned(), "true".to_owned()),
            ])),
            Ok(ListParams {
                level_id: 1,
                platformer: true,
                practice: true,
                response: ResponseType::CSV,
            })
        );

        assert_eq!(
            ListParams::parse_from_query(&HashMap::from([
                ("levelid".to_owned(), "1".to_owned()),
                ("platformer".to_owned(), "false".to_owned()),
            ])),
            Ok(ListParams {
                level_id: 1,
                platformer: false,
                practice: true,
                response: ResponseType::CSV,
            })
        );

        assert_eq!(
            ListParams::parse_from_query(&HashMap::from([
                ("levelid".to_owned(), "1".to_owned()),
                ("platformer".to_owned(), "false".to_owned()),
                ("response".to_owned(), "csv".to_owned()),
            ])),
            Ok(ListParams {
                level_id: 1,
                platformer: false,
                practice: true,
                response: ResponseType::CSV,
            })
        );

        assert_eq!(
            ListParams::parse_from_query(&HashMap::from([
                ("levelid".to_owned(), "1".to_owned()),
                ("platformer".to_owned(), "false".to_owned()),
                ("response".to_owned(), "bin".to_owned()),
            ])),
            Ok(ListParams {
                level_id: 1,
                platformer: false,
                practice: true,
                response: ResponseType::Binary,
            })
        );

        assert_eq!(
            ListParams::parse_from_query(&HashMap::from([
                ("levelid".to_owned(), "1".to_owned()),
                ("platformer".to_owned(), "false".to_owned()),
                ("practice".to_owned(), "false".to_owned()),
            ])),
            Ok(ListParams {
                level_id: 1,
                platformer: false,
                practice: false,
                response: ResponseType::CSV,
            })
        );

        assert!(
            ListParams::parse_from_query(&HashMap::from([
                ("levelid".to_owned(), "1".to_owned()),
                ("platformer".to_owned(), "false".to_owned()),
                ("response".to_owned(), "bin".to_owned()),
                ("practice".to_owned(), "gibberish".to_owned()),
            ]))
            .is_err()
        );

        assert!(
            ListParams::parse_from_query(&HashMap::from([
                ("levelid".to_owned(), "1".to_owned()),
                ("platformer".to_owned(), "gibberish".to_owned()),
            ]))
            .is_err()
        );

        assert!(
            ListParams::parse_from_query(&HashMap::from([
                ("levelid".to_owned(), "nan".to_owned()),
                ("platformer".to_owned(), "false".to_owned()),
            ]))
            .is_err()
        );

        assert!(
            ListParams::parse_from_query(&HashMap::from([
                ("levelid".to_owned(), "1".to_owned()),
                ("platformer".to_owned(), "false".to_owned()),
                ("response".to_owned(), "other".to_owned()),
            ]))
            .is_err()
        );
    }

    #[test]
    fn test_parse_analysis_params() {
        assert_eq!(
            AnalysisParams::parse_from_query(&HashMap::from([(
                "levelid".to_owned(),
                "1".to_owned()
            ),])),
            Ok(AnalysisParams {
                level_id: 1,
                response: ResponseType::CSV,
            })
        );

        assert!(AnalysisParams::parse_from_query(&HashMap::from([])).is_err());

        assert_eq!(
            AnalysisParams::parse_from_query(&HashMap::from([
                ("levelid".to_owned(), "1".to_owned()),
                ("response".to_owned(), "csv".to_owned()),
            ])),
            Ok(AnalysisParams {
                level_id: 1,
                response: ResponseType::CSV,
            })
        );

        assert_eq!(
            AnalysisParams::parse_from_query(&HashMap::from([
                ("levelid".to_owned(), "1".to_owned()),
                ("response".to_owned(), "bin".to_owned()),
            ])),
            Ok(AnalysisParams {
                level_id: 1,
                response: ResponseType::Binary,
            })
        );
    }

    #[test]
    fn test_parse_submission() {
        assert_eq!(
            SubmissionPayload::parse(
                HashMap::from([("levelid".to_owned(), "1".to_owned())]),
                json!({"levelid": 2, "format": 1, "deaths": [], "userident": NULL_SHA1_STRING})
            ),
            Ok(SubmissionPayload(
                SubmissionMetadata::<String>::new(1, 1, 0, NULL_SHA1_STRING.to_owned()),
                vec![]
            ))
        );

        assert_eq!(
            SubmissionPayload::parse(
                HashMap::from([]),
                json!({"levelid": 2, "format": 1, "deaths": [], "userident": NULL_SHA1_STRING})
            ),
            Ok(SubmissionPayload(
                SubmissionMetadata::<String>::new(2, 1, 0, NULL_SHA1_STRING.to_owned()),
                vec![]
            ))
        );

        assert_eq!(
            SubmissionPayload::parse(
                HashMap::from([]),
                json!({"levelid": 2, "format": 1, "deaths": [], "userident": NULL_SHA1_STRING})
            ),
            Ok(SubmissionPayload(
                SubmissionMetadata::<String>::new(2, 1, 0, NULL_SHA1_STRING.to_owned()),
                vec![]
            ))
        );

        assert!(
            SubmissionPayload::parse(
                HashMap::from([]),
                json!({"levelid": 2, "format": 1, "userident": NULL_SHA1_STRING})
            )
            .is_err()
        );

        assert_eq!(
            SubmissionPayload::parse(
                HashMap::from([]),
                json!({"levelid": 2, "format": 1, "userident": NULL_SHA1_STRING, "x": 1.0, "y": 2.0, "percentage": 3})
            ),
            Ok(SubmissionPayload(
                SubmissionMetadata::<String>::new(2, 1, 0, NULL_SHA1_STRING.to_owned()),
                vec![SubmissionDeath::new(false, 1.0, 2.0, 3)]
            ))
        );

        assert!(
            SubmissionPayload::parse(
                HashMap::from([]),
                json!({"levelid": [], "format": [], "deaths": [], "userident": []})
            )
            .is_err()
        );

        assert!(
            SubmissionPayload::parse(
                HashMap::from([]),
                json!({"levelid": [], "format": [], "deaths": [], "userident": []})
            )
            .is_err()
        );

        assert!(
            SubmissionPayload::parse(
                HashMap::from([]),
                json!({"levelid": "", "format": "", "deaths": [], "userident": 1})
            )
            .is_err()
        );

        assert_eq!(
            SubmissionPayload::parse(
                HashMap::from([]),
                json!({"levelid": 2, "format": 1, "userident": NULL_SHA1_STRING, "deaths": [{"x": 1.0, "y": 2.0, "percentage": 3}]})
            ),
            Ok(SubmissionPayload(
                SubmissionMetadata::<String>::new(2, 1, 0, NULL_SHA1_STRING.to_owned()),
                vec![SubmissionDeath::new(false, 1.0, 2.0, 3)]
            ))
        );
    }
}
