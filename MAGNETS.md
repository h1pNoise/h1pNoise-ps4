# Magnets — build experimental 0.1.29

A página da consola inclui Torrent, Magnet e Link PKG. Em Magnet, cola a
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

Esta build para PS4 usa o instalador manual de atualizações. O protótipo
de arranque separado para atualização pela consola continua no código,
controlado por `--runtime`, mas não faz parte destes PKG de teste de magnets.
O feed público e as releases existentes não foram alterados.

## Testes

19 testes locais com tracker e seed sintéticos: magnets HTTP, UDP, fonte
direta, hash base32, metadados com vários blocos, dados corrompidos,
rejeição, tamanho inválido/excessivo, cancelamento, tarefas concorrentes,
autenticação, conteúdos sem PKG e retoma depois de reabrir. Incluem os
testes anteriores do envio `.torrent`, armazenamento e QR.
Testes da interface confirmam o novo separador, a validação e os botões.
PKG PS4 e shadPS4 compilados e validados; falta confirmar a operação numa
PS4 real. O Windows testa downloads, não a instalação de PKG.

Implementação baseada nas especificações primárias
[BEP 9](https://www.bittorrent.org/beps/bep_0009.html) e
[BEP 10](https://www.bittorrent.org/beps/bep_0010.html).
