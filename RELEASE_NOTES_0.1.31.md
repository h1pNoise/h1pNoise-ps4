# h1pNoise 0.1.31 — teste do canal de atualizações

Esta versão permite testar o aviso e o download a partir da **0.1.30**.
Mantém a 0.1.30 instalada para fazer o teste pelo canal novo:

1. Abre **Atualizações da app** e carrega **Procurar atualização**.
2. Confirma **Versão instalada: 0.1.30 · Disponível: 0.1.31** e **Nova versão**.
3. Carrega **Descarregar atualização** e aguarda o download verificado.
   O PKG fica em `/data/pkg/h1pNoise-update-41.pkg`.
4. Fecha a h1pNoise. No GoldHEN, desliga **Enable Background Installation**.
5. Instala esse PKG pelo **Package Installer**. Se perguntar, confirma a
   substituição/reinstalação. Não apagues a app primeiro.
6. Reabre a app e confirma **Versão instalada: 0.1.31**. Uma nova procura
   deve mostrar **Atualizada** enquanto o canal anunciar a 0.1.31.

O utilizador confirmou a substituição manual ao instalar a 0.1.30 com a
instalação em segundo plano desligada. Com a opção ligada, aparecia
`Package already installed`. A 0.1.31 acrescenta estas instruções à UI.

O aviso e o download pelo canal numa PS4 real ainda precisam deste teste.
**A instalação automática continua suspensa.** O canal antigo permanece
na 0.1.25; versões 0.1.29 ou anteriores precisam da migração manual para
0.1.30 ou superior. A marca Pre-release/Latest não determina este canal.

O PKG experimental é para PS4 real; shadPS4-test é apenas para o emulador.
Mantém magnets, torrents, links PKG e downloads em `/data/pkg`.

APP_VER `00.41` · build `41` · TITLE_ID `HBRW00001`.
