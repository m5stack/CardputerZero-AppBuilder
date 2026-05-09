pub fn init() {
    let locale = sys_locale::get_locale().unwrap_or_else(|| "en".to_string());
    let lang = if locale.starts_with("zh-TW")
        || locale.starts_with("zh_TW")
        || locale.starts_with("zh-Hant")
    {
        "zh-TW"
    } else if locale.starts_with("zh") {
        "zh-CN"
    } else {
        "en"
    };
    rust_i18n::set_locale(lang);
}
