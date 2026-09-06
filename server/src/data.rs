use crate::digest::Sha1Result;

#[derive(Debug, Copy, Clone, PartialEq)]
pub struct SubmissionDeath {
    practice: bool,
    x: f32,
    y: f32,
    percentage: i16,
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
    level_id: u32,
    format: u8,
    levelversion: u8,
    userident: U,
}
#[allow(private_bounds)]
impl<U: Userident> SubmissionMetadata<U> {
    pub fn new(level_id: u32, format: u8, levelversion: u8, userident: U) -> Self {
        Self {
            level_id,
            format,
            levelversion,
            userident,
        }
    }
}
