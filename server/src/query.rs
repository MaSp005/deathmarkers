use crate::params::*;

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

pub trait Queryable {
    fn query(&self) -> (&'static str, i64);
}

impl Queryable for ListParams {
    fn query(&self) -> (&'static str, i64) {
        return (
            get_query(self.platformer, self.practice),
            self.level_id as i64,
        );
    }
}

impl Queryable for AnalysisParams {
    fn query(&self) -> (&'static str, i64) {
        (
            "SELECT userident,levelversion,practice,x,y,percentage FROM format1 WHERE levelid = $1;",
            self.level_id as i64,
        )
    }
}
