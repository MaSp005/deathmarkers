use hex::ToHex;
use sha1::{Digest, Sha1};

pub const SHA1_LENGTH: usize = 20;
pub type Sha1Result = [u8; SHA1_LENGTH];

pub fn sha1_digest(str: &str) -> Sha1Result {
    let mut slice = [0 as u8; SHA1_LENGTH];
    let digest = Sha1::digest(str);
    for (idx, chunk) in slice.iter_mut().enumerate() {
        *chunk = digest[idx];
    }
    slice
}

pub fn make_userident(playername: &str, userid: u64, levelid: u64) -> Sha1Result {
    let str = format!("{playername}_{userid}_{levelid}");
    sha1_digest(&str)
}

pub fn is_sha1(s: &str) -> bool {
    s.len() == SHA1_LENGTH * 2
        && s.chars().all(|c| match c {
            '0'..='9' | 'a'..='f' | 'A'..='F' => true,
            _ => false,
        })
}

pub fn stringify_digest(digest: Sha1Result) -> String {
    digest.encode_hex()
}

#[cfg(test)]
mod test {
    use super::*;
    use hex::FromHex;

    const NULL_SHA1_SLICE: Sha1Result = [0 as u8; SHA1_LENGTH];
    const NULL_SHA1_STRING: &str = "0000000000000000000000000000000000000000";

    #[test]
    fn test_make_userident() {
        assert_eq!(
            make_userident("RobTop", 16, 10565740),
            Sha1Result::from_hex(&"cba4a35e4ee458178b18d4c8ebb836a518b4df4b".to_owned()).unwrap()
        )
    }

    #[test]
    fn test_is_sha1() {
        assert_eq!(is_sha1("0123456789abcdef0123456789abcdef01234567"), true);
        assert_eq!(is_sha1("0123456789ABCDEF0123456789ABCDEF01234567"), true);
        assert_eq!(is_sha1(&String::from(NULL_SHA1_STRING)), true);
        assert_eq!(is_sha1("ghijklmnopqrstuvwxyzghijklmnopqrstuvwxyz"), false);
        assert_eq!(is_sha1("123"), false);
        assert_eq!(is_sha1("0123456789abcdef0123456789abcdef012345678"), false);
    }

    #[test]
    fn test_stringify_digest() {
        assert_eq!(
            stringify_digest([
                0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89, 0xab,
                0xcd, 0xef, 0x01, 0x23, 0x45, 0x67
            ]),
            "0123456789abcdef0123456789abcdef01234567"
        );
        assert_eq!(stringify_digest(NULL_SHA1_SLICE), NULL_SHA1_STRING);
    }
}
