use crate::digest::Sha1Result;

#[derive(Debug, Copy, Clone, PartialEq)]
pub struct SubmissionDeath {
    pub practice: bool,
    pub x: f32,
    pub y: f32,
    pub percentage: i16,
}
impl SubmissionDeath {
    pub fn new(practice: bool, x: f32, y: f32, percentage: i16) -> Self {
        Self {
            practice,
            x,
            y,
            percentage,
        }
    }
}

trait Userident {}
impl Userident for String {}
impl Userident for Sha1Result {}

#[derive(Debug, Copy, Clone, PartialEq)]
#[allow(private_bounds)]
pub struct SubmissionMetadata<U: Userident> {
    pub level_id: u32,
    pub format: u8,
    pub levelversion: u8,
    pub userident: U,
}
#[allow(private_bounds)]
impl<U: Userident> SubmissionMetadata<U> {
    pub fn new(level_id: u32, format: u8, levelversion: u8, userident: U) -> Result<Self, String> {
        if format != 1 {
            Err("Illegal format. Only 1 is supported.".to_owned())
        } else {
            Ok(Self {
                level_id,
                format,
                levelversion,
                userident,
            })
        }
    }

    pub fn get_insert_query(&self) -> &'static str {
        match self.format {
            1 => {
                "INSERT INTO format1 \
                (userident,levelid,levelversion,practice,x,y,percentage) \
                SELECT * FROM UNNEST\
                (\
                    $1::CHAR(40)[],\
                    $2::INTEGER[],\
                    $3::SMALLINT[],\
                    $4::BOOLEAN[],\
                    $5::FLOAT[],\
                    $6::FLOAT[],\
                    $7::SMALLINT[]\
                );"
            }
            _ => unreachable!(),
        }
    }
}
