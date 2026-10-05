# Créditos

A h1pNoise é distribuída sob GNU GPL versão 3, conforme LICENSE.

O pacote PS4 utiliza ferramentas, CRT, bibliotecas e módulos de compatibilidade
do OpenOrbis 0.5.3 (LLVM 18). Fontes e licenças de upstream:
https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain/tree/v0.5.3

O código de inicialização gráfica segue as interfaces exemplificadas nos samples
OpenOrbis. A integração BGFT segue as interfaces documentadas por flatz e os
exemplos de instalação local do ezRemote Client, creditados no README.

src/font8x8.h: fonte bitmap de Daniel Hepper / Marcel Sondaar / IBM,
disponibilizada em domínio público. O cabeçalho original de atribuição foi mantido.

src/vendor/qrcodegen.c e .h: QR Code generator v1.8.0, Project Nayuki,
licença MIT, preservada integralmente nos dois ficheiros.
https://github.com/nayuki/QR-Code-generator/tree/v1.8.0/c

Não inclui jogos, torrents de jogos, dados do Telegram, SDK da Sony nem credenciais
do utilizador. O teste de rede incluído usa apenas conteúdo sintético local.

src/vendor/monocypher.c, monocypher.h, monocypher-ed25519.c e
monocypher-ed25519.h: Monocypher 4.0.2, licenças BSD-2-Clause e CC0 conforme
os cabeçalhos preservados. https://monocypher.org/

Interface: logótipo fornecido pelo utilizador e cobertura bitmap de
glifos Segoe UI renderizados localmente. Não distribui ficheiros de fonte.
