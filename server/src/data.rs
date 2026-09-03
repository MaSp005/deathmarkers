pub const SHA1_LENGTH: usize = 20;

pub struct SubmissionDeath {
    practice: bool,
    x: f32,
    y: f32,
    percentage: i16,
}
impl SubmissionDeath {
    pub fn new(
        practice: bool,
        x: f32,
        y: f32,
        percentage: i16,
    ) -> SubmissionDeath {
        SubmissionDeath {
            practice,
            x,
            y,
            percentage,
        }
    }
}

pub struct SubmissionMetadata {
    level_id: u32,
    format: u8,
    levelversion: u8,
    userident: [u8; SHA1_LENGTH],
}
