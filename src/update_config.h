#ifndef H1PNOISE_UPDATE_CONFIG_H
#define H1PNOISE_UPDATE_CONFIG_H
#ifdef HARBOR_RUNTIME_UPDATES
#define UPDATE_FEED_URL "https://raw.githubusercontent.com/h1pNoise/h1pNoise-ps4/main/releases/runtime/current.h1p"
#elif defined(HARBOR_PKG_DIRECT_TEST)
#define UPDATE_FEED_URL "https://raw.githubusercontent.com/h1pNoise/h1pNoise-ps4/main/releases/pkg-direct-test/current.h1p"
#elif defined(HARBOR_PKG_INSTALLER_TEST)
#define UPDATE_FEED_URL "https://raw.githubusercontent.com/h1pNoise/h1pNoise-ps4/main/releases/pkg-install-test/current.h1p"
#else
#define UPDATE_FEED_URL "https://raw.githubusercontent.com/h1pNoise/h1pNoise-ps4/main/releases/manual-v1/current.h1p"
#endif
#define UPDATE_REPOSITORY "https://github.com/h1pNoise/h1pNoise-ps4"
static const unsigned char UPDATE_PUBLIC_KEY[32]={129,174,219,170,216,198,128,192,187,112,229,33,112,247,232,192,200,219,85,142,221,9,58,24,79,189,152,69,46,84,211,55};
#endif
