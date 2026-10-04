# h1pNoise 0.1.30 — canal de atualizações separado

**Instala esta versão manualmente uma vez, com a h1pNoise fechada**, pelo
GoldHEN. Usa o ficheiro experimental.pkg na PS4 real; o shadPS4-test.pkg
é apenas para o emulador. Mantém o mesmo TITLE_ID: não apagues a app primeiro.

A partir desta versão, **Procurar atualização** consulta um canal separado.
As próximas versões anunciadas nele mostram um aviso na consola e na
página, com o botão **Descarregar atualização**. O PKG fica em
`/data/pkg/h1pNoise-update-<build>.pkg`, após verificar assinatura, hash,
tamanho e identidade. Fecha a app e instala o PKG manualmente pelo GoldHEN.

**A instalação automática continua suspensa.** O canal das versões antigas
fica na 0.1.25; a 0.1.26/0.1.27 não recebe o canal novo. Não uses Instalar
agora nessas versões. Esta release nunca remove a app instalada.

O manifesto inicial anuncia a 0.1.30, por isso ela mostra **Atualizada**.
Uma futura build superior anunciada neste canal apresenta **Nova versão**.
Um canal com uma build inferior mostra **Canal desatualizado**, em vez da
mensagem incorreta de última versão publicada. A marca Pre-release/Latest
do GitHub não determina esta consulta.

Mantém magnets, torrents, links PKG, medição do disco interno e downloads
em `/data/pkg`. O shadPS4 mantém os torrents e a interface de teste; o
atualizador requer a PS4 real. O novo canal precisa de confirmação na PS4.

APP_VER `00.40` · build `40` · TITLE_ID `HBRW00001`.

Detalhes de publicação: [UPDATES.md](UPDATES.md).
