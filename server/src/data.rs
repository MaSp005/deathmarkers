pub const SHA1_LENGTH: usize = 20;

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

#[derive(Debug, Copy, Clone, PartialEq)]
pub struct SubmissionMetadata {
    level_id: u32,
    format: u8,
    levelversion: u8,
    userident: [u8; SHA1_LENGTH],
}
impl SubmissionMetadata {
    pub fn new(level_id: u32, format: u8, levelversion: u8, userident: [u8; SHA1_LENGTH]) -> Self {
        Self {
            level_id,
            format,
            levelversion,
            userident,
        }
    }
}
