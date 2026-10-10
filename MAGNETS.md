# Magnets

A página da consola inclui Torrent, Magnet, Link PKG e Real-Debrid. Em Magnet, cola a
ligação completa e carrega Enviar magnet. A app obtém os dados do torrent
a partir de fontes BitTorrent, verifica o SHA-1 indicado na ligação e valida
os nomes, tamanhos e hashes dos ficheiros. Depois aparecem os botões
Só descarregar e Descarregar e instalar. O destino continua `/data/pkg`.
Tal como nos torrents, mantém a app aberta e a PS4 ligada.

Suporta `urn:btih` v1 em hexadecimal (40 caracteres) ou base32 (32),
trackers HTTP/UDP (`tr`) e fontes IPv4/nomes com porta (`x.pe`). Ligações
apenas v2, trackers HTTPS, IPv6 e descoberta DHT ainda não são suportados.
Um magnet sem fonte suportada recebe uma mensagem explicativa. Não são
adicionados trackers públicos automaticamente. Aceita apenas conteúdo PKG
direto, com os mesmos limites do envio de `.torrent`.

Durante a procura, a transferência mostra A obter dados do magnet e permite
Cancelar procura. Se não houver dados válidos dentro do tempo de procura,
podes reenviar a ligação ou enviar o `.torrent`. Após a resolução, guarda
`source.torrent` junto dos ficheiros descarregados, para retomar após abrir a
app novamente. Fontes `x.pe` são preservadas no campo local
`h1pNoise-peers` desse ficheiro; a parte `info` mantém os bytes originais.
O código de emparelhamento e as regras de origem continuam obrigatórios.

Implementação baseada nas especificações primárias
[BEP 9](https://www.bittorrent.org/beps/bep_0009.html) e
[BEP 10](https://www.bittorrent.org/beps/bep_0010.html).

## Com Real-Debrid ativo

Guarda a API na aba Real-Debrid e envia a ligação na aba Magnet. O magnet v1 é enviado diretamente ao serviço, sem procurar peers e sem exigir trackers. A app seleciona apenas os PKG completos (até 32), incluindo subpastas, e ignora os outros ficheiros. Os PKG são preparados individualmente para evitar links agrupados em arquivos. ZIP/RAR e partes não são descompactados. A API não fornece os hashes de todos os blocos do magnet: a app verifica o hash identificador devolvido pelo serviço, os nomes, tamanhos e cabeçalhos PKG, sem equivaler à verificação SHA-1 de todos os blocos de um `.torrent`. Pausa/retoma reutiliza as tarefas durante a sessão. Depois de fechar a app, reenvia o magnet; a transferência RD em curso não é guardada.
