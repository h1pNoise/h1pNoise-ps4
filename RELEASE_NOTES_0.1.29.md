# h1pNoise 0.1.29 — magnets

**Instala manualmente, com a h1pNoise fechada.** Escolhe o PKG PS4 para a
consola real; o ficheiro shadPS4 é uma build de teste separada para o emulador.
Não uses «Instalar agora» nas versões antigas 0.1.26/0.1.27.

A página da consola tem agora o separador **Magnet**, junto a Torrent e
Link PKG. Cola a ligação completa e carrega **Enviar magnet**. Primeiro
obtém os dados do torrent e verifica o hash; depois escolhe **Só descarregar**
ou **Descarregar e instalar**. Os downloads mantêm-se em `/data/pkg`.

Durante a procura podes cancelar. Depois de obter os dados, a app guarda
o torrent para permitir retomar o download após reabrir. Dados corrompidos,
metadados excessivos e conteúdo que não seja PKG são recusados.

Suporta magnets BitTorrent v1 com hash hexadecimal ou base32, trackers
HTTP/UDP e fontes diretas IPv4/nomes com porta. Magnets apenas v2, DHT,
trackers HTTPS e fontes IPv6 ainda não estão implementados. Mantém a app
aberta e a PS4 ligada. Se não houver fontes acessíveis, envia o `.torrent`.

Validação: 19 testes locais com fontes sintéticas, testes da interface,
compilação e validação de ambos os PKG. Falta confirmar o funcionamento
dos magnets numa PS4 real; o Windows não instala PKG.

**Esta release não reativa a instalação automática pela própria app.** O
feed antigo permanece na versão retirada de teste, 0.1.25, para não acionar
o instalador problemático das versões anteriores. Partilha esta página
para os teus amigos descarregarem e instalarem o PKG manualmente.

APP_VER `00.39` · build `39` · TITLE_ID `HBRW00001`.

Detalhes e limites: [MAGNETS.md](MAGNETS.md).
