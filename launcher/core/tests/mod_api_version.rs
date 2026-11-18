//! The launcher's mod interface is the SDK header's: plugins::API_MAJOR and API_MINOR against
//! OPENRAC_MOD_API_MAJOR and _MINOR of openrac_mod.h.
use std::path::Path;

fn define(text: &str, name: &str) -> u32 {
    text.lines()
        .find_map(|l| l.strip_prefix(&format!("#define {name} ")))
        .and_then(|v| v.trim().parse().ok())
        .unwrap_or_else(|| panic!("no #define {name}"))
}

#[test]
fn launcher_api_is_the_headers() {
    let root = Path::new(env!("CARGO_MANIFEST_DIR")).join("../..");
    let header = std::fs::read_to_string(root.join("sdk/include/openrac/openrac_mod.h")).unwrap();
    let major = define(&header, "OPENRAC_MOD_API_MAJOR");
    let minor = define(&header, "OPENRAC_MOD_API_MINOR");
    assert_eq!((openrac_launcher_core::plugins::API_MAJOR, openrac_launcher_core::plugins::API_MINOR), (major, minor));
}
