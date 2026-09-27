# Atualizações da h1pNoise

A partir da versão 0.1.10, a app consulta o GitHub quando abre numa PS4 real.
Se existir uma versão mais recente, mostra um aviso na televisão e uma notificação.
Na página do telemóvel, **Atualizações da app** permite procurar, descarregar
e confirmar a instalação. Não instala silenciosamente.

Quem tem 0.1.9 ou anterior precisa de instalar o PKG 0.1.10 manualmente uma vez.
Não há notificações com a app fechada. O download da atualização precisa da app
aberta e da consola ligada. Os torrents e PKG já guardados são preservados.

Os testes desta versão executam o código C de produção com chamadas de sistema
simuladas: assinatura Ed25519, SHA-512, identidade e versão do PKG, limites,
redirecionamentos HTTPS, falhas de rede/disco, limpeza de recursos, confirmação
e prevenção de pedidos duplicados. A ligação HTTPS e a substituição da app em
execução ainda precisam de teste numa PS4 real. A confirmação envia o pacote
verificado ao BGFT; fecha a app e acompanha Notificações → Transferências.
Se a consola recusar, o pacote fica em `/data/pkg/h1pNoise-update-<build>.pkg`.
Não há desinstalação automática da app. No Windows/shadPS4 os botões ficam
desativados porque a instalação depende de uma PS4 real.

## Publicar uma nova versão

Repositório e releases: https://github.com/h1pNoise/h1pNoise-ps4

O feed da app usa o ficheiro HTTPS estável `releases/current/update.h1p` no
repositório; cada manifesto aponta para o PKG imutável dentro da pasta da sua
versão. A página GitHub Releases continua a documentar cada versão e fornece o
código-fonte da tag.

1. Aumenta `APP_VERSION`, `APP_BUILD` e `APP_SFO_VERSION` em `src/version.h`, mantendo TITLE_ID e CONTENT_ID.
2. Compila e testa o PKG PS4 normal, nunca a build shadPS4.
3. Cria uma release GitHub com uma tag nova, por exemplo `v0.1.11`.
4. Gera `update.h1p` com `tools/release_update.py`, usando o PKG final e o URL imutável da tag.
5. Guarda o PKG e `update.h1p` em `releases/v0.1.11/`, copia o manifesto para
   `releases/current/update.h1p`, e cria a release/tag com as mesmas notas.
6. Testa primeiro numa consola antes de distribuir. Nunca substituas uma tag publicada.

Exemplo:

```text
python tools/release_update.py --pkg h1pNoise-0.1.11-PS4.pkg --private-key ../h1pNoise-release-private/ed25519.pem --out update.h1p --version 0.1.11 --build 21 --url https://github.com/h1pNoise/h1pNoise-ps4/releases/download/v0.1.11/h1pNoise-0.1.11-PS4.pkg --notes "Correções e melhorias."
```

O comando prepara o ficheiro e não publica nada. A chave privada fica fora do projeto e nunca entra no GitHub, ZIP ou PKG. A app inclui apenas a chave pública em `src/update_config.h`; mudar essa chave impede instalações antigas de validar novas versões.

`update.h1p` contém 64 bytes de assinatura Ed25519 seguidos de nove linhas UTF-8:
magic, CONTENT_ID, versão, APP_VER SFO, build, tamanho, SHA-512, URL HTTPS do PKG e notas.
O limite é 4096 bytes e o pacote 128 MiB. Os certificados HTTPS permanecem ativos.
