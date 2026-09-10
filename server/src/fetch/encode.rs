use crate::digest::{SHA1_LENGTH, Sha1Result, sha1_digest};
use bytes::{Buf, BufMut, Bytes, BytesMut};
use hex::{FromHex, ToHex};
use sqlx::{Row, postgres::PgRow};

pub(super) const DATA_FORMAT: u8 = 1;
const ITEM_LENGTH_NORMAL: usize = 4 + 4 + 2;
const ITEM_LENGTH_PLATFORMER: usize = 4 + 4;
const ITEM_LENGTH_ANALYSIS: usize = 20 + 1 + 1 + 4 + 4 + 2;
const COLUMNS_NORMAL: &str = "x,y,percentage";
const COLUMNS_PLATFORMER: &str = "x,y";
const COLUMNS_ANALYSIS: &str = "userident,levelversion,practice,x,y,percentage";

pub(super) fn create_binary_response_normal(deaths: &Vec<PgRow>) -> Bytes {
    let mut bytes = BytesMut::with_capacity(deaths.len() * ITEM_LENGTH_NORMAL + 1);
    bytes.put_u8(DATA_FORMAT);
    for death in deaths {
        bytes.put_f32_le(death.get::<f64, _>(0) as _); // x
        bytes.put_f32_le(death.get::<f64, _>(1) as _); // y
        bytes.put_u16_le(death.get::<i16, _>(2) as _); // percentage
    }
    bytes.freeze()
}

pub(super) fn create_binary_response_platformer(deaths: &Vec<PgRow>) -> Bytes {
    let mut bytes = BytesMut::with_capacity(deaths.len() * ITEM_LENGTH_PLATFORMER + 1);
    bytes.put_u8(DATA_FORMAT);
    for death in deaths {
        bytes.put_f32_le(death.get::<f64, _>(0) as _); // x
        bytes.put_f32_le(death.get::<f64, _>(1) as _); // y
    }
    bytes.freeze()
}

pub(super) fn create_binary_response_analysis(deaths: &Vec<PgRow>, salt: Option<&str>) -> Bytes {
    let mut bytes = BytesMut::with_capacity(deaths.len() * ITEM_LENGTH_ANALYSIS + 1);
    bytes.put_u8(DATA_FORMAT);
    for death in deaths {
        let userident: String = death.get(0);
        let salted_ui = if let Some(salt) = salt {
            sha1_digest(&format!("{userident}{salt}"))
        } else {
            Sha1Result::from_hex(userident).expect("Stored hash should be valid")
        };
        bytes.put_slice(&salted_ui); // userident
        bytes.put_u8(death.get::<i16, _>(1) as u8); // levelversion
        bytes.put_u8(if death.get::<bool, _>(2) { 1u8 } else { 0u8 }); // practice
        bytes.put_f32_le(death.get::<f64, _>(3) as f32); // x
        bytes.put_f32_le(death.get::<f64, _>(4) as f32); // y
        bytes.put_u16_le(death.get::<i16, _>(5) as u16); // percentage
    }
    bytes.freeze()
}

pub fn transform_to_csv_normal(mut bytes: Bytes) -> String {
    assert_eq!(bytes.len() % ITEM_LENGTH_NORMAL, 1);
    let mut lines: Vec<String> = Vec::with_capacity(bytes.len() / ITEM_LENGTH_NORMAL + 1);
    lines.push(COLUMNS_NORMAL.to_owned());
    bytes.get_u8();
    while bytes.remaining() >= ITEM_LENGTH_NORMAL {
        let x = bytes.get_f32_le();
        let y = bytes.get_f32_le();
        let percentage = bytes.get_u16_le();
        lines.push(format!("{x},{y},{percentage}"));
    }
    lines.join("\n")
}

pub fn transform_to_csv_platformer(mut bytes: Bytes) -> String {
    assert_eq!(bytes.len() % ITEM_LENGTH_PLATFORMER, 1);
    let mut lines: Vec<String> = Vec::with_capacity(bytes.len() / ITEM_LENGTH_PLATFORMER + 1);
    lines.push(COLUMNS_PLATFORMER.to_owned());
    bytes.get_u8();
    while bytes.remaining() >= ITEM_LENGTH_PLATFORMER {
        let x = bytes.get_f32_le();
        let y = bytes.get_f32_le();
        lines.push(format!("{x},{y}"));
    }
    lines.join("\n")
}

pub fn transform_to_csv_analysis(mut bytes: Bytes) -> String {
    assert_eq!(bytes.len() % ITEM_LENGTH_ANALYSIS, 1);
    let mut lines: Vec<String> = Vec::with_capacity(bytes.len() / ITEM_LENGTH_ANALYSIS + 1);
    lines.push(COLUMNS_ANALYSIS.to_owned());
    bytes.get_u8();
    while bytes.remaining() >= ITEM_LENGTH_ANALYSIS {
        let ui = bytes.split_to(SHA1_LENGTH);
        let ui: String = ui.encode_hex();
        let version = bytes.get_u8();
        let practice = bytes.get_u8();
        assert!(practice < 2);
        let practice = practice == 1;
        let x = bytes.get_f32_le();
        let y = bytes.get_f32_le();
        let percentage = bytes.get_u16_le();
        lines.push(format!("{ui},{version},{practice},{x},{y},{percentage}"));
    }
    lines.join("\n")
}

#[cfg(test)]
mod test {
    use super::*;
    use crate::digest::Sha1Result;
    use bytes::Bytes;
    use hex::FromHex;
    use sqlx::query;

    #[test]
    fn test_create_binary_response_empty() {
        let deaths = vec![];
        let bytes = create_binary_response_normal(&deaths);
        assert_eq!(bytes.len(), 1);
        assert_eq!(bytes, Bytes::from(vec![DATA_FORMAT]));
        let csv = transform_to_csv_normal(bytes);
        assert_eq!(csv, COLUMNS_NORMAL);
        let bytes = create_binary_response_platformer(&deaths);
        assert_eq!(bytes.len(), 1);
        assert_eq!(bytes, Bytes::from(vec![DATA_FORMAT]));
        let csv = transform_to_csv_platformer(bytes);
        assert_eq!(csv, COLUMNS_PLATFORMER);
        let bytes = create_binary_response_analysis(&deaths, None);
        assert_eq!(bytes.len(), 1);
        assert_eq!(bytes, Bytes::from(vec![DATA_FORMAT]));
        let csv = transform_to_csv_analysis(bytes);
        assert_eq!(csv, COLUMNS_ANALYSIS);
    }

    #[sqlx::test]
    #[ignore]
    async fn test_create_binary_response_normal(pool: sqlx::PgPool) {
        let deaths = query("SELECT 5.3::FLOAT, 9.4::FLOAT, 1::SMALLINT;")
            .fetch_all(&pool)
            .await
            .unwrap();
        let bytes = create_binary_response_normal(&deaths);

        println!("{0:?}", bytes.to_vec());
        assert_eq!(bytes.len(), 1 + 4 + 4 + 2);
        assert_eq!(bytes[0], DATA_FORMAT, "Data Format should be 0x1");
        let death_slice = &bytes[1..];
        assert_eq!(
            death_slice[0x0..=0x3],
            f32::to_le_bytes(5.3),
            "x as LE float32"
        );
        assert_eq!(
            death_slice[0x4..=0x7],
            f32::to_le_bytes(9.4),
            "y as LE float32"
        );
        assert_eq!(
            death_slice[0x8..=0x9],
            u16::to_le_bytes(1),
            "percentage as LE uint16"
        );

        let csv = transform_to_csv_normal(bytes);
        println!("{csv}");
        assert_eq!(csv, format!("{COLUMNS_NORMAL}\n5.3,9.4,1"));
    }

    #[sqlx::test]
    #[ignore]
    async fn test_create_binary_response_platformer(pool: sqlx::PgPool) {
        let deaths = query("SELECT 5.3::FLOAT, 9.4::FLOAT;")
            .fetch_all(&pool)
            .await
            .unwrap();
        let bytes = create_binary_response_platformer(&deaths);
        println!("{0:?}", bytes.to_vec());

        assert_eq!(bytes.len(), 1 + 4 + 4);
        assert_eq!(bytes[0], DATA_FORMAT, "Data Format should be 0x1");
        let death_slice = &bytes[1..];
        assert_eq!(
            death_slice[0x0..=0x3],
            f32::to_le_bytes(5.3),
            "x as LE float32"
        );
        assert_eq!(
            death_slice[0x4..=0x7],
            f32::to_le_bytes(9.4),
            "y as LE float32"
        );

        let csv = transform_to_csv_platformer(bytes);
        println!("{csv}");
        assert_eq!(csv, format!("{COLUMNS_PLATFORMER}\n5.3,9.4"));
    }

    #[sqlx::test]
    #[ignore]
    async fn test_create_binary_response_analysis(pool: sqlx::PgPool) {
        let deaths = query(
            "SELECT '0123456789abcdef0123456789abcdef01234567'::CHAR(40), \
            42::SMALLINT, \
            true, \
            5.3::FLOAT, \
            9.4::FLOAT, \
            1::SMALLINT;",
        )
        .fetch_all(&pool)
        .await
        .unwrap();
        let bytes = create_binary_response_analysis(&deaths, None);
        println!("{0:?}", bytes.to_vec());

        assert_eq!(bytes.len(), 1 + 20 + 1 + 1 + 4 + 4 + 2);
        assert_eq!(bytes[0], DATA_FORMAT, "Data Format should be 0x1");
        let death_slice = &bytes[1..];
        assert_eq!(
            death_slice[0x0..=0x13],
            Sha1Result::from_hex("0123456789abcdef0123456789abcdef01234567").unwrap(),
            "userident stays intact"
        );
        assert_eq!(death_slice[0x14], 42, "level version as uint8");
        assert_eq!(death_slice[0x15], 1, "practice as bool");
        assert_eq!(
            death_slice[0x16..=0x19],
            f32::to_le_bytes(5.3),
            "x as LE float32"
        );
        assert_eq!(
            death_slice[0x1a..=0x1d],
            f32::to_le_bytes(9.4),
            "y as LE float32"
        );
        assert_eq!(
            death_slice[0x1e..=0x1f],
            u16::to_le_bytes(1),
            "percentage as LE uint16"
        );

        let csv = transform_to_csv_analysis(bytes);
        println!("{csv}");
        assert_eq!(
            csv,
            format!(
                "{COLUMNS_ANALYSIS}\n0123456789abcdef0123456789abcdef01234567,42,true,5.3,9.4,1"
            )
        );
    }
}
