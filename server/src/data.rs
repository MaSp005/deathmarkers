pub struct NormalDeath {
    x: f32,
    y: f32,
    percentage: i16,
}
impl NormalDeath {
    pub fn new(x: f32, y: f32, percentage: i16) -> NormalDeath {
        NormalDeath { x, y, percentage }
    }
}

pub struct PlatformerDeath {
    x: f32,
    y: f32,
}
impl PlatformerDeath {
    pub fn new(x: f32, y: f32) -> PlatformerDeath {
        PlatformerDeath { x, y }
    }
}

pub struct AnalysisDeath {
    userident: [u8; 20],
    levelversion: i8,
    practice: bool,
    x: f32,
    y: f32,
    percentage: i16,
}
impl AnalysisDeath {
    pub fn new(
        userident: [u8; 20],
        levelversion: i8,
        practice: bool,
        x: f32,
        y: f32,
        percentage: i16,
    ) -> AnalysisDeath {
        AnalysisDeath {
            userident,
            levelversion,
            practice,
            x,
            y,
            percentage,
        }
    }
}

pub enum Death {
    Normal(NormalDeath),
    Platformer(PlatformerDeath),
    Analysis(AnalysisDeath),
}
