#pragma once
#include <array>
namespace aeroram {
inline constexpr wchar_t Version[]=L"0.4.3";
struct SocialLink {const wchar_t* name;const wchar_t* url;};
inline constexpr std::array<SocialLink,4> Socials={{{L"Instagram",L"https://www.instagram.com/CristianSkylarr/"},{L"X",L"https://x.com/CristianSkylarr"},{L"LinkedIn",L"https://www.linkedin.com/in/cristianskylar/"},{L"Sitio web",L"https://cristianskylar.com"}}};
}
