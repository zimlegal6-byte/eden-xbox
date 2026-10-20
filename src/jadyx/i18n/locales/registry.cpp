// SPDX-FileCopyrightText: Copyright 2026 Jadyx Project
// SPDX-License-Identifier: GPL-3.0-or-later
#include "jadyx/i18n/locales/catalogs.h"

namespace Jadyx::I18n {
namespace {
// Portuguese (Brazil), English and Spanish use "one" for exactly 1 and "other" otherwise.
PluralForm OneOrOther(std::uint64_t count) {
    return count == 1 ? PluralForm::One : PluralForm::Other;
}

// English first: fallback for any language and any missing text.
const std::array<UiLanguage, 3> languages{{
    {L"en", L"English", &EnglishCatalog, &OneOrOther},
    {L"es", L"Español", &SpanishCatalog, &OneOrOther},
    {L"pt-BR", L"Português (Brasil)", &PortugueseBrazilCatalog, &OneOrOther},
}};
} // namespace

std::span<const UiLanguage> UiLanguages() {
    return languages;
}
} // namespace Jadyx::I18n
